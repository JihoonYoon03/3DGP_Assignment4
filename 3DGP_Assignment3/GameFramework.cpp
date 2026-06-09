#include "AssignmentGame.h"

#include <algorithm>


AssignmentGame::~AssignmentGame()
{
    SetLevelCursorCapture(false);

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

void AssignmentGame::Initialize(HWND hwnd, UINT width, UINT height)
{
    // 창 정보와 기본 해상도를 저장
    m_hwnd = hwnd;
    m_width = std::max(1u, width);
    m_height = std::max(1u, height);

    m_menuEntries =
    {
        { L"TUTORIAL", 1.65f },
        { L"LEVEL-1", 0.90f },
        { L"LEVEL-2", 0.15f },
        { L"LEVEL-3", -0.60f },
        { L"START", -1.35f },
        { L"END", -2.10f }
    };

    CreateDeviceResources();
    CreateWindowSizeDependentResources();
    CreatePipelineState();
    CreateMeshResources();
    ResetLevel();
}

void AssignmentGame::Tick(float deltaSeconds)
{
    const float clampedDelta = std::clamp(deltaSeconds, 0.0f, 0.05f);
    m_totalTime += clampedDelta;

    // 게임 로직을 먼저 갱신한 뒤 같은 상태를 렌더링합니다.
    Update(clampedDelta);
    if (m_scene != SceneMode::Level1 && m_cursorCaptured)
    {
        SetLevelCursorCapture(false);
    }
    Render();
}

void AssignmentGame::OnResize(UINT width, UINT height)
{
    if (width == 0 || height == 0 || !m_device || !m_swapChain)
    {
        return;
    }

    m_width = width;
    m_height = height;
    FlushCommandQueue();
    CreateWindowSizeDependentResources();
}

void AssignmentGame::OnKeyDown(WPARAM key)
{
    // 키 눌림 상태를 기록
    if (key < m_keyDown.size())
    {
        m_keyDown[key] = true;
    }

    // Level 1에서 Esc를 누르면 메뉴 화면으로 복귀
    if (key == VK_ESCAPE && m_scene == SceneMode::Level1)
    {
        SetLevelCursorCapture(false);
        m_scene = SceneMode::Menu;
    }
}

void AssignmentGame::OnKeyUp(WPARAM key)
{
    // 키 눌림 상태를 해제
    if (key < m_keyDown.size())
    {
        m_keyDown[key] = false;
    }
}

void AssignmentGame::OnMouseMove(int x, int y)
{
    // 화면 좌표는 시작 이름과 메뉴 항목의 선택 판정에 사용
    m_mouseX = x;
    m_mouseY = y;

    // 마우스로 헬리콥터의 yaw와 pitch를 조작
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

        // 커서를 중앙에 고정
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
    // 현재 조준점 방향에 투사체를 발사
    if (m_scene == SceneMode::Level1)
    {
        FireBulletAtAim();
        return;
    }

    // 시작 화면에서 회전하는 이름을 클릭하면 폭발
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

    // Start 또는 Level-1 선택으로 씬 시작
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
    // 우클릭으로 현재 락온 대상을 고정하거나 해제함
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
    // 커서를 숨기고 창 안에 고정
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

    // 메뉴나 종료 상태에선 커서 고정과 숨김 해제
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
