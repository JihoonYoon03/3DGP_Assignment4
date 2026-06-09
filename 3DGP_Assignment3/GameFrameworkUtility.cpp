#include "pch.h"
#include "GameFramework.h"


UINT GameFramework::AlignConstantBufferSize(UINT byteSize)
{
    constexpr UINT alignment = 256;
    return (byteSize + alignment - 1) & ~(alignment - 1);
}

void GameFramework::WaitForGpu()
{
    // GPU에 Fence 값을 신호하고 완료될 때 까지 대기
    const UINT64 fenceToWaitFor = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor));
    ++m_fenceValue;

    if (m_fence->GetCompletedValue() < fenceToWaitFor)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void GameFramework::FlushCommandQueue()
{
    WaitForGpu();
}

void GameFramework::CreateMeshResource(
    MeshResource& mesh,
    const std::vector<Vertex>& vertices,
    const std::vector<std::uint32_t>& indices,
    D3D12_PRIMITIVE_TOPOLOGY topology)
{
    if (vertices.empty() || indices.empty())
    {
        mesh = {};
        return;
    }

    const UINT vertexBufferSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));
    const UINT indexBufferSize = static_cast<UINT>(indices.size() * sizeof(std::uint32_t));

    const D3D12_HEAP_PROPERTIES uploadHeap = HeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_RESOURCE_DESC vertexDesc = BufferResourceDesc(vertexBufferSize);
    const D3D12_RESOURCE_DESC indexDesc = BufferResourceDesc(indexBufferSize);

    ThrowIfFailed(m_device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &vertexDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.vertexBuffer)));
    ThrowIfFailed(m_device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &indexDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.indexBuffer)));

    void* mappedVertices = nullptr;
    ThrowIfFailed(mesh.vertexBuffer->Map(0, nullptr, &mappedVertices));
    std::memcpy(mappedVertices, vertices.data(), vertexBufferSize);
    mesh.vertexBuffer->Unmap(0, nullptr);

    void* mappedIndices = nullptr;
    ThrowIfFailed(mesh.indexBuffer->Map(0, nullptr, &mappedIndices));
    std::memcpy(mappedIndices, indices.data(), indexBufferSize);
    mesh.indexBuffer->Unmap(0, nullptr);

    mesh.vertexBufferView.BufferLocation = mesh.vertexBuffer->GetGPUVirtualAddress();
    mesh.vertexBufferView.StrideInBytes = sizeof(Vertex);
    mesh.vertexBufferView.SizeInBytes = vertexBufferSize;
    mesh.indexBufferView.BufferLocation = mesh.indexBuffer->GetGPUVirtualAddress();
    mesh.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    mesh.indexBufferView.SizeInBytes = indexBufferSize;
    mesh.indexCount = static_cast<UINT>(indices.size());
    mesh.topology = topology;
}

D3D12_HEAP_PROPERTIES GameFramework::HeapProperties(D3D12_HEAP_TYPE type)
{
    D3D12_HEAP_PROPERTIES properties{};
    properties.Type = type;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    properties.CreationNodeMask = 1;
    properties.VisibleNodeMask = 1;
    return properties;
}

D3D12_RESOURCE_DESC GameFramework::BufferResourceDesc(UINT64 byteSize)
{
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = byteSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    return desc;
}

D3D12_RESOURCE_BARRIER GameFramework::TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = beforeState;
    barrier.Transition.StateAfter = afterState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    return barrier;
}

void GameFramework::ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::runtime_error("Direct3D 12 호출이 실패했습니다.");
    }
}
