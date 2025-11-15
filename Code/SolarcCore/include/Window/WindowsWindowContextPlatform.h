#pragma once

#ifdef _WIN32

#include "Window/WindowContextPlatform.h"
#include <windows.h>

class WindowsWindowContextPlatform : public WindowContextPlatform
{
public:
    WindowsWindowContextPlatform();
    ~WindowsWindowContextPlatform() override = default;

    void PollEvents() override;
    void Shutdown() override;
    std::shared_ptr<WindowPlatform> CreateWindowPlatform(
        const std::string& title, const int32_t& width, const int32_t& height) override;

private:
    static void RegisterWindowClass();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static inline const char* WINDOW_CLASS_NAME = "SolarcEngineWindowClass";
    static inline bool s_WindowClassRegistered = false;

};

std::unique_ptr<WindowContextPlatform> WindowContextPlatform::CreateWindowContextPlatform()
{
    return std::make_unique<WindowsWindowContextPlatform>();
}

#endif