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

    void PollEvents();
    void Shutdown();

    static const std::string& GetWindowClassName() { return m_WindowClassName; }
    static HINSTANCE GetInstance() { return m_hInstance; }

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    static inline std::string m_WindowClassName = "SolarcEngineWindowClass";
    static inline HINSTANCE m_hInstance = nullptr;
    bool m_ClassRegistered = false;
};
#endif