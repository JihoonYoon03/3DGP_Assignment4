#include "pch.h"
#include "Collision.h"

DirectX::XMFLOAT3 Collision::Add(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

DirectX::XMFLOAT3 Collision::Scale(const DirectX::XMFLOAT3& v, float scale)
{
    return { v.x * scale, v.y * scale, v.z * scale };
}

float Collision::Dot(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

DirectX::XMFLOAT3 Collision::Normalize(const DirectX::XMFLOAT3& v)
{
    const float length = std::sqrt(std::max(0.000001f, Dot(v, v)));
    return { v.x / length, v.y / length, v.z / length };
}

DirectX::XMFLOAT3 Collision::PointAt(const Ray& ray, float distance)
{
    return Add(ray.origin, Scale(ray.direction, distance));
}

Collision::HitResult Collision::RaycastPlaneY(const Ray& ray, float planeY, float maxDistance)
{
    HitResult result{};
    if (std::fabs(ray.direction.y) < 0.0001f)
    {
        return result;
    }

    const float distance = (planeY - ray.origin.y) / ray.direction.y;
    if (distance <= 0.0f || distance > maxDistance)
    {
        return result;
    }

    result.hit = true;
    result.distance = distance;
    result.position = PointAt(ray, distance);
    return result;
}

Collision::HitResult Collision::RaycastSphere(const Ray& ray, const DirectX::XMFLOAT3& center, float radius, float maxDistance)
{
    const DirectX::XMFLOAT3 originToCenter
    {
        ray.origin.x - center.x,
        ray.origin.y - center.y,
        ray.origin.z - center.z
    };

    const float b = Dot(originToCenter, ray.direction);
    const float c = Dot(originToCenter, originToCenter) - radius * radius;
    const float discriminant = b * b - c;
    HitResult result{};
    if (discriminant < 0.0f)
    {
        return result;
    }

    const float distance = -b - std::sqrt(discriminant);
    if (distance <= 0.0f || distance > maxDistance)
    {
        return result;
    }

    result.hit = true;
    result.distance = distance;
    result.position = PointAt(ray, distance);
    return result;
}
