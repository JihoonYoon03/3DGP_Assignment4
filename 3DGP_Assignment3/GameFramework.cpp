#include "pch.h"
#include "GameFramework.h"

GameFramework::~GameFramework()
{
    if (m_device && m_commandQueue && m_fence)
    {
        FlushCommandQueue();
    }

    if (m_constantBuffer && m_mappedConstantBuffer)
    {
        m_constantBuffer->Unmap(0, nullptr);
        m_mappedConstantBuffer = nullptr;
    }

    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
}

void GameFramework::Initialize(HWND hwnd, UINT width, UINT height)
{
    m_hwnd = hwnd;
    m_width = std::max(1u, width);
    m_height = std::max(1u, height);

    CreateDeviceResources();
    CreateWindowSizeDependentResources();
    CreatePipelineState();
}

void GameFramework::OnResize(UINT width, UINT height)
{
    if (width == 0 || height == 0 || !IsDeviceReady())
    {
        return;
    }

    m_width = width;
    m_height = height;
    FlushCommandQueue();
    CreateWindowSizeDependentResources();
}

bool GameFramework::IsDeviceReady() const
{
    return m_device && m_swapChain;
}
