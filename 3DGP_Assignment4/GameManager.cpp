#include "pch.h"
#include "GameManager.h"

GameManager::~GameManager()
{
    SetLevelCursorCapture(false);
}

void GameManager::Initialize(HWND hwnd, UINT width, UINT height)
{
    m_scene.ConfigureDefaultMenu();
    GameFramework::Initialize(hwnd, width, height);
    CreateMeshResources();
    ResetLevel();
}

void GameManager::Tick(float deltaSeconds)
{
    const float clampedDelta = std::clamp(deltaSeconds, 0.0f, 0.05f);
    m_totalTime += clampedDelta;

    Update(clampedDelta);
    if (m_scene != SceneName::Level1 && m_cursorCaptured)
    {
        SetLevelCursorCapture(false);
    }
    Render();
}

void GameManager::OnResize(UINT width, UINT height)
{
    GameFramework::OnResize(width, height);
}

void GameManager::OnKeyDown(WPARAM key)
{
    if (key < m_keyDown.size())
    {
        m_keyDown[key] = true;
    }

    if (key == VK_ESCAPE && m_scene == SceneName::Level1)
    {
        SetLevelCursorCapture(false);
        m_scene = SceneName::Menu;
    }
}

void GameManager::OnKeyUp(WPARAM key)
{
    if (key < m_keyDown.size())
    {
        m_keyDown[key] = false;
    }
}

void GameManager::OnMouseMove(int x, int y)
{
    m_mouseX = x;
    m_mouseY = y;

    if (m_scene == SceneName::Level1)
    {
        if (m_hasLastMousePosition)
        {
            const int deltaX = x - m_lastMouseX;
            const int deltaY = y - m_lastMouseY;
            m_scene.ApplyPlayerLook(deltaX, deltaY);
        }

        m_lastMouseX = x;
        m_lastMouseY = y;
        m_hasLastMousePosition = true;

        RECT clientRect{};
        GetClientRect(m_hwnd, &clientRect);
        const int centerX = (clientRect.right - clientRect.left) / 2;
        const int centerY = (clientRect.bottom - clientRect.top) / 2;
        if (x != centerX || y != centerY)
        {
            POINT centerPoint{ centerX, centerY };
            ClientToScreen(m_hwnd, &centerPoint);
            SetCursorPos(centerPoint.x, centerPoint.y);
            m_lastMouseX = centerX;
            m_lastMouseY = centerY;
        }
    }
    else
    {
        m_hasLastMousePosition = false;
    }
}

void GameManager::OnMouseDown(int x, int y)
{
    if (m_scene == SceneName::Level1)
    {
        FireBulletAtAim();
        return;
    }

    if (m_scene == SceneName::Start)
    {
        if (!m_scene.titleExploding && HitStartName(x, y))
        {
            m_scene.titleExploding = true;
            m_scene.titleExplosionTime = 0.0f;
            m_scene.titleExplosionYaw = m_totalTime * 1.7f;
        }
        return;
    }

    if (m_scene == SceneName::Menu)
    {
        const int entry = HitMenuEntry(x, y);
        if (entry < 0)
        {
            return;
        }

        const std::wstring& label = m_scene.menuEntries[entry].label;
        if (label == L"START" || label == L"LEVEL-1")
        {
            ResetLevel();
            m_scene = SceneName::Level1;
            SetLevelCursorCapture(true);
        }
        else if (label == L"END")
        {
            PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
        }
    }
}

void GameManager::OnRightMouseDown(int, int)
{
    if (m_scene != SceneName::Level1)
    {
        return;
    }

    if (m_scene.lockPinned)
    {
        m_scene.lockPinned = false;
        m_scene.lockedTargetIndex = -1;
        UpdateAimRay();
        return;
    }

    UpdateAimRay();
    if (IsTargetIndexValid(m_scene.lockedTargetIndex))
    {
        m_scene.lockPinned = true;
    }
}

void GameManager::SetLevelCursorCapture(bool enabled)
{
    if (enabled)
    {
        if (!m_hwnd)
        {
            return;
        }

        RECT clientRect{};
        GetClientRect(m_hwnd, &clientRect);

        POINT topLeft{ clientRect.left, clientRect.top };
        POINT bottomRight{ clientRect.right, clientRect.bottom };
        ClientToScreen(m_hwnd, &topLeft);
        ClientToScreen(m_hwnd, &bottomRight);

        RECT clipRect{ topLeft.x, topLeft.y, bottomRight.x, bottomRight.y };
        ClipCursor(&clipRect);

        const int centerX = (clientRect.right - clientRect.left) / 2;
        const int centerY = (clientRect.bottom - clientRect.top) / 2;
        POINT centerPoint{ centerX, centerY };
        ClientToScreen(m_hwnd, &centerPoint);
        SetCursorPos(centerPoint.x, centerPoint.y);

        m_lastMouseX = centerX;
        m_lastMouseY = centerY;
        m_hasLastMousePosition = true;

        if (!m_cursorCaptured)
        {
            while (ShowCursor(FALSE) >= 0)
            {
            }
            m_cursorCaptured = true;
        }
        return;
    }

    ClipCursor(nullptr);
    if (m_cursorCaptured)
    {
        while (ShowCursor(TRUE) < 0)
        {
        }
    }
    m_cursorCaptured = false;
    m_hasLastMousePosition = false;
}

void GameManager::Render()
{
    using namespace DirectX;

    BuildDrawItems();

    const XMMATRIX view =
        (m_scene == SceneName::Level1)
        ? LevelViewMatrix()
        : XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, -8.5f, 1.0f), XMVectorZero(), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    const XMMATRIX viewProjection = view * ProjectionMatrix();
    const XMFLOAT3 cameraPosition =
        (m_scene == SceneName::Level1)
        ? LevelCameraPosition()
        : XMFLOAT3{ 0.0f, 0.0f, -8.5f };
    const XMFLOAT4 clearColor =
        (m_scene == SceneName::Level1)
        ? XMFLOAT4{ 0.38f, 0.55f, 0.78f, 1.0f }
        : XMFLOAT4{ 0.03f, 0.05f, 0.09f, 1.0f };

    RenderFrame(m_scene.drawItems, m_assets.meshes, m_assets.modelParts, viewProjection, cameraPosition, clearColor);
}

DirectX::XMMATRIX GameManager::ProjectionMatrix() const
{
    return m_camera.ProjectionMatrix(m_width, m_height);
}

DirectX::XMMATRIX GameManager::LevelViewMatrix() const
{
    return m_camera.LevelViewMatrix(m_scene.player.position, m_scene.player.yaw, ForwardDirection());
}
