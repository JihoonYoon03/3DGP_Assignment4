#pragma once

#include "Terrain.h"

enum class MeshType : std::size_t
{
    Cube = 0,
    Terrain = 1,
    Model = 2,
    Count = 3
};

class Vertex
{
public:
    DirectX::XMFLOAT3 position{};
    DirectX::XMFLOAT4 color{};
    DirectX::XMFLOAT3 normal{ 0.0f, 1.0f, 0.0f };
};

class MeshData
{
public:
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    D3D12_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    bool Empty() const
    {
        return vertices.empty() || indices.empty();
    }
};

class TerrainMeshData
{
public:
    MeshData mesh;
    Terrain terrain;
};

class MeshFactory
{
public:
    static MeshData CreateCube();
    static TerrainMeshData CreateTerrainFromDefaultHeightMap();
};

class MeshResource
{
public:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView{};
    UINT indexCount = 0;
    D3D12_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};

class ModelMeshPart
{
public:
    MeshResource mesh;
    DirectX::XMFLOAT3 center{};
    DirectX::XMFLOAT3 extents{};
    std::string name;
    bool mainRotor = false;
    bool tailRotor = false;
};

class TextMeshModel
{
public:
    class Part
    {
    public:
        std::string name;
        MeshData meshData;
        DirectX::XMFLOAT3 center{};
        DirectX::XMFLOAT3 extents{};
        bool mainRotor = false;
        bool tailRotor = false;
    };

    static std::optional<std::filesystem::path> FindPath(const std::wstring& fileName);
    static std::optional<std::filesystem::path> FindDefaultPath();

    bool LoadFromTextFile(const std::filesystem::path& filePath);
    const std::vector<Part>& Parts() const;

private:
    std::vector<Part> m_parts;
};
