#ifdef __linux__
#include "Window/Platform/Linux/WaylandWindowContextPlatform.h"
#include "Window/Platform/Linux/WaylandWindowPlatform.h"
#include "Logging/LogMacros.h"
#include <stdexcept>
#include <cstring>

const wl_registry_listener WaylandWindowContextPlatform::s_RegistryListener = {
    .global = registry_global,
    .global_remove = registry_global_remove
};

const xdg_wm_base_listener WaylandWindowContextPlatform::s_XdgWmBaseListener = {
    .ping = xdg_wm_base_ping
};

WaylandWindowContextPlatform::WaylandWindowContextPlatform()
{
    m_Display = wl_display_connect(nullptr);
    if (!m_Display)
        throw std::runtime_error("Failed to connect to Wayland display");

    m_Registry = wl_display_get_registry(m_Display);
    if (!m_Registry)
    {
        wl_display_disconnect(m_Display);
        throw std::runtime_error("Failed to get Wayland registry");
    }

    wl_registry_add_listener(m_Registry, &s_RegistryListener, this);
    wl_display_roundtrip(m_Display);

    if (!m_Compositor || !m_XdgWmBase)
    {
        Shutdown();
        throw std::runtime_error("Missing required Wayland globals (compositor or xdg-shell)");
    }

    xdg_wm_base_add_listener(m_XdgWmBase, &s_XdgWmBaseListener, this);
    SOLARC_WINDOW_INFO("Wayland context initialized successfully");
}

WaylandWindowContextPlatform::~WaylandWindowContextPlatform()
{
    Shutdown();
}

void WaylandWindowContextPlatform::PollEvents()
{
    if (!m_Display || m_ShuttingDown) return;
    wl_display_dispatch_pending(m_Display);
    wl_display_flush(m_Display);
}

void WaylandWindowContextPlatform::Shutdown()
{
    if (m_ShuttingDown) return;
    m_ShuttingDown = true;

    if (m_XdgWmBase) { xdg_wm_base_destroy(m_XdgWmBase); m_XdgWmBase = nullptr; }
    if (m_Compositor) { wl_compositor_destroy(m_Compositor); m_Compositor = nullptr; }
    if (m_Registry) { wl_registry_destroy(m_Registry); m_Registry = nullptr; }
    if (m_Display) { wl_display_disconnect(m_Display); m_Display = nullptr; }

    SOLARC_WINDOW_INFO("Wayland context shut down");
}

wl_surface* WaylandWindowContextPlatform::CreateSurface()
{
    return m_Compositor ? wl_compositor_create_surface(m_Compositor) : nullptr;
}

xdg_surface* WaylandWindowContextPlatform::CreateXdgSurface(wl_surface* surface)
{
    return (m_XdgWmBase && surface) ? xdg_wm_base_get_xdg_surface(m_XdgWmBase, surface) : nullptr;
}

// Static callbacks
void WaylandWindowContextPlatform::registry_global(
    void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
    auto* ctx = static_cast<WaylandWindowContextPlatform*>(data);
    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        ctx->m_Compositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, 4));
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        ctx->m_XdgWmBase = static_cast<xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
    }
}

void WaylandWindowContextPlatform::registry_global_remove(void*, wl_registry*, uint32_t) {}

void WaylandWindowContextPlatform::xdg_wm_base_ping(void* data, xdg_wm_base* wm_base, uint32_t serial)
{
    xdg_wm_base_pong(wm_base, serial);
}
#endif