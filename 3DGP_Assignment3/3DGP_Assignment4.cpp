// 3DGP_Assignment4.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "pch.h"
#include "framework.h"
#include "3DGP_Assignment4.h"
#include "AssignmentGame.h"



#define MAX_LOADSTRING 100

HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
HWND g_hWnd = nullptr;
std::unique_ptr<AssignmentGame> g_game;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY3DGPASSIGNMENT4, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    try
    {
        RECT clientRect{};
        GetClientRect(g_hWnd, &clientRect);
        g_game = std::make_unique<AssignmentGame>();
        g_game->Initialize(g_hWnd, static_cast<UINT>(clientRect.right - clientRect.left), static_cast<UINT>(clientRect.bottom - clientRect.top));
    }
    catch (const std::exception& e)
    {
        MessageBoxA(g_hWnd, e.what(), "초기화 오류", MB_ICONERROR | MB_OK);
        return FALSE;
    }

    MSG msg{};
    auto previousTime = std::chrono::steady_clock::now();

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else if (g_game)
        {
            const auto currentTime = std::chrono::steady_clock::now();
            const float deltaSeconds = std::chrono::duration<float>(currentTime - previousTime).count();
            previousTime = currentTime;
            g_game->Tick(deltaSeconds);
        }
    }

    g_game.reset();

    return (int) msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY3DGPASSIGNMENT4));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = nullptr;
    wcex.lpszMenuName   = nullptr;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;

   RECT windowRect{ 0, 0, 1280, 720 };
   AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   g_hWnd = hWnd;
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_SIZE:
        if (g_game)
        {
            g_game->OnResize(LOWORD(lParam), HIWORD(lParam));
        }
        break;
    case WM_KEYDOWN:
        if (g_game)
        {
            g_game->OnKeyDown(wParam);
        }
        break;
    case WM_KEYUP:
        if (g_game)
        {
            g_game->OnKeyUp(wParam);
        }
        break;
    case WM_MOUSEMOVE:
        if (g_game)
        {
            g_game->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
    case WM_LBUTTONDOWN:
        SetCapture(hWnd);
        if (g_game)
        {
            g_game->OnMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
    case WM_LBUTTONUP:
        ReleaseCapture();
        break;
    case WM_RBUTTONDOWN:
        if (g_game)
        {
            g_game->OnRightMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
