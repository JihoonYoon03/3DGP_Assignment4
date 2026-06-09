#pragma once

#include "Mesh.h"

class DrawItem
{
public:
    MeshType mesh = MeshType::Cube;
    std::size_t meshPartIndex = 0;
    DirectX::XMFLOAT4X4 world{};
    DirectX::XMFLOAT4 color{};
};

class GameObject
{
public:
    GameObject();
    explicit GameObject(const DirectX::XMFLOAT3& position);
    virtual ~GameObject();

    virtual void Update(float deltaSeconds);

    const DirectX::XMFLOAT3& Position() const;
    void SetPosition(const DirectX::XMFLOAT3& position);
    void Move(const DirectX::XMFLOAT3& offset);

    const DirectX::XMFLOAT3& Rotation() const;
    void SetRotation(const DirectX::XMFLOAT3& rotation);
    void SetRotation(float pitch, float yaw, float roll);
    float Pitch() const;
    float Yaw() const;
    float Roll() const;
    void SetPitch(float pitch);
    void SetYaw(float yaw);
    void SetRoll(float roll);

    const DirectX::XMFLOAT3& Scale() const;
    void SetScale(const DirectX::XMFLOAT3& scale);

    bool IsActive() const;
    void SetActive(bool active);
    void Activate();
    void Deactivate();

private:
    DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 m_rotation{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 m_scale{ 1.0f, 1.0f, 1.0f };
    bool m_active = true;
};

class Player : public GameObject
{
public:
    Player();

    void Reset(const DirectX::XMFLOAT3& position);
    void ApplyLook(int deltaX, int deltaY);
    void MoveFlat(const DirectX::XMFLOAT3& direction, float distance);
    void MoveVertical(float distance);
    void ClampHorizontal(float halfWidth, float halfLength);
    void ClampAltitude(float minimumAltitude, float maximumAltitude);
    void Update(float deltaSeconds) override;

    DirectX::XMFLOAT3 FlatForward() const;
    DirectX::XMFLOAT3 FlatRight() const;
    DirectX::XMFLOAT3 ForwardDirection() const;

    float RotorAngle() const;
    float ShotCooldown() const;
    bool CanFire() const;
    void StartShotCooldown(float seconds);

private:
    float m_rotorAngle = 0.0f;
    float m_shotCooldown = 0.0f;
};

class Bullet : public GameObject
{
public:
    Bullet();
    Bullet(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity, float lifeSeconds, bool homing, int targetIndex, float homingDelaySeconds, float trailSpawnAccumulator);

    void Launch(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity, float lifeSeconds, bool homing, int targetIndex, float homingDelaySeconds, float trailSpawnAccumulator);
    void Update(float deltaSeconds) override;

    const DirectX::XMFLOAT3& Velocity() const;
    void SetVelocity(const DirectX::XMFLOAT3& velocity);
    float Speed() const;
    DirectX::XMFLOAT3 Direction() const;

    bool IsExpired() const;
    void Expire();
    float LifeSeconds() const;

    bool IsHoming() const;
    int TargetIndex() const;
    bool HasHomingTarget() const;
    bool CanHome() const;
    void UpdateHomingDelay(float deltaSeconds);
    void SteerToward(const DirectX::XMFLOAT3& targetPosition, float deltaSeconds, float turnRateRadians);

    float HitRadius() const;
    void AccumulateTrail(float deltaSeconds);
    bool ConsumeTrailSpawn(float intervalSeconds);
    DirectX::XMFLOAT3 TrailPosition(float offset) const;

private:
    DirectX::XMFLOAT3 m_velocity{ 0.0f, 0.0f, 0.0f };
    float m_lifeSeconds = 0.0f;
    float m_homingDelaySeconds = 0.0f;
    float m_trailSpawnAccumulator = 0.0f;
    bool m_homing = false;
    int m_targetIndex = -1;
};

class Enemy : public GameObject
{
public:
    Enemy();
    explicit Enemy(const DirectX::XMFLOAT3& position);

    void PlaceOnTerrain(float terrainHeight, float clearance);
    DirectX::XMFLOAT3 AimPoint(float heightOffset = 0.65f) const;
    void Destroy();
};

class Explosion : public GameObject
{
public:
    Explosion();
    Explosion(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color, float durationSeconds, float radius);

    void Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color, float durationSeconds, float radius);
    void Update(float deltaSeconds) override;

    const DirectX::XMFLOAT4& Color() const;
    float ElapsedSeconds() const;
    float DurationSeconds() const;
    float Radius() const;
    bool IsExpired() const;

private:
    DirectX::XMFLOAT4 m_color{ 1.0f, 1.0f, 1.0f, 1.0f };
    float m_elapsedSeconds = 0.0f;
    float m_durationSeconds = 0.0f;
    float m_radius = 0.0f;
};

class MissileTrailParticle : public GameObject
{
public:
    MissileTrailParticle();

    void Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& missileDirection, float durationSeconds, float startSize);
    void Update(float deltaSeconds) override;

    float ElapsedSeconds() const;
    float DurationSeconds() const;
    float StartSize() const;
    bool IsExpired() const;

private:
    DirectX::XMFLOAT3 m_velocity{ 0.0f, 0.0f, 0.0f };
    float m_elapsedSeconds = 0.0f;
    float m_durationSeconds = 0.0f;
    float m_startSize = 0.0f;
};

class MenuEntry
{
public:
    std::wstring label;
    float y = 0.0f;
};
