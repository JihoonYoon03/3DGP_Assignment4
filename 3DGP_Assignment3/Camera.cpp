#include "AssignmentGame.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;


XMMATRIX GameCamera::ProjectionMatrix(unsigned int width, unsigned int height) const
{
    const float aspectRatio = static_cast<float>(std::max(1u, width)) / static_cast<float>(std::max(1u, height));
    return XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fieldOfViewDegrees), aspectRatio, m_nearPlane, m_farPlane);
}

XMFLOAT3 GameCamera::LevelCameraPosition(const XMFLOAT3& helicopterPosition, float helicopterYaw) const
{
    const XMFLOAT3 flatForward{ std::sinf(helicopterYaw), 0.0f, std::cosf(helicopterYaw) };
    return
    {
        helicopterPosition.x - flatForward.x * m_followDistance,
        helicopterPosition.y + m_followHeight,
        helicopterPosition.z - flatForward.z * m_followDistance
    };
}

XMMATRIX GameCamera::LevelViewMatrix(const XMFLOAT3& helicopterPosition, float helicopterYaw, const XMFLOAT3& forward) const
{
    const XMFLOAT3 flatForward{ std::sinf(helicopterYaw), 0.0f, std::cosf(helicopterYaw) };
    const XMFLOAT3 eye =
    {
        helicopterPosition.x - flatForward.x * m_followDistance,
        helicopterPosition.y + m_followHeight,
        helicopterPosition.z - flatForward.z * m_followDistance
    };
    const XMFLOAT3 target =
    {
        helicopterPosition.x + forward.x * m_lookAheadDistance,
        helicopterPosition.y + m_targetHeightOffset + forward.y * m_lookAheadDistance,
        helicopterPosition.z + forward.z * m_lookAheadDistance
    };

    return XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
}

XMMATRIX AssignmentGame::ProjectionMatrix() const
{
    return m_camera.ProjectionMatrix(m_width, m_height);
}

XMMATRIX AssignmentGame::LevelViewMatrix() const
{
    return m_camera.LevelViewMatrix(m_helicopterPosition, m_helicopterYaw, ForwardDirection());
}
