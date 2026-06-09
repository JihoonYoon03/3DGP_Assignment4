#pragma once

#include "GameObject.h"

class GameFramework
{
public:
    GameFramework() = default;
    virtual ~GameFramework();

    void Initialize(HWND hwnd, UINT width, UINT height);
    void OnResize(UINT width, UINT height);

protected:
    static constexpr UINT MaxDrawItems = 8192;

    void RenderFrame(
        std::vector<DrawItem>& drawItems,
        const std::array<MeshResource, static_cast<std::size_t>(MeshType::Count)>& meshes,
        const std::vector<ApacheMeshPart>& apacheParts,
        const DirectX::XMMATRIX& viewProjection,
        const DirectX::XMFLOAT3& cameraPosition,
        const DirectX::XMFLOAT4& clearColor);

    void CreateMeshResource(
        MeshResource& mesh,
        const std::vector<Vertex>& vertices,
        const std::vector<std::uint32_t>& indices,
        D3D12_PRIMITIVE_TOPOLOGY topology);

    bool IsDeviceReady() const;

    HWND m_hwnd = nullptr;
    UINT m_width = 1280;
    UINT m_height = 720;

private:
    static constexpr UINT FrameCount = 2;

    struct ObjectConstants
    {
        DirectX::XMFLOAT4X4 world{};
        DirectX::XMFLOAT4X4 worldInverseTranspose{};
        DirectX::XMFLOAT4X4 worldViewProjection{};
        DirectX::XMFLOAT4 color{};
        DirectX::XMFLOAT4 cameraPosition{};
        DirectX::XMFLOAT4 lightDirection{};
        DirectX::XMFLOAT4 ambientColor{};
        DirectX::XMFLOAT4 diffuseColor{};
        DirectX::XMFLOAT4 specularColor{};
        DirectX::XMFLOAT4 lightingOptions{};
    };

    void CreateDeviceResources();
    void CreateWindowSizeDependentResources();
    void CreatePipelineState();

    void PopulateCommandList(
        const std::vector<DrawItem>& drawItems,
        const std::array<MeshResource, static_cast<std::size_t>(MeshType::Count)>& meshes,
        const std::vector<ApacheMeshPart>& apacheParts,
        const DirectX::XMMATRIX& viewProjection,
        const DirectX::XMFLOAT3& cameraPosition,
        const DirectX::XMFLOAT4& clearColor);

    void UploadObjectConstants(
        const std::vector<DrawItem>& drawItems,
        const DirectX::XMMATRIX& viewProjection,
        const DirectX::XMFLOAT3& cameraPosition);

    void WaitForGpu();
    void FlushCommandQueue();

    static UINT AlignConstantBufferSize(UINT byteSize);
    static D3D12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE type);
    static D3D12_RESOURCE_DESC BufferResourceDesc(UINT64 byteSize);
    static D3D12_RESOURCE_BARRIER TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
    static void ThrowIfFailed(HRESULT hr);

    Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencil;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;

    std::uint8_t* m_mappedConstantBuffer = nullptr;

    UINT m_rtvDescriptorSize = 0;
    UINT m_dsvDescriptorSize = 0;
    UINT m_frameIndex = 0;

    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue = 0;
    HANDLE m_fenceEvent = nullptr;

    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};
};
