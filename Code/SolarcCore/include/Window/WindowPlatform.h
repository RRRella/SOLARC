#pragma once
#include <string>
#include "Event/EventProducer.h"
#include <memory>
#include "Preprocessor/API.h"
#include "Event/WindowEvent.h"
#include "Window/WindowContextPlatform.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#endif

class SOLARC_CORE_API WindowPlatform : public EventProducer<WindowEvent>
{
public:
    // Restored: Only title, width, height
    WindowPlatform(const std::string& title, const int32_t& width, const int32_t& height);
    ~WindowPlatform();

    void Show();
    void Hide();
    bool IsVisible() const;
    void* GetNativeHandle() const;

    void Resize(int32_t width, int32_t height);
    void Minimize();
    void Maximize();
    void Restore();

    void SetTitle(const std::string& title);

    const std::string& GetTitle() const { return m_Title; }
    const int32_t& GetWidth() const { return m_Width; }
    const int32_t& GetHeight() const { return m_Height; }

    void ReceiveWindowEvent(const std::shared_ptr<const WindowEvent>& e)
    {
        DispatchEvent(e);
    }

private:
    std::string m_Title;
    int32_t m_Width;
    int32_t m_Height;
    bool m_Visible = false;

#ifdef _WIN32
    HWND m_hWnd = nullptr;

#elif defined(__linux__)
    void HandleConfigure(int32_t width, int32_t height);
    void HandleClose();

    static void xdg_surface_configure(void* data, xdg_surface* xdg_surface, uint32_t serial);
    static void xdg_toplevel_configure(void* data, xdg_toplevel* toplevel,
        int32_t width, int32_t height, wl_array* states);
    static void xdg_toplevel_close(void* data, xdg_toplevel* toplevel);

    wl_surface* m_Surface = nullptr;
    xdg_surface* m_XdgSurface = nullptr;
    xdg_toplevel* m_XdgToplevel = nullptr;
    bool m_Configured = false;

    static const xdg_surface_listener s_XdgSurfaceListener;
    static const xdg_toplevel_listener s_XdgToplevelListener;
#endif
};