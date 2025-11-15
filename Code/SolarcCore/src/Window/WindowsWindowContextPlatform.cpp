#ifdef _WIN32

#include "Window/WindowsWindowContextPlatform.h"
#include "Window/WindowsWindowPlatform.h"

void WindowsWindowContextPlatform::RegisterWindowClass()
{
    if (s_WindowClassRegistered) return;
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowsWindowContextPlatform::WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    if (!RegisterClassEx(&wc))
        throw std::runtime_error("Failed to register Win32 window class");
    s_WindowClassRegistered = true;
}

WindowsWindowContextPlatform::WindowsWindowContextPlatform()
{
    RegisterWindowClass();
}

std::shared_ptr<WindowPlatform> WindowsWindowContextPlatform::CreateWindowPlatform(
    const std::string& title, const int32_t& width, const int32_t& height)
{
    auto platform = std::make_shared<WindowsWindowPlatform>(title, width, height);
    HWND hwnd = platform->GetHWND();

    // Store THIS context platform in the window's user data
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    return platform;
}

void WindowsWindowContextPlatform::PollEvents()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK WindowsWindowContextPlatform::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        WindowsWindowPlatform* platform = static_cast<WindowsWindowPlatform*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(platform));
        return TRUE;
    }

    // Retrieve the WindowsWindowContextPlatform instance
    WindowsWindowContextPlatform* contextPlatform =
        reinterpret_cast<WindowsWindowContextPlatform*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (!contextPlatform)
        return DefWindowProc(hWnd, msg, wParam, lParam);

    switch (msg)
    {
    case WM_CLOSE:
    {
        auto ev = std::make_shared<WindowCloseEvent>();
        contextPlatform->DispatchWindowEvent(ev);
        return 0; // Prevent default close — let engine decide
    }

    case WM_DESTROY:
        // Do nothing — window is already being destroyed by engine
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void WindowsWindowContextPlatform::Shutdown()
{
    // Optional: additional cleanup
}

#endif