    #pragma once

#include "GameObject.h"
#include "Terrain.h"

enum class SceneName
{
    Start,
    Menu,
    Level1
};

class GameScene
{
public:
    static constexpr std::size_t MaxMissileTrailParticles = 384;

    GameScene();

    SceneName Name() const;
    const std::wstring& DisplayName() const;
    void SetName(SceneName name);
    bool Is(SceneName name) const;
    void ConfigureDefaultMenu();
    void ApplyPlayerLook(int deltaX, int deltaY);

    operator SceneName() const;
    GameScene& operator=(SceneName name);

    std::vector<MenuEntry> menuEntries;
    std::vector<DrawItem> drawItems;

    Terrain terrain{};
    Player player{};
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<Explosion> explosions;
    std::array<MissileTrailParticle, MaxMissileTrailParticles> missileTrails{};
    std::size_t nextMissileTrailIndex = 0;

    int lockedTargetIndex = -1;
    bool lockPinned = false;

    bool crosshairValid = false;
    DirectX::XMFLOAT3 crosshairPosition{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 aimDirection{ 0.0f, 0.0f, 1.0f };

    bool titleExploding = false;
    float titleExplosionTime = 0.0f;
    float titleExplosionYaw = 0.0f;

private:
    SceneName m_name = SceneName::Start;
    std::wstring m_displayName = L"Start";
};
