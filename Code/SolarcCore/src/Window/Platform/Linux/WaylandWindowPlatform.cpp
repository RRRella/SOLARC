#ifdef __linux__

#include "Window/WindowPlatform.h"
#include "Window/WindowContextPlatform.h"
#include "Logging/LogMacros.h"
#include <stdexcept>

const xdg_surface_listener WindowPlatform::s_XdgSurfaceListener = {
    .configure = xdg_surface_configure
};

const xdg_toplevel_listener WindowPlatform::s_XdgToplevelListener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close
};

WindowPlatform::WindowPlatform(
    const std::string& title, const int32_t& width, const int32_t& height)
    :m_Title(title)
    , m_Width(width > 0 ? width : 800)
    , m_Height(height > 0 ? height : 600)
    , m_Surface(nullptr)
    , m_XdgSurface(nullptr)
    , m_XdgToplevel(nullptr)
    , m_Visible(false)
    , m_Configured(false)
{
    auto& context = WindowContextPlatform::Get(); // Use singleton

    // Create Wayland surface
    m_Surface = context.CreateSurface();
    if (!m_Surface)
    {
        throw std::runtime_error("Failed to create Wayland surface");
    }

    // Create XDG surface
    m_XdgSurface = context.CreateXdgSurface(m_Surface);
    if (!m_XdgSurface)
    {
        wl_surface_destroy(m_Surface);
        throw std::runtime_error("Failed to create XDG surface");
    }

    xdg_surface_add_listener(m_XdgSurface, &s_XdgSurfaceListener, this);

    // Create XDG toplevel (window)
    m_XdgToplevel = xdg_surface_get_toplevel(m_XdgSurface);
    if (!m_XdgToplevel)
    {
        xdg_surface_destroy(m_XdgSurface);
        wl_surface_destroy(m_Surface);
        throw std::runtime_error("Failed to create XDG toplevel");
    }

    xdg_toplevel_add_listener(m_XdgToplevel, &s_XdgToplevelListener, this);

    // Set initial properties
    xdg_toplevel_set_title(m_XdgToplevel, m_Title.c_str());
    xdg_toplevel_set_app_id(m_XdgToplevel, "com.solarc.engine");

    // Request server-side decorations if available
    // Note: This requires xdg-decoration protocol which we'll add later if needed
    
    // Commit the surface to apply initial configuration
    wl_surface_commit(m_Surface);

    SOLARC_WINDOW_TRACE("Wayland window platform created: '{}'", m_Title);
}

WindowPlatform::~WindowPlatform()
{
    if (m_XdgToplevel)
        xdg_toplevel_destroy(m_XdgToplevel);
    
    if (m_XdgSurface)
        xdg_surface_destroy(m_XdgSurface);
    
    if (m_Surface)
        wl_surface_destroy(m_Surface);

    SOLARC_WINDOW_TRACE("Wayland window platform destroyed: '{}'", m_Title);
}

void WindowPlatform::Show()
{
    if (!m_Visible)
    {
        m_Visible = true;
        
        // Wait for initial configure before showing
        if (!m_Configured)
        {
            SOLARC_WINDOW_DEBUG("Waiting for XDG surface configure before showing: '{}'", m_Title);
            // The surface will become visible after configure event
        }
        else
        {
            wl_surface_commit(m_Surface);
            SOLARC_WINDOW_DEBUG("Wayland window shown: '{}'", m_Title);
        }
    }
}

void WindowPlatform::Hide()
{
    if (m_Visible)
    {
        m_Visible = false;
        
        // Wayland doesn't have explicit hide - we can attach a null buffer
        wl_surface_attach(m_Surface, nullptr, 0, 0);
        wl_surface_commit(m_Surface);
        
        SOLARC_WINDOW_DEBUG("Wayland window hidden: '{}'", m_Title);
    }
}

bool WindowPlatform::IsVisible() const
{
    return m_Visible && m_Configured;
}

void WindowPlatform::SetTitle(const std::string& title)
{
    m_Title = title;
    
    if (m_XdgToplevel)
    {
        xdg_toplevel_set_title(m_XdgToplevel, title.c_str());
        SOLARC_WINDOW_DEBUG("Wayland window title changed: '{}'", title);
    }
}

void WindowPlatform::Resize(int32_t width, int32_t height)
{
    m_Width = width;
    m_Height = height;
    
    // Note: On Wayland, we can only suggest sizes via min/max size hints
    // The compositor ultimately decides the actual size
    if (m_XdgToplevel)
    {
        xdg_toplevel_set_min_size(m_XdgToplevel, width, height);
        xdg_toplevel_set_max_size(m_XdgToplevel, width, height);
        wl_surface_commit(m_Surface);
        
        SOLARC_WINDOW_DEBUG("Wayland window resize requested: {}x{}", width, height);
    }
}

void WindowPlatform::Minimize()
{
    if (m_XdgToplevel)
    {
        xdg_toplevel_set_minimized(m_XdgToplevel);
        SOLARC_WINDOW_DEBUG("Wayland window minimize requested: '{}'", m_Title);
    }
}

void WindowPlatform::Maximize()
{
    if (m_XdgToplevel)
    {
        xdg_toplevel_set_maximized(m_XdgToplevel);
        wl_surface_commit(m_Surface);
        SOLARC_WINDOW_DEBUG("Wayland window maximize requested: '{}'", m_Title);
    }
}

void WindowPlatform::Restore()
{
    if (m_XdgToplevel)
    {
        xdg_toplevel_unset_maximized(m_XdgToplevel);
        xdg_toplevel_unset_fullscreen(m_XdgToplevel);
        wl_surface_commit(m_Surface);
        SOLARC_WINDOW_DEBUG("Wayland window restore requested: '{}'", m_Title);
    }
}

void WindowPlatform::HandleConfigure(int32_t width, int32_t height)
{
    if (width > 0 && height > 0)
    {
        m_Width = width;
        m_Height = height;
        SOLARC_WINDOW_TRACE("Wayland window configured: {}x{}", width, height);
    }
    
    m_Configured = true;
}

void WindowPlatform::HandleClose()
{
    SOLARC_WINDOW_INFO("Wayland window close requested: '{}'", m_Title);
    // Context will dispatch close event
}

void WindowPlatform::xdg_surface_configure(
    void* data,
    xdg_surface* xdg_surface,
    uint32_t serial)
{
    // NULL safety check
    if (!data) return;
    
    auto* window = static_cast<WindowPlatform*>(data);
    
    // Acknowledge the configure event
    xdg_surface_ack_configure(xdg_surface, serial);
    
    // Surface is now configured and can be shown
    if (window->m_Visible && window->m_Surface)
    {
        wl_surface_commit(window->m_Surface);
    }
    
    SOLARC_WINDOW_TRACE("XDG surface configure acknowledged: '{}'", window->m_Title);
}

void WindowPlatform::xdg_toplevel_configure(
    void* data,
    xdg_toplevel* toplevel,
    int32_t width,
    int32_t height,
    wl_array* states)
{
    // NULL safety check
    if (!data) return;
    
    auto* window = static_cast<WindowPlatform*>(data);
    window->HandleConfigure(width, height);
}

void WindowPlatform::xdg_toplevel_close(
    void* data,
    xdg_toplevel* toplevel)
{
    if (!data) return;
    auto* window = static_cast<WindowPlatform*>(data);
    window->HandleClose();

    auto ev = std::make_shared<WindowCloseEvent>();
    window->ReceiveWindowEvent(std::move(ev));
}

void* WindowPlatform::GetNativeHandle() const
{
    return m_Surface;
}

#endif