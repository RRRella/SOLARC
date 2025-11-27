#pragma once

#ifdef _WIN32

#include "Window/WindowContextPlatform.h"
#include <windows.h>
#include <unordered_map>
#include <mutex>
#include <atomic>

class WindowsWindowContextPlatform : public WindowContextPlatform
{
public:
    WindowsWindowContextPlatform();
    ~WindowsWindowContextPlatform() override;

    void RegisterWindow(WindowPlatform* window) override;
    void UnregisterWindow(WindowPlatform* window) override;
    void PollEvents() override;
    void Shutdown() override;

private:
    static void RegisterWindowClass();
    static void UnregisterWindowClass();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static inline const char* WINDOW_CLASS_NAME = "SolarcEngineWindowClass";
    static inline std::atomic<int> s_WindowClassRefCount{0};
    static inline std::mutex s_WindowClassMutex;

    // Map HWND -> this context platform for event routing
    std::unordered_map<HWND, WindowsWindowContextPlatform*> m_WindowMap;
    mutable std::mutex m_WindowMapMutex;
    
    bool m_ShuttingDown = false;
};

#endif