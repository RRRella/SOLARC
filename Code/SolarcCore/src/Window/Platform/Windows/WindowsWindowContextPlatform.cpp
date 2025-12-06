#ifdef _WIN32
#include "Window/WindowContextPlatform.h"
#include "Window/WindowPlatform.h"
#include <stdexcept>

WindowContextPlatform::WindowContextPlatform()
{
    std::lock_guard lock(m_WindowClassMtx);
    if (m_ClassRegistered) return;

    m_hInstance = GetModuleHandle(nullptr);
    if (!m_hInstance)
        throw std::runtime_error("Failed to get module handle");

    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = m_WindowClassName.c_str();

    if (!RegisterClassEx(&wc))
        throw std::runtime_error("Failed to register Win32 window class");

    m_ClassRegistered = true;
}

WindowContextPlatform::~WindowContextPlatform()
{
    Shutdown();
}

WindowContextPlatform& WindowContextPlatform::Get()
{
    static WindowContextPlatform instance;
    return instance;
}

void WindowContextPlatform::Shutdown()
{
}

void WindowContextPlatform::PollEvents()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK WindowContextPlatform::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreate->lpCreateParams));
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    auto* windowPlatform = reinterpret_cast<WindowPlatform*>(
        GetWindowLongPtr(hWnd, GWLP_USERDATA)
        );
    if (!windowPlatform)
        return DefWindowProc(hWnd, msg, wParam, lParam);

    switch (msg)
    {
    case WM_CLOSE:
    {
        auto ev = std::make_shared<WindowCloseEvent>();
        windowPlatform->ReceiveWindowEvent(std::move(ev));
        return 0;
    }
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
#endif