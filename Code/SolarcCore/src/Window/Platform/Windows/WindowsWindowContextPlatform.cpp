#ifdef _WIN32
#include "Window/WindowContextPlatform.h"
#include "Window/WindowPlatform.h"
#include "Input/Platform/Windows/WindowsKeyMapping.h"
#include "Input/MouseButton.h"
#include <windowsx.h>
#include <stdexcept>

WindowContextPlatform::WindowContextPlatform()
{
    std::lock_guard lock(m_WindowClassMtx);
    if (m_ClassRegistered) return;

    // ========================================================================
    // Enable DPI awareness (for correct mouse coordinates)
    // ========================================================================
    // This ensures mouse coordinates are in logical pixels, not physical
    // Note: This should ideally be set via manifest, but we do it here too
#if defined(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#endif

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

    SOLARC_WINDOW_INFO("Win32 window context initialized (DPI-aware)");
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
    // ========================================================================
    // Initial Setup: Associate WindowPlatform with HWND
    // ========================================================================
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

    // ========================================================================
    // Input Message Handling
    // ========================================================================

    switch (msg)
    {
        // ========================================================================
        // Keyboard Input
        // ========================================================================

    case WM_KEYDOWN:

    case WM_SYSKEYDOWN: // Alt+Key combinations
    {
        // Extract scancode from lParam
        uint16_t scancode = Win32KeyMapping::ExtractScancode(lParam);

        // Check if this is a repeat
        bool isRepeat = Win32KeyMapping::IsKeyRepeat(lParam);
        bool isSys = (msg == WM_SYSKEYDOWN);

        windowPlatform->RecordKeyTransition(scancode, true, isRepeat);

        SOLARC_WINDOW_DEBUG("WM_{}KEYDOWN: scancode=0x{:02X}, repeat={}",
            isSys ? "SYS" : "", scancode, isRepeat ? "true" : "false");

        // Don't return 0 for WM_SYSKEYDOWN - let DefWindowProc handle Alt+F4, etc.
        if (isSys)
            break;

        return 0;
    }

    case WM_KEYUP:

    case WM_SYSKEYUP:
    {
        uint16_t scancode = Win32KeyMapping::ExtractScancode(lParam);
        bool isSys = (msg == WM_SYSKEYUP);

        windowPlatform->RecordKeyTransition(scancode, false, false);

        SOLARC_WINDOW_DEBUG("WM_{}KEYUP: scancode=0x{:02X}",
            isSys ? "SYS" : "", scancode);

        if (isSys)
            break;

        return 0;
    }

    // ========================================================================
    // Mouse Movement
    // ========================================================================

    case WM_MOUSEMOVE:
    {
        // Extract absolute position (client coordinates)
        int32_t x = GET_X_LPARAM(lParam);
        int32_t y = GET_Y_LPARAM(lParam);

        windowPlatform->RecordMousePosition(x, y);


        SOLARC_WINDOW_DEBUG("WM_MOUSEMOVE: pos=({},{})", x, y);

        return 0;
    }

    // ========================================================================
    // Mouse Buttons
    // ========================================================================

    case WM_LBUTTONDOWN:
    {

        windowPlatform->RecordMouseButton(MouseButton::Left, true);

        SOLARC_WINDOW_DEBUG("WM_LBUTTONDOWN");
        return 0;
    }

    case WM_LBUTTONUP:
    {
        windowPlatform->RecordMouseButton(MouseButton::Left, false);

        SOLARC_WINDOW_DEBUG("WM_LBUTTONUP");
        return 0;
    }

    case WM_RBUTTONDOWN:
    {
        windowPlatform->RecordMouseButton(MouseButton::Right, true);

        SOLARC_WINDOW_DEBUG("WM_RBUTTONDOWN");
        return 0;
    }

    case WM_RBUTTONUP:
    {
        windowPlatform->RecordMouseButton(MouseButton::Right, false);

        SOLARC_WINDOW_DEBUG("WM_RBUTTONUP");
        return 0;
    }

    case WM_MBUTTONDOWN:
    {
        windowPlatform->RecordMouseButton(MouseButton::Middle, true);

        SOLARC_WINDOW_DEBUG("WM_MBUTTONDOWN");
        return 0;
    }

    case WM_MBUTTONUP:
    {
        windowPlatform->RecordMouseButton(MouseButton::Middle, false);

        SOLARC_WINDOW_DEBUG("WM_MBUTTONUP");
        return 0;
    }

    case WM_XBUTTONDOWN:
    {
        // Extract which X button (1 or 2)
        WORD xButton = GET_XBUTTON_WPARAM(wParam);

        MouseButton button = (xButton == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;

        windowPlatform->RecordMouseButton(button, true);

        SOLARC_WINDOW_DEBUG("WM_XBUTTONDOWN: button={}", MouseButtonToString(button));

        return TRUE; // Must return TRUE for XBUTTON messages
    }

    case WM_XBUTTONUP:
    {
        WORD xButton = GET_XBUTTON_WPARAM(wParam);

        MouseButton button = (xButton == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;

        windowPlatform->RecordMouseButton(button, false);

        SOLARC_WINDOW_DEBUG("WM_XBUTTONUP: button={}", MouseButtonToString(button));

        return TRUE;
    }

    // ========================================================================
    // Mouse Wheel (Vertical)
    // ========================================================================

    case WM_MOUSEWHEEL:
    {
        // Extract wheel delta (120 units per "notch")
        int16_t wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

        windowPlatform->RecordMouseWheel(static_cast<float>(wheelDelta), 0);

        SOLARC_WINDOW_DEBUG("WM_MOUSEWHEEL: delta={}", wheelDelta);

        return 0;
    }

    // ========================================================================
    // Mouse Wheel (Horizontal)
    // ========================================================================

    case WM_MOUSEHWHEEL:
    {
        int16_t wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

        windowPlatform->RecordMouseWheel(0.0f, static_cast<float>(wheelDelta));

        SOLARC_WINDOW_DEBUG("WM_MOUSEHWHEEL: delta={}", wheelDelta);

        return 0;
    }

    // ========================================================================
    // Focus Management
    // ========================================================================

    case WM_SETFOCUS:
    {
        windowPlatform->SetKeyboardFocus(true);

        SOLARC_WINDOW_DEBUG("Window '{}' gained keyboard focus",
            windowPlatform->GetTitle());
        return 0;
    }

    case WM_KILLFOCUS:
    {
        // This calls OnFocusLost() which:
        // 1. Synthesizes release events for all held keys
        // 2. Clears m_CurrentKeyState
        // 3. Sets m_HasKeyboardFocus = false
        windowPlatform->OnFocusLost();

        SOLARC_WINDOW_DEBUG("Window '{}' lost keyboard focus",
            windowPlatform->GetTitle());
        return 0;
    }

    // ========================================================================
    // Window Lifecycle Events (Existing - Unchanged)
    // ========================================================================

    case WM_CLOSE:
    {
        windowPlatform->DispatchWindowEvent(std::make_shared<WindowCloseEvent>());

        SOLARC_WINDOW_DEBUG("WM_CLOSE: '{}'", windowPlatform->GetTitle());

        return 0;
    }

    case WM_SIZE:
    {
        if (wParam == SIZE_MINIMIZED)
        {
            windowPlatform->SyncMaximized(false);
            windowPlatform->SyncMinimized(true);
            windowPlatform->DispatchWindowEvent(std::make_shared<WindowMinimizedEvent>());
        }

        if (wParam == SIZE_MAXIMIZED)
        {
            windowPlatform->SyncMaximized(true);
            windowPlatform->SyncMinimized(false);
            windowPlatform->DispatchWindowEvent(std::move(std::make_shared<WindowMaximizedEvent>()));
        }

        if (wParam == SIZE_RESTORED)
        {
            windowPlatform->SyncMaximized(false);
            windowPlatform->SyncMinimized(false);
            windowPlatform->DispatchWindowEvent(std::move(std::make_shared<WindowRestoredEvent>()));
        }

        int32_t newWidth = LOWORD(lParam);
        int32_t newHeight = HIWORD(lParam);

        windowPlatform->SyncDimensions(newWidth, newHeight);
        windowPlatform->DispatchWindowEvent(std::make_shared<WindowResizeEvent>(newWidth, newHeight));

        SOLARC_WINDOW_DEBUG("Window resize msg: {}x{}", newWidth, newHeight);

        return 0;
    }

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

#endif