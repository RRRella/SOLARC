
#ifdef _WIN32

#include "Window/WindowsWindowPlatform.h"
#include <stdexcept>

WindowsWindowPlatform::WindowsWindowPlatform(const std::string& title, const int32_t& width, const int32_t& height)
    : WindowPlatform(title, width , height )
{
    m_hWnd = CreateWindowEx(
        0,
        "SolarcEngineWindowClass",
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        m_Width, m_Height,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
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

LRESULT CALLBACK WindowsWindowPlatform::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

#endif