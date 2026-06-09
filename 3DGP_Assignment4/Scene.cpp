#include "pch.h"
#include "GameManager.h"

#include "Collision.h"

GameScene::GameScene()
{
    ConfigureDefaultMenu();
}

SceneName GameScene::Name() const
{
    return m_name;
}

const std::wstring& GameScene::DisplayName() const
{
    return m_displayName;
}

void GameScene::SetName(SceneName name)
{
    m_name = name;
    switch (m_name)
    {
    case SceneName::Start:
        m_displayName = L"Start";
        break;
    case SceneName::Menu:
        m_displayName = L"Menu";
        break;
    case SceneName::Tutorial:
        m_displayName = L"Tutorial";
        break;
    case SceneName::Level1:
        m_displayName = L"Level 1";
        break;
    case SceneName::Level2:
        m_displayName = L"Level 2";
        break;
    case SceneName::Level3:
        m_displayName = L"Level 3";
        break;
    }
}

bool GameScene::Is(SceneName name) const
{
    return m_name == name;
}

void GameScene::ConfigureDefaultMenu()
{
    menuEntries =
    {
        { L"Tutorial", 1.65f },
        { L"Level-1", 0.90f },
        { L"Level-2", 0.15f },
        { L"Level-3", -0.60f },
        { L"Start", -1.35f },
        { L"End", -2.10f }
    };
}

void GameScene::ApplyPlayerLook(int deltaX, int deltaY)
{
    player.ApplyLook(deltaX, deltaY);
}

GameScene::operator SceneName() const
{
    return m_name;
}

GameScene& GameScene::operator=(SceneName name)
{
    SetName(name);
    return *this;
}


using namespace DirectX;


namespace
{
    constexpr float Pi = 3.1415926535f;

    constexpr float MenuTextUnitSize = 0.066f;
    constexpr float MenuTextDepth = 0.085f;
    constexpr float MenuGlyphSpacing = 0.25f;

    constexpr float MissileHomingDelaySeconds = 0.55f;
    constexpr float MissileTurnRateRadians = 0.70f;
    constexpr float MissileTerrainCollisionRadius = 0.32f;
    constexpr float TankShellTurnRateRadians = 1.25f;
    constexpr float TankShellTerrainCollisionRadius = 0.28f;
    constexpr float TankHitRadius = 4.25f;

    constexpr float MissileTrailSpawnIntervalSeconds = 0.035f;
    constexpr float MissileTrailDurationSeconds = 0.62f;
    constexpr float MissileTrailStartSize = 0.48f;

    constexpr int ExplosionParticleCount = 34;
    constexpr float ExplosionDurationSeconds = 0.85f;

    // 5x7 도트 글자 하나를 표현하는 타입
    using GlyphPattern = std::array<std::string_view, 7>;

    // 아무것도 없을 때 기본 문자 패턴 '?'
    const GlyphPattern FallbackGlyph =
    {
        "11110",
        "00001",
        "00001",
        "01110",
        "00100",
        "00000",
        "00100"
    };

    // 키는 나타낼 문자, 값은 GlyphPattern에 실제로 들어갈 데이터를 행 단위로 나열
    const std::unordered_map<wchar_t, GlyphPattern>& GlyphTable()
    {
        static const std::unordered_map<wchar_t, GlyphPattern> table =
        {
            { L'0', { "11111", "10001", "10011", "10101", "11001", "10001", "11111" } },
            { L'1', { "00100", "01100", "00100", "00100", "00100", "00100", "01110" } },
            { L'2', { "11110", "00001", "00001", "11110", "10000", "10000", "11111" } },
            { L'3', { "11110", "00001", "00001", "01110", "00001", "00001", "11110" } },
            { L'4', { "10010", "10010", "10010", "11111", "00010", "00010", "00010" } },
            { L'5', { "11111", "10000", "10000", "11110", "00001", "00001", "11110" } },
            { L'6', { "01111", "10000", "10000", "11110", "10001", "10001", "01110" } },
            { L'7', { "11111", "00001", "00010", "00100", "01000", "01000", "01000" } },
            { L'8', { "01110", "10001", "10001", "01110", "10001", "10001", "01110" } },
            { L'9', { "01110", "10001", "10001", "01111", "00001", "00001", "11110" } },
            { L'A', { "01110", "10001", "10001", "11111", "10001", "10001", "10001" } },
            { L'B', { "11110", "10001", "10001", "11110", "10001", "10001", "11110" } },
            { L'C', { "01111", "10000", "10000", "10000", "10000", "10000", "01111" } },
            { L'D', { "11110", "10001", "10001", "10001", "10001", "10001", "11110" } },
            { L'E', { "11111", "10000", "10000", "11110", "10000", "10000", "11111" } },
            { L'F', { "11111", "10000", "10000", "11110", "10000", "10000", "10000" } },
            { L'G', { "01111", "10000", "10000", "10011", "10001", "10001", "01111" } },
            { L'H', { "10001", "10001", "10001", "11111", "10001", "10001", "10001" } },
            { L'I', { "11111", "00100", "00100", "00100", "00100", "00100", "11111" } },
            { L'J', { "00111", "00010", "00010", "00010", "10010", "10010", "01100" } },
            { L'K', { "10001", "10010", "10100", "11000", "10100", "10010", "10001" } },
            { L'L', { "10000", "10000", "10000", "10000", "10000", "10000", "11111" } },
            { L'M', { "10001", "11011", "10101", "10101", "10001", "10001", "10001" } },
            { L'N', { "10001", "11001", "10101", "10011", "10001", "10001", "10001" } },
            { L'O', { "01110", "10001", "10001", "10001", "10001", "10001", "01110" } },
            { L'P', { "11110", "10001", "10001", "11110", "10000", "10000", "10000" } },
            { L'Q', { "01110", "10001", "10001", "10001", "10101", "10010", "01101" } },
            { L'R', { "11110", "10001", "10001", "11110", "10100", "10010", "10001" } },
            { L'S', { "01111", "10000", "10000", "01110", "00001", "00001", "11110" } },
            { L'T', { "11111", "00100", "00100", "00100", "00100", "00100", "00100" } },
            { L'U', { "10001", "10001", "10001", "10001", "10001", "10001", "01110" } },
            { L'V', { "10001", "10001", "10001", "10001", "10001", "01010", "00100" } },
            { L'W', { "10001", "10001", "10001", "10101", "10101", "10101", "01010" } },
            { L'X', { "10001", "10001", "01010", "00100", "01010", "10001", "10001" } },
            { L'Y', { "10001", "10001", "01010", "00100", "00100", "00100", "00100" } },
            { L'Z', { "11111", "00001", "00010", "00100", "01000", "10000", "11111" } },
            { L'-', { "00000", "00000", "00000", "11111", "00000", "00000", "00000" } },
            { L':', { "00000", "00100", "00100", "00000", "00100", "00100", "00000" } },
            { L'!', { "00100", "00100", "00100", "00100", "00100", "00000", "00100" } },
            { L'게', { "1111001", "0001001", "0001001", "1111001", "1000001", "1111001", "0000001" } },
            { L'임', { "0111010", "1000110", "1000110", "0111010", "1111110", "1000010", "1111110" } },
            { L'프', { "1111110", "1000010", "1111110", "0000000", "1111111", "0010000", "0010000" } },
            { L'로', { "1111110", "0000010", "1111110", "1000000", "1111111", "0000000", "1111111" } },
            { L'그', { "1111111", "1000000", "1000000", "1111111", "0000000", "1111111", "0010000" } },
            { L'래', { "1111001", "0001001", "1111001", "1000001", "1111001", "1000001", "1111001" } },
            { L'밍', { "0111010", "1000110", "0111010", "1111110", "1000010", "1000010", "1111110" } },
            { L'게', { "1111001", "0001001", "0001001", "1111001", "1000001", "1111001", "0000001" } },
            { L'임', { "0111010", "1000110", "1000110", "0111010", "1111110", "1000010", "1111110" } },
            { L'프', { "1111110", "1000010", "1111110", "0000000", "1111111", "0010000", "0010000" } },
            { L'로', { "1111110", "0000010", "1111110", "1000000", "1111111", "0000000", "1111111" } },
            { L'그', { "1111111", "1000000", "1000000", "1111111", "0000000", "1111111", "0010000" } },
            { L'래', { "1111001", "0001001", "1111001", "1000001", "1111001", "1000001", "1111001" } },
            { L'밍', { "0111010", "1000110", "0111010", "1111110", "1000010", "1000010", "1111110" } },
            { L'이', { "0111110", "0010000", "0010000", "0010000", "0010000", "0010000", "0111110" } },
            { L'름', { "1111110", "1000010", "1111110", "1000000", "1111111", "1000010", "1111110" } }
        };

        return table;
    }

    // 문자를 찾고 없으면 기본 문자 리턴
    const GlyphPattern& FindGlyph(wchar_t ch)
    {
        const auto& table = GlyphTable();
        const auto found = table.find(static_cast<wchar_t>(std::towupper(ch)));
        if (found != table.end())
        {
            return found->second;
        }

        const auto original = table.find(ch);
        if (original != table.end())
        {
            return original->second;
        }

        return FallbackGlyph;
    }

    float GlyphWidth(const GlyphPattern& glyph)
    {
        std::size_t width = 0;
        for (const std::string_view row : glyph)
        {
            width = std::max(width, row.size());
        }

        return static_cast<float>(width);
    }

    // 글씨가 실제 월드에서 차지하는 가로 폭을 계산
    float TextWorldWidth(const std::wstring& text, float unitSize, float glyphSpacing = 0.25f)
    {
        if (text.empty())
        {
            return 0.0f;
        }

        float width = 0.0f;
        for (const wchar_t ch : text)
        {
            width += (ch == L' ') ? 2.2f : GlyphWidth(FindGlyph(ch)) + glyphSpacing;
        }

        return std::max(0.0f, width - glyphSpacing) * unitSize;
    }

    // 폭발 파티클 방향 결정
    XMFLOAT3 BurstDirection(float xBias, float yBias, int seed)
    {
        const float seedValue = static_cast<float>(seed);
        const float xNoise = std::cos(seedValue * 3.91f) * 0.35f;
        const float yNoise = std::sin(seedValue * 2.17f) * 0.28f;
        const float zNoise = std::sin(seedValue * 5.13f + xBias * 7.0f - yBias * 4.0f) * 0.95f;
        const XMVECTOR direction = XMVector3Normalize(XMVectorSet(xBias + xNoise, yBias + yNoise + 0.12f, zNoise, 0.0f));
        XMFLOAT3 result{};
        XMStoreFloat3(&result, direction);
        return result;
    }

    float DistanceSquared(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        const float dz = a.z - b.z;
        return dx * dx + dy * dy + dz * dz;
    }

    // 선형 보간 후 정규화, 완만한 방향 전환에 사용
    XMFLOAT3 TankAimPoint(const Tank& tank, float heightOffset = 1.65f)
    {
        const XMFLOAT3 position = tank.Position();
        return { position.x, position.y + heightOffset, position.z };
    }

    XMFLOAT3 ClampToTerrainBounds(const Terrain& terrain, XMFLOAT3 position, float fallbackHalfSize)
    {
        const float halfWidth = std::max(5.0f, (terrain.HalfWidth() > 0.0f ? terrain.HalfWidth() : fallbackHalfSize) - 8.0f);
        const float halfLength = std::max(5.0f, (terrain.HalfLength() > 0.0f ? terrain.HalfLength() : fallbackHalfSize) - 8.0f);
        position.x = std::clamp(position.x, -halfWidth, halfWidth);
        position.z = std::clamp(position.z, -halfLength, halfLength);
        return position;
    }

    bool AnyKeyDown(const std::array<bool, 256>& keys, std::initializer_list<int> keyCodes)
    {
        for (const int keyCode : keyCodes)
        {
            if (keyCode >= 0 && keyCode < static_cast<int>(keys.size()) && keys[static_cast<std::size_t>(keyCode)])
            {
                return true;
            }
        }
        return false;
    }
}

void GameManager::Update(float deltaSeconds)
{
    switch (m_scene.Name())
    {
    case SceneName::Start:
        UpdateStart(deltaSeconds);
        break;
    case SceneName::Menu:
    case SceneName::Tutorial:
        break;
    case SceneName::Level1:
        UpdateLevel(deltaSeconds);
        break;
    case SceneName::Level2:
        UpdateLevel2(deltaSeconds);
        break;
    case SceneName::Level3:
        UpdateLevel3(deltaSeconds);
        break;
    }
}

void GameManager::UpdateStart(float deltaSeconds)
{
    if (m_scene.titleExploding)
    {
        m_scene.titleExplosionTime += deltaSeconds;
        if (m_scene.titleExplosionTime > 1.25f)
        {
            m_scene = SceneName::Menu;
            m_scene.titleExploding = false;
            m_scene.titleExplosionTime = 0.0f;
        }
    }
}

void GameManager::UpdateLevel(float deltaSeconds)
{
    Player& player = m_scene.player;
    const XMFLOAT3 forward = player.FlatForward();
    const XMFLOAT3 right = player.FlatRight();
    constexpr float moveSpeed = 18.0f * GP_WORLD_UNITS_PER_METER;

    if (m_keyDown['W'])
    {
        player.MoveFlat(forward, moveSpeed * deltaSeconds);
    }
    if (m_keyDown['S'])
    {
        player.MoveFlat(forward, -moveSpeed * deltaSeconds);
    }
    if (m_keyDown['A'])
    {
        player.MoveFlat(right, -moveSpeed * deltaSeconds);
    }
    if (m_keyDown['D'])
    {
        player.MoveFlat(right, moveSpeed * deltaSeconds);
    }

    if (m_keyDown[VK_SPACE])
    {
        player.MoveVertical(moveSpeed * 0.55f * deltaSeconds);
    }
    if (m_keyDown[VK_CONTROL] || m_keyDown[VK_LCONTROL])
    {
        player.MoveVertical(-moveSpeed * 0.55f * deltaSeconds);
    }

    const float movementLimitX = std::max(5.0f, (m_scene.terrain.HalfWidth() > 0.0f ? m_scene.terrain.HalfWidth() : GP_TERRAIN_HALF_SIZE_METERS) - 8.0f);
    const float movementLimitZ = std::max(5.0f, (m_scene.terrain.HalfLength() > 0.0f ? m_scene.terrain.HalfLength() : GP_TERRAIN_HALF_SIZE_METERS) - 8.0f);
    player.ClampHorizontal(movementLimitX, movementLimitZ);

    const XMFLOAT3 playerPosition = player.Position();
    const float terrainHeight = TerrainHeightAt(playerPosition.x, playerPosition.z);
    const float minimumAltitude = terrainHeight + GP_PLAYER_TERRAIN_CLEARANCE_METERS * GP_WORLD_UNITS_PER_METER;
    const float maximumAltitude = minimumAltitude + 85.0f * GP_WORLD_UNITS_PER_METER;
    player.ClampAltitude(minimumAltitude, maximumAltitude);

    for (Enemy& target : m_scene.enemies)
    {
        if (target.IsActive())
        {
            const XMFLOAT3 targetPosition = target.Position();
            target.PlaceOnTerrain(
                TerrainHeightAt(targetPosition.x, targetPosition.z),
                GP_ENEMY_TERRAIN_CLEARANCE_METERS * GP_WORLD_UNITS_PER_METER);
        }
    }

    player.Update(deltaSeconds);

    for (Bullet& bullet : m_scene.bullets)
    {
        if (bullet.HasHomingTarget() && IsTargetIndexValid(bullet.TargetIndex()))
        {
            if (bullet.CanHome())
            {
                const Enemy& target = m_scene.enemies[static_cast<std::size_t>(bullet.TargetIndex())];
                bullet.SteerToward(target.AimPoint(), deltaSeconds, MissileTurnRateRadians);
            }
            else
            {
                bullet.UpdateHomingDelay(deltaSeconds);
            }
        }

        const XMFLOAT3 previousPosition = bullet.Position();
        bullet.Update(deltaSeconds);

        const XMFLOAT3 currentPosition = bullet.Position();
        const float travelDistanceSquared = DistanceSquared(previousPosition, currentPosition);
        if (travelDistanceSquared > 0.000001f)
        {
            const float travelDistance = std::sqrt(travelDistanceSquared);
            const XMFLOAT3 travelDirection = Collision::Normalize(
                {
                    currentPosition.x - previousPosition.x,
                    currentPosition.y - previousPosition.y,
                    currentPosition.z - previousPosition.z
                });
            Collision::HitResult terrainHit{};
            if (RaycastTerrain({ previousPosition, travelDirection }, travelDistance, terrainHit, MissileTerrainCollisionRadius))
            {
                bullet.SetPosition(terrainHit.position);
                bullet.Expire();
                SpawnExplosion(terrainHit.position, { 1.0f, 0.58f, 0.16f, 1.0f }, 5.0f);
            }
        }

        if (!bullet.IsExpired())
        {
            bullet.AccumulateTrail(deltaSeconds);
            const XMFLOAT3 missileDirection = bullet.Direction();
            while (bullet.ConsumeTrailSpawn(MissileTrailSpawnIntervalSeconds))
            {
                SpawnMissileTrail(bullet.TrailPosition(0.55f), missileDirection);
            }
        }
    }
    std::erase_if(m_scene.bullets, [](const Bullet& bullet)
    {
        return bullet.IsExpired();
    });

    for (Bullet& bullet : m_scene.bullets)
    {
        if (bullet.IsExpired())
        {
            continue;
        }

        for (Enemy& target : m_scene.enemies)
        {
            if (target.IsActive() && DistanceSquared(bullet.Position(), target.Position()) < bullet.HitRadius() * bullet.HitRadius())
            {
                const XMFLOAT3 targetPosition = target.Position();
                target.Destroy();
                bullet.Expire();
                SpawnExplosion({ targetPosition.x, targetPosition.y + 0.75f, targetPosition.z }, { 1.0f, 0.42f, 0.10f, 1.0f }, 6.5f);
                break;
            }
        }
    }
    std::erase_if(m_scene.bullets, [](const Bullet& bullet)
    {
        return bullet.IsExpired();
    });

    for (Explosion& explosion : m_scene.explosions)
    {
        explosion.Update(deltaSeconds);
    }
    std::erase_if(m_scene.explosions, [](const Explosion& explosion)
    {
        return explosion.IsExpired();
    });

    for (MissileTrailParticle& particle : m_scene.missileTrails)
    {
        particle.Update(deltaSeconds);
    }

    UpdateAimRay();
    if (ReachedLevelExit())
    {
        SetLevelCursorCapture(false);
        ResetLevel2();
        m_scene = SceneName::Level2;
        return;
    }

    if (!m_scene.enemies.empty())
    {
        bool anyActiveTarget = false;
        for (const Enemy& target : m_scene.enemies)
        {
            if (target.IsActive())
            {
                anyActiveTarget = true;
                break;
            }
        }

        if (!anyActiveTarget)
        {
            SetLevelCursorCapture(false);
            ResetLevel2();
            m_scene = SceneName::Level2;
        }
    }
}

void GameManager::UpdateLevel2(float deltaSeconds)
{
    Tank& playerTank = m_scene.playerTank;
    if (playerTank.IsActive())
    {
        const float moveDistance = GP_TANK_MOVE_SPEED * deltaSeconds;
        if (AnyKeyDown(m_keyDown, { VK_UP, 'W' }))
        {
            playerTank.MoveForward(moveDistance);
        }
        if (AnyKeyDown(m_keyDown, { VK_DOWN, 'S' }))
        {
            playerTank.MoveForward(-moveDistance);
        }
        if (AnyKeyDown(m_keyDown, { VK_LEFT, 'A' }))
        {
            playerTank.RotateYaw(-1.35f * deltaSeconds);
        }
        if (AnyKeyDown(m_keyDown, { VK_RIGHT, 'D' }))
        {
            playerTank.RotateYaw(1.35f * deltaSeconds);
        }

        XMFLOAT3 tankPosition = ClampToTerrainBounds(m_scene.terrain, playerTank.Position(), GP_TERRAIN_HALF_SIZE_METERS);
        tankPosition.y = TerrainHeightAt(tankPosition.x, tankPosition.z) + GP_TANK_TERRAIN_CLEARANCE_METERS * GP_WORLD_UNITS_PER_METER;
        playerTank.SetPosition(tankPosition);
        playerTank.Update(deltaSeconds);
    }

    const XMFLOAT3 playerPosition = playerTank.Position();
    for (Tank& tank : m_scene.enemyTanks)
    {
        if (!tank.IsActive())
        {
            continue;
        }

        XMFLOAT3 tankPosition = ClampToTerrainBounds(m_scene.terrain, tank.Position(), GP_TERRAIN_HALF_SIZE_METERS);
        tankPosition.y = TerrainHeightAt(tankPosition.x, tankPosition.z) + GP_TANK_TERRAIN_CLEARANCE_METERS * GP_WORLD_UNITS_PER_METER;
        tank.SetPosition(tankPosition);
        tank.SetYaw(std::atan2(playerPosition.x - tankPosition.x, playerPosition.z - tankPosition.z));
        tank.Update(deltaSeconds);
    }

    if (!m_scene.level2Win && playerTank.AutoAttackEnabled())
    {
        if (!IsTankIndexValid(m_scene.selectedTankIndex))
        {
            m_scene.selectedTankIndex = NearestActiveTankIndex(playerTank.Position());
        }
        FireTankShellAt(m_scene.selectedTankIndex);
    }

    for (Bullet& bullet : m_scene.bullets)
    {
        if (bullet.HasHomingTarget() && IsTankIndexValid(bullet.TargetIndex()))
        {
            if (bullet.CanHome())
            {
                const Tank& target = m_scene.enemyTanks[static_cast<std::size_t>(bullet.TargetIndex())];
                bullet.SteerToward(TankAimPoint(target), deltaSeconds, TankShellTurnRateRadians);
            }
            else
            {
                bullet.UpdateHomingDelay(deltaSeconds);
            }
        }

        const XMFLOAT3 previousPosition = bullet.Position();
        bullet.Update(deltaSeconds);

        const XMFLOAT3 currentPosition = bullet.Position();
        const float travelDistanceSquared = DistanceSquared(previousPosition, currentPosition);
        if (travelDistanceSquared > 0.000001f)
        {
            const float travelDistance = std::sqrt(travelDistanceSquared);
            const XMFLOAT3 travelDirection = Collision::Normalize(
                {
                    currentPosition.x - previousPosition.x,
                    currentPosition.y - previousPosition.y,
                    currentPosition.z - previousPosition.z
                });
            Collision::HitResult terrainHit{};
            if (RaycastTerrain({ previousPosition, travelDirection }, travelDistance, terrainHit, TankShellTerrainCollisionRadius))
            {
                bullet.SetPosition(terrainHit.position);
                bullet.Expire();
                SpawnExplosion(terrainHit.position, { 1.0f, 0.60f, 0.18f, 1.0f }, 4.5f);
            }
        }

        if (!bullet.IsExpired())
        {
            bullet.AccumulateTrail(deltaSeconds);
            const XMFLOAT3 missileDirection = bullet.Direction();
            while (bullet.ConsumeTrailSpawn(MissileTrailSpawnIntervalSeconds))
            {
                SpawnMissileTrail(bullet.TrailPosition(0.45f), missileDirection);
            }
        }
    }
    std::erase_if(m_scene.bullets, [](const Bullet& bullet)
    {
        return bullet.IsExpired();
    });

    for (Bullet& bullet : m_scene.bullets)
    {
        if (bullet.IsExpired())
        {
            continue;
        }

        for (Tank& target : m_scene.enemyTanks)
        {
            if (!target.IsActive())
            {
                continue;
            }

            if (DistanceSquared(bullet.Position(), TankAimPoint(target, 1.0f)) < TankHitRadius * TankHitRadius)
            {
                const XMFLOAT3 targetPosition = target.Position();
                const bool destroyed = target.Damage(1);
                bullet.Expire();
                SpawnExplosion({ targetPosition.x, targetPosition.y + 1.4f, targetPosition.z }, { 1.0f, 0.38f, 0.08f, 1.0f }, destroyed ? 7.0f : 4.0f);
                break;
            }
        }
    }
    std::erase_if(m_scene.bullets, [](const Bullet& bullet)
    {
        return bullet.IsExpired();
    });

    for (Explosion& explosion : m_scene.explosions)
    {
        explosion.Update(deltaSeconds);
    }
    std::erase_if(m_scene.explosions, [](const Explosion& explosion)
    {
        return explosion.IsExpired();
    });

    for (MissileTrailParticle& particle : m_scene.missileTrails)
    {
        particle.Update(deltaSeconds);
    }

    if (!m_scene.level2Win && AllEnemyTanksDestroyed())
    {
        m_scene.level2Win = true;
    }

    UpdateTankAimRay();
}

void GameManager::UpdateLevel3(float deltaSeconds)
{
    Player& player = m_scene.player;
    const XMFLOAT3 forward = player.FlatForward();
    const XMFLOAT3 right = player.FlatRight();
    constexpr float moveSpeed = 18.0f * GP_WORLD_UNITS_PER_METER;

    if (m_keyDown['W'])
    {
        player.MoveFlat(forward, moveSpeed * deltaSeconds);
    }
    if (m_keyDown['S'])
    {
        player.MoveFlat(forward, -moveSpeed * deltaSeconds);
    }
    if (m_keyDown['A'])
    {
        player.MoveFlat(right, -moveSpeed * deltaSeconds);
    }
    if (m_keyDown['D'])
    {
        player.MoveFlat(right, moveSpeed * deltaSeconds);
    }

    if (m_keyDown[VK_SPACE])
    {
        player.MoveVertical(moveSpeed * 0.55f * deltaSeconds);
    }
    if (m_keyDown[VK_CONTROL] || m_keyDown[VK_LCONTROL])
    {
        player.MoveVertical(-moveSpeed * 0.55f * deltaSeconds);
    }

    const float movementLimitX = std::max(5.0f, (m_scene.terrain.HalfWidth() > 0.0f ? m_scene.terrain.HalfWidth() : GP_TERRAIN_HALF_SIZE_METERS) - 8.0f);
    const float movementLimitZ = std::max(5.0f, (m_scene.terrain.HalfLength() > 0.0f ? m_scene.terrain.HalfLength() : GP_TERRAIN_HALF_SIZE_METERS) - 8.0f);
    player.ClampHorizontal(movementLimitX, movementLimitZ);

    const XMFLOAT3 playerPosition = player.Position();
    const float terrainHeight = TerrainHeightAt(playerPosition.x, playerPosition.z);
    const float minimumAltitude = terrainHeight + GP_PLAYER_TERRAIN_CLEARANCE_METERS * GP_WORLD_UNITS_PER_METER;
    const float maximumAltitude = minimumAltitude + 85.0f * GP_WORLD_UNITS_PER_METER;
    player.ClampAltitude(minimumAltitude, maximumAltitude);
    player.Update(deltaSeconds);

    const XMFLOAT3 updatedPlayerPosition = player.Position();
    for (Tank& tank : m_scene.enemyTanks)
    {
        if (!tank.IsActive())
        {
            continue;
        }

        XMFLOAT3 tankPosition = ClampToTerrainBounds(m_scene.terrain, tank.Position(), GP_TERRAIN_HALF_SIZE_METERS);
        tankPosition.y = TerrainHeightAt(tankPosition.x, tankPosition.z) + GP_TANK_TERRAIN_CLEARANCE_METERS * GP_WORLD_UNITS_PER_METER;
        tank.SetPosition(tankPosition);
        tank.SetYaw(std::atan2(updatedPlayerPosition.x - tankPosition.x, updatedPlayerPosition.z - tankPosition.z));
        tank.Update(deltaSeconds);
    }

    for (Bullet& bullet : m_scene.bullets)
    {
        if (bullet.HasHomingTarget() && IsTankIndexValid(bullet.TargetIndex()))
        {
            if (bullet.CanHome())
            {
                const Tank& target = m_scene.enemyTanks[static_cast<std::size_t>(bullet.TargetIndex())];
                bullet.SteerToward(TankAimPoint(target), deltaSeconds, MissileTurnRateRadians);
            }
            else
            {
                bullet.UpdateHomingDelay(deltaSeconds);
            }
        }

        const XMFLOAT3 previousPosition = bullet.Position();
        bullet.Update(deltaSeconds);

        const XMFLOAT3 currentPosition = bullet.Position();
        const float travelDistanceSquared = DistanceSquared(previousPosition, currentPosition);
        if (travelDistanceSquared > 0.000001f)
        {
            const float travelDistance = std::sqrt(travelDistanceSquared);
            const XMFLOAT3 travelDirection = Collision::Normalize(
                {
                    currentPosition.x - previousPosition.x,
                    currentPosition.y - previousPosition.y,
                    currentPosition.z - previousPosition.z
                });
            Collision::HitResult terrainHit{};
            if (RaycastTerrain({ previousPosition, travelDirection }, travelDistance, terrainHit, MissileTerrainCollisionRadius))
            {
                bullet.SetPosition(terrainHit.position);
                bullet.Expire();
                SpawnExplosion(terrainHit.position, { 1.0f, 0.58f, 0.16f, 1.0f }, 5.0f);
            }
        }

        if (!bullet.IsExpired())
        {
            bullet.AccumulateTrail(deltaSeconds);
            const XMFLOAT3 missileDirection = bullet.Direction();
            while (bullet.ConsumeTrailSpawn(MissileTrailSpawnIntervalSeconds))
            {
                SpawnMissileTrail(bullet.TrailPosition(0.55f), missileDirection);
            }
        }
    }
    std::erase_if(m_scene.bullets, [](const Bullet& bullet)
    {
        return bullet.IsExpired();
    });

    for (Bullet& bullet : m_scene.bullets)
    {
        if (bullet.IsExpired())
        {
            continue;
        }

        for (Tank& target : m_scene.enemyTanks)
        {
            if (target.IsActive() && DistanceSquared(bullet.Position(), TankAimPoint(target, 1.0f)) < TankHitRadius * TankHitRadius)
            {
                const XMFLOAT3 targetPosition = target.Position();
                const bool destroyed = target.Damage(1);
                bullet.Expire();
                SpawnExplosion({ targetPosition.x, targetPosition.y + 1.4f, targetPosition.z }, { 1.0f, 0.42f, 0.10f, 1.0f }, destroyed ? 6.5f : 4.0f);
                break;
            }
        }
    }
    std::erase_if(m_scene.bullets, [](const Bullet& bullet)
    {
        return bullet.IsExpired();
    });

    for (Explosion& explosion : m_scene.explosions)
    {
        explosion.Update(deltaSeconds);
    }
    std::erase_if(m_scene.explosions, [](const Explosion& explosion)
    {
        return explosion.IsExpired();
    });

    for (MissileTrailParticle& particle : m_scene.missileTrails)
    {
        particle.Update(deltaSeconds);
    }

    UpdateTankAimRay();
}

void GameManager::UpdateAimRay()
{
    // 헬리콥터 총구에서 광선 발사
    const float maxAimDistance = std::max({ GP_TERRAIN_HALF_SIZE_METERS, m_scene.terrain.HalfWidth(), m_scene.terrain.HalfLength() }) * 2.0f;
    m_scene.aimDirection = ForwardDirection();
    const Collision::Ray ray{ MuzzlePosition(), m_scene.aimDirection };

    Collision::HitResult bestHit{};
    bestHit.distance = maxAimDistance;
    int hitTargetIndex = -1;
    int lockCandidateIndex = -1;
    float bestLockScore = -1.0f;

    // 가장 가까운 충돌점 선택
    for (std::size_t targetIndex = 0; targetIndex < m_scene.enemies.size(); ++targetIndex)
    {
        const Enemy& target = m_scene.enemies[targetIndex];
        if (!target.IsActive())
        {
            continue;
        }

        const XMFLOAT3 targetPoint = target.AimPoint();
        const XMFLOAT3 toTarget
        {
            targetPoint.x - ray.origin.x,
            targetPoint.y - ray.origin.y,
            targetPoint.z - ray.origin.z
        };
        const float targetDistance = std::sqrt(std::max(0.0001f, DistanceSquared(targetPoint, ray.origin)));
        const XMFLOAT3 targetDirection = Collision::Normalize(toTarget);
        const float aimDot = Collision::Dot(targetDirection, ray.direction);
        if (aimDot > 0.975f && targetDistance < maxAimDistance)
        {
            const float lockScore = aimDot - targetDistance * 0.0008f;
            if (lockScore > bestLockScore)
            {
                bestLockScore = lockScore;
                lockCandidateIndex = static_cast<int>(targetIndex);
            }
        }

        const Collision::HitResult objectHit = Collision::RaycastSphere(ray, targetPoint, 1.9f, bestHit.distance);
        if (objectHit.hit)
        {
            bestHit = objectHit;
            hitTargetIndex = static_cast<int>(targetIndex);
        }
    }

    // 락온 고정 X -> 직접 맞춘 표적 최우선, 락온 고정 중이면 기존 대상 유지
    const int automaticLockIndex = (hitTargetIndex >= 0) ? hitTargetIndex : lockCandidateIndex;
    if (m_scene.lockPinned)
    {
        if (!IsTargetIndexValid(m_scene.lockedTargetIndex))
        {
            m_scene.lockPinned = false;
            m_scene.lockedTargetIndex = automaticLockIndex;
        }
    }
    else
    {
        m_scene.lockedTargetIndex = automaticLockIndex;
    }

    Collision::HitResult terrainHit{};
    if (RaycastTerrain(ray, bestHit.distance, terrainHit))
    {
        bestHit = terrainHit;
    }

    if (!bestHit.hit)
    {
        bestHit.hit = true;
        bestHit.distance = maxAimDistance;
        bestHit.position = Collision::PointAt(ray, maxAimDistance);
    }

    m_scene.crosshairValid = bestHit.hit;
    m_scene.crosshairPosition = bestHit.position;
}

void GameManager::UpdateTankAimRay()
{
    const float maxAimDistance = std::max({ GP_TERRAIN_HALF_SIZE_METERS, m_scene.terrain.HalfWidth(), m_scene.terrain.HalfLength() }) * 2.0f;

    Collision::Ray ray{};
    if (m_scene == SceneName::Level2)
    {
        ray.origin = m_scene.playerTank.FirePoint(4.6f, 1.4f);
        ray.direction = m_scene.playerTank.ForwardDirection();
    }
    else
    {
        ray.origin = MuzzlePosition();
        ray.direction = ForwardDirection();
    }

    m_scene.aimDirection = ray.direction;

    Collision::HitResult bestHit{};
    bestHit.distance = maxAimDistance;
    int hitTargetIndex = -1;
    int lockCandidateIndex = -1;
    float bestLockScore = -1.0f;
    const float lockThreshold = (m_scene == SceneName::Level2) ? 0.925f : 0.965f;

    for (std::size_t tankIndex = 0; tankIndex < m_scene.enemyTanks.size(); ++tankIndex)
    {
        const Tank& target = m_scene.enemyTanks[tankIndex];
        if (!target.IsActive())
        {
            continue;
        }

        const XMFLOAT3 targetPoint = TankAimPoint(target);
        const XMFLOAT3 toTarget
        {
            targetPoint.x - ray.origin.x,
            targetPoint.y - ray.origin.y,
            targetPoint.z - ray.origin.z
        };
        const float targetDistance = std::sqrt(std::max(0.0001f, DistanceSquared(targetPoint, ray.origin)));
        const XMFLOAT3 targetDirection = Collision::Normalize(toTarget);
        const float aimDot = Collision::Dot(targetDirection, ray.direction);
        if (aimDot > lockThreshold && targetDistance < maxAimDistance)
        {
            const float lockScore = aimDot - targetDistance * 0.0008f;
            if (lockScore > bestLockScore)
            {
                bestLockScore = lockScore;
                lockCandidateIndex = static_cast<int>(tankIndex);
            }
        }

        const Collision::HitResult objectHit = Collision::RaycastSphere(ray, targetPoint, TankHitRadius * 0.85f, bestHit.distance);
        if (objectHit.hit)
        {
            bestHit = objectHit;
            hitTargetIndex = static_cast<int>(tankIndex);
        }
    }

    const int automaticTargetIndex = (hitTargetIndex >= 0) ? hitTargetIndex : lockCandidateIndex;
    if (m_scene == SceneName::Level3 || !IsTankIndexValid(m_scene.selectedTankIndex))
    {
        m_scene.selectedTankIndex = automaticTargetIndex;
    }

    Collision::HitResult terrainHit{};
    if (RaycastTerrain(ray, bestHit.distance, terrainHit))
    {
        bestHit = terrainHit;
    }

    if (!bestHit.hit)
    {
        bestHit.hit = true;
        bestHit.distance = maxAimDistance;
        bestHit.position = Collision::PointAt(ray, maxAimDistance);
    }

    m_scene.crosshairValid = bestHit.hit;
    m_scene.crosshairPosition = bestHit.position;
}

void GameManager::FireBulletAtAim()
{
    if (!m_scene.player.CanFire())
    {
        return;
    }

    if (m_scene == SceneName::Level3)
    {
        UpdateTankAimRay();
        const XMFLOAT3 muzzle = MuzzlePosition();
        const XMFLOAT3 launchDirection = ForwardDirection();
        const bool homing = IsTankIndexValid(m_scene.selectedTankIndex);
        const int targetIndex = homing ? m_scene.selectedTankIndex : -1;

        m_scene.bullets.emplace_back(
            muzzle,
            Collision::Scale(launchDirection, 55.0f * GP_WORLD_UNITS_PER_METER),
            10.0f,
            homing,
            targetIndex,
            homing ? MissileHomingDelaySeconds : 0.0f,
            MissileTrailSpawnIntervalSeconds);
        m_scene.player.StartShotCooldown(0.18f);
        return;
    }

    UpdateAimRay();
    const XMFLOAT3 muzzle = MuzzlePosition();
    const XMFLOAT3 launchDirection = ForwardDirection();
    const bool homing = IsTargetIndexValid(m_scene.lockedTargetIndex);
    const int targetIndex = homing ? m_scene.lockedTargetIndex : -1;

    m_scene.bullets.emplace_back(
        muzzle,
        Collision::Scale(launchDirection, 55.0f * GP_WORLD_UNITS_PER_METER),
        10.0f,
        homing,
        targetIndex,
        homing ? MissileHomingDelaySeconds : 0.0f,
        MissileTrailSpawnIntervalSeconds);
    m_scene.player.StartShotCooldown(0.18f);
}

void GameManager::FireTankShellAt(int targetIndex)
{
    if (!IsTankIndexValid(targetIndex) || !m_scene.playerTank.CanFire())
    {
        return;
    }

    const Tank& target = m_scene.enemyTanks[static_cast<std::size_t>(targetIndex)];
    const XMFLOAT3 muzzle = m_scene.playerTank.FirePoint(5.2f, 1.4f);
    const XMFLOAT3 targetPoint = TankAimPoint(target, 1.35f);
    const XMFLOAT3 launchDirection = Collision::Normalize(
        {
            targetPoint.x - muzzle.x,
            targetPoint.y - muzzle.y,
            targetPoint.z - muzzle.z
        });

    m_scene.bullets.emplace_back(
        muzzle,
        Collision::Scale(launchDirection, GP_TANK_SHELL_SPEED),
        8.0f,
        true,
        targetIndex,
        0.08f,
        MissileTrailSpawnIntervalSeconds);
    m_scene.playerTank.StartReload(GP_TANK_FIRE_RELOAD_SECONDS);
}

void GameManager::SpawnMissileTrail(const XMFLOAT3& position, const XMFLOAT3& missileDirection)
{
    MissileTrailParticle& particle = m_scene.missileTrails[m_scene.nextMissileTrailIndex];
    m_scene.nextMissileTrailIndex = (m_scene.nextMissileTrailIndex + 1) % m_scene.missileTrails.size();

    particle.Initialize(position, missileDirection, MissileTrailDurationSeconds, MissileTrailStartSize);
}

void GameManager::SpawnExplosion(const XMFLOAT3& position, const XMFLOAT4& color, float radius)
{
    m_scene.explosions.emplace_back(position, color, ExplosionDurationSeconds, radius);
}

void GameManager::BuildDrawItems()
{
    m_scene.drawItems.clear();
    m_scene.drawItems.reserve(1024);

    switch (m_scene.Name())
    {
    case SceneName::Start:
        BuildStartScene();
        break;
    case SceneName::Menu:
        BuildMenuScene();
        break;
    case SceneName::Tutorial:
        BuildTutorialScene();
        break;
    case SceneName::Level1:
        BuildLevelScene();
        break;
    case SceneName::Level2:
        BuildLevel2Scene();
        break;
    case SceneName::Level3:
        BuildLevel3Scene();
        break;
    }
}

void GameManager::BuildStartScene()
{
    AddText3D(L"3D 게임프로그래밍 1", { 0.0f, 1.35f, 0.0f }, 0.080f, 0.13f, { 0.65f, 0.88f, 1.0f, 1.0f });

    const float nameYaw = m_scene.titleExploding ? m_scene.titleExplosionYaw : m_totalTime * 1.7f;
    if (m_scene.titleExploding)
    {
        AddExplodingText3D(L"JIHOON", { 0.0f, -0.55f, 0.0f }, 0.20f, 0.20f, { 1.0f, 0.82f, 0.20f, 1.0f }, nameYaw, m_scene.titleExplosionTime);
    }
    else
    {
        AddText3D(L"JIHOON", { 0.0f, -0.55f, 0.0f }, 0.20f, 0.20f, { 1.0f, 0.82f, 0.20f, 1.0f }, nameYaw);
    }

    AddText3D(L"CLICK NAME", { 0.0f, -2.1f, 0.0f }, 0.10f, 0.10f, { 0.78f, 0.80f, 0.86f, 1.0f });
}

void GameManager::BuildMenuScene()
{
    AddText3D(L"MENU", { 0.0f, 2.55f, 0.0f }, 0.13f, 0.13f, { 0.85f, 0.95f, 1.0f, 1.0f });

    for (const MenuEntry& entry : m_scene.menuEntries)
    {
        const int hoveredIndex = HitMenuEntry(m_mouseX, m_mouseY);
        const bool hovered = hoveredIndex >= 0 && m_scene.menuEntries[hoveredIndex].label == entry.label;
        const XMFLOAT4 color = hovered ? XMFLOAT4{ 1.0f, 0.82f, 0.25f, 1.0f } : XMFLOAT4{ 0.68f, 0.86f, 0.95f, 1.0f };
        AddText3D(entry.label, { 0.0f, entry.y, 0.0f }, MenuTextUnitSize, MenuTextDepth, color, 0.0f, true, MenuGlyphSpacing);
    }
}

void GameManager::BuildTutorialScene()
{
    AddText3D(L"Tutorial", { 0.0f, 2.35f, 0.0f }, 0.12f, 0.12f, { 0.86f, 0.96f, 1.0f, 1.0f });
    AddText3D(L"Level-1: WASD SPACE CTRL FIRE", { 0.0f, 1.35f, 0.0f }, 0.055f, 0.075f, { 0.72f, 0.90f, 1.0f, 1.0f }, 0.0f, true, 0.18f);
    AddText3D(L"Reach TARGET or press N", { 0.0f, 0.65f, 0.0f }, 0.060f, 0.075f, { 0.80f, 1.0f, 0.72f, 1.0f }, 0.0f, true, 0.18f);
    AddText3D(L"Level-2: WASD ARROWS RIGHTCLICK", { 0.0f, -0.20f, 0.0f }, 0.052f, 0.075f, { 0.95f, 0.90f, 0.65f, 1.0f }, 0.0f, true, 0.18f);
    AddText3D(L"A AUTO  S SHIELD  W WIN", { 0.0f, -0.88f, 0.0f }, 0.062f, 0.075f, { 0.95f, 0.90f, 0.65f, 1.0f }, 0.0f, true, 0.18f);
    AddText3D(L"Level-3: V or 1 3 VIEW", { 0.0f, -1.62f, 0.0f }, 0.062f, 0.075f, { 0.78f, 0.96f, 1.0f, 1.0f }, 0.0f, true, 0.18f);
    AddText3D(L"CLICK TO MENU", { 0.0f, -2.45f, 0.0f }, 0.065f, 0.075f, { 1.0f, 0.82f, 0.25f, 1.0f }, 0.0f, true, 0.18f);
}

void GameManager::BuildLevelScene()
{
    DrawItem terrainItem{};
    terrainItem.mesh = MeshType::Terrain;
    XMStoreFloat4x4(&terrainItem.world, XMMatrixIdentity());
    terrainItem.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_scene.drawItems.push_back(terrainItem);

    // 헬리콥터는 Apache 모델을 우선 사용, 표적과 탄환은 박스
    AddHelicopter();
    AddLevelExitMarker();
    AddTargets();
    AddMissileTrails();
    AddBullets();
    AddExplosions();
    AddCrosshair();
    AddLockOnIndicator();
}

void GameManager::BuildLevel2Scene()
{
    DrawItem terrainItem{};
    terrainItem.mesh = MeshType::Terrain;
    XMStoreFloat4x4(&terrainItem.world, XMMatrixIdentity());
    terrainItem.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_scene.drawItems.push_back(terrainItem);

    AddObstacles();
    AddTank(m_scene.playerTank, { 0.72f, 0.86f, 0.62f, 1.0f });

    for (std::size_t tankIndex = 0; tankIndex < m_scene.enemyTanks.size(); ++tankIndex)
    {
        const bool selected = static_cast<int>(tankIndex) == m_scene.selectedTankIndex;
        AddTank(m_scene.enemyTanks[tankIndex], selected ? XMFLOAT4{ 1.0f, 0.76f, 0.20f, 1.0f } : XMFLOAT4{ 0.78f, 0.22f, 0.16f, 1.0f });
    }

    AddMissileTrails();
    AddBullets();
    AddExplosions();
    AddCrosshair();
    AddLockOnIndicator();

    const XMFLOAT3 tankPosition = m_scene.playerTank.Position();
    const XMFLOAT3 forward = m_scene.playerTank.ForwardDirection();
    if (m_scene.level2Win)
    {
        AddText3D(
            L"YOU WIN!",
            { tankPosition.x + forward.x * 18.0f, tankPosition.y + 8.0f, tankPosition.z + forward.z * 18.0f },
            0.62f,
            0.25f,
            { 1.0f, 0.90f, 0.20f, 1.0f },
            m_scene.playerTank.Yaw(),
            true,
            0.22f);
    }

    if (m_scene.playerTank.AutoAttackEnabled())
    {
        AddText3D(
            L"AUTO",
            { tankPosition.x - forward.x * 5.5f, tankPosition.y + 4.6f, tankPosition.z - forward.z * 5.5f },
            0.24f,
            0.12f,
            { 0.24f, 0.95f, 1.0f, 1.0f },
            m_scene.playerTank.Yaw(),
            true,
            0.20f);
    }
}

void GameManager::BuildLevel3Scene()
{
    DrawItem terrainItem{};
    terrainItem.mesh = MeshType::Terrain;
    XMStoreFloat4x4(&terrainItem.world, XMMatrixIdentity());
    terrainItem.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_scene.drawItems.push_back(terrainItem);

    if (!m_scene.firstPersonHelicopter)
    {
        AddHelicopter();
    }

    for (std::size_t tankIndex = 0; tankIndex < m_scene.enemyTanks.size(); ++tankIndex)
    {
        const bool selected = static_cast<int>(tankIndex) == m_scene.selectedTankIndex;
        AddTank(m_scene.enemyTanks[tankIndex], selected ? XMFLOAT4{ 1.0f, 0.76f, 0.20f, 1.0f } : XMFLOAT4{ 0.78f, 0.22f, 0.16f, 1.0f });
    }

    AddMissileTrails();
    AddBullets();
    AddExplosions();
    AddCrosshair();
    AddLockOnIndicator();

    if (m_scene.firstPersonHelicopter)
    {
        const XMFLOAT3 position = m_scene.player.Position();
        const XMFLOAT3 forward = ForwardDirection();
        AddText3D(
            L"1P",
            { position.x + forward.x * 10.0f, position.y - 1.2f + forward.y * 10.0f, position.z + forward.z * 10.0f },
            0.20f,
            0.08f,
            { 0.78f, 0.96f, 1.0f, 1.0f },
            m_scene.player.Yaw(),
            true,
            0.18f);
    }
}

void GameManager::AddModel(ModelType type, const XMMATRIX& world, const XMFLOAT4& color)
{
    if (!m_assets.HasModel(type))
    {
        return;
    }

    const ModelHandle& model = m_assets.Model(type);
    for (std::size_t localPartIndex = 0; localPartIndex < model.partCount; ++localPartIndex)
    {
        if (m_scene.drawItems.size() >= MaxDrawItems)
        {
            return;
        }

        DrawItem item{};
        item.mesh = MeshType::Model;
        item.meshPartIndex = model.firstPart + localPartIndex;
        XMStoreFloat4x4(&item.world, world);
        item.color = color;
        m_scene.drawItems.push_back(item);
    }
}

void GameManager::AddTank(const Tank& tank, const XMFLOAT4& color)
{
    if (!tank.IsActive())
    {
        return;
    }

    const XMFLOAT3 position = tank.Position();
    if (m_assets.HasModel(ModelType::Tank))
    {
        const XMMATRIX world =
            XMMatrixScaling(GP_TANK_MODEL_SCALE, GP_TANK_MODEL_SCALE, GP_TANK_MODEL_SCALE) *
            XMMatrixRotationY(tank.Yaw()) *
            XMMatrixTranslation(position.x, position.y, position.z);
        AddModel(ModelType::Tank, world, color);
    }
    else
    {
        AddBox(position, { 4.2f, 1.25f, 6.0f }, color, tank.Yaw());
        AddBox({ position.x, position.y + 1.1f, position.z }, { 2.4f, 1.0f, 2.6f }, color, tank.Yaw());
        const XMFLOAT3 forward = tank.ForwardDirection();
        AddBox({ position.x + forward.x * 3.9f, position.y + 1.45f, position.z + forward.z * 3.9f }, { 0.55f, 0.45f, 4.6f }, color, tank.Yaw());
    }

    if (tank.ShieldEnabled())
    {
        AddBox({ position.x, position.y + 2.3f, position.z }, { 7.5f, 0.18f, 7.5f }, { 0.12f, 0.75f, 1.0f, 1.0f }, tank.Yaw() + m_totalTime);
        AddBox({ position.x, position.y + 2.3f, position.z }, { 0.18f, 4.4f, 7.5f }, { 0.12f, 0.75f, 1.0f, 1.0f }, tank.Yaw() + m_totalTime * 0.7f);
        AddBox({ position.x, position.y + 2.3f, position.z }, { 7.5f, 4.4f, 0.18f }, { 0.12f, 0.75f, 1.0f, 1.0f }, tank.Yaw() - m_totalTime * 0.7f);
    }
}

void GameManager::AddObstacles()
{
    for (const Obstacle& obstacle : m_scene.obstacles)
    {
        if (!obstacle.IsActive())
        {
            continue;
        }

        const XMFLOAT3 position = obstacle.Position();
        const ModelType modelType = (obstacle.Variant() % 2 == 0) ? ModelType::Rock : ModelType::Rock2;
        const float scale = GP_ROCK_MODEL_SCALE * obstacle.Radius();
        if (m_assets.HasModel(modelType))
        {
            const XMMATRIX world =
                XMMatrixScaling(scale, scale, scale) *
                XMMatrixRotationY(obstacle.Yaw()) *
                XMMatrixTranslation(position.x, position.y, position.z);
            AddModel(modelType, world, { 0.72f, 0.70f, 0.66f, 1.0f });
        }
        else
        {
            AddBox(position, { scale * 0.55f, scale * 0.45f, scale * 0.55f }, { 0.46f, 0.44f, 0.40f, 1.0f }, obstacle.Yaw(), 0.35f, 0.18f);
        }
    }
}

void GameManager::AddLevelExitMarker()
{
    const XMFLOAT3 center = m_scene.levelExitPosition;
    const float radius = m_scene.levelExitRadius;
    const float thickness = 0.35f;
    const XMFLOAT4 markerColor{ 0.20f, 1.0f, 0.35f, 1.0f };
    const XMFLOAT4 textColor{ 0.76f, 1.0f, 0.72f, 1.0f };

    AddBox({ center.x, center.y + 0.20f, center.z - radius }, { radius * 2.0f, thickness, thickness }, markerColor);
    AddBox({ center.x, center.y + 0.20f, center.z + radius }, { radius * 2.0f, thickness, thickness }, markerColor);
    AddBox({ center.x - radius, center.y + 0.20f, center.z }, { thickness, thickness, radius * 2.0f }, markerColor);
    AddBox({ center.x + radius, center.y + 0.20f, center.z }, { thickness, thickness, radius * 2.0f }, markerColor);
    AddBox({ center.x, center.y + 4.0f, center.z }, { 0.40f, 8.0f, 0.40f }, { 0.18f, 0.88f, 0.28f, 1.0f }, m_totalTime * 1.4f);

    AddText3D(
        L"TARGET",
        { center.x, center.y + 7.0f, center.z },
        0.58f,
        0.20f,
        textColor,
        m_scene.player.Yaw() + Pi,
        true,
        0.22f);
}

void GameManager::AddHelicopter()
{
    if (m_assets.HasModel(ModelType::Apache))
    {
        const ModelHandle& apache = m_assets.Model(ModelType::Apache);
        const XMMATRIX modelWorld = PlayerModelWorldMatrix();
        for (std::size_t localPartIndex = 0; localPartIndex < apache.partCount; ++localPartIndex)
        {
            if (m_scene.drawItems.size() >= MaxDrawItems)
            {
                return;
            }

            const std::size_t partIndex = apache.firstPart + localPartIndex;
            const ModelMeshPart& part = m_assets.modelParts[partIndex];
            XMMATRIX partAnimation = XMMatrixIdentity();
            if (part.mainRotor)
            {
                partAnimation =
                    XMMatrixTranslation(-part.center.x, -part.center.y, -part.center.z) *
                    XMMatrixRotationY(m_scene.player.RotorAngle() * 1.8f) *
                    XMMatrixTranslation(part.center.x, part.center.y, part.center.z);
            }
            else if (part.tailRotor)
            {
                partAnimation =
                    XMMatrixTranslation(-part.center.x, -part.center.y, -part.center.z) *
                    XMMatrixRotationX(m_scene.player.RotorAngle() * 3.2f) *
                    XMMatrixTranslation(part.center.x, part.center.y, part.center.z);
            }

            DrawItem item{};
            item.mesh = MeshType::Model;
            item.meshPartIndex = partIndex;
            XMStoreFloat4x4(&item.world, partAnimation * modelWorld);
            item.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            m_scene.drawItems.push_back(item);
        }
        return;
    }

    const XMFLOAT3 playerPosition = m_scene.player.Position();
    const XMMATRIX parent =
        XMMatrixRotationRollPitchYaw(-m_scene.player.Pitch() * 0.45f, m_scene.player.Yaw(), 0.0f) *
        XMMatrixTranslation(playerPosition.x, playerPosition.y, playerPosition.z);

    auto addPart = [this, parent](const XMFLOAT3& localPosition, const XMFLOAT3& size, const XMFLOAT4& color, const XMMATRIX& localRotation = XMMatrixIdentity())
    {
        const XMMATRIX world = XMMatrixScaling(size.x, size.y, size.z) * localRotation * XMMatrixTranslation(localPosition.x, localPosition.y, localPosition.z) * parent;
        AddBoxWithWorld(world, color);
    };

    // apache 로드 실패 시 사용할 모델 데이터
    addPart({ 0.0f, 0.0f, 0.0f }, { 1.15f, 0.45f, 1.85f }, { 0.12f, 0.38f, 0.92f, 1.0f });
    addPart({ 0.0f, 0.08f, 0.65f }, { 0.82f, 0.38f, 0.65f }, { 0.45f, 0.88f, 1.0f, 1.0f });
    addPart({ 0.0f, 0.03f, -1.55f }, { 0.24f, 0.24f, 1.75f }, { 0.10f, 0.22f, 0.58f, 1.0f });
    addPart({ 0.0f, 0.02f, -2.55f }, { 0.48f, 0.48f, 0.12f }, { 0.16f, 0.28f, 0.70f, 1.0f });

    addPart({ 0.0f, 0.48f, 0.0f }, { 3.25f, 0.05f, 0.14f }, { 0.95f, 0.95f, 0.98f, 1.0f }, XMMatrixRotationY(m_scene.player.RotorAngle()));
    addPart({ 0.0f, 0.49f, 0.0f }, { 0.14f, 0.05f, 3.25f }, { 0.95f, 0.95f, 0.98f, 1.0f }, XMMatrixRotationY(m_scene.player.RotorAngle()));
    addPart({ 0.34f, 0.02f, -2.72f }, { 0.08f, 0.85f, 0.10f }, { 0.95f, 0.92f, 0.75f, 1.0f }, XMMatrixRotationX(m_scene.player.RotorAngle() * 1.7f));
    addPart({ 0.34f, 0.02f, -2.72f }, { 0.08f, 0.10f, 0.85f }, { 0.95f, 0.92f, 0.75f, 1.0f }, XMMatrixRotationX(m_scene.player.RotorAngle() * 1.7f));
    addPart({ 0.0f, -0.02f, 1.35f }, { 0.18f, 0.18f, 0.50f }, { 0.08f, 0.08f, 0.10f, 1.0f });

    addPart({ -0.48f, -0.48f, 0.1f }, { 0.10f, 0.08f, 1.65f }, { 0.08f, 0.10f, 0.18f, 1.0f });
    addPart({ 0.48f, -0.48f, 0.1f }, { 0.10f, 0.08f, 1.65f }, { 0.08f, 0.10f, 0.18f, 1.0f });
    addPart({ -0.35f, -0.27f, 0.55f }, { 0.08f, 0.42f, 0.08f }, { 0.08f, 0.10f, 0.18f, 1.0f });
    addPart({ 0.35f, -0.27f, 0.55f }, { 0.08f, 0.42f, 0.08f }, { 0.08f, 0.10f, 0.18f, 1.0f });
    addPart({ -0.35f, -0.27f, -0.55f }, { 0.08f, 0.42f, 0.08f }, { 0.08f, 0.10f, 0.18f, 1.0f });
    addPart({ 0.35f, -0.27f, -0.55f }, { 0.08f, 0.42f, 0.08f }, { 0.08f, 0.10f, 0.18f, 1.0f });
}

XMMATRIX GameManager::PlayerModelWorldMatrix() const
{
    const XMFLOAT3 playerPosition = m_scene.player.Position();
    return
        XMMatrixScaling(GP_APACHE_MODEL_SCALE, GP_APACHE_MODEL_SCALE, GP_APACHE_MODEL_SCALE) *
        XMMatrixRotationRollPitchYaw(-m_scene.player.Pitch() * 0.45f, m_scene.player.Yaw(), 0.0f) *
        XMMatrixTranslation(playerPosition.x, playerPosition.y, playerPosition.z);
}

void GameManager::AddTargets()
{
    for (const Enemy& target : m_scene.enemies)
    {
        if (!target.IsActive())
        {
            continue;
        }

        AddBox(target.Position(), { 1.0f, 1.6f, 1.0f }, { 0.86f, 0.18f, 0.16f, 1.0f }, m_totalTime * 0.5f);
    }
}

void GameManager::AddMissileTrails()
{
    for (const MissileTrailParticle& particle : m_scene.missileTrails)
    {
        if (!particle.IsActive())
        {
            continue;
        }

        const float t = std::clamp(particle.ElapsedSeconds() / std::max(0.0001f, particle.DurationSeconds()), 0.0f, 1.0f);
        const float size = particle.StartSize() * (1.0f - t);
        if (size <= 0.03f)
        {
            continue;
        }

        const float gray = 0.38f + 0.18f * (1.0f - t);
        AddBox(particle.Position(), { size, size, size }, { gray, gray, gray, 1.0f }, t * 2.0f, t * 1.3f, t * 1.7f);
    }
}

void GameManager::AddBullets()
{
    for (const Bullet& bullet : m_scene.bullets)
    {
        const XMFLOAT3 direction = bullet.Direction();
        const float yaw = std::atan2(direction.x, direction.z);
        const float pitch = std::asin(std::clamp(direction.y, -1.0f, 1.0f));
        const XMFLOAT3 position = bullet.Position();
        const float visualScale = ScreenConstantScaleAt(position, 0.012f);
        const XMFLOAT4 color = bullet.IsHoming() ? XMFLOAT4{ 1.0f, 0.38f, 0.12f, 1.0f } : XMFLOAT4{ 1.0f, 0.95f, 0.35f, 1.0f };
        AddBox(position, { visualScale * 0.45f, visualScale * 0.45f, visualScale * 1.20f }, color, yaw, -pitch, 0.0f);
    }
}

void GameManager::AddExplosions()
{
    for (const Explosion& explosion : m_scene.explosions)
    {
        const float t = std::clamp(explosion.ElapsedSeconds() / std::max(0.0001f, explosion.DurationSeconds()), 0.0f, 1.0f);
        const float burst = 1.0f - (1.0f - t) * (1.0f - t);
        const float fadeScale = std::max(0.0f, 1.0f - t);
        const XMFLOAT3 explosionPosition = explosion.Position();
        const XMFLOAT4 explosionColor = explosion.Color();

        for (int particleIndex = 0; particleIndex < ExplosionParticleCount; ++particleIndex)
        {
            const float seed = static_cast<float>(particleIndex);
            const XMFLOAT3 direction = BurstDirection(std::cos(seed * 0.73f), std::sin(seed * 1.11f), particleIndex + 17);
            const float distanceScale = explosion.Radius() * (0.35f + static_cast<float>(particleIndex % 7) * 0.08f) * burst;
            const XMFLOAT3 position
            {
                explosionPosition.x + direction.x * distanceScale,
                explosionPosition.y + direction.y * distanceScale,
                explosionPosition.z + direction.z * distanceScale
            };
            const float particleSize = std::max(0.14f, explosion.Radius() * 0.075f * fadeScale);
            const XMFLOAT4 color
            {
                std::min(1.0f, explosionColor.x + static_cast<float>(particleIndex % 3) * 0.08f),
                std::max(0.18f, explosionColor.y * (0.72f + fadeScale * 0.28f)),
                std::max(0.04f, explosionColor.z * fadeScale),
                1.0f
            };
            AddBox(position, { particleSize, particleSize, particleSize }, color, seed * 0.37f + t * 5.0f, seed * 0.19f + t * 4.0f, seed * 0.23f);
        }
    }
}

void GameManager::AddCrosshair()
{
    if (!m_scene.crosshairValid)
    {
        return;
    }

    const XMFLOAT3 p{ m_scene.crosshairPosition.x, m_scene.crosshairPosition.y + 0.05f, m_scene.crosshairPosition.z };
    const float markerSize = ScreenConstantScaleAt(p, 0.035f);
    const float markerThickness = std::max(0.10f, markerSize * 0.08f);
    AddBox(p, { markerSize, markerThickness, markerThickness }, { 1.0f, 0.12f, 0.10f, 1.0f });
    AddBox(p, { markerThickness, markerSize, markerThickness }, { 1.0f, 0.12f, 0.10f, 1.0f });
    AddBox(p, { markerThickness, markerThickness, markerSize }, { 1.0f, 0.12f, 0.10f, 1.0f });
}

void GameManager::AddLockOnIndicator()
{
    if (m_scene == SceneName::Level2 || m_scene == SceneName::Level3)
    {
        if (!IsTankIndexValid(m_scene.selectedTankIndex))
        {
            return;
        }

        const Tank& target = m_scene.enemyTanks[static_cast<std::size_t>(m_scene.selectedTankIndex)];
        const XMFLOAT3 center = TankAimPoint(target, 2.2f);
        const float size = ScreenConstantScaleAt(center, 0.050f);
        const float thickness = std::max(0.12f, size * 0.075f);
        const float half = size * 0.65f;
        const float segment = size * 0.42f;
        const XMFLOAT4 lockColor{ 1.0f, 0.86f, 0.08f, 1.0f };

        AddBox({ center.x - half, center.y + half, center.z }, { thickness, segment, thickness }, lockColor);
        AddBox({ center.x - half + segment * 0.5f, center.y + half, center.z }, { segment, thickness, thickness }, lockColor);
        AddBox({ center.x + half, center.y + half, center.z }, { thickness, segment, thickness }, lockColor);
        AddBox({ center.x + half - segment * 0.5f, center.y + half, center.z }, { segment, thickness, thickness }, lockColor);
        AddBox({ center.x - half, center.y - half, center.z }, { thickness, segment, thickness }, lockColor);
        AddBox({ center.x - half + segment * 0.5f, center.y - half, center.z }, { segment, thickness, thickness }, lockColor);
        AddBox({ center.x + half, center.y - half, center.z }, { thickness, segment, thickness }, lockColor);
        AddBox({ center.x + half - segment * 0.5f, center.y - half, center.z }, { segment, thickness, thickness }, lockColor);
        return;
    }

    if (!IsTargetIndexValid(m_scene.lockedTargetIndex))
    {
        return;
    }

    const Enemy& target = m_scene.enemies[static_cast<std::size_t>(m_scene.lockedTargetIndex)];
    const XMFLOAT3 center = target.AimPoint(1.15f);
    const float size = ScreenConstantScaleAt(center, 0.050f);
    const float thickness = std::max(0.12f, size * 0.075f);
    const float half = size * 0.65f;
    const float segment = size * 0.42f;
    const XMFLOAT4 lockColor = m_scene.lockPinned ? XMFLOAT4{ 0.18f, 0.96f, 1.0f, 1.0f } : XMFLOAT4{ 1.0f, 0.86f, 0.08f, 1.0f };

    AddBox({ center.x - half, center.y + half, center.z }, { thickness, segment, thickness }, lockColor);
    AddBox({ center.x - half + segment * 0.5f, center.y + half, center.z }, { segment, thickness, thickness }, lockColor);
    AddBox({ center.x + half, center.y + half, center.z }, { thickness, segment, thickness }, lockColor);
    AddBox({ center.x + half - segment * 0.5f, center.y + half, center.z }, { segment, thickness, thickness }, lockColor);
    AddBox({ center.x - half, center.y - half, center.z }, { thickness, segment, thickness }, lockColor);
    AddBox({ center.x - half + segment * 0.5f, center.y - half, center.z }, { segment, thickness, thickness }, lockColor);
    AddBox({ center.x + half, center.y - half, center.z }, { thickness, segment, thickness }, lockColor);
    AddBox({ center.x + half - segment * 0.5f, center.y - half, center.z }, { segment, thickness, thickness }, lockColor);
}

void GameManager::AddBox(const XMFLOAT3& center, const XMFLOAT3& size, const XMFLOAT4& color, float yaw, float pitch, float roll)
{
    const XMMATRIX world = XMMatrixScaling(size.x, size.y, size.z) * XMMatrixRotationRollPitchYaw(pitch, yaw, roll) * XMMatrixTranslation(center.x, center.y, center.z);
    AddBoxWithWorld(world, color);
}

void GameManager::AddBoxWithWorld(const XMMATRIX& world, const XMFLOAT4& color)
{
    if (m_scene.drawItems.size() >= MaxDrawItems)
    {
        return;
    }

    DrawItem item{};
    item.mesh = MeshType::Cube;
    XMStoreFloat4x4(&item.world, world);
    item.color = color;
    m_scene.drawItems.push_back(item);
}

void GameManager::AddText3D(const std::wstring& text, const XMFLOAT3& origin, float unitSize, float depth, const XMFLOAT4& color, float yaw, bool centered, float glyphSpacing)
{
    float totalUnits = 0.0f;
    for (wchar_t ch : text)
    {
        totalUnits += (ch == L' ') ? 2.2f : GlyphWidth(FindGlyph(ch)) + glyphSpacing;
    }
    if (!text.empty())
    {
        totalUnits -= glyphSpacing;
    }

    const float startX = centered ? -totalUnits * unitSize * 0.5f : 0.0f;
    const XMMATRIX parent = XMMatrixRotationY(yaw) * XMMatrixTranslation(origin.x, origin.y, origin.z);
    float cursor = 0.0f;

    for (wchar_t ch : text)
    {
        if (ch == L' ')
        {
            cursor += 2.2f;
            continue;
        }

        const GlyphPattern& glyph = FindGlyph(ch);
        for (int row = 0; row < 7; ++row)
        {
            const int glyphWidth = static_cast<int>(glyph[row].size());
            for (int col = 0; col < glyphWidth; ++col)
            {
                if (glyph[row][col] != '1')
                {
                    continue;
                }

                const float localX = startX + (cursor + static_cast<float>(col)) * unitSize;
                const float localY = (3.0f - static_cast<float>(row)) * unitSize;
                const XMMATRIX world = XMMatrixScaling(unitSize * 1.04f, unitSize * 1.04f, depth) * XMMatrixTranslation(localX, localY, 0.0f) * parent;
                AddBoxWithWorld(world, color);
            }
        }

        cursor += GlyphWidth(glyph) + glyphSpacing;
    }
}

void GameManager::AddExplodingText3D(const std::wstring& text, const XMFLOAT3& origin, float unitSize, float depth, const XMFLOAT4& color, float yaw, float explosionTime)
{
    // 폭발 애니메이션은 각 글자 블록이 중심에서 바깥으로 멀어지게
    float totalUnits = 0.0f;
    for (wchar_t ch : text)
    {
        totalUnits += (ch == L' ') ? 2.2f : GlyphWidth(FindGlyph(ch)) + 0.25f;
    }
    if (!text.empty())
    {
        totalUnits -= 0.25f;
    }

    const float startX = -totalUnits * unitSize * 0.5f;
    const float t = std::clamp(explosionTime / 1.1f, 0.0f, 1.0f);
    const XMMATRIX parent = XMMatrixRotationY(yaw) * XMMatrixTranslation(origin.x, origin.y, origin.z);
    float cursor = 0.0f;

    for (wchar_t ch : text)
    {
        if (ch == L' ')
        {
            cursor += 2.2f;
            continue;
        }

        const GlyphPattern& glyph = FindGlyph(ch);
        for (int row = 0; row < 7; ++row)
        {
            const int glyphWidth = static_cast<int>(glyph[row].size());
            for (int col = 0; col < glyphWidth; ++col)
            {
                if (glyph[row][col] != '1')
                {
                    continue;
                }

                const float localX = startX + (cursor + static_cast<float>(col)) * unitSize;
                const float localY = (3.0f - static_cast<float>(row)) * unitSize;
                const XMFLOAT3 direction = BurstDirection(localX, localY + 0.15f, row * 37 + col * 11 + static_cast<int>(cursor * 13.0f));
                const float burst = t * t * 3.0f;
                const XMMATRIX spin = XMMatrixRotationRollPitchYaw(t * row, t * col, t * (row + col));
                const XMMATRIX world =
                    XMMatrixScaling(unitSize * 1.04f, unitSize * 1.04f, depth) *
                    spin *
                    XMMatrixTranslation(localX + direction.x * burst, localY + direction.y * burst, direction.z * burst) *
                    parent;
                AddBoxWithWorld(world, color);
            }
        }

        cursor += GlyphWidth(glyph) + 0.25f;
    }
}

bool GameManager::HitStartName(int x, int y) const
{
    const float nx = static_cast<float>(x) / static_cast<float>(std::max(1u, m_width));
    const float ny = static_cast<float>(y) / static_cast<float>(std::max(1u, m_height));
    return nx >= 0.30f && nx <= 0.70f && ny >= 0.45f && ny <= 0.63f;
}

int GameManager::HitMenuEntry(int x, int y) const
{
    const float mouseX = static_cast<float>(x) / static_cast<float>(std::max(1u, m_width));
    const float mouseY = static_cast<float>(y) / static_cast<float>(std::max(1u, m_height));
    const XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, -8.5f, 1.0f), XMVectorZero(), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    const XMMATRIX viewProjection = view * ProjectionMatrix();

    const auto projectToScreen = [viewProjection](const XMFLOAT3& worldPosition)
    {
        const XMVECTOR projected = XMVector3TransformCoord(XMLoadFloat3(&worldPosition), viewProjection);
        return XMFLOAT2
        {
            (XMVectorGetX(projected) + 1.0f) * 0.5f,
            (1.0f - XMVectorGetY(projected)) * 0.5f
        };
    };

    for (std::size_t i = 0; i < m_scene.menuEntries.size(); ++i)
    {
        const MenuEntry& entry = m_scene.menuEntries[i];
        const float halfWidth = TextWorldWidth(entry.label, MenuTextUnitSize, MenuGlyphSpacing) * 0.5f + MenuTextUnitSize * 0.6f;
        const float halfHeight = 7.0f * MenuTextUnitSize * 0.5f + MenuTextUnitSize * 0.6f;
        const XMFLOAT2 topLeft = projectToScreen({ -halfWidth, entry.y + halfHeight, 0.0f });
        const XMFLOAT2 bottomRight = projectToScreen({ halfWidth, entry.y - halfHeight, 0.0f });
        const float minX = std::min(topLeft.x, bottomRight.x);
        const float maxX = std::max(topLeft.x, bottomRight.x);
        const float minY = std::min(topLeft.y, bottomRight.y);
        const float maxY = std::max(topLeft.y, bottomRight.y);

        if (mouseX >= minX && mouseX <= maxX && mouseY >= minY && mouseY <= maxY)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

void GameManager::ResetLevel()
{
    const auto placeOnTerrain = [this](float x, float z, float clearanceMeters)
    {
        return XMFLOAT3
        {
            x,
            TerrainHeightAt(x, z) + clearanceMeters * GP_WORLD_UNITS_PER_METER,
            z
        };
    };

    const float usableHalfX = std::max(80.0f * GP_WORLD_UNITS_PER_METER, (m_scene.terrain.HalfWidth() > 0.0f ? m_scene.terrain.HalfWidth() : GP_TERRAIN_HALF_SIZE_METERS) - 24.0f * GP_WORLD_UNITS_PER_METER);
    const float usableHalfZ = std::max(120.0f * GP_WORLD_UNITS_PER_METER, (m_scene.terrain.HalfLength() > 0.0f ? m_scene.terrain.HalfLength() : GP_TERRAIN_HALF_SIZE_METERS) - 24.0f * GP_WORLD_UNITS_PER_METER);
    m_scene.player.Reset(placeOnTerrain(0.0f, -usableHalfZ * 0.45f, GP_PLAYER_TERRAIN_CLEARANCE_METERS));
    m_scene.levelExitPosition = placeOnTerrain(0.0f, usableHalfZ * 0.82f, 0.25f);
    m_scene.levelExitRadius = 13.5f * GP_WORLD_UNITS_PER_METER;
    m_scene.crosshairValid = false;
    m_scene.lockedTargetIndex = -1;
    m_scene.lockPinned = false;
    m_hasLastMousePosition = false;
    m_scene.bullets.clear();
    m_scene.explosions.clear();
    for (MissileTrailParticle& particle : m_scene.missileTrails)
    {
        particle.Deactivate();
    }
    m_scene.nextMissileTrailIndex = 0;
    m_scene.enemies.clear();
    m_scene.enemyTanks.clear();
    m_scene.obstacles.clear();
    m_scene.selectedTankIndex = -1;
    m_scene.level2Win = false;
    m_scene.firstPersonHelicopter = false;
    m_scene.enemies.reserve(static_cast<std::size_t>(std::max(0, GP_LEVEL_TARGET_COUNT)));
    for (int targetIndex = 0; targetIndex < GP_LEVEL_TARGET_COUNT; ++targetIndex)
    {
        const float normalizedIndex = (GP_LEVEL_TARGET_COUNT <= 1) ? 0.0f : static_cast<float>(targetIndex) / static_cast<float>(GP_LEVEL_TARGET_COUNT - 1);
        const float angle = static_cast<float>(targetIndex) * 2.399963f;
        const float spread = 0.28f + normalizedIndex * 0.60f;
        const float x = std::sin(angle) * usableHalfX * 0.72f * spread;
        const float z = usableHalfZ * (-0.04f + normalizedIndex * 0.78f) + std::cos(angle) * usableHalfZ * 0.07f;
        m_scene.enemies.emplace_back(placeOnTerrain(x, z, GP_ENEMY_TERRAIN_CLEARANCE_METERS));
    }
    UpdateAimRay();
}

void GameManager::ResetLevel2()
{
    const auto placeOnTerrain = [this](float x, float z, float clearanceMeters)
    {
        return XMFLOAT3
        {
            x,
            TerrainHeightAt(x, z) + clearanceMeters * GP_WORLD_UNITS_PER_METER,
            z
        };
    };

    const float usableHalfX = std::max(80.0f * GP_WORLD_UNITS_PER_METER, (m_scene.terrain.HalfWidth() > 0.0f ? m_scene.terrain.HalfWidth() : GP_TERRAIN_HALF_SIZE_METERS) - 24.0f * GP_WORLD_UNITS_PER_METER);
    const float usableHalfZ = std::max(120.0f * GP_WORLD_UNITS_PER_METER, (m_scene.terrain.HalfLength() > 0.0f ? m_scene.terrain.HalfLength() : GP_TERRAIN_HALF_SIZE_METERS) - 24.0f * GP_WORLD_UNITS_PER_METER);

    m_scene.playerTank.Reset(placeOnTerrain(0.0f, -usableHalfZ * 0.55f, GP_TANK_TERRAIN_CLEARANCE_METERS), 0.0f, 3);
    m_scene.crosshairValid = false;
    m_scene.lockedTargetIndex = -1;
    m_scene.lockPinned = false;
    m_scene.selectedTankIndex = -1;
    m_scene.level2Win = false;
    m_scene.firstPersonHelicopter = false;
    m_leftMouseDragging = false;
    m_hasLastMousePosition = false;

    m_scene.bullets.clear();
    m_scene.explosions.clear();
    m_scene.enemies.clear();
    m_scene.enemyTanks.clear();
    m_scene.obstacles.clear();
    for (MissileTrailParticle& particle : m_scene.missileTrails)
    {
        particle.Deactivate();
    }
    m_scene.nextMissileTrailIndex = 0;

    m_scene.enemyTanks.reserve(static_cast<std::size_t>(std::max(0, GP_LEVEL2_ENEMY_TANK_COUNT)));
    for (int tankIndex = 0; tankIndex < GP_LEVEL2_ENEMY_TANK_COUNT; ++tankIndex)
    {
        const float normalizedIndex = static_cast<float>(tankIndex) / static_cast<float>(std::max(1, GP_LEVEL2_ENEMY_TANK_COUNT - 1));
        const float angle = static_cast<float>(tankIndex) * 2.399963f;
        const float laneOffset = (static_cast<float>(tankIndex % 4) - 1.5f) * usableHalfX * 0.28f;
        const float x = laneOffset + std::sin(angle) * usableHalfX * 0.08f;
        const float z = usableHalfZ * (-0.08f + normalizedIndex * 0.82f) + std::cos(angle) * usableHalfZ * 0.08f;
        const XMFLOAT3 position = placeOnTerrain(x, z, GP_TANK_TERRAIN_CLEARANCE_METERS);
        const float yaw = std::atan2(-position.x, -usableHalfZ * 0.55f - position.z);
        m_scene.enemyTanks.emplace_back(position, yaw, 1);
    }

    m_scene.obstacles.reserve(static_cast<std::size_t>(std::max(0, GP_LEVEL2_OBSTACLE_COUNT)));
    for (int obstacleIndex = 0; obstacleIndex < GP_LEVEL2_OBSTACLE_COUNT; ++obstacleIndex)
    {
        const float normalizedIndex = static_cast<float>(obstacleIndex) / static_cast<float>(std::max(1, GP_LEVEL2_OBSTACLE_COUNT - 1));
        const float angle = static_cast<float>(obstacleIndex) * 2.399963f + 0.6f;
        const float ring = 0.22f + 0.70f * std::fmod(static_cast<float>(obstacleIndex) * 0.618034f, 1.0f);
        float x = std::sin(angle) * usableHalfX * ring;
        float z = usableHalfZ * (-0.34f + normalizedIndex * 1.12f) + std::cos(angle) * usableHalfZ * 0.16f;
        if (std::fabs(x) < 12.0f && z < -usableHalfZ * 0.35f)
        {
            x += (x < 0.0f ? -1.0f : 1.0f) * 18.0f;
        }
        z = std::clamp(z, -usableHalfZ * 0.42f, usableHalfZ * 0.92f);

        const float radius = 0.45f + static_cast<float>(obstacleIndex % 5) * 0.10f;
        const float yaw = angle + static_cast<float>(obstacleIndex % 7) * 0.31f;
        m_scene.obstacles.emplace_back(placeOnTerrain(x, z, GP_OBSTACLE_TERRAIN_CLEARANCE_METERS), yaw, radius, obstacleIndex);
    }

    m_scene.crosshairValid = false;
}

void GameManager::ResetLevel3()
{
    const auto placeOnTerrain = [this](float x, float z, float clearanceMeters)
    {
        return XMFLOAT3
        {
            x,
            TerrainHeightAt(x, z) + clearanceMeters * GP_WORLD_UNITS_PER_METER,
            z
        };
    };

    const float usableHalfX = std::max(80.0f * GP_WORLD_UNITS_PER_METER, (m_scene.terrain.HalfWidth() > 0.0f ? m_scene.terrain.HalfWidth() : GP_TERRAIN_HALF_SIZE_METERS) - 24.0f * GP_WORLD_UNITS_PER_METER);
    const float usableHalfZ = std::max(120.0f * GP_WORLD_UNITS_PER_METER, (m_scene.terrain.HalfLength() > 0.0f ? m_scene.terrain.HalfLength() : GP_TERRAIN_HALF_SIZE_METERS) - 24.0f * GP_WORLD_UNITS_PER_METER);

    m_scene.player.Reset(placeOnTerrain(0.0f, -usableHalfZ * 0.48f, GP_PLAYER_TERRAIN_CLEARANCE_METERS));
    m_scene.player.SetYaw(0.0f);
    m_scene.crosshairValid = false;
    m_scene.lockedTargetIndex = -1;
    m_scene.lockPinned = false;
    m_scene.selectedTankIndex = -1;
    m_scene.level2Win = false;
    m_scene.firstPersonHelicopter = false;
    m_hasLastMousePosition = false;

    m_scene.bullets.clear();
    m_scene.explosions.clear();
    m_scene.enemies.clear();
    m_scene.enemyTanks.clear();
    m_scene.obstacles.clear();
    for (MissileTrailParticle& particle : m_scene.missileTrails)
    {
        particle.Deactivate();
    }
    m_scene.nextMissileTrailIndex = 0;

    m_scene.enemyTanks.reserve(static_cast<std::size_t>(std::max(0, GP_LEVEL3_ENEMY_TANK_COUNT)));
    for (int tankIndex = 0; tankIndex < GP_LEVEL3_ENEMY_TANK_COUNT; ++tankIndex)
    {
        const float normalizedIndex = static_cast<float>(tankIndex) / static_cast<float>(std::max(1, GP_LEVEL3_ENEMY_TANK_COUNT - 1));
        const float angle = static_cast<float>(tankIndex) * 2.399963f + 0.35f;
        const float spread = 0.32f + normalizedIndex * 0.56f;
        const float x = std::sin(angle) * usableHalfX * 0.70f * spread;
        const float z = usableHalfZ * (-0.02f + normalizedIndex * 0.80f) + std::cos(angle) * usableHalfZ * 0.08f;
        const XMFLOAT3 position = placeOnTerrain(x, z, GP_TANK_TERRAIN_CLEARANCE_METERS);
        const float yaw = std::atan2(-position.x, -usableHalfZ * 0.48f - position.z);
        m_scene.enemyTanks.emplace_back(position, yaw, 1);
    }

    m_scene.crosshairValid = false;
}

bool GameManager::IsTargetIndexValid(int targetIndex) const
{
    if (targetIndex < 0 || targetIndex >= static_cast<int>(m_scene.enemies.size()))
    {
        return false;
    }

    return m_scene.enemies[static_cast<std::size_t>(targetIndex)].IsActive();
}

bool GameManager::IsTankIndexValid(int targetIndex) const
{
    if (targetIndex < 0 || targetIndex >= static_cast<int>(m_scene.enemyTanks.size()))
    {
        return false;
    }

    return m_scene.enemyTanks[static_cast<std::size_t>(targetIndex)].IsActive();
}

int GameManager::PickTankAtScreen(int x, int y) const
{
    if (m_scene != SceneName::Level2 && m_scene != SceneName::Level3)
    {
        return -1;
    }

    const XMMATRIX view = (m_scene == SceneName::Level2) ? Level2ViewMatrix() : Level3ViewMatrix();
    const Collision::Ray ray = ScreenRay(x, y, view);
    const float maxPickDistance = std::max({ GP_TERRAIN_HALF_SIZE_METERS, m_scene.terrain.HalfWidth(), m_scene.terrain.HalfLength() }) * 2.5f;

    int pickedTankIndex = -1;
    float nearestDistance = maxPickDistance;
    for (std::size_t tankIndex = 0; tankIndex < m_scene.enemyTanks.size(); ++tankIndex)
    {
        const Tank& tank = m_scene.enemyTanks[tankIndex];
        if (!tank.IsActive())
        {
            continue;
        }

        const Collision::HitResult hit = Collision::RaycastSphere(ray, TankAimPoint(tank), TankHitRadius, nearestDistance);
        if (hit.hit)
        {
            nearestDistance = hit.distance;
            pickedTankIndex = static_cast<int>(tankIndex);
        }
    }

    return pickedTankIndex;
}

int GameManager::NearestActiveTankIndex(const XMFLOAT3& origin) const
{
    int nearestIndex = -1;
    float nearestDistance = std::numeric_limits<float>::max();
    for (std::size_t tankIndex = 0; tankIndex < m_scene.enemyTanks.size(); ++tankIndex)
    {
        const Tank& tank = m_scene.enemyTanks[tankIndex];
        if (!tank.IsActive())
        {
            continue;
        }

        const float distance = DistanceSquared(origin, tank.Position());
        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestIndex = static_cast<int>(tankIndex);
        }
    }
    return nearestIndex;
}

bool GameManager::AllEnemyTanksDestroyed() const
{
    if (m_scene.enemyTanks.empty())
    {
        return false;
    }

    for (const Tank& tank : m_scene.enemyTanks)
    {
        if (tank.IsActive())
        {
            return false;
        }
    }
    return true;
}

bool GameManager::ReachedLevelExit() const
{
    const XMFLOAT3 playerPosition = m_scene.player.Position();
    const XMFLOAT3 exitPosition = m_scene.levelExitPosition;
    const float dx = playerPosition.x - exitPosition.x;
    const float dz = playerPosition.z - exitPosition.z;
    return dx * dx + dz * dz <= m_scene.levelExitRadius * m_scene.levelExitRadius;
}

float GameManager::ScreenConstantScaleAt(const XMFLOAT3& position, float scalePerMeter) const
{
    XMFLOAT3 cameraPosition = LevelCameraPosition();
    if (m_scene == SceneName::Level2)
    {
        cameraPosition = TankCameraPosition();
    }
    else if (m_scene == SceneName::Level3 && m_scene.firstPersonHelicopter)
    {
        cameraPosition = m_scene.player.Position();
    }
    const float distance = std::sqrt(std::max(0.0001f, DistanceSquared(position, cameraPosition)));
    return std::clamp(distance * scalePerMeter, 0.35f, 8.0f);
}

float GameManager::TerrainHeightAt(float worldX, float worldZ) const
{
    return m_scene.terrain.HeightAt(worldX, worldZ);
}

bool GameManager::RaycastTerrain(const Collision::Ray& ray, float maxDistance, Collision::HitResult& hit, float heightOffset) const
{
    hit = {};
    if (maxDistance <= 0.0f)
    {
        return false;
    }

    if (m_scene.terrain.Empty())
    {
        hit = Collision::RaycastPlaneY(ray, heightOffset, maxDistance);
        if (hit.hit)
        {
            hit.position.y = 0.0f;
        }
        return hit.hit;
    }

    const auto isInsideTerrain = [this](float worldX, float worldZ)
    {
        return m_scene.terrain.Contains(worldX, worldZ);
    };

    const auto sampleDelta = [this, heightOffset, &isInsideTerrain](const XMFLOAT3& point, float& delta)
    {
        if (!isInsideTerrain(point.x, point.z))
        {
            return false;
        }

        delta = point.y - (TerrainHeightAt(point.x, point.z) + heightOffset);
        return true;
    };

    const float step = std::max(0.35f, std::min(m_scene.terrain.CellX(), m_scene.terrain.CellZ()) * 0.5f);
    float previousDistance = 0.0f;
    XMFLOAT3 previousPoint = ray.origin;
    float previousDelta = 0.0f;
    bool hasPrevious = sampleDelta(previousPoint, previousDelta);
    if (hasPrevious && previousDelta <= 0.0f)
    {
        hit.hit = true;
        hit.distance = 0.0f;
        hit.position = previousPoint;
        hit.position.y = TerrainHeightAt(previousPoint.x, previousPoint.z);
        return true;
    }

    for (float distance = std::min(step, maxDistance); ; distance = std::min(distance + step, maxDistance))
    {
        const XMFLOAT3 point = Collision::PointAt(ray, distance);
        float delta = 0.0f;
        const bool hasSample = sampleDelta(point, delta);
        if (hasPrevious && hasSample && previousDelta >= 0.0f && delta <= 0.0f)
        {
            float low = previousDistance;
            float high = distance;
            for (int iteration = 0; iteration < 10; ++iteration)
            {
                const float mid = (low + high) * 0.5f;
                const XMFLOAT3 midPoint = Collision::PointAt(ray, mid);
                float midDelta = 0.0f;
                if (!sampleDelta(midPoint, midDelta) || midDelta > 0.0f)
                {
                    low = mid;
                }
                else
                {
                    high = mid;
                }
            }

            hit.hit = true;
            hit.distance = high;
            hit.position = Collision::PointAt(ray, high);
            hit.position.y = TerrainHeightAt(hit.position.x, hit.position.z);
            return true;
        }

        if (hasSample)
        {
            if (delta <= 0.0f)
            {
                hit.hit = true;
                hit.distance = distance;
                hit.position = point;
                hit.position.y = TerrainHeightAt(point.x, point.z);
                return true;
            }

            previousDistance = distance;
            previousPoint = point;
            previousDelta = delta;
            hasPrevious = true;
        }
        else
        {
            hasPrevious = false;
        }

        if (distance >= maxDistance)
        {
            break;
        }
    }
    return false;
}

Collision::Ray GameManager::ScreenRay(int x, int y, const XMMATRIX& view) const
{
    const float width = static_cast<float>(std::max(1u, m_width));
    const float height = static_cast<float>(std::max(1u, m_height));
    const float ndcX = (static_cast<float>(x) / width) * 2.0f - 1.0f;
    const float ndcY = 1.0f - (static_cast<float>(y) / height) * 2.0f;

    const XMMATRIX inverseViewProjection = XMMatrixInverse(nullptr, view * ProjectionMatrix());
    const XMVECTOR nearPoint = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 0.0f, 1.0f), inverseViewProjection);
    const XMVECTOR farPoint = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 1.0f, 1.0f), inverseViewProjection);
    const XMVECTOR direction = XMVector3Normalize(farPoint - nearPoint);

    Collision::Ray ray{};
    XMStoreFloat3(&ray.origin, nearPoint);
    XMStoreFloat3(&ray.direction, direction);
    return ray;
}

XMFLOAT3 GameManager::LevelCameraPosition() const
{
    return m_camera.LevelCameraPosition(m_scene.player.Position(), m_scene.player.Yaw());
}

XMFLOAT3 GameManager::TankCameraPosition() const
{
    const XMFLOAT3 tankPosition = m_scene.playerTank.Position();
    const XMFLOAT3 forward = m_scene.playerTank.ForwardDirection();
    return
    {
        tankPosition.x - forward.x * 18.0f,
        tankPosition.y + 10.0f,
        tankPosition.z - forward.z * 18.0f
    };
}

XMFLOAT3 GameManager::ForwardDirection() const
{
    return m_scene.player.ForwardDirection();
}

XMFLOAT3 GameManager::MuzzlePosition() const
{
    const XMFLOAT3 forward = ForwardDirection();
    const XMFLOAT3 playerPosition = m_scene.player.Position();
    const float muzzleOffset = m_assets.HasModel(ModelType::Apache) ? GP_APACHE_MUZZLE_OFFSET_METERS * GP_WORLD_UNITS_PER_METER : 1.55f;
    return
    {
        playerPosition.x + forward.x * muzzleOffset,
        playerPosition.y + 0.02f + forward.y * 0.25f,
        playerPosition.z + forward.z * muzzleOffset
    };
}
