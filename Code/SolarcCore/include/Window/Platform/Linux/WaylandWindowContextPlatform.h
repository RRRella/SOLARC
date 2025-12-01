#pragma once
#ifdef __linux__
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

    void PollEvents() override;
    void Shutdown() override;

    // Wayland helpers for window creation
    wl_surface* CreateSurface();
    xdg_surface* CreateXdgSurface(wl_surface* surface);

private:
    static void registry_global(void* data, wl_registry* registry,
        uint32_t name, const char* interface, uint32_t version);
    static void registry_global_remove(void* data, wl_registry* registry, uint32_t name);
    static void xdg_wm_base_ping(void* data, xdg_wm_base* xdg_wm_base, uint32_t serial);

    wl_display* m_Display = nullptr;
    wl_registry* m_Registry = nullptr;
    wl_compositor* m_Compositor = nullptr;
    xdg_wm_base* m_XdgWmBase = nullptr;
    bool m_ShuttingDown = false;

    static const wl_registry_listener s_RegistryListener;
    static const xdg_wm_base_listener s_XdgWmBaseListener;
};
#endif