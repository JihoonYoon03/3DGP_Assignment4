#pragma once

#include "Camera.h"
#include "Collision.h"
#include "GameConfig.h"
#include "GameFramework.h"
#include "Assets.h"
#include "Scene.h"

class GameManager : public GameFramework
{
public:
    GameManager() = default;
    ~GameManager() override;

    void Initialize(HWND hwnd, UINT width, UINT height);
    void Tick(float deltaSeconds);
    void OnResize(UINT width, UINT height);

    void OnKeyDown(WPARAM key);
    void OnKeyUp(WPARAM key);

    void OnMouseMove(int x, int y);
    void OnMouseDown(int x, int y);
    void OnRightMouseDown(int x, int y);

private:
    void Render();

    void CreateMeshResources();
    DirectX::XMMATRIX PlayerModelWorldMatrix() const;

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

    Camera m_camera{};
    GameAssets m_assets{};
    GameScene m_scene{};

    std::array<bool, 256> m_keyDown{};
    int m_mouseX = 0;
    int m_mouseY = 0;
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;
    bool m_hasLastMousePosition = false;
    bool m_cursorCaptured = false;

    float m_totalTime = 0.0f;
};
