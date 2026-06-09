#include "pch.h"
#include "GameObject.h"

#include "Collision.h"

namespace
{
    DirectX::XMFLOAT3 AddVector(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    DirectX::XMFLOAT3 ScaleVector(const DirectX::XMFLOAT3& value, float scale)
    {
        return { value.x * scale, value.y * scale, value.z * scale };
    }

    float Length(const DirectX::XMFLOAT3& value)
    {
        return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    }

    DirectX::XMFLOAT3 BlendDirection(const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to, float amount)
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

GameObject::GameObject() = default;

GameObject::GameObject(const DirectX::XMFLOAT3& position) :
    m_position(position)
{
}

GameObject::~GameObject() = default;

void GameObject::Update(float)
{
}

const DirectX::XMFLOAT3& GameObject::Position() const
{
    return m_position;
}

void GameObject::SetPosition(const DirectX::XMFLOAT3& position)
{
    m_position = position;
}

void GameObject::Move(const DirectX::XMFLOAT3& offset)
{
    m_position = AddVector(m_position, offset);
}

const DirectX::XMFLOAT3& GameObject::Rotation() const
{
    return m_rotation;
}

void GameObject::SetRotation(const DirectX::XMFLOAT3& rotation)
{
    m_rotation = rotation;
}

void GameObject::SetRotation(float pitch, float yaw, float roll)
{
    m_rotation = { pitch, yaw, roll };
}

float GameObject::Pitch() const
{
    return m_rotation.x;
}

float GameObject::Yaw() const
{
    return m_rotation.y;
}

float GameObject::Roll() const
{
    return m_rotation.z;
}

void GameObject::SetPitch(float pitch)
{
    m_rotation.x = pitch;
}

void GameObject::SetYaw(float yaw)
{
    m_rotation.y = yaw;
}

void GameObject::SetRoll(float roll)
{
    m_rotation.z = roll;
}

const DirectX::XMFLOAT3& GameObject::Scale() const
{
    return m_scale;
}

void GameObject::SetScale(const DirectX::XMFLOAT3& scale)
{
    m_scale = scale;
}

bool GameObject::IsActive() const
{
    return m_active;
}

void GameObject::SetActive(bool active)
{
    m_active = active;
}

void GameObject::Activate()
{
    SetActive(true);
}

void GameObject::Deactivate()
{
    SetActive(false);
}

Player::Player()
{
    Reset({ 0.0f, 1.3f, -8.0f });
}

void Player::Reset(const DirectX::XMFLOAT3& position)
{
    SetPosition(position);
    SetRotation(0.0f, 0.0f, 0.0f);
    m_rotorAngle = 0.0f;
    m_shotCooldown = 0.0f;
    Activate();
}

void Player::ApplyLook(int deltaX, int deltaY)
{
    SetYaw(Yaw() + static_cast<float>(deltaX) * 0.0045f);
    SetPitch(std::clamp(Pitch() - static_cast<float>(deltaY) * 0.0035f, -0.55f, 0.45f));
}

void Player::MoveFlat(const DirectX::XMFLOAT3& direction, float distance)
{
    Move(ScaleVector(direction, distance));
}

void Player::MoveVertical(float distance)
{
    Move({ 0.0f, distance, 0.0f });
}

void Player::ClampHorizontal(float halfWidth, float halfLength)
{
    DirectX::XMFLOAT3 position = Position();
    position.x = std::clamp(position.x, -halfWidth, halfWidth);
    position.z = std::clamp(position.z, -halfLength, halfLength);
    SetPosition(position);
}

void Player::ClampAltitude(float minimumAltitude, float maximumAltitude)
{
    DirectX::XMFLOAT3 position = Position();
    position.y = std::clamp(position.y, minimumAltitude, maximumAltitude);
    SetPosition(position);
}

void Player::Update(float deltaSeconds)
{
    m_rotorAngle += 22.0f * deltaSeconds;
    m_shotCooldown = std::max(0.0f, m_shotCooldown - deltaSeconds);
}

DirectX::XMFLOAT3 Player::FlatForward() const
{
    return { std::sinf(Yaw()), 0.0f, std::cosf(Yaw()) };
}

DirectX::XMFLOAT3 Player::FlatRight() const
{
    return { std::cosf(Yaw()), 0.0f, -std::sinf(Yaw()) };
}

DirectX::XMFLOAT3 Player::ForwardDirection() const
{
    const float cosPitch = std::cos(Pitch());
    return Collision::Normalize({ std::sin(Yaw()) * cosPitch, std::sin(Pitch()), std::cos(Yaw()) * cosPitch });
}

float Player::RotorAngle() const
{
    return m_rotorAngle;
}

float Player::ShotCooldown() const
{
    return m_shotCooldown;
}

bool Player::CanFire() const
{
    return m_shotCooldown <= 0.0f;
}

void Player::StartShotCooldown(float seconds)
{
    m_shotCooldown = std::max(0.0f, seconds);
}

Bullet::Bullet()
{
    Deactivate();
}

Bullet::Bullet(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity, float lifeSeconds, bool homing, int targetIndex, float homingDelaySeconds, float trailSpawnAccumulator)
{
    Launch(position, velocity, lifeSeconds, homing, targetIndex, homingDelaySeconds, trailSpawnAccumulator);
}

void Bullet::Launch(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity, float lifeSeconds, bool homing, int targetIndex, float homingDelaySeconds, float trailSpawnAccumulator)
{
    SetPosition(position);
    m_velocity = velocity;
    m_lifeSeconds = lifeSeconds;
    m_homing = homing;
    m_targetIndex = targetIndex;
    m_homingDelaySeconds = homingDelaySeconds;
    m_trailSpawnAccumulator = trailSpawnAccumulator;
    Activate();
}

void Bullet::Update(float deltaSeconds)
{
    if (!IsActive())
    {
        return;
    }

    Move(ScaleVector(m_velocity, deltaSeconds));
    m_lifeSeconds -= deltaSeconds;
    if (m_lifeSeconds <= 0.0f)
    {
        Deactivate();
    }
}

const DirectX::XMFLOAT3& Bullet::Velocity() const
{
    return m_velocity;
}

void Bullet::SetVelocity(const DirectX::XMFLOAT3& velocity)
{
    m_velocity = velocity;
}

float Bullet::Speed() const
{
    return Length(m_velocity);
}

DirectX::XMFLOAT3 Bullet::Direction() const
{
    return Collision::Normalize(m_velocity);
}

bool Bullet::IsExpired() const
{
    return !IsActive() || m_lifeSeconds <= 0.0f;
}

void Bullet::Expire()
{
    m_lifeSeconds = 0.0f;
    Deactivate();
}

float Bullet::LifeSeconds() const
{
    return m_lifeSeconds;
}

bool Bullet::IsHoming() const
{
    return m_homing;
}

int Bullet::TargetIndex() const
{
    return m_targetIndex;
}

bool Bullet::HasHomingTarget() const
{
    return m_homing && m_targetIndex >= 0;
}

bool Bullet::CanHome() const
{
    return HasHomingTarget() && m_homingDelaySeconds <= 0.0f;
}

void Bullet::UpdateHomingDelay(float deltaSeconds)
{
    m_homingDelaySeconds = std::max(0.0f, m_homingDelaySeconds - deltaSeconds);
}

void Bullet::SteerToward(const DirectX::XMFLOAT3& targetPosition, float deltaSeconds, float turnRateRadians)
{
    const float currentSpeed = std::max(0.0001f, Speed());
    const DirectX::XMFLOAT3 desiredDirection = Collision::Normalize(
        {
            targetPosition.x - Position().x,
            targetPosition.y - Position().y,
            targetPosition.z - Position().z
        });
    const DirectX::XMFLOAT3 currentDirection = Direction();
    const float dot = std::clamp(Collision::Dot(currentDirection, desiredDirection), -1.0f, 1.0f);
    const float angle = std::acos(dot);
    const float blend = (angle <= 0.0001f) ? 1.0f : std::min(1.0f, (turnRateRadians * deltaSeconds) / angle);
    SetVelocity(ScaleVector(BlendDirection(currentDirection, desiredDirection, blend), currentSpeed));
}

float Bullet::HitRadius() const
{
    return m_homing ? 4.0f : 1.8f;
}

void Bullet::AccumulateTrail(float deltaSeconds)
{
    m_trailSpawnAccumulator += deltaSeconds;
}

bool Bullet::ConsumeTrailSpawn(float intervalSeconds)
{
    if (m_trailSpawnAccumulator < intervalSeconds)
    {
        return false;
    }

    m_trailSpawnAccumulator -= intervalSeconds;
    return true;
}

DirectX::XMFLOAT3 Bullet::TrailPosition(float offset) const
{
    const DirectX::XMFLOAT3 direction = Direction();
    return
    {
        Position().x - direction.x * offset,
        Position().y - direction.y * offset,
        Position().z - direction.z * offset
    };
}

Enemy::Enemy() = default;

Enemy::Enemy(const DirectX::XMFLOAT3& position) :
    GameObject(position)
{
}

void Enemy::PlaceOnTerrain(float terrainHeight, float clearance)
{
    DirectX::XMFLOAT3 position = Position();
    position.y = terrainHeight + clearance;
    SetPosition(position);
}

DirectX::XMFLOAT3 Enemy::AimPoint(float heightOffset) const
{
    return { Position().x, Position().y + heightOffset, Position().z };
}

void Enemy::Destroy()
{
    Deactivate();
}

Tank::Tank() = default;

Tank::Tank(const DirectX::XMFLOAT3& position, float yaw, int health)
{
    Reset(position, yaw, health);
}

void Tank::Reset(const DirectX::XMFLOAT3& position, float yaw, int health)
{
    SetPosition(position);
    SetRotation(0.0f, yaw, 0.0f);
    m_health = std::max(1, health);
    m_reloadSeconds = 0.0f;
    m_shieldEnabled = false;
    m_autoAttackEnabled = false;
    Activate();
}

void Tank::Update(float deltaSeconds)
{
    m_reloadSeconds = std::max(0.0f, m_reloadSeconds - deltaSeconds);
}

void Tank::MoveForward(float distance)
{
    Move(ScaleVector(ForwardDirection(), distance));
}

void Tank::RotateYaw(float amount)
{
    SetYaw(Yaw() + amount);
}

DirectX::XMFLOAT3 Tank::ForwardDirection() const
{
    return { std::sin(Yaw()), 0.0f, std::cos(Yaw()) };
}

DirectX::XMFLOAT3 Tank::FirePoint(float forwardOffset, float heightOffset) const
{
    const DirectX::XMFLOAT3 forward = ForwardDirection();
    return
    {
        Position().x + forward.x * forwardOffset,
        Position().y + heightOffset,
        Position().z + forward.z * forwardOffset
    };
}

bool Tank::CanFire() const
{
    return IsActive() && m_reloadSeconds <= 0.0f;
}

void Tank::StartReload(float seconds)
{
    m_reloadSeconds = std::max(0.0f, seconds);
}

bool Tank::Damage(int amount)
{
    if (!IsActive() || m_shieldEnabled)
    {
        return false;
    }

    m_health -= std::max(0, amount);
    if (m_health <= 0)
    {
        Deactivate();
        return true;
    }
    return false;
}

bool Tank::Destroyed() const
{
    return !IsActive();
}

void Tank::SetShieldEnabled(bool enabled)
{
    m_shieldEnabled = enabled;
}

void Tank::ToggleShield()
{
    m_shieldEnabled = !m_shieldEnabled;
}

bool Tank::ShieldEnabled() const
{
    return m_shieldEnabled;
}

void Tank::SetAutoAttackEnabled(bool enabled)
{
    m_autoAttackEnabled = enabled;
}

void Tank::ToggleAutoAttack()
{
    m_autoAttackEnabled = !m_autoAttackEnabled;
}

bool Tank::AutoAttackEnabled() const
{
    return m_autoAttackEnabled;
}

Obstacle::Obstacle() = default;

Obstacle::Obstacle(const DirectX::XMFLOAT3& position, float yaw, float radius, int variant)
{
    Reset(position, yaw, radius, variant);
}

void Obstacle::Reset(const DirectX::XMFLOAT3& position, float yaw, float radius, int variant)
{
    SetPosition(position);
    SetRotation(0.0f, yaw, 0.0f);
    m_radius = std::max(0.1f, radius);
    m_variant = variant;
    Activate();
}

float Obstacle::Radius() const
{
    return m_radius;
}

int Obstacle::Variant() const
{
    return m_variant;
}

Explosion::Explosion()
{
    Deactivate();
}

Explosion::Explosion(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color, float durationSeconds, float radius)
{
    Initialize(position, color, durationSeconds, radius);
}

void Explosion::Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color, float durationSeconds, float radius)
{
    SetPosition(position);
    m_color = color;
    m_elapsedSeconds = 0.0f;
    m_durationSeconds = durationSeconds;
    m_radius = radius;
    Activate();
}

void Explosion::Update(float deltaSeconds)
{
    if (!IsActive())
    {
        return;
    }

    m_elapsedSeconds += deltaSeconds;
    if (m_elapsedSeconds >= m_durationSeconds)
    {
        Deactivate();
    }
}

const DirectX::XMFLOAT4& Explosion::Color() const
{
    return m_color;
}

float Explosion::ElapsedSeconds() const
{
    return m_elapsedSeconds;
}

float Explosion::DurationSeconds() const
{
    return m_durationSeconds;
}

float Explosion::Radius() const
{
    return m_radius;
}

bool Explosion::IsExpired() const
{
    return !IsActive() || m_elapsedSeconds >= m_durationSeconds;
}

MissileTrailParticle::MissileTrailParticle()
{
    Deactivate();
}

void MissileTrailParticle::Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& missileDirection, float durationSeconds, float startSize)
{
    SetPosition(position);
    m_velocity =
    {
        -missileDirection.x * 3.0f,
        -missileDirection.y * 3.0f + 0.25f,
        -missileDirection.z * 3.0f
    };
    m_elapsedSeconds = 0.0f;
    m_durationSeconds = durationSeconds;
    m_startSize = startSize;
    Activate();
}

void MissileTrailParticle::Update(float deltaSeconds)
{
    if (!IsActive())
    {
        return;
    }

    m_elapsedSeconds += deltaSeconds;
    if (m_elapsedSeconds >= m_durationSeconds)
    {
        Deactivate();
        return;
    }

    Move(ScaleVector(m_velocity, deltaSeconds));
}

float MissileTrailParticle::ElapsedSeconds() const
{
    return m_elapsedSeconds;
}

float MissileTrailParticle::DurationSeconds() const
{
    return m_durationSeconds;
}

float MissileTrailParticle::StartSize() const
{
    return m_startSize;
}

bool MissileTrailParticle::IsExpired() const
{
    return !IsActive() || m_elapsedSeconds >= m_durationSeconds;
}
