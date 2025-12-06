#pragma once
#include "Event/WindowEvent.h"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#endif

class WindowPlatform; // Forward declare

class WindowContextPlatform
{
public:
    // Singleton access
    static WindowContextPlatform& Get();

    // Prevent copying/moving
    WindowContextPlatform(const WindowContextPlatform&) = delete;
    WindowContextPlatform& operator=(const WindowContextPlatform&) = delete;

    WindowContextPlatform(WindowContextPlatform&&) = delete;
    WindowContextPlatform& operator=(WindowContextPlatform&&) = delete;

    void PollEvents();
    void Shutdown();

#ifdef _WIN32
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static const std::string& GetWindowClassName() { return m_WindowClassName; }
    static HINSTANCE GetInstance() { return m_hInstance; }

#elif defined(__linux__)
    wl_surface* CreateSurface();
    xdg_surface* CreateXdgSurface(wl_surface* surface);
#endif

private:
    WindowContextPlatform();  // Private constructor
    ~WindowContextPlatform(); // Private destructor

#ifdef _WIN32
    static inline std::mutex m_WindowClassMtx;
    static inline std::string m_WindowClassName = "SolarcEngineWindowClass";
    static inline HINSTANCE m_hInstance = nullptr;
    static inline bool m_ClassRegistered = false;

#elif defined(__linux__)
    static void registry_global(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
    static void registry_global_remove(void* data, wl_registry* registry, uint32_t name);
    static void xdg_wm_base_ping(void* data, xdg_wm_base* xdg_wm_base, uint32_t serial);

    wl_display* m_Display = nullptr;
    wl_registry* m_Registry = nullptr;
    wl_compositor* m_Compositor = nullptr;
    xdg_wm_base* m_XdgWmBase = nullptr;
    bool m_ShuttingDown = false;

    static const wl_registry_listener s_RegistryListener;
    static const xdg_wm_base_listener s_XdgWmBaseListener;
#endif
};