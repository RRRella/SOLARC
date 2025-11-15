#pragma once

// Abstract platform for a window (Win32, Wayland).

class WindowPlatform
{
public:
    WindowPlatform(const std::string& title, const int32_t& width, const int32_t& height) :
        m_Title(title) , m_Width(width > 0 ? width : 800) , m_Height(height > 0 ? height : 600)
    {};
    virtual ~WindowPlatform() = default;

    // Show the platform window.
    virtual void Show() = 0;

    // Hide the platform window.
    virtual void Hide() = 0;

    // Query whether the platform window is visible.
    virtual bool IsVisible() const = 0;

    const std::string& GetTitle() const { return m_Title; }

    const int32_t& GetWidth() const { return m_Width; }

    const int32_t& GetHeight() const { return m_Height; }

protected:
    std::string m_Title;
    int32_t m_Width;
    int32_t m_Height;
};
