#pragma once

#include "Renderer.h"

class GameFramework
{
public:
    GameFramework() = default;
    virtual ~GameFramework() = default;

    void Initialize(HWND hwnd, UINT width, UINT height);
    void OnResize(UINT width, UINT height);

protected:
    static constexpr UINT MaxDrawItems = Renderer::MaxDrawItems;

    void RenderFrame(
        std::vector<DrawItem>& drawItems,
        const std::array<MeshResource, static_cast<std::size_t>(MeshType::Count)>& meshes,
        const std::vector<ApacheMeshPart>& apacheParts,
        const DirectX::XMMATRIX& viewProjection,
        const DirectX::XMFLOAT3& cameraPosition,
        const DirectX::XMFLOAT4& clearColor);

    void CreateMeshResource(MeshResource& mesh, const MeshData& meshData);
    bool IsDeviceReady() const;

    HWND m_hwnd = nullptr;
    UINT m_width = 1280;
    UINT m_height = 720;

private:
    Renderer m_renderer;
};
