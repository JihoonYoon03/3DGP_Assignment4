#pragma once

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>

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

    inline DirectX::XMFLOAT3 Add(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    inline DirectX::XMFLOAT3 Scale(const DirectX::XMFLOAT3& v, float scale)
    {
        return { v.x * scale, v.y * scale, v.z * scale };
    }

    inline float Dot(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline DirectX::XMFLOAT3 Normalize(const DirectX::XMFLOAT3& v)
    {
        const float length = std::sqrt(std::max(0.000001f, Dot(v, v)));
        return { v.x / length, v.y / length, v.z / length };
    }

    inline DirectX::XMFLOAT3 PointAt(const Ray& ray, float distance)
    {
        return Add(ray.origin, Scale(ray.direction, distance));
    }

    inline HitResult RaycastPlaneY(const Ray& ray, float planeY, float maxDistance)
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

    inline HitResult RaycastSphere(const Ray& ray, const DirectX::XMFLOAT3& center, float radius, float maxDistance)
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
}
