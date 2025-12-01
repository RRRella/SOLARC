#ifdef _WIN32
#include "Window/Platform/Windows/WindowsWindowPlatform.h"
#include "Window/Platform/Windows/WindowsWindowContextPlatform.h"
#include <stdexcept>

WindowsWindowPlatform::WindowsWindowPlatform(
    const std::string& title,
    int32_t width,
    int32_t height,
    WindowsWindowContextPlatform* context)
    : WindowPlatform(title, width, height)
    , m_Context(context)
{
    if (!m_Context)
        throw std::invalid_argument("Context must not be null");

    m_hWnd = CreateWindowEx(
        0,
        m_Context->GetWindowClassName().c_str(),
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        m_Width, m_Height,
        nullptr,
        nullptr,
        m_Context->GetInstance(),
        this
    );

    if (!m_hWnd)
        throw std::runtime_error("Failed to create Win32 window");
}

WindowsWindowPlatform::~WindowsWindowPlatform()
{
    if (m_hWnd)
        DestroyWindow(m_hWnd);
}

void WindowsWindowPlatform::Show()
{
    if (m_hWnd)
    {
        ShowWindow(m_hWnd, SW_SHOW);
        UpdateWindow(m_hWnd);
        m_Visible = true;
    }
}

void WindowsWindowPlatform::Hide()
{
    if (m_hWnd)
    {
        ShowWindow(m_hWnd, SW_HIDE);
        m_Visible = false;
    }
}

bool WindowsWindowPlatform::IsVisible() const
{
    return m_Visible;
}

void WindowsWindowPlatform::SetTitle(const std::string& title)
{
    WindowPlatform::SetTitle(title);
    if (m_hWnd)
        SetWindowTextA(m_hWnd, title.c_str());
}

void WindowsWindowPlatform::Resize(int32_t width, int32_t height)
{
    WindowPlatform::Resize(width, height);
    if (m_hWnd)
    {
        SetWindowPos(m_hWnd, nullptr, 0, 0, width, height,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void WindowsWindowPlatform::Minimize()
{
    if (m_hWnd) ShowWindow(m_hWnd, SW_MINIMIZE);
}

void WindowsWindowPlatform::Maximize()
{
    if (m_hWnd) ShowWindow(m_hWnd, SW_MAXIMIZE);
}

void WindowsWindowPlatform::Restore()
{
    if (m_hWnd) ShowWindow(m_hWnd, SW_RESTORE);
}
#endif