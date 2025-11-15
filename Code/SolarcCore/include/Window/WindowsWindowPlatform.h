#pragma once

#ifdef _WIN32


#include "Window/WindowPlatform.h"
#include <windows.h>

class WindowsWindowPlatform : public WindowPlatform
{
public:
    WindowsWindowPlatform(const std::string& title, const int32_t& width, const int32_t& height);
    ~WindowsWindowPlatform() override;

    void Show() override;
    void Hide() override;
    bool IsVisible() const override;
    HWND GetHWND() const { return m_hWnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hWnd = nullptr;
    bool m_Visible = false;
};

#endif