#include "WindowsWindowPlatform.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool WindowsWindowPlatform::PeekNextMessage()
{   
    return PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
}

void WindowsWindowPlatform::ProcessMessage()
{
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}

void WindowsWindowPlatform::Update()
{
    UpdateWindow(m_Hwnd);
}

void WindowsWindowPlatform::SetHWND(HWND hwnd)
{
    m_Hwnd = hwnd;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}