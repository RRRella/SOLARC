#ifdef __linux__
#include "Window/WindowContextPlatform.h"
#include "Window/WindowPlatform.h"
#include "Logging/LogMacros.h"
#include <stdexcept>
#include <cstring>
#include <poll.h>

const wl_registry_listener WindowContextPlatform::s_RegistryListener = {
    .global = registry_global,
    .global_remove = registry_global_remove
};

const xdg_wm_base_listener WindowContextPlatform::s_XdgWmBaseListener = {
    .ping = xdg_wm_base_ping
};

WindowContextPlatform::WindowContextPlatform()
{
    m_Display = wl_display_connect(nullptr);
    if (!m_Display)
        throw std::runtime_error("Failed to connect to Wayland display");

    m_Registry = wl_display_get_registry(m_Display);
    wl_registry_add_listener(m_Registry, &s_RegistryListener, this);
    wl_display_roundtrip(m_Display);

    if (!m_Compositor || !m_XdgWmBase)
    {
        Shutdown();
        throw std::runtime_error("Missing required Wayland globals");
    }

    xdg_wm_base_add_listener(m_XdgWmBase, &s_XdgWmBaseListener, this);
    SOLARC_WINDOW_INFO("Wayland context initialized");
}

WindowContextPlatform::~WindowContextPlatform()
{
    Shutdown();
}

WindowContextPlatform& WindowContextPlatform::Get()
{
    static WindowContextPlatform instance;
    return instance;
}

void WindowContextPlatform::PollEvents()
{
    if (!m_Display || m_ShuttingDown) return;

    if (wl_display_prepare_read(m_Display) == 0)
    {
        struct pollfd pfd = 
        {
            .fd = wl_display_get_fd(m_Display),
            .events = POLLIN
        };
        

        if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN))
        {
            wl_display_read_events(m_Display);
        }
        else
        {
            wl_display_cancel_read(m_Display);
        }

    }



    // Dispatch any events now available

    wl_display_dispatch_pending(m_Display);
    wl_display_flush(m_Display);
}

void WindowContextPlatform::Shutdown()
{
    if (m_ShuttingDown) return;
    m_ShuttingDown = true;

    if (m_XdgWmBase) { xdg_wm_base_destroy(m_XdgWmBase); m_XdgWmBase = nullptr; }
    if (m_Compositor) { wl_compositor_destroy(m_Compositor); m_Compositor = nullptr; }
    if (m_Registry) { wl_registry_destroy(m_Registry); m_Registry = nullptr; }
    if (m_Display) { wl_display_disconnect(m_Display); m_Display = nullptr; }

    SOLARC_WINDOW_INFO("Wayland context shut down");
}

wl_surface* WindowContextPlatform::CreateSurface()
{
    return m_Compositor ? wl_compositor_create_surface(m_Compositor) : nullptr;
}

xdg_surface* WindowContextPlatform::CreateXdgSurface(wl_surface* surface)
{
    return (m_XdgWmBase && surface) ? xdg_wm_base_get_xdg_surface(m_XdgWmBase, surface) : nullptr;
}

void WindowContextPlatform::registry_global(
    void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
    auto* ctx = static_cast<WindowContextPlatform*>(data);
    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        ctx->m_Compositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4u)));
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        ctx->m_XdgWmBase = static_cast<xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, std::min(version, 1u)));
    }
}

void WindowContextPlatform::registry_global_remove(void*, wl_registry*, uint32_t) {}

void WindowContextPlatform::xdg_wm_base_ping(void* data, xdg_wm_base* wm_base, uint32_t serial)
{
    xdg_wm_base_pong(wm_base, serial);
}
#endif