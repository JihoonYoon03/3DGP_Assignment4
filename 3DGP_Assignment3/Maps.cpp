#include "AssignmentGame.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <optional>
#include <vector>
#include <wincodec.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;


namespace
{
    constexpr const wchar_t* HeightMapFileName = L"Cheongsando.png";

    // 하이트맵 배열
    struct HeightMapData
    {
        UINT width = 0;
        UINT height = 0;
        std::vector<float> samples;
    };

    std::filesystem::path ExecutableDirectory()
    {
        std::array<wchar_t, MAX_PATH> modulePath{};
        const DWORD length = GetModuleFileNameW(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
        if (length == 0 || length >= modulePath.size())
        {
            return {};
        }
        return std::filesystem::path(modulePath.data()).parent_path();
    }

    std::optional<std::filesystem::path> FindHeightMapPath()
    {
        std::vector<std::filesystem::path> candidates =
        {
            std::filesystem::path(L"Textures") / HeightMapFileName,
            std::filesystem::path(L"3DGP_Assignment4") / L"Textures" / HeightMapFileName,
            std::filesystem::path(L"..") / L"Textures" / HeightMapFileName,
            std::filesystem::path(L"..") / L".." / L"3DGP_Assignment4" / L"Textures" / HeightMapFileName
        };

        const std::filesystem::path exeDirectory = ExecutableDirectory();
        if (!exeDirectory.empty())
        {
            candidates.push_back(exeDirectory / L"Textures" / HeightMapFileName);
            candidates.push_back(exeDirectory / L".." / L"Textures" / HeightMapFileName);
            candidates.push_back(exeDirectory / L".." / L".." / L"3DGP_Assignment4" / L"Textures" / HeightMapFileName);
        }

        for (const std::filesystem::path& candidate : candidates)
        {
            std::error_code error;
            if (std::filesystem::exists(candidate, error))
            {
                return candidate;
            }
        }
        return std::nullopt;
    }

    // WIC를 사용해 PNG 하이트맵을 0~1 높이 샘플로 읽습니다.
    std::optional<HeightMapData> LoadHeightMap()
    {
        const std::optional<std::filesystem::path> path = FindHeightMapPath();
        if (!path)
        {
            return std::nullopt;
        }

        const HRESULT initResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(initResult) && initResult != RPC_E_CHANGED_MODE)
        {
            return std::nullopt;
        }

        ComPtr<IWICImagingFactory> factory;
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
        if (FAILED(hr))
        {
            return std::nullopt;
        }

        ComPtr<IWICBitmapDecoder> decoder;
        hr = factory->CreateDecoderFromFilename(path->c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
        if (FAILED(hr))
        {
            return std::nullopt;
        }

        ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr))
        {
            return std::nullopt;
        }

        UINT width = 0;
        UINT height = 0;
        frame->GetSize(&width, &height);
        if (width == 0 || height == 0)
        {
            return std::nullopt;
        }

        ComPtr<IWICFormatConverter> converter;
        hr = factory->CreateFormatConverter(&converter);
        if (SUCCEEDED(hr))
        {
            hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
        }
        if (FAILED(hr))
        {
            return std::nullopt;
        }

        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 4);
        hr = converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data());
        if (FAILED(hr))
        {
            return std::nullopt;
        }

        HeightMapData data{};
        data.width = width;
        data.height = height;
        data.samples.resize(static_cast<std::size_t>(width) * height);
        for (std::size_t index = 0; index < data.samples.size(); ++index)
        {
            const std::uint8_t r = pixels[index * 4 + 0];
            const std::uint8_t g = pixels[index * 4 + 1];
            const std::uint8_t b = pixels[index * 4 + 2];
            data.samples[index] = (static_cast<float>(r) + static_cast<float>(g) + static_cast<float>(b)) / (3.0f * 255.0f);
        }

        return data;
    }

    float HeightSample(const HeightMapData& data, int x, int z)
    {
        const int clampedX = std::clamp(x, 0, static_cast<int>(data.width) - 1);
        const int clampedZ = std::clamp(z, 0, static_cast<int>(data.height) - 1);
        return (GP_TERRAIN_BASE_HEIGHT_OFFSET_METERS + data.samples[static_cast<std::size_t>(clampedZ) * data.width + clampedX] * GP_HEIGHTMAP_MAX_HEIGHT_METERS) * GP_WORLD_UNITS_PER_METER;
    }

    // 지형 노멀 계산
    XMFLOAT3 TerrainNormalAt(const HeightMapData& data, int x, int z)
    {
        const float left = HeightSample(data, x - 1, z);
        const float right = HeightSample(data, x + 1, z);
        const float down = HeightSample(data, x, z - 1);
        const float up = HeightSample(data, x, z + 1);

        const XMVECTOR xTangent = XMVectorSet(GP_HEIGHTMAP_CELL_X_METERS * 2.0f, right - left, 0.0f, 0.0f);
        const XMVECTOR zTangent = XMVectorSet(0.0f, up - down, GP_HEIGHTMAP_CELL_Z_METERS * 2.0f, 0.0f);
        XMFLOAT3 normal{};
        XMStoreFloat3(&normal, XMVector3Normalize(XMVector3Cross(zTangent, xTangent)));
        return normal;
    }

    // 높이에 따른 정점 색상
    XMFLOAT4 TerrainColor(float normalizedHeight)
    {
        const float grass = std::clamp(1.0f - normalizedHeight * 1.2f, 0.0f, 1.0f);
        const float rock = 1.0f - grass;
        return
        {
            0.10f * grass + 0.43f * rock,
            0.34f * grass + 0.39f * rock,
            0.13f * grass + 0.32f * rock,
            1.0f
        };
    }
}


void AssignmentGame::CreateMeshResources()
{
    // 기본 도형인 큐브 정의
    const XMFLOAT4 white{ 1.0f, 1.0f, 1.0f, 1.0f };
    std::vector<Vertex> cubeVertices =
    {
        { { -0.5f, -0.5f, -0.5f }, white, { 0.0f, 0.0f, -1.0f } }, { { -0.5f,  0.5f, -0.5f }, white, { 0.0f, 0.0f, -1.0f } }, { {  0.5f,  0.5f, -0.5f }, white, { 0.0f, 0.0f, -1.0f } }, { {  0.5f, -0.5f, -0.5f }, white, { 0.0f, 0.0f, -1.0f } },
        { { -0.5f, -0.5f,  0.5f }, white, { 0.0f, 0.0f,  1.0f } }, { {  0.5f, -0.5f,  0.5f }, white, { 0.0f, 0.0f,  1.0f } }, { {  0.5f,  0.5f,  0.5f }, white, { 0.0f, 0.0f,  1.0f } }, { { -0.5f,  0.5f,  0.5f }, white, { 0.0f, 0.0f,  1.0f } },
        { { -0.5f,  0.5f, -0.5f }, white, { 0.0f, 1.0f, 0.0f } }, { { -0.5f,  0.5f,  0.5f }, white, { 0.0f, 1.0f, 0.0f } }, { {  0.5f,  0.5f,  0.5f }, white, { 0.0f, 1.0f, 0.0f } }, { {  0.5f,  0.5f, -0.5f }, white, { 0.0f, 1.0f, 0.0f } },
        { { -0.5f, -0.5f, -0.5f }, white, { 0.0f, -1.0f, 0.0f } }, { {  0.5f, -0.5f, -0.5f }, white, { 0.0f, -1.0f, 0.0f } }, { {  0.5f, -0.5f,  0.5f }, white, { 0.0f, -1.0f, 0.0f } }, { { -0.5f, -0.5f,  0.5f }, white, { 0.0f, -1.0f, 0.0f } },
        { { -0.5f, -0.5f,  0.5f }, white, { -1.0f, 0.0f, 0.0f } }, { { -0.5f,  0.5f,  0.5f }, white, { -1.0f, 0.0f, 0.0f } }, { { -0.5f,  0.5f, -0.5f }, white, { -1.0f, 0.0f, 0.0f } }, { { -0.5f, -0.5f, -0.5f }, white, { -1.0f, 0.0f, 0.0f } },
        { {  0.5f, -0.5f, -0.5f }, white, { 1.0f, 0.0f, 0.0f } }, { {  0.5f,  0.5f, -0.5f }, white, { 1.0f, 0.0f, 0.0f } }, { {  0.5f,  0.5f,  0.5f }, white, { 1.0f, 0.0f, 0.0f } }, { {  0.5f, -0.5f,  0.5f }, white, { 1.0f, 0.0f, 0.0f } }
    };

    std::vector<std::uint32_t> cubeIndices =
    {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };

    CreateMesh(m_meshes[static_cast<std::size_t>(MeshType::Cube)], cubeVertices, cubeIndices, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 하이트맵 이미지의 픽셀 수를 지형 정점 수로 그대로 사용
    const std::optional<HeightMapData> heightMap = LoadHeightMap();
    const UINT terrainWidth = heightMap ? heightMap->width : GP_TERRAIN_GRID_VERTEX_COUNT;
    const UINT terrainLength = heightMap ? heightMap->height : GP_TERRAIN_GRID_VERTEX_COUNT;
    const float cellX = heightMap ? GP_HEIGHTMAP_CELL_X_METERS * GP_WORLD_UNITS_PER_METER : GP_TERRAIN_CELL_METERS * GP_WORLD_UNITS_PER_METER;
    const float cellZ = heightMap ? GP_HEIGHTMAP_CELL_Z_METERS * GP_WORLD_UNITS_PER_METER : GP_TERRAIN_CELL_METERS * GP_WORLD_UNITS_PER_METER;
    const float halfWidth = static_cast<float>(terrainWidth - 1) * cellX * 0.5f;
    const float halfLength = static_cast<float>(terrainLength - 1) * cellZ * 0.5f;

    m_terrain.Reset(terrainWidth, terrainLength, cellX, cellZ, halfWidth, halfLength);

    std::vector<Vertex> terrainVertices;
    terrainVertices.reserve(static_cast<std::size_t>(terrainWidth) * terrainLength);
    for (UINT z = 0; z < terrainLength; ++z)
    {
        for (UINT x = 0; x < terrainWidth; ++x)
        {
            const float worldX = static_cast<float>(x) * cellX - halfWidth;
            const float worldZ = static_cast<float>(z) * cellZ - halfLength;
            const float normalizedHeight = heightMap ? heightMap->samples[static_cast<std::size_t>(z) * terrainWidth + x] : 0.0f;
            const float height = heightMap ? (GP_TERRAIN_BASE_HEIGHT_OFFSET_METERS + normalizedHeight * GP_HEIGHTMAP_MAX_HEIGHT_METERS) * GP_WORLD_UNITS_PER_METER : 0.0f;
            const XMFLOAT3 normal = heightMap ? TerrainNormalAt(*heightMap, static_cast<int>(x), static_cast<int>(z)) : XMFLOAT3{ 0.0f, 1.0f, 0.0f };
            const XMFLOAT4 color = heightMap ? TerrainColor(normalizedHeight) : XMFLOAT4{ 0.16f, 0.45f, 0.18f, 1.0f };
            m_terrain.PushHeight(height);
            terrainVertices.push_back({ { worldX, height, worldZ }, color, normal });
        }
    }

    std::vector<std::uint32_t> terrainIndices;
    terrainIndices.reserve(static_cast<std::size_t>(terrainWidth - 1) * (terrainLength - 1) * 6);
    for (UINT z = 0; z < terrainLength - 1; ++z)
    {
        for (UINT x = 0; x < terrainWidth - 1; ++x)
        {
            const std::uint32_t topLeft = z * terrainWidth + x;
            const std::uint32_t topRight = topLeft + 1;
            const std::uint32_t bottomLeft = (z + 1) * terrainWidth + x;
            const std::uint32_t bottomRight = bottomLeft + 1;
            terrainIndices.push_back(topLeft);
            terrainIndices.push_back(bottomLeft);
            terrainIndices.push_back(topRight);
            terrainIndices.push_back(topRight);
            terrainIndices.push_back(bottomLeft);
            terrainIndices.push_back(bottomRight);
        }
    }

    CreateMesh(m_meshes[static_cast<std::size_t>(MeshType::Terrain)], terrainVertices, terrainIndices, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_apacheModelLoaded = CreateApacheMesh();
}
