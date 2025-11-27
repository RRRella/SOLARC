#ifdef linux

#include "Window/Platform/Linux/WaylandWindowContextPlatform.h"
#include "Window/Platform/Linux/WaylandWindowPlatform.h"
#include "Event/WindowEvent.h"
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
    : m_Display(nullptr)
    , m_Registry(nullptr)
    , m_Compositor(nullptr)
    , m_XdgWmBase(nullptr)
{
    // Connect to Wayland display
    m_Display = wl_display_connect(nullptr);
    if (!m_Display)
    {
        throw std::runtime_error("Failed to connect to Wayland display");
    }

    // Get registry to discover global objects
    m_Registry = wl_display_get_registry(m_Display);
    if (!m_Registry)
    {
        wl_display_disconnect(m_Display);
        throw std::runtime_error("Failed to get Wayland registry");
    }

    wl_registry_add_listener(m_Registry, &s_RegistryListener, this);

    // Roundtrip to get all globals
    wl_display_roundtrip(m_Display);

    // Verify we got required globals
    if (!m_Compositor)
    {
        Shutdown();
        throw std::runtime_error("Wayland compositor not available");
    }

    if (!m_XdgWmBase)
    {
        Shutdown();
        throw std::runtime_error("XDG shell not available");
    }

    xdg_wm_base_add_listener(m_XdgWmBase, &s_XdgWmBaseListener, this);

    SOLARC_WINDOW_INFO("Wayland context initialized successfully");
}

WaylandWindowContextPlatform::~WaylandWindowContextPlatform()
{
    Shutdown();
}

void WaylandWindowContextPlatform::RegisterWindow(WindowPlatform* window)
{
    if (!window) return;

    auto* waylandPlatform = static_cast<WaylandWindowPlatform*>(window);
    wl_surface* surface = waylandPlatform->GetSurface();
    
    if (!surface) return;

    {
        std::lock_guard lock(m_WindowMapMutex);
        m_WindowMap[surface] = this;
    }

    // Set user data on surface for event callbacks
    wl_surface_set_user_data(surface, this);
    
    SOLARC_WINDOW_TRACE("Wayland window registered");
}

void WaylandWindowContextPlatform::UnregisterWindow(WindowPlatform* window)
{
    if (!window) return;

    auto* waylandPlatform = static_cast<WaylandWindowPlatform*>(window);
    wl_surface* surface = waylandPlatform->GetSurface();
    
    if (!surface) return;

    // Clear user data first to prevent callbacks from accessing this context
    wl_surface_set_user_data(surface, nullptr);

    {
        std::lock_guard lock(m_WindowMapMutex);
        m_WindowMap.erase(surface);
    }

    // Process any pending events for this surface to prevent race conditions
    if (m_Display)
    {
        wl_display_dispatch_pending(m_Display);
        wl_display_flush(m_Display);
    }
    
    SOLARC_WINDOW_TRACE("Wayland window unregistered");
}

void WaylandWindowContextPlatform::PollEvents()
{
    if (!m_Display || m_ShuttingDown) return;

    // Dispatch pending events (non-blocking, like PeekMessage on Windows)
    wl_display_dispatch_pending(m_Display);
    wl_display_flush(m_Display);
}

void WaylandWindowContextPlatform::Shutdown()
{
    if (m_ShuttingDown) return;
    
    m_ShuttingDown = true;
    
    std::lock_guard lock(m_WindowMapMutex);

    // Clear all window associations
    for (auto& [surface, _] : m_WindowMap)
    {
        wl_surface_set_user_data(surface, nullptr);
    }
    m_WindowMap.clear();

    if (m_XdgWmBase)
    {
        xdg_wm_base_destroy(m_XdgWmBase);
        m_XdgWmBase = nullptr;
    }

    if (m_Compositor)
    {
        wl_compositor_destroy(m_Compositor);
        m_Compositor = nullptr;
    }

    if (m_Registry)
    {
        wl_registry_destroy(m_Registry);
        m_Registry = nullptr;
    }

    if (m_Display)
    {
        wl_display_disconnect(m_Display);
        m_Display = nullptr;
    }

    SOLARC_WINDOW_INFO("Wayland context shut down");
}
wl_surface* WaylandWindowContextPlatform::CreateSurface()
{
    if (!m_Compositor)
    {
        SOLARC_WINDOW_ERROR("Cannot create surface: compositor not available");
        return nullptr;
    }

    return wl_compositor_create_surface(m_Compositor);
}

xdg_surface* WaylandWindowContextPlatform::CreateXdgSurface(wl_surface* surface)
{
    if (!m_XdgWmBase || !surface)
    {
        SOLARC_WINDOW_ERROR("Cannot create XDG surface: prerequisites not met");
        return nullptr;
    }

    return xdg_wm_base_get_xdg_surface(m_XdgWmBase, surface);
}

void WaylandWindowContextPlatform::DispatchCloseEvent(wl_surface* surface)
{
    // Safety check: only dispatch if not shutting down and surface is registered
    if (m_ShuttingDown) return;
    
    {
        std::lock_guard lock(m_WindowMapMutex);
        if (m_WindowMap.find(surface) == m_WindowMap.end())
        {
            // Surface was unregistered, don't dispatch event
            return;
        }
    }

    auto ev = std::make_shared<WindowCloseEvent>(surface);
    DispatchWindowEvent(ev);
}

void WaylandWindowContextPlatform::registry_global(
    void* data,
    wl_registry* registry,
    uint32_t name,
    const char* interface,
    uint32_t version)
{
    auto* context = static_cast<WaylandWindowContextPlatform*>(data);

    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        context->m_Compositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, 4));
        SOLARC_WINDOW_TRACE("Bound wl_compositor");
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        context->m_XdgWmBase = static_cast<xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
        SOLARC_WINDOW_TRACE("Bound xdg_wm_base");
    }
}

void WaylandWindowContextPlatform::registry_global_remove(
    void* data,
    wl_registry* registry,
    uint32_t name)
{
    SOLARC_WINDOW_TRACE("Wayland global removed: {}", name);
}

void WaylandWindowContextPlatform::xdg_wm_base_ping(
    void* data,
    xdg_wm_base* xdg_wm_base,
    uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

#endif