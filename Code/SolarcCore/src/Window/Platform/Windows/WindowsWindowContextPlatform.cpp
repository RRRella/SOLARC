#ifdef _WIN32

#include "Window/Platform/Windows/WindowsWindowContextPlatform.h"
#include "Window/Platform/Windows/WindowsWindowPlatform.h"

void WindowsWindowContextPlatform::RegisterWindowClass()
{
    std::lock_guard lock(s_WindowClassMutex);
    
    if (s_WindowClassRefCount++ > 0)
    {
        return; // Already registered
    }
    
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowsWindowContextPlatform::WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    
    if (!RegisterClassEx(&wc))
        throw std::runtime_error("Failed to register Win32 window class");
}

void WindowsWindowContextPlatform::UnregisterWindowClass()
{
    std::lock_guard lock(s_WindowClassMutex);
    
    if (--s_WindowClassRefCount > 0)
    {
        return; // Still in use by other contexts
    }
    
    UnregisterClass(WINDOW_CLASS_NAME, GetModuleHandle(nullptr));
}

WindowsWindowContextPlatform::WindowsWindowContextPlatform()
{
    RegisterWindowClass();
}

WindowsWindowContextPlatform::~WindowsWindowContextPlatform()
{
    Shutdown();
    UnregisterWindowClass();
}

void WindowsWindowContextPlatform::RegisterWindow(WindowPlatform* window)
{
    if (!window) return;

    auto* winPlatform = static_cast<WindowsWindowPlatform*>(window);
    HWND hwnd = winPlatform->GetHWND();
    
    if (!hwnd) return;

    {
        std::lock_guard lock(m_WindowMapMutex);
        m_WindowMap[hwnd] = this;
    }

    // Store this context platform in GWLP_USERDATA for WndProc access
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

void WindowsWindowContextPlatform::UnregisterWindow(WindowPlatform* window)
{
    if (!window) return;

    auto* winPlatform = static_cast<WindowsWindowPlatform*>(window);
    HWND hwnd = winPlatform->GetHWND();
    
    if (!hwnd) return;

    // Clear GWLP_USERDATA first to prevent WndProc from accessing this context
    SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);

    {
        std::lock_guard lock(m_WindowMapMutex);
        m_WindowMap.erase(hwnd);
    }

    // Process any remaining messages for this window to prevent race conditions
    // This ensures WM_CLOSE or other messages are handled before we finish unregistering
    MSG msg;
    while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            // Re-post WM_QUIT so it's not lost
            PostQuitMessage(static_cast<int>(msg.wParam));
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void WindowsWindowContextPlatform::PollEvents()
{
    if (m_ShuttingDown) return;

    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK WindowsWindowContextPlatform::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Retrieve the WindowsWindowContextPlatform instance
    WindowsWindowContextPlatform* contextPlatform =
        reinterpret_cast<WindowsWindowContextPlatform*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    // Safety check: if context is null or shutting down, use default behavior
    if (!contextPlatform || contextPlatform->m_ShuttingDown)
        return DefWindowProc(hWnd, msg, wParam, lParam);

    switch (msg)
    {
    case WM_CLOSE:
    {
        // Verify context is still valid (additional safety)
        {
            std::lock_guard lock(contextPlatform->m_WindowMapMutex);
            if (contextPlatform->m_WindowMap.find(hWnd) == contextPlatform->m_WindowMap.end())
            {
                // Window was unregistered, don't dispatch event
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
        }

        // Create event with HWND as the window identifier
        auto ev = std::make_shared<WindowCloseEvent>(hWnd);
        contextPlatform->DispatchWindowEvent(ev);
        return 0; // Prevent default close – let engine decide
    }
    case WM_DESTROY:
        // Do nothing – window is already being destroyed by engine
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void WindowsWindowContextPlatform::Shutdown()
{
    if (m_ShuttingDown) return;
    
    m_ShuttingDown = true;
    
    std::lock_guard lock(m_WindowMapMutex);
    
    // Clear all window associations
    for (auto& [hwnd, _] : m_WindowMap)
    {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
    }
    
    m_WindowMap.clear();
}

#endif