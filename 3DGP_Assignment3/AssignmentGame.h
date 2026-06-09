#pragma once

#include "pch.h"
#include "Camera.h"
#include "Collision.h"
#include "GameObject.h"
#include "Maps.h"
#include "Mesh.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#include <DirectXMath.h>

#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class AssignmentGame {
public:
    AssignmentGame() = default;
    ~AssignmentGame();

    void Initialize(HWND hwnd, UINT width, UINT height);

    void Tick(float deltaSeconds);

    void OnResize(UINT width, UINT height);

    void OnKeyDown(WPARAM key);
    void OnKeyUp(WPARAM key);

    void OnMouseMove(int x, int y);
    void OnMouseDown(int x, int y);
    void OnRightMouseDown(int x, int y);

private:
    static constexpr UINT FrameCount = 2;

    static constexpr UINT MaxDrawItems = 8192;
    static constexpr std::size_t MaxMissileTrailParticles = 384;

    static constexpr UINT ConstantBufferAlignment = 256;

    enum class SceneMode
    {
        Start,
        Menu,
        Level1
    };

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

    void Render();

    void PopulateCommandList(const DirectX::XMMATRIX& viewProjection);
    void UploadObjectConstants(const DirectX::XMMATRIX& viewProjection);
    void WaitForGpu();
    void FlushCommandQueue();

    static UINT AlignConstantBufferSize(UINT byteSize);

    static D3D12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE type);
    static D3D12_RESOURCE_DESC BufferResourceDesc(UINT64 byteSize);
    static D3D12_RESOURCE_BARRIER TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

    static void ThrowIfFailed(HRESULT hr);

    void CreateMeshResources();
    void CreateMesh(MeshResource& mesh, const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices, D3D12_PRIMITIVE_TOPOLOGY topology);
    bool CreateApacheMesh();
    bool LoadApacheModelFile(const std::wstring& filePath);
    DirectX::XMMATRIX ApacheModelWorldMatrix() const;

    void Update(float deltaSeconds);
    void UpdateStart(float deltaSeconds);
    void UpdateLevel(float deltaSeconds);
    void UpdateAimRay();

    void FireBulletAtAim();

    void BuildDrawItems();
    void BuildStartScene();
    void BuildMenuScene();
    void BuildLevelScene();

    void AddHelicopter();
    void AddTargets();
    void AddMissileTrails();
    void AddBullets();
    void AddExplosions();
    void AddCrosshair();
    void AddLockOnIndicator();
    void AddBox(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& size, const DirectX::XMFLOAT4& color, float yaw = 0.0f, float pitch = 0.0f, float roll = 0.0f);
    void AddBoxWithWorld(const DirectX::XMMATRIX& world, const DirectX::XMFLOAT4& color);
    void AddText3D(const std::wstring& text, const DirectX::XMFLOAT3& origin, float unitSize, float depth, const DirectX::XMFLOAT4& color, float yaw = 0.0f, bool centered = true, float glyphSpacing = 0.25f);
    void AddExplodingText3D(const std::wstring& text, const DirectX::XMFLOAT3& origin, float unitSize, float depth, const DirectX::XMFLOAT4& color, float yaw, float explosionTime);

    void SpawnMissileTrail(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& missileDirection);
    void SpawnExplosion(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color, float radius);

    bool HitStartName(int x, int y) const;
    int HitMenuEntry(int x, int y) const;

    void ResetLevel();

    void SetLevelCursorCapture(bool enabled);

    bool IsTargetIndexValid(int targetIndex) const;

    float ScreenConstantScaleAt(const DirectX::XMFLOAT3& position, float scalePerMeter) const;
    float TerrainHeightAt(float worldX, float worldZ) const;

    bool RaycastTerrain(const Collision::Ray& ray, float maxDistance, Collision::HitResult& hit, float heightOffset = 0.0f) const;

    DirectX::XMFLOAT3 LevelCameraPosition() const;
    DirectX::XMFLOAT3 ForwardDirection() const;
    DirectX::XMFLOAT3 MuzzlePosition() const;

    DirectX::XMMATRIX ProjectionMatrix() const;
    DirectX::XMMATRIX LevelViewMatrix() const;

    HWND m_hwnd = nullptr;

    UINT m_width = 1280;
    UINT m_height = 720;

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

    GameCamera m_camera{};

    std::array<MeshResource, static_cast<std::size_t>(MeshType::Count)> m_meshes{};
    std::vector<ApacheMeshPart> m_apacheParts;
    bool m_apacheModelLoaded = false;

    std::vector<DrawItem> m_drawItems;

    Terrain m_terrain;

    std::array<bool, 256> m_keyDown{};
    int m_mouseX = 0;
    int m_mouseY = 0;
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;
    bool m_hasLastMousePosition = false;
    bool m_cursorCaptured = false;

    SceneMode m_scene = SceneMode::Start;
    float m_totalTime = 0.0f;
    bool m_nameExploding = false;
    float m_nameExplosionTime = 0.0f;
    float m_nameExplosionYaw = 0.0f;

    std::vector<MenuEntry> m_menuEntries;

    DirectX::XMFLOAT3 m_helicopterPosition{ 0.0f, 1.3f, -8.0f };
    float m_helicopterYaw = 0.0f;
    float m_helicopterPitch = 0.0f;
    float m_rotorAngle = 0.0f;
    float m_shotCooldown = 0.0f;
    std::vector<Bullet> m_bullets;
    std::vector<Target> m_targets;
    std::vector<Explosion> m_explosions;
    std::array<MissileTrailParticle, MaxMissileTrailParticles> m_missileTrails{};
    std::size_t m_nextMissileTrailIndex = 0;
    int m_lockedTargetIndex = -1;
    bool m_lockPinned = false;

    bool m_crosshairValid = false;
    DirectX::XMFLOAT3 m_crosshairPosition{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 m_aimDirection{ 0.0f, 0.0f, 1.0f };
};
