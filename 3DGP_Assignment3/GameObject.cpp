#include "AssignmentGame.h"

#include "Collision.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cwctype>
#include <limits>
#include <string_view>
#include <unordered_map>

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

    XMFLOAT3 AddVector(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    XMFLOAT3 ScaleVector(const XMFLOAT3& v, float scale)
    {
        return { v.x * scale, v.y * scale, v.z * scale };
    }

    float DistanceSquared(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        const float dz = a.z - b.z;
        return dx * dx + dy * dy + dz * dz;
    }

    // 선형 보간 후 정규화, 완만한 방향 전환에 사용
    XMFLOAT3 BlendDirection(const XMFLOAT3& from, const XMFLOAT3& to, float amount)
    {
        const float t = std::clamp(amount, 0.0f, 1.0f);
        return Collision::Normalize(
            {
                from.x + (to.x - from.x) * t,
                from.y + (to.y - from.y) * t,
                from.z + (to.z - from.z) * t
            });
    }

}

void AssignmentGame::Update(float deltaSeconds)
{
    switch (m_scene)
    {
    case SceneMode::Start:
        UpdateStart(deltaSeconds);
        break;
    case SceneMode::Menu:
        break;
    case SceneMode::Level1:
        UpdateLevel(deltaSeconds);
        break;
    }
}

void AssignmentGame::UpdateStart(float deltaSeconds)
{
    if (m_nameExploding)
    {
        m_nameExplosionTime += deltaSeconds;
        if (m_nameExplosionTime > 1.25f)
        {
            m_scene = SceneMode::Menu;
            m_nameExploding = false;
            m_nameExplosionTime = 0.0f;
        }
    }
}

void AssignmentGame::UpdateLevel(float deltaSeconds)
{
    // 이동은 yaw 방향 기준, pitch는 마우스 조준에만 사용.
    const XMFLOAT3 forward{ std::sinf(m_helicopterYaw), 0.0f, std::cosf(m_helicopterYaw) };
    const XMFLOAT3 right{ std::cosf(m_helicopterYaw), 0.0f, -std::sinf(m_helicopterYaw) };
    constexpr float moveSpeed = 18.0f * GP_WORLD_UNITS_PER_METER;

    // 헬리콥터 수평 이동
    if (m_keyDown['W'])
    {
        m_helicopterPosition = AddVector(m_helicopterPosition, ScaleVector(forward, moveSpeed * deltaSeconds));
    }
    if (m_keyDown['S'])
    {
        m_helicopterPosition = AddVector(m_helicopterPosition, ScaleVector(forward, -moveSpeed * deltaSeconds));
    }
    if (m_keyDown['A'])
    {
        m_helicopterPosition = AddVector(m_helicopterPosition, ScaleVector(right, -moveSpeed * deltaSeconds));
    }
    if (m_keyDown['D'])
    {
        m_helicopterPosition = AddVector(m_helicopterPosition, ScaleVector(right, moveSpeed * deltaSeconds));
    }

    // Space 상승, Ctrl 하강
    if (m_keyDown[VK_SPACE])
    {
        m_helicopterPosition.y += moveSpeed * 0.55f * deltaSeconds;
    }
    if (m_keyDown[VK_CONTROL] || m_keyDown[VK_LCONTROL])
    {
        m_helicopterPosition.y -= moveSpeed * 0.55f * deltaSeconds;
    }

    // 지형 바깥으로 너무 멀리 나가지 않도록 위치 제한
    const float movementLimitX = std::max(5.0f, (m_terrain.HalfWidth() > 0.0f ? m_terrain.HalfWidth() : GP_TERRAIN_HALF_SIZE_METERS) - 8.0f);
    const float movementLimitZ = std::max(5.0f, (m_terrain.HalfLength() > 0.0f ? m_terrain.HalfLength() : GP_TERRAIN_HALF_SIZE_METERS) - 8.0f);
    m_helicopterPosition.x = std::clamp(m_helicopterPosition.x, -movementLimitX, movementLimitX);
    m_helicopterPosition.z = std::clamp(m_helicopterPosition.z, -movementLimitZ, movementLimitZ);

    // 하이트 맵에서 현재 x/z 위치의 지형 높이를 구해 헬기가 지면 아래로 내려가지 않게 조정
    const float terrainHeight = TerrainHeightAt(m_helicopterPosition.x, m_helicopterPosition.z);
    const float minimumAltitude = terrainHeight + GP_PLAYER_TERRAIN_CLEARANCE_METERS * GP_WORLD_UNITS_PER_METER;
    const float maximumAltitude = minimumAltitude + 85.0f * GP_WORLD_UNITS_PER_METER;
    m_helicopterPosition.y = std::clamp(m_helicopterPosition.y, minimumAltitude, maximumAltitude);

    // 적 객체도 하이트맵 높이에 맞춰 지면 위로 조정
    for (Target& target : m_targets)
    {
        if (target.active)
        {
            target.position.y = TerrainHeightAt(target.position.x, target.position.z) + GP_ENEMY_TERRAIN_CLEARANCE_METERS * GP_WORLD_UNITS_PER_METER;
        }
    }

    m_rotorAngle += 22.0f * deltaSeconds;

    // 발사는 마우스 클릭에서 처리, 여기선 쿨다운만 줄이기
    m_shotCooldown = std::max(0.0f, m_shotCooldown - deltaSeconds);

    // 미사일 이동
    for (Bullet& bullet : m_bullets)
    {
        const float currentSpeed = std::sqrt(std::max(0.0001f, DistanceSquared(bullet.velocity, { 0.0f, 0.0f, 0.0f })));
        if (bullet.homing && IsTargetIndexValid(bullet.targetIndex))
        {
            // 발사 직후에는 헬기 전방으로 사출
            if (bullet.homingDelaySeconds > 0.0f)
            {
                bullet.homingDelaySeconds = std::max(0.0f, bullet.homingDelaySeconds - deltaSeconds);
            }
            else
            {
                const Target& target = m_targets[static_cast<std::size_t>(bullet.targetIndex)];
                const XMFLOAT3 targetPoint{ target.position.x, target.position.y + 0.65f, target.position.z };
                const XMFLOAT3 desiredDirection = Collision::Normalize(
                    {
                        targetPoint.x - bullet.position.x,
                        targetPoint.y - bullet.position.y,
                        targetPoint.z - bullet.position.z
                    });
                const XMFLOAT3 currentDirection = Collision::Normalize(bullet.velocity);
                const float dot = std::clamp(Collision::Dot(currentDirection, desiredDirection), -1.0f, 1.0f);
                const float angle = std::acos(dot);
                const float blend = (angle <= 0.0001f) ? 1.0f : std::min(1.0f, (MissileTurnRateRadians * deltaSeconds) / angle);
                const XMFLOAT3 newDirection = BlendDirection(currentDirection, desiredDirection, blend);
                bullet.velocity = ScaleVector(newDirection, currentSpeed);
            }
        }

        const XMFLOAT3 previousPosition = bullet.position;
        bullet.position = AddVector(bullet.position, ScaleVector(bullet.velocity, deltaSeconds));
        bullet.lifeSeconds -= deltaSeconds;

        // 이번 프레임의 이동 구간을 검사해서 지형에 충돌하도록 계산
        const float travelDistanceSquared = DistanceSquared(previousPosition, bullet.position);
        if (travelDistanceSquared > 0.000001f)
        {
            const float travelDistance = std::sqrt(travelDistanceSquared);
            const XMFLOAT3 travelDirection = Collision::Normalize(
                {
                    bullet.position.x - previousPosition.x,
                    bullet.position.y - previousPosition.y,
                    bullet.position.z - previousPosition.z
                });
            Collision::HitResult terrainHit{};
            if (RaycastTerrain({ previousPosition, travelDirection }, travelDistance, terrainHit, MissileTerrainCollisionRadius))
            {
                bullet.position = terrainHit.position;
                bullet.lifeSeconds = 0.0f;
                SpawnExplosion(terrainHit.position, { 1.0f, 0.58f, 0.16f, 1.0f }, 5.0f);
            }
        }

        // 미사일 뒤쪽에 회색 큐브로 트레일 이펙트를 남김
        if (bullet.lifeSeconds > 0.0f)
        {
            bullet.trailSpawnAccumulator += deltaSeconds;
            const XMFLOAT3 missileDirection = Collision::Normalize(bullet.velocity);
            while (bullet.trailSpawnAccumulator >= MissileTrailSpawnIntervalSeconds)
            {
                bullet.trailSpawnAccumulator -= MissileTrailSpawnIntervalSeconds;
                const XMFLOAT3 trailPosition
                {
                    bullet.position.x - missileDirection.x * 0.55f,
                    bullet.position.y - missileDirection.y * 0.55f,
                    bullet.position.z - missileDirection.z * 0.55f
                };
                SpawnMissileTrail(trailPosition, missileDirection);
            }
        }
    }
    std::erase_if(m_bullets, [](const Bullet& bullet)
    {
        return bullet.lifeSeconds <= 0.0f;
    });

    // 탄환/표적 충돌 검사
    for (Bullet& bullet : m_bullets)
    {
        if (bullet.lifeSeconds <= 0.0f)
        {
            continue;
        }

        for (Target& target : m_targets)
        {
            const float hitRadius = bullet.homing ? 4.0f : 1.8f;
            if (target.active && DistanceSquared(bullet.position, target.position) < hitRadius * hitRadius)
            {
                target.active = false;
                bullet.lifeSeconds = 0.0f;
                SpawnExplosion({ target.position.x, target.position.y + 0.75f, target.position.z }, { 1.0f, 0.42f, 0.10f, 1.0f }, 6.5f);
                break;
            }
        }
    }
    std::erase_if(m_bullets, [](const Bullet& bullet)
    {
        return bullet.lifeSeconds <= 0.0f;
    });

    // 폭발 파티클 수명 갱신, 끝났으면 제거
    for (Explosion& explosion : m_explosions)
    {
        explosion.elapsedSeconds += deltaSeconds;
    }
    std::erase_if(m_explosions, [](const Explosion& explosion)
    {
        return explosion.elapsedSeconds >= explosion.durationSeconds;
    });

    // 미사일 트레일 이펙트는 재사용
    for (MissileTrailParticle& particle : m_missileTrails)
    {
        if (!particle.active)
        {
            continue;
        }

        particle.elapsedSeconds += deltaSeconds;
        if (particle.elapsedSeconds >= particle.durationSeconds)
        {
            particle.active = false;
            continue;
        }

        particle.position = AddVector(particle.position, ScaleVector(particle.velocity, deltaSeconds));
    }

    // 총구 광선이 맞는 지형/오브젝트 위치 갱신
    UpdateAimRay();
}

void AssignmentGame::UpdateAimRay()
{
    // 헬리콥터 총구에서 광선 발사
    const float maxAimDistance = std::max({ GP_TERRAIN_HALF_SIZE_METERS, m_terrain.HalfWidth(), m_terrain.HalfLength() }) * 2.0f;
    m_aimDirection = ForwardDirection();
    const Collision::Ray ray{ MuzzlePosition(), m_aimDirection };

    Collision::HitResult bestHit{};
    bestHit.distance = maxAimDistance;
    int hitTargetIndex = -1;
    int lockCandidateIndex = -1;
    float bestLockScore = -1.0f;

    // 가장 가까운 충돌점 선택
    for (std::size_t targetIndex = 0; targetIndex < m_targets.size(); ++targetIndex)
    {
        const Target& target = m_targets[targetIndex];
        if (!target.active)
        {
            continue;
        }

        const XMFLOAT3 targetPoint{ target.position.x, target.position.y + 0.65f, target.position.z };
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
    if (m_lockPinned)
    {
        if (!IsTargetIndexValid(m_lockedTargetIndex))
        {
            m_lockPinned = false;
            m_lockedTargetIndex = automaticLockIndex;
        }
    }
    else
    {
        m_lockedTargetIndex = automaticLockIndex;
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

    m_crosshairValid = bestHit.hit;
    m_crosshairPosition = bestHit.position;
}

void AssignmentGame::FireBulletAtAim()
{
    if (m_shotCooldown > 0.0f)
    {
        return;
    }

    UpdateAimRay();
    const XMFLOAT3 muzzle = MuzzlePosition();
    const XMFLOAT3 launchDirection = ForwardDirection();

    Bullet bullet{};
    bullet.position = muzzle;
    bullet.velocity = Collision::Scale(launchDirection, 55.0f * GP_WORLD_UNITS_PER_METER);
    bullet.lifeSeconds = 10.0f;
    bullet.homing = IsTargetIndexValid(m_lockedTargetIndex);
    bullet.targetIndex = bullet.homing ? m_lockedTargetIndex : -1;
    bullet.homingDelaySeconds = bullet.homing ? MissileHomingDelaySeconds : 0.0f;
    bullet.trailSpawnAccumulator = MissileTrailSpawnIntervalSeconds;
    m_bullets.push_back(bullet);
    m_shotCooldown = 0.18f;
}

void AssignmentGame::SpawnMissileTrail(const XMFLOAT3& position, const XMFLOAT3& missileDirection)
{
    MissileTrailParticle& particle = m_missileTrails[m_nextMissileTrailIndex];
    m_nextMissileTrailIndex = (m_nextMissileTrailIndex + 1) % m_missileTrails.size();

    particle.position = position;
    particle.velocity =
    {
        -missileDirection.x * 3.0f,
        -missileDirection.y * 3.0f + 0.25f,
        -missileDirection.z * 3.0f
    };
    particle.elapsedSeconds = 0.0f;
    particle.durationSeconds = MissileTrailDurationSeconds;
    particle.startSize = MissileTrailStartSize;
    particle.active = true;
}

void AssignmentGame::SpawnExplosion(const XMFLOAT3& position, const XMFLOAT4& color, float radius)
{
    Explosion explosion{};
    explosion.position = position;
    explosion.color = color;
    explosion.elapsedSeconds = 0.0f;
    explosion.durationSeconds = ExplosionDurationSeconds;
    explosion.radius = radius;
    m_explosions.push_back(explosion);
}

void AssignmentGame::BuildDrawItems()
{
    m_drawItems.clear();
    m_drawItems.reserve(1024);

    switch (m_scene)
    {
    case SceneMode::Start:
        BuildStartScene();
        break;
    case SceneMode::Menu:
        BuildMenuScene();
        break;
    case SceneMode::Level1:
        BuildLevelScene();
        break;
    }
}

void AssignmentGame::BuildStartScene()
{
    AddText3D(L"3D GAME PROGRAMMING 1", { 0.0f, 1.35f, 0.0f }, 0.090f, 0.13f, { 0.65f, 0.88f, 1.0f, 1.0f });

    const float nameYaw = m_nameExploding ? m_nameExplosionYaw : m_totalTime * 1.7f;
    if (m_nameExploding)
    {
        AddExplodingText3D(L"PLAY", { 0.0f, -0.55f, 0.0f }, 0.20f, 0.20f, { 1.0f, 0.82f, 0.20f, 1.0f }, nameYaw, m_nameExplosionTime);
    }
    else
    {
        AddText3D(L"PLAY", { 0.0f, -0.55f, 0.0f }, 0.20f, 0.20f, { 1.0f, 0.82f, 0.20f, 1.0f }, nameYaw);
    }

    AddText3D(L"CLICK PLAY", { 0.0f, -2.1f, 0.0f }, 0.10f, 0.10f, { 0.78f, 0.80f, 0.86f, 1.0f });
}

void AssignmentGame::BuildMenuScene()
{
    AddText3D(L"MENU", { 0.0f, 2.55f, 0.0f }, 0.13f, 0.13f, { 0.85f, 0.95f, 1.0f, 1.0f });

    for (const MenuEntry& entry : m_menuEntries)
    {
        const int hoveredIndex = HitMenuEntry(m_mouseX, m_mouseY);
        const bool hovered = hoveredIndex >= 0 && m_menuEntries[hoveredIndex].label == entry.label;
        const XMFLOAT4 color = hovered ? XMFLOAT4{ 1.0f, 0.82f, 0.25f, 1.0f } : XMFLOAT4{ 0.68f, 0.86f, 0.95f, 1.0f };
        AddText3D(entry.label, { 0.0f, entry.y, 0.0f }, MenuTextUnitSize, MenuTextDepth, color, 0.0f, true, MenuGlyphSpacing);
    }
}

void AssignmentGame::BuildLevelScene()
{
    DrawItem terrainItem{};
    terrainItem.mesh = MeshType::Terrain;
    XMStoreFloat4x4(&terrainItem.world, XMMatrixIdentity());
    terrainItem.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_drawItems.push_back(terrainItem);

    // 헬리콥터는 Apache 모델을 우선 사용, 표적과 탄환은 박스
    AddHelicopter();
    AddTargets();
    AddMissileTrails();
    AddBullets();
    AddExplosions();
    AddCrosshair();
    AddLockOnIndicator();
}

void AssignmentGame::AddHelicopter()
{
    if (m_apacheModelLoaded)
    {
        const XMMATRIX modelWorld = ApacheModelWorldMatrix();
        for (std::size_t partIndex = 0; partIndex < m_apacheParts.size(); ++partIndex)
        {
            if (m_drawItems.size() >= MaxDrawItems)
            {
                return;
            }

            const ApacheMeshPart& part = m_apacheParts[partIndex];
            XMMATRIX partAnimation = XMMatrixIdentity();
            if (part.mainRotor)
            {
                partAnimation =
                    XMMatrixTranslation(-part.center.x, -part.center.y, -part.center.z) *
                    XMMatrixRotationY(m_rotorAngle * 1.8f) *
                    XMMatrixTranslation(part.center.x, part.center.y, part.center.z);
            }
            else if (part.tailRotor)
            {
                partAnimation =
                    XMMatrixTranslation(-part.center.x, -part.center.y, -part.center.z) *
                    XMMatrixRotationX(m_rotorAngle * 3.2f) *
                    XMMatrixTranslation(part.center.x, part.center.y, part.center.z);
            }

            DrawItem item{};
            item.mesh = MeshType::Apache;
            item.meshPartIndex = partIndex;
            XMStoreFloat4x4(&item.world, partAnimation * modelWorld);
            item.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            m_drawItems.push_back(item);
        }
        return;
    }

    const XMMATRIX parent = XMMatrixRotationRollPitchYaw(-m_helicopterPitch * 0.45f, m_helicopterYaw, 0.0f) * XMMatrixTranslation(m_helicopterPosition.x, m_helicopterPosition.y, m_helicopterPosition.z);

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

    addPart({ 0.0f, 0.48f, 0.0f }, { 3.25f, 0.05f, 0.14f }, { 0.95f, 0.95f, 0.98f, 1.0f }, XMMatrixRotationY(m_rotorAngle));
    addPart({ 0.0f, 0.49f, 0.0f }, { 0.14f, 0.05f, 3.25f }, { 0.95f, 0.95f, 0.98f, 1.0f }, XMMatrixRotationY(m_rotorAngle));
    addPart({ 0.34f, 0.02f, -2.72f }, { 0.08f, 0.85f, 0.10f }, { 0.95f, 0.92f, 0.75f, 1.0f }, XMMatrixRotationX(m_rotorAngle * 1.7f));
    addPart({ 0.34f, 0.02f, -2.72f }, { 0.08f, 0.10f, 0.85f }, { 0.95f, 0.92f, 0.75f, 1.0f }, XMMatrixRotationX(m_rotorAngle * 1.7f));
    addPart({ 0.0f, -0.02f, 1.35f }, { 0.18f, 0.18f, 0.50f }, { 0.08f, 0.08f, 0.10f, 1.0f });

    addPart({ -0.48f, -0.48f, 0.1f }, { 0.10f, 0.08f, 1.65f }, { 0.08f, 0.10f, 0.18f, 1.0f });
    addPart({ 0.48f, -0.48f, 0.1f }, { 0.10f, 0.08f, 1.65f }, { 0.08f, 0.10f, 0.18f, 1.0f });
    addPart({ -0.35f, -0.27f, 0.55f }, { 0.08f, 0.42f, 0.08f }, { 0.08f, 0.10f, 0.18f, 1.0f });
    addPart({ 0.35f, -0.27f, 0.55f }, { 0.08f, 0.42f, 0.08f }, { 0.08f, 0.10f, 0.18f, 1.0f });
    addPart({ -0.35f, -0.27f, -0.55f }, { 0.08f, 0.42f, 0.08f }, { 0.08f, 0.10f, 0.18f, 1.0f });
    addPart({ 0.35f, -0.27f, -0.55f }, { 0.08f, 0.42f, 0.08f }, { 0.08f, 0.10f, 0.18f, 1.0f });
}

XMMATRIX AssignmentGame::ApacheModelWorldMatrix() const
{
    return
        XMMatrixScaling(GP_APACHE_MODEL_SCALE, GP_APACHE_MODEL_SCALE, GP_APACHE_MODEL_SCALE) *
        XMMatrixRotationRollPitchYaw(-m_helicopterPitch * 0.45f, m_helicopterYaw, 0.0f) *
        XMMatrixTranslation(m_helicopterPosition.x, m_helicopterPosition.y, m_helicopterPosition.z);
}

void AssignmentGame::AddTargets()
{
    for (const Target& target : m_targets)
    {
        if (!target.active)
        {
            continue;
        }

        AddBox(target.position, { 1.0f, 1.6f, 1.0f }, { 0.86f, 0.18f, 0.16f, 1.0f }, m_totalTime * 0.5f);
    }
}

void AssignmentGame::AddMissileTrails()
{
    for (const MissileTrailParticle& particle : m_missileTrails)
    {
        if (!particle.active)
        {
            continue;
        }

        const float t = std::clamp(particle.elapsedSeconds / std::max(0.0001f, particle.durationSeconds), 0.0f, 1.0f);
        const float size = particle.startSize * (1.0f - t);
        if (size <= 0.03f)
        {
            continue;
        }

        const float gray = 0.38f + 0.18f * (1.0f - t);
        AddBox(particle.position, { size, size, size }, { gray, gray, gray, 1.0f }, t * 2.0f, t * 1.3f, t * 1.7f);
    }
}

void AssignmentGame::AddBullets()
{
    for (const Bullet& bullet : m_bullets)
    {
        const XMFLOAT3 direction = Collision::Normalize(bullet.velocity);
        const float yaw = std::atan2(direction.x, direction.z);
        const float pitch = std::asin(std::clamp(direction.y, -1.0f, 1.0f));
        const float visualScale = ScreenConstantScaleAt(bullet.position, 0.012f);
        const XMFLOAT4 color = bullet.homing ? XMFLOAT4{ 1.0f, 0.38f, 0.12f, 1.0f } : XMFLOAT4{ 1.0f, 0.95f, 0.35f, 1.0f };
        AddBox(bullet.position, { visualScale * 0.45f, visualScale * 0.45f, visualScale * 1.20f }, color, yaw, -pitch, 0.0f);
    }
}

void AssignmentGame::AddExplosions()
{
    for (const Explosion& explosion : m_explosions)
    {
        const float t = std::clamp(explosion.elapsedSeconds / std::max(0.0001f, explosion.durationSeconds), 0.0f, 1.0f);
        const float burst = 1.0f - (1.0f - t) * (1.0f - t);
        const float fadeScale = std::max(0.0f, 1.0f - t);

        for (int particleIndex = 0; particleIndex < ExplosionParticleCount; ++particleIndex)
        {
            const float seed = static_cast<float>(particleIndex);
            const XMFLOAT3 direction = BurstDirection(std::cos(seed * 0.73f), std::sin(seed * 1.11f), particleIndex + 17);
            const float distanceScale = explosion.radius * (0.35f + static_cast<float>(particleIndex % 7) * 0.08f) * burst;
            const XMFLOAT3 position
            {
                explosion.position.x + direction.x * distanceScale,
                explosion.position.y + direction.y * distanceScale,
                explosion.position.z + direction.z * distanceScale
            };
            const float particleSize = std::max(0.14f, explosion.radius * 0.075f * fadeScale);
            const XMFLOAT4 color
            {
                std::min(1.0f, explosion.color.x + static_cast<float>(particleIndex % 3) * 0.08f),
                std::max(0.18f, explosion.color.y * (0.72f + fadeScale * 0.28f)),
                std::max(0.04f, explosion.color.z * fadeScale),
                1.0f
            };
            AddBox(position, { particleSize, particleSize, particleSize }, color, seed * 0.37f + t * 5.0f, seed * 0.19f + t * 4.0f, seed * 0.23f);
        }
    }
}

void AssignmentGame::AddCrosshair()
{
    if (!m_crosshairValid)
    {
        return;
    }

    const XMFLOAT3 p{ m_crosshairPosition.x, m_crosshairPosition.y + 0.05f, m_crosshairPosition.z };
    const float markerSize = ScreenConstantScaleAt(p, 0.035f);
    const float markerThickness = std::max(0.10f, markerSize * 0.08f);
    AddBox(p, { markerSize, markerThickness, markerThickness }, { 1.0f, 0.12f, 0.10f, 1.0f });
    AddBox(p, { markerThickness, markerSize, markerThickness }, { 1.0f, 0.12f, 0.10f, 1.0f });
    AddBox(p, { markerThickness, markerThickness, markerSize }, { 1.0f, 0.12f, 0.10f, 1.0f });
}

void AssignmentGame::AddLockOnIndicator()
{
    if (!IsTargetIndexValid(m_lockedTargetIndex))
    {
        return;
    }

    const Target& target = m_targets[static_cast<std::size_t>(m_lockedTargetIndex)];
    const XMFLOAT3 center{ target.position.x, target.position.y + 1.15f, target.position.z };
    const float size = ScreenConstantScaleAt(center, 0.050f);
    const float thickness = std::max(0.12f, size * 0.075f);
    const float half = size * 0.65f;
    const float segment = size * 0.42f;
    const XMFLOAT4 lockColor = m_lockPinned ? XMFLOAT4{ 0.18f, 0.96f, 1.0f, 1.0f } : XMFLOAT4{ 1.0f, 0.86f, 0.08f, 1.0f };

    AddBox({ center.x - half, center.y + half, center.z }, { thickness, segment, thickness }, lockColor);
    AddBox({ center.x - half + segment * 0.5f, center.y + half, center.z }, { segment, thickness, thickness }, lockColor);
    AddBox({ center.x + half, center.y + half, center.z }, { thickness, segment, thickness }, lockColor);
    AddBox({ center.x + half - segment * 0.5f, center.y + half, center.z }, { segment, thickness, thickness }, lockColor);
    AddBox({ center.x - half, center.y - half, center.z }, { thickness, segment, thickness }, lockColor);
    AddBox({ center.x - half + segment * 0.5f, center.y - half, center.z }, { segment, thickness, thickness }, lockColor);
    AddBox({ center.x + half, center.y - half, center.z }, { thickness, segment, thickness }, lockColor);
    AddBox({ center.x + half - segment * 0.5f, center.y - half, center.z }, { segment, thickness, thickness }, lockColor);
}

void AssignmentGame::AddBox(const XMFLOAT3& center, const XMFLOAT3& size, const XMFLOAT4& color, float yaw, float pitch, float roll)
{
    const XMMATRIX world = XMMatrixScaling(size.x, size.y, size.z) * XMMatrixRotationRollPitchYaw(pitch, yaw, roll) * XMMatrixTranslation(center.x, center.y, center.z);
    AddBoxWithWorld(world, color);
}

void AssignmentGame::AddBoxWithWorld(const XMMATRIX& world, const XMFLOAT4& color)
{
    if (m_drawItems.size() >= MaxDrawItems)
    {
        return;
    }

    DrawItem item{};
    item.mesh = MeshType::Cube;
    XMStoreFloat4x4(&item.world, world);
    item.color = color;
    m_drawItems.push_back(item);
}

void AssignmentGame::AddText3D(const std::wstring& text, const XMFLOAT3& origin, float unitSize, float depth, const XMFLOAT4& color, float yaw, bool centered, float glyphSpacing)
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

void AssignmentGame::AddExplodingText3D(const std::wstring& text, const XMFLOAT3& origin, float unitSize, float depth, const XMFLOAT4& color, float yaw, float explosionTime)
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

bool AssignmentGame::HitStartName(int x, int y) const
{
    const float nx = static_cast<float>(x) / static_cast<float>(std::max(1u, m_width));
    const float ny = static_cast<float>(y) / static_cast<float>(std::max(1u, m_height));
    return nx >= 0.38f && nx <= 0.62f && ny >= 0.45f && ny <= 0.63f;
}

int AssignmentGame::HitMenuEntry(int x, int y) const
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

    for (std::size_t i = 0; i < m_menuEntries.size(); ++i)
    {
        const MenuEntry& entry = m_menuEntries[i];
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

void AssignmentGame::ResetLevel()
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

    const float usableHalfX = std::max(80.0f * GP_WORLD_UNITS_PER_METER, (m_terrain.HalfWidth() > 0.0f ? m_terrain.HalfWidth() : GP_TERRAIN_HALF_SIZE_METERS) - 24.0f * GP_WORLD_UNITS_PER_METER);
    const float usableHalfZ = std::max(120.0f * GP_WORLD_UNITS_PER_METER, (m_terrain.HalfLength() > 0.0f ? m_terrain.HalfLength() : GP_TERRAIN_HALF_SIZE_METERS) - 24.0f * GP_WORLD_UNITS_PER_METER);
    m_helicopterPosition = placeOnTerrain(0.0f, -usableHalfZ * 0.45f, GP_PLAYER_TERRAIN_CLEARANCE_METERS);
    m_helicopterYaw = 0.0f;
    m_helicopterPitch = 0.0f;
    m_rotorAngle = 0.0f;
    m_shotCooldown = 0.0f;
    m_crosshairValid = false;
    m_lockedTargetIndex = -1;
    m_lockPinned = false;
    m_hasLastMousePosition = false;
    m_bullets.clear();
    m_explosions.clear();
    for (MissileTrailParticle& particle : m_missileTrails)
    {
        particle.active = false;
    }
    m_nextMissileTrailIndex = 0;
    m_targets.clear();
    m_targets.reserve(static_cast<std::size_t>(std::max(0, GP_LEVEL_TARGET_COUNT)));
    for (int targetIndex = 0; targetIndex < GP_LEVEL_TARGET_COUNT; ++targetIndex)
    {
        const float normalizedIndex = (GP_LEVEL_TARGET_COUNT <= 1) ? 0.0f : static_cast<float>(targetIndex) / static_cast<float>(GP_LEVEL_TARGET_COUNT - 1);
        const float angle = static_cast<float>(targetIndex) * 2.399963f;
        const float spread = 0.28f + normalizedIndex * 0.60f;
        const float x = std::sin(angle) * usableHalfX * 0.72f * spread;
        const float z = usableHalfZ * (-0.04f + normalizedIndex * 0.78f) + std::cos(angle) * usableHalfZ * 0.07f;
        m_targets.push_back({ placeOnTerrain(x, z, GP_ENEMY_TERRAIN_CLEARANCE_METERS), true });
    }
    UpdateAimRay();
}

bool AssignmentGame::IsTargetIndexValid(int targetIndex) const
{
    if (targetIndex < 0 || targetIndex >= static_cast<int>(m_targets.size()))
    {
        return false;
    }

    return m_targets[static_cast<std::size_t>(targetIndex)].active;
}

float AssignmentGame::ScreenConstantScaleAt(const XMFLOAT3& position, float scalePerMeter) const
{
    const XMFLOAT3 cameraPosition = LevelCameraPosition();
    const float distance = std::sqrt(std::max(0.0001f, DistanceSquared(position, cameraPosition)));
    return std::clamp(distance * scalePerMeter, 0.35f, 8.0f);
}

float AssignmentGame::TerrainHeightAt(float worldX, float worldZ) const
{
    return m_terrain.HeightAt(worldX, worldZ);
}

bool AssignmentGame::RaycastTerrain(const Collision::Ray& ray, float maxDistance, Collision::HitResult& hit, float heightOffset) const
{
    hit = {};
    if (maxDistance <= 0.0f)
    {
        return false;
    }

    if (m_terrain.Empty())
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
        return m_terrain.Contains(worldX, worldZ);
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

    const float step = std::max(0.35f, std::min(m_terrain.CellX(), m_terrain.CellZ()) * 0.5f);
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

XMFLOAT3 AssignmentGame::LevelCameraPosition() const
{
    return m_camera.LevelCameraPosition(m_helicopterPosition, m_helicopterYaw);
}

XMFLOAT3 AssignmentGame::ForwardDirection() const
{
    const float cosPitch = std::cos(m_helicopterPitch);
    return Collision::Normalize({ std::sin(m_helicopterYaw) * cosPitch, std::sin(m_helicopterPitch), std::cos(m_helicopterYaw) * cosPitch });
}

XMFLOAT3 AssignmentGame::MuzzlePosition() const
{
    const XMFLOAT3 forward = ForwardDirection();
    const float muzzleOffset = m_apacheModelLoaded ? GP_APACHE_MUZZLE_OFFSET_METERS * GP_WORLD_UNITS_PER_METER : 1.55f;
    return
    {
        m_helicopterPosition.x + forward.x * muzzleOffset,
        m_helicopterPosition.y + 0.02f + forward.y * 0.25f,
        m_helicopterPosition.z + forward.z * muzzleOffset
    };
}
