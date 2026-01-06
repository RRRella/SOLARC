#pragma once
#include <string>
#include <memory>
#include "Event/EventProducer.h"
#include "Event/WindowEvent.h"
#include "Window/WindowContextPlatform.h"
#include "Preprocessor/API.h"
#include "Input/KeyCode.h"
#include "Input/MouseButton.h"
#include "Input/InputFrame.h"

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

    /**
    * Reset this frame's input accumulator.
    * Called by WindowContext::PollEvents() at the start of each frame.
    *
    * Must be called BEFORE OS event processing begins.
    */
    void ResetThisFrameInput()
    {
        std::lock_guard lk(mtx);
        m_ThisFrameInput.Reset();
    }

    /**
     * Get the input captured this frame (read-only).
     * Called by Window::UpdateInput() to process input.
     */
    const InputFrame& GetThisFrameInput() const { return m_ThisFrameInput; }

    /**
     * Check if this window currently has keyboard focus.
     *
     * Returns true if the window can receive keyboard input.
     * Used by Window::UpdateInput() to gate keyboard state updates.
     */
    bool HasKeyboardFocus() const
    {
        std::lock_guard lk(mtx);
        return m_HasKeyboardFocus;
    }

    /**
     * Handle focus loss.
     *
     * Called when window loses keyboard focus (WM_KILLFOCUS / wl_keyboard::leave).
     * Synthesizes key release transitions for all currently held keys to prevent
     * stuck input (user won't receive WM_KEYUP for keys held during Alt+Tab).
     */
    void OnFocusLost();


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
#elif defined(__linux__)
    wl_surface* GetWaylandSurface() const { return m_Surface; }
#endif

private:

    void SetKeyboardFocus(bool focused)
    {
        std::lock_guard lock(mtx);
        m_HasKeyboardFocus = focused;
    }

    void RecordKeyTransition(uint16_t scancode, bool pressed, bool isRepeat)
    {
        std::lock_guard lock(mtx);
        if (scancode < m_CurrentKeyState.size())
        {
            if (pressed && !isRepeat)
            {
                m_CurrentKeyState[scancode] = true;
            }
            else if (!pressed)
            {
                m_CurrentKeyState[scancode] = false;
            }
            m_ThisFrameInput.keyTransitions.emplace_back(scancode, pressed, isRepeat);
        }
    }

    void RecordMousePosition(int32_t x, int32_t y)
    {
        std::lock_guard lock(mtx);


        if (!m_MousePositionInitialized)
        {
            // First mouse event: initialize without delta
            m_ThisFrameInput.mouseX = x;
            m_ThisFrameInput.mouseY = y;
            m_PrevMouseX = x;
            m_PrevMouseY = y;
            m_MousePositionInitialized = true;
            return;
        }

        int32_t deltaX = x - m_PrevMouseX;
        int32_t deltaY = y - m_PrevMouseY;
        m_ThisFrameInput.mouseDeltaX += deltaX;
        m_ThisFrameInput.mouseDeltaY += deltaY;
        m_ThisFrameInput.mouseX = x;
        m_ThisFrameInput.mouseY = y;
        m_PrevMouseX = x;
        m_PrevMouseY = y;
    }

    void RecordMouseButton(MouseButton button, bool pressed)
    {
        std::lock_guard lock(mtx);
        m_ThisFrameInput.mouseButtonTransitions.emplace_back(button, pressed);
    }

    void RecordMouseWheel(float verticalDelta, float horizontalDelta)
    {
        std::lock_guard lock(mtx);
        m_ThisFrameInput.wheelDelta += verticalDelta;
        m_ThisFrameInput.hWheelDelta += horizontalDelta;
    }

    std::string m_Title;
    int32_t m_Width;
    int32_t m_Height;
    bool m_Visible = false;
    bool m_Maximized = false;

    mutable std::recursive_mutex mtx;

    /**
     * Per-frame input accumulator (populated during OS event processing).
     */
    InputFrame m_ThisFrameInput;

    /**
     * Keyboard focus state.
     * true = window has focus and can receive keyboard input.
     * false = window is unfocused, keyboard input should be ignored.
     */
    bool m_HasKeyboardFocus = false;

    /**
     * Current keyboard state (which keys are held down).
     * Indexed by scancode (0-511).
     *
     * Used to synthesize release events on focus loss (we need to know
     * which keys were held so we can release them).
     */
    std::array<bool, 512> m_CurrentKeyState;

#ifdef _WIN32

    void SyncDimensions(int32_t width, int32_t height)
    {
        std::lock_guard lk(mtx);
        m_Width = width;
        m_Height = height;
    }

    void SyncVisibility(bool visible)
    {
        std::lock_guard lk(mtx);
        m_Visible = visible;
    }

    void SyncMaximized(bool maximized)
    {
        std::lock_guard lk(mtx);
        m_Maximized = maximized;
    }

    friend LRESULT CALLBACK WindowContextPlatform::WndProc(HWND, UINT, WPARAM, LPARAM);
    void SyncMinimized(bool minimized)
    {
        std::lock_guard lk(mtx);
        m_Minimized = minimized;
    }

    HWND m_hWnd = nullptr;
    bool m_Minimized = false;
    bool m_MousePositionInitialized = false;

    /**
    * Previous mouse position (for delta computation).
    * Win32 gives us absolute position in WM_MOUSEMOVE, we compute delta.
    */
    int32_t m_PrevMouseX = 0;
    int32_t m_PrevMouseY = 0;
  
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
    zxdg_toplevel_decoration_v1* m_Decoration = nullptr;
    bool m_Configured = false;

    static const xdg_surface_listener s_XdgSurfaceListener;
    static const xdg_toplevel_listener s_XdgToplevelListener;

    friend class WindowContextPlatform;

#endif
};