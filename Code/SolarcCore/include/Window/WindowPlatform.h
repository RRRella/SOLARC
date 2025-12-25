#pragma once
#include <string>
#include <memory>
#include "Event/EventProducer.h"
#include "Event/WindowEvent.h"
#include "Window/WindowContextPlatform.h"
#include "Preprocessor/API.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <wayland-client.h>
#endif

class WindowPlatform : public EventProducer<WindowEvent>
{
public:
    // Restored: Only title, width, height
    WindowPlatform(const std::string& title, const int32_t& width, const int32_t& height);
    ~WindowPlatform();

    // -- Commands (Tell the OS what to do) --
    void Show();
    void Hide();
    void Resize(int32_t width, int32_t height);
    void Minimize();
    void Maximize();
    void Restore();
    void SetTitle(const std::string& title);

    // -- Getters --
    const std::string& GetTitle() const { return m_Title; }
    const int32_t& GetWidth() const { return m_Width; }
    const int32_t& GetHeight() const { return m_Height; }
    bool IsVisible() const;
    bool IsMinimized() const;
    bool IsMaximized() const;

    // Helper for internal dispatch
    void DispatchWindowEvent(const std::shared_ptr<const WindowEvent>& e)
    {
        DispatchEvent(e);
    }

#ifdef _WIN32
    HWND GetWin32Handle() const { return m_hWnd; }
#elif defined(linux)
    wl_surface* GetWaylandSurface() const { return m_Surface; }
#endif

private:
    std::string m_Title;
    int32_t m_Width;
    int32_t m_Height;
    bool m_Visible = false;
    bool m_Minimized = false;
    bool m_Maximized = false;

    mutable std::mutex mtx;

    void SetDimensions(int32_t width, int32_t height)
    {
        std::lock_guard lk(mtx);
        m_Width = width;
        m_Height = height;
    }

#ifdef _WIN32
    HWND m_hWnd = nullptr;
    friend LRESULT CALLBACK WindowContextPlatform::WndProc(HWND, UINT, WPARAM, LPARAM);

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