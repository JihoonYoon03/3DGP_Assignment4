#pragma once

namespace Collision
{
    struct Ray
    {
        DirectX::XMFLOAT3 origin{};
        DirectX::XMFLOAT3 direction{ 0.0f, 0.0f, 1.0f };
    };

    struct HitResult
    {
        bool hit = false;
        float distance = 0.0f;
        DirectX::XMFLOAT3 position{};
    };

    DirectX::XMFLOAT3 Add(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b);
    DirectX::XMFLOAT3 Scale(const DirectX::XMFLOAT3& v, float scale);
    float Dot(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b);
    DirectX::XMFLOAT3 Normalize(const DirectX::XMFLOAT3& v);
    DirectX::XMFLOAT3 PointAt(const Ray& ray, float distance);
    HitResult RaycastPlaneY(const Ray& ray, float planeY, float maxDistance);
    HitResult RaycastSphere(const Ray& ray, const DirectX::XMFLOAT3& center, float radius, float maxDistance);
}
