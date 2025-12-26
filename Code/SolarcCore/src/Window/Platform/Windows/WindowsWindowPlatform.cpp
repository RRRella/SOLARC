#ifdef _WIN32
#include "Window/WindowPlatform.h"
#include "Logging/LogMacros.h"
#include <stdexcept>

WindowPlatform::WindowPlatform(
    const std::string& title,
    const int32_t& width,
    const int32_t& height)
    : m_Title(title)
    , m_Width(width > 0 ? width : 800)
    , m_Height(height > 0 ? height : 600)
{
    auto& context = WindowContextPlatform::Get();

    // Compute window rect that gives us desired client area
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exStyle = 0;

    RECT rect = { 0, 0, m_Width, m_Height };
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);

    int winWidth = rect.right - rect.left;
    int winHeight = rect.bottom - rect.top;

    m_hWnd = CreateWindowEx(
        0,
        context.GetWindowClassName().c_str(),
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        winWidth, winHeight,
        nullptr,
        nullptr,
        context.GetInstance(),
        this
    );

    if (!m_hWnd)
        throw std::runtime_error("Failed to create Win32 window");

    SOLARC_WINDOW_TRACE("Win32 window platform created: '{}'", m_Title);
}

WindowPlatform::~WindowPlatform()
{
    if (m_hWnd)
        DestroyWindow(m_hWnd);

    SOLARC_WINDOW_TRACE("Win32 window platform destroyed: '{}'", m_Title);
}

void WindowPlatform::Show()
{
    std::lock_guard lk(mtx);
    if (m_hWnd)
    {
        ShowWindow(m_hWnd, SW_SHOW);
        UpdateWindow(m_hWnd);

        m_Visible = true;

        DispatchWindowEvent(std::make_shared<WindowShownEvent>());
        SOLARC_WINDOW_TRACE("Win32 window shown request: '{}'", m_Title);
    }
}

void WindowPlatform::Hide()
{
    std::lock_guard lk(mtx);
    if (m_hWnd)
    {
        ShowWindow(m_hWnd, SW_HIDE);

        m_Visible = false;

        DispatchWindowEvent(std::make_shared<WindowHiddenEvent>());
        SOLARC_WINDOW_TRACE("Win32 window hidden request: '{}'", m_Title);
    }
}

bool WindowPlatform::IsVisible() const
{
    std::lock_guard lk(mtx);

    return m_Visible;
}

void WindowPlatform::SetTitle(const std::string& title)
{
    std::lock_guard lk(mtx);

    if (m_hWnd)
    {
        m_Title = title;
        SetWindowTextA(m_hWnd, title.c_str());
        SOLARC_WINDOW_TRACE("Win32 window title changed: '{}'", m_Title);
    }
}

void WindowPlatform::Resize(int32_t width, int32_t height)
{
    std::lock_guard lk(mtx);
    if (!m_hWnd) return;

    // Get current window style
    DWORD style = GetWindowLong(m_hWnd, GWL_STYLE);
    DWORD exStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE);

    // Compute window rect needed for desired client area
    RECT r = { 0, 0, width, height };
    AdjustWindowRectEx(&r, style, FALSE, exStyle);

    int windowWidth = r.right - r.left;
    int windowHeight = r.bottom - r.top;

    SetWindowPos(m_hWnd, nullptr, 0, 0, windowWidth, windowHeight,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    SOLARC_WINDOW_TRACE("Win32 resize: client {}x{} -> window {}x{}",
        width, height, windowWidth, windowHeight);
}

void WindowPlatform::Minimize()
{
    std::lock_guard lk(mtx);

    if (m_hWnd)
    {
        ShowWindow(m_hWnd, SW_MINIMIZE);

        SOLARC_WINDOW_TRACE("Win32 window minimize requested: '{}'", m_Title);
    }
}

void WindowPlatform::Maximize()
{
    std::lock_guard lk(mtx);

    if (m_hWnd)
    {
        ShowWindow(m_hWnd, SW_MAXIMIZE);

        SOLARC_WINDOW_TRACE("Win32 window maximize requested: '{}'", m_Title);
    }
}

void WindowPlatform::Restore()
{
    std::lock_guard lk(mtx);

    if (m_hWnd)
    {
        ShowWindow(m_hWnd, SW_RESTORE);

        SOLARC_WINDOW_DEBUG("Win32 window restore requested: '{}'", m_Title);
    }
}

bool WindowPlatform::IsMinimized() const
{
    std::lock_guard lk(mtx);

    return m_Minimized; 
}

bool WindowPlatform::IsMaximized() const
{
    std::lock_guard lk(mtx);
    return m_Maximized;
}

#endif