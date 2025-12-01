#pragma once
#include <string>
#include "Event/EventProducer.h"
#include <Event/WindowEvent.h>
#include <memory>

// Abstract platform for a window (Win32, Wayland).

class WindowPlatform : public EventProducer<WindowEvent>
{
public:
    WindowPlatform(const std::string& title, const int32_t& width, const int32_t& height)
        : m_Title(title)
        , m_Width(width > 0 ? width : 800)
        , m_Height(height > 0 ? height : 600)
    {}
    
    virtual ~WindowPlatform() = default;

    // Show the platform window.
    virtual void Show() = 0;

    // Hide the platform window.
    virtual void Hide() = 0;

    // Query whether the platform window is visible.
    virtual bool IsVisible() const = 0;

    // Get the native platform handle (HWND on Windows, wl_surface* on Wayland)
    virtual void* GetNativeHandle() const = 0;

    // Set window title (optional, may not be supported by all platforms)
    virtual void SetTitle(const std::string& title) 
    { 
        m_Title = title; 
    }

    // Request window resize (optional, actual resize may be denied by compositor)
    virtual void Resize(int32_t width, int32_t height) 
    { 
        m_Width = width; 
        m_Height = height; 
    }

    // Request window minimize (optional, may not be supported)
    virtual void Minimize() {}

    // Request window maximize (optional, may not be supported)
    virtual void Maximize() {}

    // Request window restore to normal size (optional, may not be supported)
    virtual void Restore() {}

    const std::string& GetTitle() const { return m_Title; }
    const int32_t& GetWidth() const { return m_Width; }
    const int32_t& GetHeight() const { return m_Height; }

    void ReceiveWindowEvent(const std::shared_ptr<const WindowEvent>& e)
    {
        DispatchEvent(e);
    }

protected:
    std::string m_Title;
    int32_t m_Width;
    int32_t m_Height;
};