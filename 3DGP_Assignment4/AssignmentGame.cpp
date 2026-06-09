#include "pch.h"
#include "AssignmentGame.h"

AssignmentGame::~AssignmentGame()
{
    SetLevelCursorCapture(false);
}

void AssignmentGame::Initialize(HWND hwnd, UINT width, UINT height)
{
    m_menuEntries =
    {
        { L"TUTORIAL", 1.65f },
        { L"LEVEL-1", 0.90f },
        { L"LEVEL-2", 0.15f },
        { L"LEVEL-3", -0.60f },
        { L"START", -1.35f },
        { L"END", -2.10f }
    };

    GameFramework::Initialize(hwnd, width, height);
    CreateMeshResources();
    ResetLevel();
}

void AssignmentGame::Tick(float deltaSeconds)
{
    const float clampedDelta = std::clamp(deltaSeconds, 0.0f, 0.05f);
    m_totalTime += clampedDelta;

    Update(clampedDelta);
    if (m_scene != SceneMode::Level1 && m_cursorCaptured)
    {
        SetLevelCursorCapture(false);
    }
    Render();
}

void AssignmentGame::OnResize(UINT width, UINT height)
{
    GameFramework::OnResize(width, height);
}

void AssignmentGame::OnKeyDown(WPARAM key)
{
    if (key < m_keyDown.size())
    {
        m_keyDown[key] = true;
    }

    if (key == VK_ESCAPE && m_scene == SceneMode::Level1)
    {
        SetLevelCursorCapture(false);
        m_scene = SceneMode::Menu;
    }
}

void AssignmentGame::OnKeyUp(WPARAM key)
{
    if (key < m_keyDown.size())
    {
        m_keyDown[key] = false;
    }
}

void AssignmentGame::OnMouseMove(int x, int y)
{
    m_mouseX = x;
    m_mouseY = y;

    if (m_scene == SceneMode::Level1)
    {
        if (m_hasLastMousePosition)
        {
            const int deltaX = x - m_lastMouseX;
            const int deltaY = y - m_lastMouseY;
            m_helicopterYaw += static_cast<float>(deltaX) * 0.0045f;
            m_helicopterPitch = std::clamp(m_helicopterPitch - static_cast<float>(deltaY) * 0.0035f, -0.55f, 0.45f);
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

void AssignmentGame::OnMouseDown(int x, int y)
{
    if (m_scene == SceneMode::Level1)
    {
        FireBulletAtAim();
        return;
    }

    if (m_scene == SceneMode::Start)
    {
        if (!m_nameExploding && HitStartName(x, y))
        {
            m_nameExploding = true;
            m_nameExplosionTime = 0.0f;
            m_nameExplosionYaw = m_totalTime * 1.7f;
        }
        return;
    }

    if (m_scene == SceneMode::Menu)
    {
        const int entry = HitMenuEntry(x, y);
        if (entry < 0)
        {
            return;
        }

        const std::wstring& label = m_menuEntries[entry].label;
        if (label == L"START" || label == L"LEVEL-1")
        {
            ResetLevel();
            m_scene = SceneMode::Level1;
            SetLevelCursorCapture(true);
        }
        else if (label == L"END")
        {
            PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
        }
    }
}

void AssignmentGame::OnRightMouseDown(int, int)
{
    if (m_scene != SceneMode::Level1)
    {
        return;
    }

    if (m_lockPinned)
    {
        m_lockPinned = false;
        m_lockedTargetIndex = -1;
        UpdateAimRay();
        return;
    }

    UpdateAimRay();
    if (IsTargetIndexValid(m_lockedTargetIndex))
    {
        m_lockPinned = true;
    }
}

void AssignmentGame::SetLevelCursorCapture(bool enabled)
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

void AssignmentGame::Render()
{
    using namespace DirectX;

    BuildDrawItems();

    const XMMATRIX view =
        (m_scene == SceneMode::Level1)
        ? LevelViewMatrix()
        : XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, -8.5f, 1.0f), XMVectorZero(), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    const XMMATRIX viewProjection = view * ProjectionMatrix();
    const XMFLOAT3 cameraPosition =
        (m_scene == SceneMode::Level1)
        ? LevelCameraPosition()
        : XMFLOAT3{ 0.0f, 0.0f, -8.5f };
    const XMFLOAT4 clearColor =
        (m_scene == SceneMode::Level1)
        ? XMFLOAT4{ 0.38f, 0.55f, 0.78f, 1.0f }
        : XMFLOAT4{ 0.03f, 0.05f, 0.09f, 1.0f };

    RenderFrame(m_drawItems, m_meshes, m_apacheParts, viewProjection, cameraPosition, clearColor);
}

DirectX::XMMATRIX AssignmentGame::ProjectionMatrix() const
{
    return m_camera.ProjectionMatrix(m_width, m_height);
}

DirectX::XMMATRIX AssignmentGame::LevelViewMatrix() const
{
    return m_camera.LevelViewMatrix(m_helicopterPosition, m_helicopterYaw, ForwardDirection());
}
