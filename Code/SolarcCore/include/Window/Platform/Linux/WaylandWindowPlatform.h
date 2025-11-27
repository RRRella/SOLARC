#pragma once

#ifdef linux

#include "Window/WindowPlatform.h"
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

class WaylandWindowContextPlatform;

class WaylandWindowPlatform : public WindowPlatform
{
public:
    WaylandWindowPlatform(
        const std::string& title,
        int32_t width,
        int32_t height,
        WaylandWindowContextPlatform* context);
    
    ~WaylandWindowPlatform() override;

    void Show() override;
    void Hide() override;
    bool IsVisible() const override;
    void* GetNativeHandle() const override { return m_Surface; }
    
    void SetTitle(const std::string& title) override;
    void Resize(int32_t width, int32_t height) override;
    void Minimize() override;
    void Maximize() override;
    void Restore() override;

    wl_surface* GetSurface() const { return m_Surface; }
    xdg_surface* GetXdgSurface() const { return m_XdgSurface; }
    xdg_toplevel* GetXdgToplevel() const { return m_XdgToplevel; }

    // Called by context when configure event arrives
    void HandleConfigure(int32_t width, int32_t height);
    void HandleClose();

private:
    static void xdg_surface_configure(void* data, xdg_surface* xdg_surface, uint32_t serial);
    static void xdg_toplevel_configure(void* data, xdg_toplevel* toplevel,
        int32_t width, int32_t height, wl_array* states);
    static void xdg_toplevel_close(void* data, xdg_toplevel* toplevel);

    WaylandWindowContextPlatform* m_Context;
    wl_surface* m_Surface;
    xdg_surface* m_XdgSurface;
    xdg_toplevel* m_XdgToplevel;
    bool m_Visible;
    bool m_Configured;
    
    static const xdg_surface_listener s_XdgSurfaceListener;
    static const xdg_toplevel_listener s_XdgToplevelListener;
};

#endif