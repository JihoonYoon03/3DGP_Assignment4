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
    if (m_scene != SceneName::Level1 && m_scene != SceneName::Level3 && m_cursorCaptured)
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
    const bool wasDown = key < m_keyDown.size() && m_keyDown[key];
    if (key < m_keyDown.size())
    {
        m_keyDown[key] = true;
    }

    if (key == VK_ESCAPE && (m_scene == SceneName::Tutorial || m_scene == SceneName::Level1 || m_scene == SceneName::Level2 || m_scene == SceneName::Level3))
    {
        SetLevelCursorCapture(false);
        m_leftMouseDragging = false;
        m_scene = SceneName::Menu;
        return;
    }

    if (!wasDown && m_scene == SceneName::Level1 && key == 'N')
    {
        ResetLevel2();
        m_scene = SceneName::Level2;
        return;
    }

    if (!wasDown && m_scene == SceneName::Level2)
    {
        if (key == 'A')
        {
            m_scene.playerTank.ToggleAutoAttack();
            return;
        }
        if (key == 'S')
        {
            m_scene.playerTank.ToggleShield();
            return;
        }
        if (key == 'W')
        {
            m_scene.level2Win = true;
            return;
        }
    }

    if (!wasDown && m_scene == SceneName::Level3 && key == 'V')
    {
        m_scene.firstPersonHelicopter = !m_scene.firstPersonHelicopter;
        return;
    }

    if (!wasDown && m_scene == SceneName::Level3 && key == '1')
    {
        m_scene.firstPersonHelicopter = true;
        return;
    }

    if (!wasDown && m_scene == SceneName::Level3 && key == '3')
    {
        m_scene.firstPersonHelicopter = false;
        return;
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

    if (m_scene == SceneName::Level2)
    {
        if (m_leftMouseDragging && m_hasLastMousePosition)
        {
            const int deltaX = x - m_lastMouseX;
            m_scene.playerTank.RotateYaw(static_cast<float>(deltaX) * 0.0065f);
        }
        m_lastMouseX = x;
        m_lastMouseY = y;
        m_hasLastMousePosition = true;
        return;
    }

    if (m_scene == SceneName::Level1 || m_scene == SceneName::Level3)
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
    if (m_scene == SceneName::Level1 || m_scene == SceneName::Level3)
    {
        FireBulletAtAim();
        return;
    }

    if (m_scene == SceneName::Level2)
    {
        m_leftMouseDragging = true;
        m_lastMouseX = x;
        m_lastMouseY = y;
        m_hasLastMousePosition = true;
        return;
    }

    if (m_scene == SceneName::Tutorial)
    {
        m_scene = SceneName::Menu;
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
        if (label == L"Tutorial")
        {
            m_scene = SceneName::Tutorial;
        }
        else if (label == L"Start" || label == L"Level-1")
        {
            ResetLevel();
            m_scene = SceneName::Level1;
            SetLevelCursorCapture(true);
        }
        else if (label == L"Level-2")
        {
            ResetLevel2();
            m_scene = SceneName::Level2;
        }
        else if (label == L"Level-3")
        {
            ResetLevel3();
            m_scene = SceneName::Level3;
            SetLevelCursorCapture(true);
        }
        else if (label == L"End")
        {
            PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
        }
    }
}

void GameManager::OnMouseUp(int, int)
{
    m_leftMouseDragging = false;
}

void GameManager::OnRightMouseDown(int x, int y)
{
    if (m_scene == SceneName::Level2)
    {
        const int pickedTankIndex = PickTankAtScreen(x, y);
        if (IsTankIndexValid(pickedTankIndex))
        {
            m_scene.selectedTankIndex = pickedTankIndex;
        }
        else
        {
            UpdateTankAimRay();
        }
        return;
    }

    if (m_scene == SceneName::Level3)
    {
        const int pickedTankIndex = PickTankAtScreen(x, y);
        if (IsTankIndexValid(pickedTankIndex))
        {
            m_scene.selectedTankIndex = pickedTankIndex;
        }
        else
        {
            UpdateTankAimRay();
        }
        return;
    }

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

    XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, -8.5f, 1.0f), XMVectorZero(), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    XMFLOAT3 cameraPosition{ 0.0f, 0.0f, -8.5f };
    XMFLOAT4 clearColor{ 0.03f, 0.05f, 0.09f, 1.0f };
    if (m_scene == SceneName::Level1)
    {
        view = LevelViewMatrix();
        cameraPosition = LevelCameraPosition();
        clearColor = { 0.38f, 0.55f, 0.78f, 1.0f };
    }
    else if (m_scene == SceneName::Level2)
    {
        view = Level2ViewMatrix();
        cameraPosition = TankCameraPosition();
        clearColor = { 0.30f, 0.42f, 0.32f, 1.0f };
    }
    else if (m_scene == SceneName::Level3)
    {
        view = Level3ViewMatrix();
        cameraPosition = m_scene.firstPersonHelicopter ? m_scene.player.Position() : LevelCameraPosition();
        clearColor = { 0.38f, 0.55f, 0.78f, 1.0f };
    }

    const XMMATRIX viewProjection = view * ProjectionMatrix();

    RenderFrame(m_scene.drawItems, m_assets.meshes, m_assets.modelParts, viewProjection, cameraPosition, clearColor);
}

DirectX::XMMATRIX GameManager::ProjectionMatrix() const
{
    return m_camera.ProjectionMatrix(m_width, m_height);
}

DirectX::XMMATRIX GameManager::LevelViewMatrix() const
{
    return m_camera.LevelViewMatrix(m_scene.player.Position(), m_scene.player.Yaw(), ForwardDirection());
}

DirectX::XMMATRIX GameManager::Level2ViewMatrix() const
{
    using namespace DirectX;

    const XMFLOAT3 tankPosition = m_scene.playerTank.Position();
    const XMFLOAT3 forward = m_scene.playerTank.ForwardDirection();
    const XMVECTOR eye = XMVectorSet(
        tankPosition.x - forward.x * 18.0f,
        tankPosition.y + 10.0f,
        tankPosition.z - forward.z * 18.0f,
        1.0f);
    const XMVECTOR target = XMVectorSet(
        tankPosition.x + forward.x * 8.0f,
        tankPosition.y + 2.0f,
        tankPosition.z + forward.z * 8.0f,
        1.0f);
    return XMMatrixLookAtLH(eye, target, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
}

DirectX::XMMATRIX GameManager::Level3ViewMatrix() const
{
    using namespace DirectX;

    if (!m_scene.firstPersonHelicopter)
    {
        return LevelViewMatrix();
    }

    const XMFLOAT3 position = m_scene.player.Position();
    const XMFLOAT3 forward = ForwardDirection();
    const XMVECTOR eye = XMVectorSet(position.x, position.y + 0.35f, position.z, 1.0f);
    const XMVECTOR target = XMVectorSet(position.x + forward.x * 10.0f, position.y + forward.y * 10.0f, position.z + forward.z * 10.0f, 1.0f);
    return XMMatrixLookAtLH(eye, target, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
}
