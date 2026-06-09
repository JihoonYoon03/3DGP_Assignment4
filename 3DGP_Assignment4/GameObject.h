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

class Player
{
public:
    DirectX::XMFLOAT3 position{ 0.0f, 1.3f, -8.0f };
    float yaw = 0.0f;
    float pitch = 0.0f;
    float rotorAngle = 0.0f;
    float shotCooldown = 0.0f;
};

class Bullet
{
public:
    DirectX::XMFLOAT3 position{};
    DirectX::XMFLOAT3 velocity{};
    float lifeSeconds = 0.0f;
    float homingDelaySeconds = 0.0f;
    float trailSpawnAccumulator = 0.0f;
    bool homing = false;
    int targetIndex = -1;
};

class Enemy
{
public:
    DirectX::XMFLOAT3 position{};
    bool active = true;
};

class Explosion
{
public:
    DirectX::XMFLOAT3 position{};
    DirectX::XMFLOAT4 color{};
    float elapsedSeconds = 0.0f;
    float durationSeconds = 0.0f;
    float radius = 0.0f;
};

class MissileTrailParticle
{
public:
    DirectX::XMFLOAT3 position{};
    DirectX::XMFLOAT3 velocity{};
    float elapsedSeconds = 0.0f;
    float durationSeconds = 0.0f;
    float startSize = 0.0f;
    bool active = false;
};

class MenuEntry
{
public:
    std::wstring label;
    float y = 0.0f;
};
