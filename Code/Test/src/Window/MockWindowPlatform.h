
// ============================================================================
// Mock Window Platform
// ============================================================================

/**
 * A mock platform that satisfies WindowPlatformConcept.
 *
 * It must inherit from EventProducer<WindowEvent> because
 * WindowT registers the platform as a producer in its constructor.
 */

class MockWindowPlatform : public EventProducer<WindowEvent>
{
public:
    std::string m_Title;
    int32_t m_Width;
    int32_t m_Height;

    // State tracking
    bool m_IsVisible = false;
    bool m_IsMinimized = false;

    MockWindowPlatform(const std::string& title, int32_t width, int32_t height)
        : m_Title(title), m_Width(width), m_Height(height)
    {
    }

    // -- Getters --
    const std::string& GetTitle() const { return m_Title; }
    const int32_t& GetWidth() const { return m_Width; }
    const int32_t& GetHeight() const { return m_Height; }
    bool IsVisible() const { return m_IsVisible; }
    bool IsMinimized() const { return m_IsMinimized; }

    // -- Commands (Simulate OS Requests) --

    void Show()
    {
        m_IsVisible = true;
        // In the real app, this sends a request to the OS.
        // For testing, we simulate the OS immediately responding with an event.
        DispatchEvent(std::make_shared<WindowShownEvent>());
    }

    void Hide()
    {
        m_IsVisible = false;
        // Simulate OS response
        DispatchEvent(std::make_shared<WindowHiddenEvent>());
    }

    void Resize(int32_t width, int32_t height)
    {
        m_Width = width;
        m_Height = height;
        // Simulate OS response: Dispatch Resize Event
        DispatchEvent(std::make_shared<WindowResizeEvent>(width, height));
    }

    void Minimize()
    {
        m_IsMinimized = true;
        // Simulate OS response: Dispatch Minimized Event
        DispatchEvent(std::make_shared<WindowMinimizedEvent>());
    }

    void Maximize()
    {
        m_IsMinimized = false;
        DispatchEvent(std::make_shared<WindowMaximizedEvent>());
    }

    void Restore()
    {
        m_IsMinimized = false;
        // Simulate OS response: Dispatch Restored Event
        DispatchEvent(std::make_shared<WindowRestoredEvent>());
    }

    // === Input Interface ===
    bool HasKeyboardFocus() const { return m_HasFocus; }
    const InputFrame& GetThisFrameInput() const { return m_InputFrame; }
    void ResetThisFrameInput() { m_InputFrame.Reset(); }
    void OnFocusLost()
    {
        // Simulate releasing all pressed keys
        for (const auto& kt : m_InputFrame.keyTransitions) {
            if (kt.pressed) {
                m_InputFrame.keyTransitions.push_back({ kt.scancode, false, false });
            }
        }
        m_HasFocus = false;
    }

    // Expose mutable access for tests
    InputFrame& MutableThisFrameInput() { return m_InputFrame; }

    bool m_HasFocus = true;
    InputFrame m_InputFrame;
};

// Define the Window type using our Mock Platform
using TestWindow = WindowT<MockWindowPlatform>;