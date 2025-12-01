#pragma once
#ifdef _WIN32
#include "Window/WindowContextPlatform.h"
#include <windows.h>
#include <string>

class WindowsWindowContextPlatform : public WindowContextPlatform
{
public:
    WindowsWindowContextPlatform();
    ~WindowsWindowContextPlatform() override;

    void PollEvents() override;
    void Shutdown() override;

    const std::string& GetWindowClassName() const { return m_WindowClassName; }
    HINSTANCE GetInstance() const { return m_hInstance; }

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    std::string m_WindowClassName = "SolarcEngineWindowClass";
    HINSTANCE m_hInstance = nullptr;
    bool m_ClassRegistered = false;
};
#endif