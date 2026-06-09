#include "pch.h"
#include "GameFramework.h"

void GameFramework::Initialize(HWND hwnd, UINT width, UINT height)
{
    m_hwnd = hwnd;
    m_width = std::max(1u, width);
    m_height = std::max(1u, height);

    m_renderer.Initialize(m_hwnd, m_width, m_height);
}

void GameFramework::OnResize(UINT width, UINT height)
{
    if (width == 0 || height == 0 || !IsDeviceReady())
    {
        return;
    }

    m_width = width;
    m_height = height;
    m_renderer.Resize(m_width, m_height);
}

void GameFramework::RenderFrame(
    std::vector<DrawItem>& drawItems,
    const std::array<MeshResource, static_cast<std::size_t>(MeshType::Count)>& meshes,
    const std::vector<ApacheMeshPart>& apacheParts,
    const DirectX::XMMATRIX& viewProjection,
    const DirectX::XMFLOAT3& cameraPosition,
    const DirectX::XMFLOAT4& clearColor)
{
    m_renderer.RenderFrame(drawItems, meshes, apacheParts, viewProjection, cameraPosition, clearColor);
}

void GameFramework::CreateMeshResource(MeshResource& mesh, const MeshData& meshData)
{
    m_renderer.CreateMeshResource(mesh, meshData);
}

bool GameFramework::IsDeviceReady() const
{
    return m_renderer.IsReady();
}
