#include "pch.h"
#include "GameFramework.h"

#include "GameConfig.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
    constexpr const wchar_t* DefaultShaderFileName = L"Default.hlsl";

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

void GameFramework::CreatePipelineState()
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

void GameFramework::RenderFrame(
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

void GameFramework::PopulateCommandList(
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

void GameFramework::UploadObjectConstants(
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

        std::memcpy(m_mappedConstantBuffer + itemIndex * constantBufferStride, &constants, sizeof(constants));
    }
}
