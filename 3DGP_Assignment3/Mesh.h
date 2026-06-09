#pragma once

#include <d3d12.h>

#include <DirectXMath.h>

#include <wrl/client.h>

#include <cstddef>
#include <cstdint>
#include <string>

enum class MeshType : std::size_t
{
    Cube = 0,
    Terrain = 1,
    Apache = 2,
    Count = 3
};

class Vertex
{
public:
    DirectX::XMFLOAT3 position{};
    DirectX::XMFLOAT4 color{};
    DirectX::XMFLOAT3 normal{ 0.0f, 1.0f, 0.0f };
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

class ApacheMeshPart
{
public:
    MeshResource mesh;
    DirectX::XMFLOAT3 center{};
    DirectX::XMFLOAT3 extents{};
    std::string name;
    bool mainRotor = false;
    bool tailRotor = false;
};
