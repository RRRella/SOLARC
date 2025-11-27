#pragma once

#ifdef linux

#include "Window/WindowContextPlatform.h"
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <unordered_map>
#include <mutex>

class WaylandWindowContextPlatform : public WindowContextPlatform
{
public:
    WaylandWindowContextPlatform();
    ~WaylandWindowContextPlatform() override;

    void RegisterWindow(WindowPlatform* window) override;
    void UnregisterWindow(WindowPlatform* window) override;
    void PollEvents() override;
    void Shutdown() override;

    // Wayland-specific helpers for window creation
    wl_surface* CreateSurface();
    xdg_surface* CreateXdgSurface(wl_surface* surface);
    
    // Event dispatching
    void DispatchCloseEvent(wl_surface* surface);

private:
    static void registry_global(void* data, wl_registry* registry,
        uint32_t name, const char* interface, uint32_t version);
    static void registry_global_remove(void* data, wl_registry* registry, uint32_t name);

    void BindGlobals();

    wl_display* m_Display;
    wl_registry* m_Registry;
    wl_compositor* m_Compositor;
    xdg_wm_base* m_XdgWmBase;
    
    std::unordered_map<wl_surface*, WaylandWindowContextPlatform*> m_WindowMap;
    mutable std::mutex m_WindowMapMutex;
    
    bool m_ShuttingDown = false;
    
    static const wl_registry_listener s_RegistryListener;
    
    static void xdg_wm_base_ping(void* data, xdg_wm_base* xdg_wm_base, uint32_t serial);
    static const xdg_wm_base_listener s_XdgWmBaseListener;
};

#endif