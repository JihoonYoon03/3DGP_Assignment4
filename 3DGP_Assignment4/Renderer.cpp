#include "pch.h"
#include "Renderer.h"

#include "GameConfig.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

Renderer::~Renderer()
{
    if (m_device && m_commandQueue && m_fence)
    {
        FlushCommandQueue();
    }

    if (m_constantBuffer && m_mappedConstantBuffer)
    {
        m_constantBuffer->Unmap(0, nullptr);
        m_mappedConstantBuffer = nullptr;
    }

    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
}

void Renderer::Initialize(HWND hwnd, UINT width, UINT height)
{
    m_hwnd = hwnd;
    m_width = std::max(1u, width);
    m_height = std::max(1u, height);

    CreateDeviceResources();
    CreateWindowSizeDependentResources();
    CreatePipelineState();
}

void Renderer::Resize(UINT width, UINT height)
{
    if (width == 0 || height == 0 || !IsReady())
    {
        return;
    }

    m_width = width;
    m_height = height;
    FlushCommandQueue();
    CreateWindowSizeDependentResources();
}

bool Renderer::IsReady() const
{
    return m_device && m_swapChain;
}
void Renderer::CreateDeviceResources()
{
    UINT factoryFlags = 0;

#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    // 어댑터 선택과 스왑 체인 생성
    ThrowIfFailed(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_factory)));

    // 하드웨어 어댑터를 우선 사용, 실패하면 WARP 소프트웨어 어댑터로
    ComPtr<IDXGIAdapter1> selectedAdapter;
    for (UINT adapterIndex = 0; m_factory->EnumAdapters1(adapterIndex, &selectedAdapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 adapterDesc{};
        selectedAdapter->GetDesc1(&adapterDesc);
        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }

        if (SUCCEEDED(D3D12CreateDevice(selectedAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
        {
            break;
        }
    }

    // 장치 생성에 실패하면 WARP 장치를 생성
    if (!m_device)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_hwnd, &swapChainDesc, nullptr, nullptr, &swapChain));
    ThrowIfFailed(m_factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));
    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // RTV/DSV. 렌더 타깃과 깊이 버퍼 디스크립터를 보관
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // command allocator, list 생성
    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
    ThrowIfFailed(m_commandList->Close());

    const UINT constantBufferSize = AlignConstantBufferSize(sizeof(ObjectConstants)) * MaxDrawItems;
    const D3D12_HEAP_PROPERTIES uploadHeap = HeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_RESOURCE_DESC constantBufferDesc = BufferResourceDesc(constantBufferSize);
    ThrowIfFailed(m_device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBuffer)));
    ThrowIfFailed(m_constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedConstantBuffer)));

    // Fence로 대기
    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValue = 1;
    m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void Renderer::CreateWindowSizeDependentResources()
{
    for (auto& renderTarget : m_renderTargets)
    {
        renderTarget.Reset();
    }
    m_depthStencil.Reset();

    // 스왑 체인을 현재 창 크기에 맞춰 다시 만들기
    ThrowIfFailed(m_swapChain->ResizeBuffers(FrameCount, m_width, m_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // 후면 버퍼마다 RTV를 생성
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT bufferIndex = 0; bufferIndex < FrameCount; ++bufferIndex)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&m_renderTargets[bufferIndex])));
        m_device->CreateRenderTargetView(m_renderTargets[bufferIndex].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }

    D3D12_CLEAR_VALUE depthClearValue{};
    depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthClearValue.DepthStencil.Depth = 1.0f;
    depthClearValue.DepthStencil.Stencil = 0;

    D3D12_RESOURCE_DESC depthDesc{};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Alignment = 0;
    depthDesc.Width = m_width;
    depthDesc.Height = m_height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    const D3D12_HEAP_PROPERTIES defaultHeap = HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(m_device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&m_depthStencil)));
    m_device->CreateDepthStencilView(m_depthStencil.Get(), nullptr, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    m_viewport = { 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
    m_scissorRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
}


namespace
{
    constexpr const wchar_t* DefaultShaderFileName = L"Default.hlsl";

    template <typename T>
    void CopyObjectToBytes(const T& value, std::span<std::byte> destination)
    {
        const std::span<const std::byte> source = std::as_bytes(std::span{ &value, 1 });
        std::copy(source.begin(), source.end(), destination.begin());
    }

    template <typename T>
    void CopyVectorToBytes(const std::vector<T>& values, std::span<std::byte> destination)
    {
        const std::span<const std::byte> source = std::as_bytes(std::span{ values });
        std::copy(source.begin(), source.end(), destination.begin());
    }

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

    std::optional<std::filesystem::path> FindDefaultShaderPath()
    {
        std::vector<std::filesystem::path> candidates =
        {
            std::filesystem::path(L"Shaders") / DefaultShaderFileName,
            std::filesystem::path(L"3DGP_Assignment4") / L"Shaders" / DefaultShaderFileName,
            std::filesystem::path(L"..") / L"Shaders" / DefaultShaderFileName,
            std::filesystem::path(L"..") / L".." / L"3DGP_Assignment4" / L"Shaders" / DefaultShaderFileName
        };

        const std::filesystem::path exeDirectory = ExecutableDirectory();
        if (!exeDirectory.empty())
        {
            candidates.push_back(exeDirectory / L"Shaders" / DefaultShaderFileName);
            candidates.push_back(exeDirectory / L".." / L"Shaders" / DefaultShaderFileName);
            candidates.push_back(exeDirectory / L".." / L".." / L"3DGP_Assignment4" / L"Shaders" / DefaultShaderFileName);
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
}

void Renderer::CreatePipelineState()
{
    D3D12_ROOT_PARAMETER rootParameter{};
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameter.Descriptor.ShaderRegister = 0;
    rootParameter.Descriptor.RegisterSpace = 0;
    rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pParameters = &rootParameter;
    rootSignatureDesc.NumStaticSamplers = 0;
    rootSignatureDesc.pStaticSamplers = nullptr;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob));
    ThrowIfFailed(m_device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    const std::optional<std::filesystem::path> shaderPath = FindDefaultShaderPath();
    if (!shaderPath)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    }

    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    ThrowIfFailed(D3DCompileFromFile(shaderPath->c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, &errorBlob));
    ThrowIfFailed(D3DCompileFromFile(shaderPath->c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &errorBlob));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blendDesc{};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthStencilDesc.StencilEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.BlendState = blendDesc;
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void Renderer::RenderFrame(
    std::vector<DrawItem>& drawItems,
    const std::array<MeshResource, static_cast<std::size_t>(MeshType::Count)>& meshes,
    const std::vector<ApacheMeshPart>& apacheParts,
    const XMMATRIX& viewProjection,
    const XMFLOAT3& cameraPosition,
    const XMFLOAT4& clearColor)
{
    if (drawItems.size() > MaxDrawItems)
    {
        drawItems.resize(MaxDrawItems);
    }

    PopulateCommandList(drawItems, meshes, apacheParts, viewProjection, cameraPosition, clearColor);
    ID3D12CommandList* commandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    ThrowIfFailed(m_swapChain->Present(1, 0));
    WaitForGpu();
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void Renderer::PopulateCommandList(
    const std::vector<DrawItem>& drawItems,
    const std::array<MeshResource, static_cast<std::size_t>(MeshType::Count)>& meshes,
    const std::vector<ApacheMeshPart>& apacheParts,
    const XMMATRIX& viewProjection,
    const XMFLOAT3& cameraPosition,
    const XMFLOAT4& clearColor)
{
    ThrowIfFailed(m_commandAllocator->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

    UploadObjectConstants(drawItems, viewProjection, cameraPosition);

    const D3D12_RESOURCE_BARRIER toRenderTarget = TransitionBarrier(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &toRenderTarget);

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += static_cast<SIZE_T>(m_frameIndex) * m_rtvDescriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    const float clearColorValues[4] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColorValues, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    const UINT constantBufferStride = AlignConstantBufferSize(sizeof(ObjectConstants));
    for (std::size_t itemIndex = 0; itemIndex < drawItems.size(); ++itemIndex)
    {
        const DrawItem& item = drawItems[itemIndex];
        const MeshResource* mesh = &meshes[static_cast<std::size_t>(item.mesh)];
        if (item.mesh == MeshType::Apache)
        {
            if (item.meshPartIndex >= apacheParts.size())
            {
                continue;
            }
            mesh = &apacheParts[item.meshPartIndex].mesh;
        }

        if (mesh->indexCount == 0 || !mesh->vertexBuffer || !mesh->indexBuffer)
        {
            continue;
        }

        m_commandList->IASetPrimitiveTopology(mesh->topology);
        m_commandList->IASetVertexBuffers(0, 1, &mesh->vertexBufferView);
        m_commandList->IASetIndexBuffer(&mesh->indexBufferView);
        m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress() + itemIndex * constantBufferStride);
        m_commandList->DrawIndexedInstanced(mesh->indexCount, 1, 0, 0, 0);
    }

    const D3D12_RESOURCE_BARRIER toPresent = TransitionBarrier(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &toPresent);
    ThrowIfFailed(m_commandList->Close());
}

void Renderer::UploadObjectConstants(
    const std::vector<DrawItem>& drawItems,
    const XMMATRIX& viewProjection,
    const XMFLOAT3& cameraPosition)
{
    const UINT constantBufferStride = AlignConstantBufferSize(sizeof(ObjectConstants));

    const XMVECTOR lightDirectionVector = XMVector3Normalize(XMVectorSet(-0.45f, -0.85f, 0.25f, 0.0f));
    XMFLOAT3 lightDirection{};
    XMStoreFloat3(&lightDirection, lightDirectionVector);

    for (std::size_t itemIndex = 0; itemIndex < drawItems.size(); ++itemIndex)
    {
        const DrawItem& item = drawItems[itemIndex];
        const XMMATRIX world = XMLoadFloat4x4(&item.world);
        const XMMATRIX worldInverseTranspose = XMMatrixInverse(nullptr, world);
        const XMMATRIX worldViewProjection = XMMatrixTranspose(world * viewProjection);

        ObjectConstants constants{};
        XMStoreFloat4x4(&constants.world, XMMatrixTranspose(world));
        XMStoreFloat4x4(&constants.worldInverseTranspose, XMMatrixTranspose(worldInverseTranspose));
        XMStoreFloat4x4(&constants.worldViewProjection, worldViewProjection);
        constants.color = item.color;
        constants.cameraPosition = { cameraPosition.x, cameraPosition.y, cameraPosition.z, 1.0f };
        constants.lightDirection = { lightDirection.x, lightDirection.y, lightDirection.z, 0.0f };
        constants.ambientColor = { GP_LIGHT_AMBIENT_STRENGTH, GP_LIGHT_AMBIENT_STRENGTH, GP_LIGHT_AMBIENT_STRENGTH, 1.0f };
        constants.diffuseColor = { GP_LIGHT_DIFFUSE_STRENGTH, GP_LIGHT_DIFFUSE_STRENGTH, GP_LIGHT_DIFFUSE_STRENGTH, 1.0f };
        constants.specularColor = { GP_LIGHT_SPECULAR_STRENGTH, GP_LIGHT_SPECULAR_STRENGTH, GP_LIGHT_SPECULAR_STRENGTH, 1.0f };
        constants.lightingOptions = { GP_LIGHT_SPECULAR_POWER, 0.0f, 0.0f, 0.0f };

        CopyObjectToBytes(
            constants,
            std::span<std::byte>{ m_mappedConstantBuffer + itemIndex * constantBufferStride, sizeof(constants) });
    }
}


UINT Renderer::AlignConstantBufferSize(UINT byteSize)
{
    constexpr UINT alignment = 256;
    return (byteSize + alignment - 1) & ~(alignment - 1);
}

void Renderer::WaitForGpu()
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

void Renderer::FlushCommandQueue()
{
    WaitForGpu();
}

void Renderer::CreateMeshResource(
    MeshResource& mesh,
    const MeshData& meshData)
{
    if (meshData.Empty())
    {
        mesh = {};
        return;
    }

    const UINT vertexBufferSize = static_cast<UINT>(meshData.vertices.size() * sizeof(Vertex));
    const UINT indexBufferSize = static_cast<UINT>(meshData.indices.size() * sizeof(std::uint32_t));

    const D3D12_HEAP_PROPERTIES uploadHeap = HeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_RESOURCE_DESC vertexDesc = BufferResourceDesc(vertexBufferSize);
    const D3D12_RESOURCE_DESC indexDesc = BufferResourceDesc(indexBufferSize);

    ThrowIfFailed(m_device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &vertexDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.vertexBuffer)));
    ThrowIfFailed(m_device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &indexDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.indexBuffer)));

    void* mappedVertices = nullptr;
    ThrowIfFailed(mesh.vertexBuffer->Map(0, nullptr, &mappedVertices));
    CopyVectorToBytes(meshData.vertices, std::span<std::byte>{ static_cast<std::byte*>(mappedVertices), vertexBufferSize });
    mesh.vertexBuffer->Unmap(0, nullptr);

    void* mappedIndices = nullptr;
    ThrowIfFailed(mesh.indexBuffer->Map(0, nullptr, &mappedIndices));
    CopyVectorToBytes(meshData.indices, std::span<std::byte>{ static_cast<std::byte*>(mappedIndices), indexBufferSize });
    mesh.indexBuffer->Unmap(0, nullptr);

    mesh.vertexBufferView.BufferLocation = mesh.vertexBuffer->GetGPUVirtualAddress();
    mesh.vertexBufferView.StrideInBytes = sizeof(Vertex);
    mesh.vertexBufferView.SizeInBytes = vertexBufferSize;
    mesh.indexBufferView.BufferLocation = mesh.indexBuffer->GetGPUVirtualAddress();
    mesh.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    mesh.indexBufferView.SizeInBytes = indexBufferSize;
    mesh.indexCount = static_cast<UINT>(meshData.indices.size());
    mesh.topology = meshData.topology;
}

D3D12_HEAP_PROPERTIES Renderer::HeapProperties(D3D12_HEAP_TYPE type)
{
    D3D12_HEAP_PROPERTIES properties{};
    properties.Type = type;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    properties.CreationNodeMask = 1;
    properties.VisibleNodeMask = 1;
    return properties;
}

D3D12_RESOURCE_DESC Renderer::BufferResourceDesc(UINT64 byteSize)
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

D3D12_RESOURCE_BARRIER Renderer::TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
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

void Renderer::ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::runtime_error("Direct3D 12 호출이 실패했습니다.");
    }
}
