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
    void* GetNativeHandle() const override { return m_hWnd; }
    
    void SetTitle(const std::string& title) override;
    void Resize(int32_t width, int32_t height) override;
    void Minimize() override;
    void Maximize() override;
    void Restore() override;
    
    HWND GetHWND() const { return m_hWnd; }

private:
    HWND m_hWnd = nullptr;
    bool m_Visible = false;
};

#endif