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
    auto& context = WindowContextPlatform::Get(); // Use singleton

    m_hWnd = CreateWindowEx(
        0,
        context.GetWindowClassName().c_str(),
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        m_Width, m_Height,
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
    if (m_hWnd)
    {
        SetWindowPos(m_hWnd, nullptr, 0, 0, width, height,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        SOLARC_WINDOW_TRACE("Win32 window resize requested: {}x{}", width, height);
    }
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

#endif