#include "pch.h"
#include "Mesh.h"

#include "GameConfig.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

MeshData MeshFactory::CreateCube()
{
    const DirectX::XMFLOAT4 white{ 1.0f, 1.0f, 1.0f, 1.0f };

    MeshData mesh{};
    mesh.vertices =
    {
        { { -0.5f, -0.5f, -0.5f }, white, { 0.0f, 0.0f, -1.0f } }, { { -0.5f,  0.5f, -0.5f }, white, { 0.0f, 0.0f, -1.0f } }, { {  0.5f,  0.5f, -0.5f }, white, { 0.0f, 0.0f, -1.0f } }, { {  0.5f, -0.5f, -0.5f }, white, { 0.0f, 0.0f, -1.0f } },
        { { -0.5f, -0.5f,  0.5f }, white, { 0.0f, 0.0f,  1.0f } }, { {  0.5f, -0.5f,  0.5f }, white, { 0.0f, 0.0f,  1.0f } }, { {  0.5f,  0.5f,  0.5f }, white, { 0.0f, 0.0f,  1.0f } }, { { -0.5f,  0.5f,  0.5f }, white, { 0.0f, 0.0f,  1.0f } },
        { { -0.5f,  0.5f, -0.5f }, white, { 0.0f, 1.0f, 0.0f } }, { { -0.5f,  0.5f,  0.5f }, white, { 0.0f, 1.0f, 0.0f } }, { {  0.5f,  0.5f,  0.5f }, white, { 0.0f, 1.0f, 0.0f } }, { {  0.5f,  0.5f, -0.5f }, white, { 0.0f, 1.0f, 0.0f } },
        { { -0.5f, -0.5f, -0.5f }, white, { 0.0f, -1.0f, 0.0f } }, { {  0.5f, -0.5f, -0.5f }, white, { 0.0f, -1.0f, 0.0f } }, { {  0.5f, -0.5f,  0.5f }, white, { 0.0f, -1.0f, 0.0f } }, { { -0.5f, -0.5f,  0.5f }, white, { 0.0f, -1.0f, 0.0f } },
        { { -0.5f, -0.5f,  0.5f }, white, { -1.0f, 0.0f, 0.0f } }, { { -0.5f,  0.5f,  0.5f }, white, { -1.0f, 0.0f, 0.0f } }, { { -0.5f,  0.5f, -0.5f }, white, { -1.0f, 0.0f, 0.0f } }, { { -0.5f, -0.5f, -0.5f }, white, { -1.0f, 0.0f, 0.0f } },
        { {  0.5f, -0.5f, -0.5f }, white, { 1.0f, 0.0f, 0.0f } }, { {  0.5f,  0.5f, -0.5f }, white, { 1.0f, 0.0f, 0.0f } }, { {  0.5f,  0.5f,  0.5f }, white, { 1.0f, 0.0f, 0.0f } }, { {  0.5f, -0.5f,  0.5f }, white, { 1.0f, 0.0f, 0.0f } }
    };

    mesh.indices =
    {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };

    mesh.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    return mesh;
}


namespace
{
    constexpr const wchar_t* HeightMapFileName = L"Cheongsando.png";

    class HeightMapData
    {
    public:
        UINT width = 0;
        UINT height = 0;
        std::vector<float> samples;
    };

    std::filesystem::path HeightMapExecutableDirectory()
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

        const std::filesystem::path exeDirectory = HeightMapExecutableDirectory();
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

TerrainMeshData MeshFactory::CreateTerrainFromDefaultHeightMap()
{
    TerrainMeshData result{};

    const std::optional<HeightMapData> heightMap = LoadHeightMap();
    const UINT terrainWidth = heightMap ? heightMap->width : GP_TERRAIN_GRID_VERTEX_COUNT;
    const UINT terrainLength = heightMap ? heightMap->height : GP_TERRAIN_GRID_VERTEX_COUNT;
    const float cellX = heightMap ? GP_HEIGHTMAP_CELL_X_METERS * GP_WORLD_UNITS_PER_METER : GP_TERRAIN_CELL_METERS * GP_WORLD_UNITS_PER_METER;
    const float cellZ = heightMap ? GP_HEIGHTMAP_CELL_Z_METERS * GP_WORLD_UNITS_PER_METER : GP_TERRAIN_CELL_METERS * GP_WORLD_UNITS_PER_METER;
    const float halfWidth = static_cast<float>(terrainWidth - 1) * cellX * 0.5f;
    const float halfLength = static_cast<float>(terrainLength - 1) * cellZ * 0.5f;

    result.terrain.Reset(terrainWidth, terrainLength, cellX, cellZ, halfWidth, halfLength);

    result.mesh.vertices.reserve(static_cast<std::size_t>(terrainWidth) * terrainLength);
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
            result.terrain.PushHeight(height);
            result.mesh.vertices.push_back({ { worldX, height, worldZ }, color, normal });
        }
    }

    result.mesh.indices.reserve(static_cast<std::size_t>(terrainWidth - 1) * (terrainLength - 1) * 6);
    for (UINT z = 0; z < terrainLength - 1; ++z)
    {
        for (UINT x = 0; x < terrainWidth - 1; ++x)
        {
            const std::uint32_t topLeft = z * terrainWidth + x;
            const std::uint32_t topRight = topLeft + 1;
            const std::uint32_t bottomLeft = (z + 1) * terrainWidth + x;
            const std::uint32_t bottomRight = bottomLeft + 1;
            result.mesh.indices.push_back(topLeft);
            result.mesh.indices.push_back(bottomLeft);
            result.mesh.indices.push_back(topRight);
            result.mesh.indices.push_back(topRight);
            result.mesh.indices.push_back(bottomLeft);
            result.mesh.indices.push_back(bottomRight);
        }
    }

    result.mesh.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    return result;
}


namespace
{
    constexpr const wchar_t* TextMeshModelFileName = L"Apache.txt";

    XMFLOAT4X4 IdentityMatrix()
    {
        XMFLOAT4X4 matrix{};
        XMStoreFloat4x4(&matrix, XMMatrixIdentity());
        return matrix;
    }

    std::string_view TrimLeft(std::string_view text)
    {
        while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
        {
            text.remove_prefix(1);
        }
        return text;
    }

    int CountIndent(std::string_view text)
    {
        int indent = 0;
        for (const char ch : text)
        {
            if (ch != ' ' && ch != '\t')
            {
                break;
            }
            ++indent;
        }
        return indent;
    }

    std::optional<std::string_view> PayloadAfterTag(std::string_view line, std::string_view tag)
    {
        const std::string_view trimmed = TrimLeft(line);
        if (!trimmed.starts_with(tag))
        {
            return std::nullopt;
        }
        return TrimLeft(trimmed.substr(tag.size()));
    }

    // <Frame> 행에서 프레임 이름만 분리
    std::string ParseFrameName(std::string_view payload)
    {
        std::istringstream stream{ std::string(payload) };
        int frameIndex = 0;
        std::string frameName;
        stream >> frameIndex >> frameName;
        return frameName;
    }

    std::optional<XMFLOAT4X4> ParseMatrix(std::string_view payload)
    {
        std::istringstream stream{ std::string(payload) };
        float values[16]{};
        for (float& value : values)
        {
            if (!(stream >> value))
            {
                return std::nullopt;
            }
        }

        XMFLOAT4X4 matrix{};
        XMStoreFloat4x4(
            &matrix,
            XMMatrixSet(
                values[0], values[1], values[2], values[3],
                values[4], values[5], values[6], values[7],
                values[8], values[9], values[10], values[11],
                values[12], values[13], values[14], values[15]));
        return matrix;
    }

    // 색상 값은 0 ~ 1 범위로 보정
    XMFLOAT4 NormalizeMaterialColor(const XMFLOAT4& color)
    {
        XMFLOAT4 normalized
        {
            std::clamp(color.x, 0.0f, 1.0f),
            std::clamp(color.y, 0.0f, 1.0f),
            std::clamp(color.z, 0.0f, 1.0f),
            1.0f
        };

        // 값이 너무 작으면 최소 밝기로 설정
        if (normalized.x + normalized.y + normalized.z < 0.03f)
        {
            normalized = { 0.04f, 0.04f, 0.04f, 1.0f };
        }
        return normalized;
    }

    std::filesystem::path TextMeshModelExecutableDirectory()
    {
        std::array<wchar_t, MAX_PATH> modulePath{};
        const DWORD length = GetModuleFileNameW(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
        if (length == 0 || length >= modulePath.size())
        {
            return {};
        }
        return std::filesystem::path(modulePath.data()).parent_path();
    }

    std::optional<std::filesystem::path> FindTextMeshModelPath()
    {
        std::vector<std::filesystem::path> candidates =
        {
            std::filesystem::path(L"Models") / TextMeshModelFileName,
            std::filesystem::path(L"3DGP_Assignment4") / L"Models" / TextMeshModelFileName,
            std::filesystem::path(L"..") / L"Models" / TextMeshModelFileName,
            std::filesystem::path(L"..") / L".." / L"3DGP_Assignment4" / L"Models" / TextMeshModelFileName
        };

        const std::filesystem::path exeDirectory = TextMeshModelExecutableDirectory();
        if (!exeDirectory.empty())
        {
            candidates.push_back(exeDirectory / L"Models" / TextMeshModelFileName);
            candidates.push_back(exeDirectory / L".." / L"Models" / TextMeshModelFileName);
            candidates.push_back(exeDirectory / L".." / L".." / L"3DGP_Assignment4" / L"Models" / TextMeshModelFileName);
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

    struct TextMeshFrameState
    {
        int indent = 0;
        XMFLOAT4X4 parentWorld = IdentityMatrix();
        XMFLOAT4X4 world = IdentityMatrix();
        std::string name;
    };

    // GPU 리소스를 만들기 전 CPU에서 보관하는 텍스트 모델 파트 데이터
    struct TextMeshCpuPart
    {
        std::string name;
        std::vector<XMFLOAT3> positions;
        std::vector<XMFLOAT3> normals;
        std::vector<std::uint32_t> indices;
        XMFLOAT4 color{ 0.72f, 0.72f, 0.72f, 1.0f };
        bool hasColor = false;
    };

    // 메시 데이터 임시 보관용
    struct PendingTextMesh
    {
        bool active = false;
        std::string name;
        XMFLOAT4X4 world = IdentityMatrix();
        std::vector<XMFLOAT3> positions;
        std::vector<XMFLOAT3> normals;
        std::vector<std::uint32_t> indices;
        XMFLOAT4 color{ 0.72f, 0.72f, 0.72f, 1.0f };
        bool hasColor = false;

        void Reset()
        {
            active = false;
            name.clear();
            world = IdentityMatrix();
            positions.clear();
            normals.clear();
            indices.clear();
            color = { 0.72f, 0.72f, 0.72f, 1.0f };
            hasColor = false;
        }
    };
}


std::optional<std::filesystem::path> TextMeshModel::FindDefaultPath()
{
    return FindTextMeshModelPath();
}

const std::vector<TextMeshModel::Part>& TextMeshModel::Parts() const
{
    return m_parts;
}

bool TextMeshModel::LoadFromTextFile(const std::filesystem::path& filePath)
{
    std::ifstream file{ filePath };
    if (!file)
    {
        return false;
    }

    std::vector<TextMeshCpuPart> cpuParts;
    cpuParts.reserve(40);

    const XMFLOAT4X4 identity = IdentityMatrix();
    std::vector<TextMeshFrameState> frameStack;
    PendingTextMesh pendingMesh;

    auto flushPendingMesh = [&]()
    {
        if (!pendingMesh.active)
        {
            return;
        }

        if (!pendingMesh.positions.empty() && !pendingMesh.indices.empty())
        {
            if (pendingMesh.normals.size() != pendingMesh.positions.size())
            {
                pendingMesh.normals.assign(pendingMesh.positions.size(), { 0.0f, 1.0f, 0.0f });
            }

            TextMeshCpuPart part{};
            part.name = pendingMesh.name;
            part.positions = std::move(pendingMesh.positions);
            part.normals = std::move(pendingMesh.normals);
            part.indices = std::move(pendingMesh.indices);
            part.color = pendingMesh.hasColor ? NormalizeMaterialColor(pendingMesh.color) : pendingMesh.color;
            part.hasColor = pendingMesh.hasColor;
            cpuParts.push_back(std::move(part));
        }

        pendingMesh.Reset();
    };

    std::string line;
    while (std::getline(file, line))
    {
        const std::string_view lineView(line);
        const std::string_view trimmed = TrimLeft(lineView);

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<Frame>:"))
        {
            flushPendingMesh();

            const int indent = CountIndent(lineView);
            while (!frameStack.empty() && frameStack.back().indent >= indent)
            {
                frameStack.pop_back();
            }

            TextMeshFrameState frame{};
            frame.indent = indent;
            frame.parentWorld = frameStack.empty() ? identity : frameStack.back().world;
            frame.world = frame.parentWorld;
            frame.name = ParseFrameName(*payload);
            frameStack.push_back(frame);
            continue;
        }

        if (trimmed.starts_with("</Frame>"))
        {
            flushPendingMesh();
            if (!frameStack.empty())
            {
                frameStack.pop_back();
            }
            continue;
        }

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<TransformMatrix>:"))
        {
            if (!frameStack.empty())
            {
                if (const std::optional<XMFLOAT4X4> localMatrix = ParseMatrix(*payload))
                {
                    const XMMATRIX local = XMLoadFloat4x4(&(*localMatrix));
                    const XMMATRIX parent = XMLoadFloat4x4(&frameStack.back().parentWorld);
                    XMStoreFloat4x4(&frameStack.back().world, local * parent);
                }
            }
            continue;
        }

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<Mesh>:"))
        {
            flushPendingMesh();

            pendingMesh.active = true;
            pendingMesh.name = frameStack.empty() ? "ModelPart" : frameStack.back().name;
            pendingMesh.world = frameStack.empty() ? identity : frameStack.back().world;
            std::istringstream stream{ std::string(*payload) };
            int vertexCount = 0;
            if (stream >> vertexCount)
            {
                pendingMesh.positions.reserve(static_cast<std::size_t>(std::max(vertexCount, 0)));
                pendingMesh.normals.reserve(static_cast<std::size_t>(std::max(vertexCount, 0)));
            }
            continue;
        }

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<Positions>:"))
        {
            if (pendingMesh.active)
            {
                std::istringstream stream{ std::string(*payload) };
                int positionCount = 0;
                stream >> positionCount;

                const XMMATRIX world = XMLoadFloat4x4(&pendingMesh.world);
                for (int index = 0; index < positionCount; ++index)
                {
                    float x = 0.0f;
                    float y = 0.0f;
                    float z = 0.0f;
                    if (!(stream >> x >> y >> z))
                    {
                        break;
                    }

                    const XMVECTOR localPosition = XMVectorSet(x, y, z, 1.0f);
                    XMFLOAT3 worldPosition{};
                    XMStoreFloat3(&worldPosition, XMVector3TransformCoord(localPosition, world));
                    pendingMesh.positions.push_back(worldPosition);
                }
            }
            continue;
        }

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<Normals>:"))
        {
            if (pendingMesh.active)
            {
                std::istringstream stream{ std::string(*payload) };
                int normalCount = 0;
                stream >> normalCount;

                const XMMATRIX world = XMLoadFloat4x4(&pendingMesh.world);
                for (int index = 0; index < normalCount; ++index)
                {
                    float x = 0.0f;
                    float y = 1.0f;
                    float z = 0.0f;
                    if (!(stream >> x >> y >> z))
                    {
                        break;
                    }

                    const XMVECTOR localNormal = XMVectorSet(x, y, z, 0.0f);
                    XMFLOAT3 worldNormal{};
                    XMStoreFloat3(&worldNormal, XMVector3Normalize(XMVector3TransformNormal(localNormal, world)));
                    pendingMesh.normals.push_back(worldNormal);
                }
            }
            continue;
        }

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<SubMesh>:"))
        {
            if (pendingMesh.active)
            {
                std::istringstream stream{ std::string(*payload) };
                int subMeshIndex = 0;
                int indexCount = 0;
                stream >> subMeshIndex >> indexCount;

                pendingMesh.indices.reserve(pendingMesh.indices.size() + static_cast<std::size_t>(std::max(indexCount, 0)));
                for (int index = 0; index < indexCount; ++index)
                {
                    std::uint32_t vertexIndex = 0;
                    if (!(stream >> vertexIndex))
                    {
                        break;
                    }
                    pendingMesh.indices.push_back(vertexIndex);
                }
            }
            continue;
        }

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<AlbedoColor>:"))
        {
            if (pendingMesh.active)
            {
                std::istringstream stream{ std::string(*payload) };
                stream >> pendingMesh.color.x >> pendingMesh.color.y >> pendingMesh.color.z >> pendingMesh.color.w;
                pendingMesh.hasColor = true;
                flushPendingMesh();
            }
            continue;
        }
    }

    flushPendingMesh();
    if (cpuParts.empty())
    {
        return false;
    }

    // 텍스트 모델의 중심 구하기 / 각 파트 정점을 같은 모델 로컬 좌표계로
    XMFLOAT3 modelMin{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    XMFLOAT3 modelMax{ std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };
    for (const TextMeshCpuPart& part : cpuParts)
    {
        for (const XMFLOAT3& position : part.positions)
        {
            modelMin.x = std::min(modelMin.x, position.x);
            modelMin.y = std::min(modelMin.y, position.y);
            modelMin.z = std::min(modelMin.z, position.z);
            modelMax.x = std::max(modelMax.x, position.x);
            modelMax.y = std::max(modelMax.y, position.y);
            modelMax.z = std::max(modelMax.z, position.z);
        }
    }

    const XMFLOAT3 modelCenter
    {
        (modelMin.x + modelMax.x) * 0.5f,
        (modelMin.y + modelMax.y) * 0.5f,
        (modelMin.z + modelMax.z) * 0.5f
    };

    m_parts.clear();
    m_parts.reserve(cpuParts.size());
    for (TextMeshCpuPart& cpuPart : cpuParts)
    {
        std::vector<Vertex> vertices;
        vertices.reserve(cpuPart.positions.size());

        XMFLOAT3 partMin{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
        XMFLOAT3 partMax{ std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };
        for (std::size_t index = 0; index < cpuPart.positions.size(); ++index)
        {
            XMFLOAT3 position = cpuPart.positions[index];
            position.x -= modelCenter.x;
            position.y -= modelCenter.y;
            position.z -= modelCenter.z;

            partMin.x = std::min(partMin.x, position.x);
            partMin.y = std::min(partMin.y, position.y);
            partMin.z = std::min(partMin.z, position.z);
            partMax.x = std::max(partMax.x, position.x);
            partMax.y = std::max(partMax.y, position.y);
            partMax.z = std::max(partMax.z, position.z);

            vertices.push_back({ position, cpuPart.color, cpuPart.normals[index] });
        }

        TextMeshModel::Part part{};
        part.name = std::move(cpuPart.name);
        part.center =
        {
            (partMin.x + partMax.x) * 0.5f,
            (partMin.y + partMax.y) * 0.5f,
            (partMin.z + partMax.z) * 0.5f
        };
        part.extents =
        {
            (partMax.x - partMin.x) * 0.5f,
            (partMax.y - partMin.y) * 0.5f,
            (partMax.z - partMin.z) * 0.5f
        };

        const XMFLOAT3 fullSize{ part.extents.x * 2.0f, part.extents.y * 2.0f, part.extents.z * 2.0f };
        part.mainRotor = (part.name.find("rotor") != std::string::npos);
        part.tailRotor = !part.mainRotor && part.center.z < -35.0f && fullSize.z > 8.0f && fullSize.x < 4.0f && fullSize.y < 4.0f;

        part.meshData.vertices = std::move(vertices);
        part.meshData.indices = std::move(cpuPart.indices);
        part.meshData.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        if (!part.meshData.Empty())
        {
            m_parts.push_back(std::move(part));
        }
    }

    return !m_parts.empty();
}
