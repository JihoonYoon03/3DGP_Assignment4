#pragma once

#include <DirectXMath.h>

class GameCamera
{
public:
    DirectX::XMMATRIX ProjectionMatrix(unsigned int width, unsigned int height) const;
    DirectX::XMMATRIX LevelViewMatrix(const DirectX::XMFLOAT3& helicopterPosition, float helicopterYaw, const DirectX::XMFLOAT3& forward) const;
    DirectX::XMFLOAT3 LevelCameraPosition(const DirectX::XMFLOAT3& helicopterPosition, float helicopterYaw) const;

private:
    float m_fieldOfViewDegrees = 60.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 1200.0f;
    float m_followDistance = 7.0f;
    float m_followHeight = 4.0f;
    float m_lookAheadDistance = 3.0f;
    float m_targetHeightOffset = 0.65f;
};
