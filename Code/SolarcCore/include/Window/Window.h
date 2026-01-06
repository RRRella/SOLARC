#pragma once
#include "WindowPlatform.h"
#include "Event/EventListener.h"
#include "Event/EventProducer.h"
#include "Event/ObserverBus.h"
#include "Event/WindowEvent.h"
#include "Logging/LogMacros.h"
#include "Preprocessor/API.h"
#include <memory>
#include <mutex>
#include <functional>
#include <type_traits>
#include <stdexcept>
#include "Input/InputState.h"
#include "Input/KeyCode.h"
#include "Input/MouseButton.h"
#include "Event/InputEvent.h"
#include "MT/ThreadChecker.h"
#include "Input/KeyMapping.h"

template<typename T>
concept WindowPlatformConcept = requires(T & t) {
    // Basic properties
    { t.GetTitle() } -> std::same_as<const std::string&>;
    { t.GetWidth() } -> std::same_as<const int32_t&>;
    { t.GetHeight() } -> std::same_as<const int32_t&>;

    // Visibility
    { t.Show() } -> std::same_as<void>;
    { t.Hide() } -> std::same_as<void>;
    { t.IsVisible() } -> std::convertible_to<bool>;
    #ifdef _Win32
    { t.IsMinimized() } -> std::convertible_to<bool>;
    #endif

    // Window State Commands
    { t.Resize(0, 0) } -> std::same_as<void>;
    { t.Minimize() } -> std::same_as<void>;
    { t.Maximize() } -> std::same_as<void>;
    { t.Restore() } -> std::same_as<void>;
    { t.OnFocusLost() } -> std::same_as<void>;

    // Window input
    { t.HasKeyboardFocus() } -> std::same_as<bool>;
    { t.GetThisFrameInput() } -> std::same_as<const InputFrame&>;
    { t.ResetThisFrameInput() } -> std::same_as<void>;

    // Events (optional but expected for integration)
    // We assume it derives from EventProducer<WindowEvent> externally
};

/**
 * Platform-agnostic window class
 *
 * Lifetime:
 * - Created by WindowContext::CreateWindow()
 * - Owned by shared_ptr (multiple owners possible)
 * - Destruction triggers callback to WindowContext for cleanup
 *
 * Thread Safety:
 * - All methods must be called from main thread only
 * - Destroy: Idempotent - safe to call multiple times
 */

template<WindowPlatformConcept PlatformT>
class WindowT : public EventListener<WindowEvent> , public EventProducer<WindowEvent>
{

public:

    using PlatformType = PlatformT;

    WindowT(
        std::unique_ptr<PlatformT> platform,
        std::function<void(WindowT<PlatformT>*)> onDestroy = nullptr
    );

    ~WindowT() override;

    /**
     * Manually destroy the window
     * note: Idempotent - safe to call multiple times
     * note: Must be called from main thread
     */
    void Destroy();

    /**
     * Show the window
     * note: Must be called from main thread
     */
    void Show();

    /**
     * Hide the window
     * note: Must be called from main thread
     */
    void Hide();

    /**
    * Resize the window
    * note: Must be called from main thread
    */
    void Resize(int32_t width, int32_t height);

    /**
     * Minimize the window
     * note: Must be called from main thread
     */
    void Minimize();

    /**
     * Maximize the window
     * note: Must be called from main thread
     */
    void Maximize();

    /**
     * Restore the window (from minimized/maximized state)
     * note: Must be called from main thread
     */
    void Restore();

    /**
     * Check if window is visible
     * return true if visible, false otherwise
     * note: Must be called from main thread
     */
    bool IsVisible() const;

    /**
    * Check if window is minimized
    * return true if minimized, false otherwise
    * note: Must be called from main thread
    */
    bool IsMinimized() const;

    /**
     * Check if window is closed
     * return true if closed, false otherwise
     * note: Must be called from main thread
     */
    bool IsClosed() const;

    // ========================================================================
    // Input State Queries (Fast Path - Polling)
    // ========================================================================

    /**
     * Check if a key is currently held down.
     *
     * Returns the immediate state from the current frame's InputState.
     * This is the "fast path" for continuous input (WASD movement, etc.)
     *
     * Thread safety: Main thread only (enforced by ThreadChecker).
     *
     * Example:
     *   if (window->IsKeyDown(KeyCode::W)) {
     *       player.MoveForward(deltaTime);
     *   }
     */
    bool IsKeyDown(KeyCode key) const;

    /**
     * Check if a key was just pressed this frame (transition: up -> down).
     *
     * Returns true only on the frame where the key transitioned to pressed.
     * Useful for one-shot actions without needing event listeners.
     *
     * Example:
     *   if (window->WasKeyJustPressed(KeyCode::Space)) {
     *       player.Jump();
     *   }
     */
    bool WasKeyJustPressed(KeyCode key) const;

    /**
     * Check if a key was just released this frame (transition: down -> up).
     */
    bool WasKeyJustReleased(KeyCode key) const;

    /**
     * Get the current repeat count for a key.
     *
     * Returns:
     * - 0 = Key is not pressed
     * - 1 = Initial press (first frame key went down)
     * - 2+ = Key repeat (key is held, OS sent repeat events)
     *
     * Useful for implementing custom repeat logic or filtering repeats.
     */
    uint16_t GetKeyRepeatCount(KeyCode key) const;

    /**
     * Get current mouse position in client-area coordinates (logical pixels).
     *
     * Origin (0,0) = top-left of window client area.
     * Positive X = right, Positive Y = down.
     *
     * Returns position even if mouse is outside window (can be negative
     * or exceed window dimensions if mouse was captured).
     */
    int32_t GetMouseX() const;
    int32_t GetMouseY() const;

    /**
     * Get mouse movement delta accumulated this frame (logical pixels).
     *
     * Represents cumulative mouse motion since last frame.
     * Useful for camera rotation, viewport panning, etc.
     *
     * Reset to (0,0) automatically each frame.
     *
     * Example:
     *   int32_t dx = window->GetMouseDeltaX();
     *   int32_t dy = window->GetMouseDeltaY();
     *   camera.Rotate(dx * sensitivity, dy * sensitivity);
     */
    int32_t GetMouseDeltaX() const;
    int32_t GetMouseDeltaY() const;

    /**
     * Check if a mouse button is currently held down.
     */
    bool IsMouseButtonDown(MouseButton button) const;

    /**
     * Check if a mouse button was just pressed this frame.
     */
    bool WasMouseButtonJustPressed(MouseButton button) const;

    /**
     * Check if a mouse button was just released this frame.
     */
    bool WasMouseButtonJustReleased(MouseButton button) const;

    /**
     * Get mouse wheel delta accumulated this frame (vertical scroll).
     *
     * Positive = scroll up, Negative = scroll down.
     * Reset to 0.0f automatically each frame.
     */
    float GetMouseWheelDelta() const;

    /**
     * Get mouse wheel delta accumulated this frame (horizontal scroll).
     *
     * Positive = scroll right, Negative = scroll left.
     * Reset to 0.0f automatically each frame.
     * *
     * Note: Not all mice support horizontal scrolling.
     */
    float GetMouseWheelHDelta() const;

    // ========================================================================
    // Modifier Key Helpers
    // ========================================================================

    /**
     * Check if ANY Shift key is down (left or right).
     */
    bool IsShiftDown() const;

    /**
     * Check if ANY Ctrl key is down (left or right).
     */
    bool IsCtrlDown() const;

    /**
     * Check if ANY Alt key is down (left or right).
     */
    bool IsAltDown() const;

    /**
     * Check if ANY Super/Windows/Command key is down (left or right).
     */
    bool IsSuperDown() const;

    /**
     * Check specific left/right modifier keys.
     */
    bool IsLeftShiftDown() const;
    bool IsRightShiftDown() const;
    bool IsLeftCtrlDown() const;
    bool IsRightCtrlDown() const;
    bool IsLeftAltDown() const;
    bool IsRightAltDown() const;
    bool IsLeftSuperDown() const;
    bool IsRightSuperDown() const;

    /**
     * Get window title
     */
    const std::string& GetTitle() const { return m_Platform->GetTitle(); }

    /**
     * Get window width
     */
    const int32_t& GetWidth() const { return m_Platform->GetWidth(); }

    /**
     * Get window height
     */
    const int32_t& GetHeight() const { return m_Platform->GetHeight(); }

    /**
     * Get platform handle (for internal use by WindowContext)
     * return Raw pointer to platform (never null while window is alive)
     */
    PlatformT* GetPlatform() const { return m_Platform.get(); }

    /**
     * Process queued window events
     * note: Must be called from main thread
     */
    void Update();

protected:
    /**
     * Handle a window event
     * param e: Event to handle
     * note: Override to customize event handling
     * note: Automatically filters events by window handle
     */
    void OnEvent(const std::shared_ptr<const WindowEvent>& e) override;

    /**
    * Process input captured this frame.
    *
    * Called by Update() before event communication.
    *
    * Responsibilities:
    * 1. Check keyboard focus (skip keyboard input if unfocused)
    * 2. Read WindowPlatform::GetThisFrameInput()
    * 3. Apply transitions to m_CurrentInput
    * 4. Detect state changes (compare current vs previous)
    * 5. Emit transition events (KeyPressed, MouseButtonDown, etc.)
    * 6. Swap current <-> previous for next frame
    */
    void UpdateInput();

    /**
     * Map scancode to KeyCode.
     *
     * Platform-specific mapping will be implemented in Step 6.
     * For now, returns KeyCode::Unknown as a stub.
     */
    KeyCode ScancodeToKeyCode(uint16_t scancode) const;

    // ========================================================================
    // Input State
    // ========================================================================

    /**
     * Current frame's input state (updated by UpdateInput).
     */
    InputState m_CurrentInput;

    /**
     * Previous frame's input state (for transition detection).
     */
    InputState m_PreviousInput;

    /**
     * Thread checker for input queries.
     * Ensures all input access happens on main thread.
     */
    ThreadChecker m_InputThreadChecker;

private:
    std::unique_ptr<PlatformT> m_Platform;
    std::function<void(WindowT<PlatformT>*)> m_OnDestroy;
    bool m_Destroyed = false;
    mutable std::mutex m_DestroyMutex;

    ObserverBus<WindowEvent> m_Bus;
};

extern template class WindowT<WindowPlatform>;

// Production alias
using Window = WindowT<WindowPlatform>;

template<WindowPlatformConcept PlatformT>
inline WindowT<PlatformT>::WindowT(
    std::unique_ptr<PlatformT> platform,
    std::function<void(WindowT<PlatformT>*)> onDestroy)
    : m_Platform(std::move(platform))
    , m_OnDestroy(std::move(onDestroy))
{
    if (!m_Platform)
    {
        SOLARC_ERROR("Window: Platform cannot be null");
        throw std::invalid_argument("Window platform must not be null");
    }

    m_Bus.RegisterProducer(m_Platform.get());
    m_Bus.RegisterListener(this);

    SOLARC_WINDOW_TRACE("Window created: '{}'", m_Platform->GetTitle());
}

template<WindowPlatformConcept PlatformT>
inline WindowT<PlatformT>::~WindowT()
{
    SOLARC_WINDOW_TRACE("Window destructor: '{}'",
        m_Platform ? m_Platform->GetTitle() : "null");
    Destroy();
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Destroy()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Destroyed) return;

    m_Destroyed = true;

    SOLARC_WINDOW_INFO("Destroying window: '{}'",
        m_Platform ? m_Platform->GetTitle() : "null");

    if (m_OnDestroy)
    {
        m_OnDestroy(this);
    }

    m_Platform.reset();
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Show()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Show();
        SOLARC_WINDOW_DEBUG("Window shown: '{}'", m_Platform->GetTitle());
    }
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Hide()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Hide();
        SOLARC_WINDOW_DEBUG("Window hidden: '{}'", m_Platform->GetTitle());
    }
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Resize(int32_t width, int32_t height)
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Resize(width, height);
        SOLARC_WINDOW_DEBUG("Window resize requested: '{}' to {}x{}",
            m_Platform->GetTitle(), width, height);
    }
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Minimize()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Minimize();
        SOLARC_WINDOW_DEBUG("Window minimize requested: '{}'", m_Platform->GetTitle());
    }
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Maximize()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Maximize();
        SOLARC_WINDOW_DEBUG("Window maximize requested: '{}'", m_Platform->GetTitle());
    }
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Restore()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Restore();
        SOLARC_WINDOW_DEBUG("Window restore requested: '{}'", m_Platform->GetTitle());
    }
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsVisible() const
{
    std::lock_guard lock(m_DestroyMutex);
    return m_Platform && !m_Destroyed ? m_Platform->IsVisible() : false;
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsMinimized() const
{
    std::lock_guard lock(m_DestroyMutex);
    return m_Platform && !m_Destroyed ? m_Platform->IsMinimized() : false;
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsClosed() const
{
    std::lock_guard lock(m_DestroyMutex);
    return m_Destroyed;
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Update()
{
    // ========================================================================
    // Step 1: Process input FIRST (before event communication)
    // ========================================================================
    UpdateInput();

    // ========================================================================
    // Step 2: Communicate events through the bus
    // ========================================================================
    // Process all queued events

    m_Bus.Communicate();

    ProcessEvents();
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::OnEvent(const std::shared_ptr<const WindowEvent>& e)
{
    switch (e->GetWindowEventType())
    {

    case WindowEvent::TYPE::CLOSE:
    {
        SOLARC_WINDOW_DEBUG("Window close event received: '{}'", GetTitle());
        Destroy();
        break;
    }

    case WindowEvent::TYPE::SHOWN:
    {
        // Update Platform State
        SOLARC_WINDOW_DEBUG("Window shown event: '{}'", GetTitle());
        break;
    }

    case WindowEvent::TYPE::HIDDEN:
    {
        // Update Platform State
        SOLARC_WINDOW_DEBUG("Window hidden event: '{}'", GetTitle());
        break;
    }

    case WindowEvent::TYPE::RESIZED:
    {
        auto resizeEvent = std::static_pointer_cast<const WindowResizeEvent>(e);

        SOLARC_WINDOW_DEBUG("Window resize event: '{}' ({}x{})",
            GetTitle(), resizeEvent->GetWidth(), resizeEvent->GetHeight());
        break;
    }

    case WindowEvent::TYPE::MINIMIZED:
    {
        SOLARC_WINDOW_DEBUG("Window minimized event: '{}'", GetTitle());
        break;
    }

    case WindowEvent::TYPE::MAXIMIZED:
    {
        SOLARC_WINDOW_DEBUG("Window maximized event: '{}'", GetTitle());
        break;
    }

    case WindowEvent::TYPE::RESTORED:
    {
        SOLARC_WINDOW_DEBUG("Window restored event: '{}'", GetTitle());
        break;
    }

    default:
        SOLARC_WINDOW_DEBUG("Window generic event: '{}'", GetTitle());
        break;
    }

    DispatchEvent(e);
}


// ============================================================================
// Input System Implementation
// ============================================================================

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::UpdateInput()
{
    std::lock_guard lock(m_DestroyMutex);
    if (!m_Platform || m_Destroyed) return;

    // ========================================================================
    // Swap previous ? current (for transition detection)
    // ========================================================================
    m_PreviousInput = m_CurrentInput;

    // ========================================================================
    // Check keyboard focus
    // ========================================================================
    bool hasKeyboardFocus = m_Platform->HasKeyboardFocus();

    // ========================================================================
    // Read captured input from platform
    // ========================================================================
    const auto& thisFrame = m_Platform->GetThisFrameInput();

    // ========================================================================
    // Update mouse state (always, even without focus)
    // ========================================================================

    // Update absolute position (last reported position)
    m_CurrentInput.mouseX = thisFrame.mouseX;
    m_CurrentInput.mouseY = thisFrame.mouseY;

    // Update accumulated deltas
    m_CurrentInput.mouseDeltaX = thisFrame.mouseDeltaX;
    m_CurrentInput.mouseDeltaY = thisFrame.mouseDeltaY;

    // Update wheel deltas
    m_CurrentInput.mouseWheelDelta = thisFrame.wheelDelta;
    m_CurrentInput.mouseWheelHDelta = thisFrame.hWheelDelta;

    // ========================================================================
    // Apply mouse button transitions
    // ========================================================================
    for (const auto& transition : thisFrame.mouseButtonTransitions)
    {
        uint8_t buttonIndex = static_cast<uint8_t>(transition.button);

        if (transition.pressed)
        {
            // Button pressed
            m_CurrentInput.mouseButtons = SetButton(m_CurrentInput.mouseButtons, transition.button);

            // Emit event
            auto evt = std::make_shared<MouseButtonDownEvent>(
                transition.button,
                m_CurrentInput.mouseX,
                m_CurrentInput.mouseY,
                IsShiftDown(),
                IsCtrlDown(),
                IsAltDown()
            );
            DispatchEvent(evt);
        }
        else
        {
            // Button released
            m_CurrentInput.mouseButtons = ClearButton(m_CurrentInput.mouseButtons, transition.button);

            // Emit event
            auto evt = std::make_shared<MouseButtonUpEvent>(
                transition.button,
                m_CurrentInput.mouseX,
                m_CurrentInput.mouseY,
                IsShiftDown(),
                IsCtrlDown(),
                IsAltDown()
            );
            DispatchEvent(evt);
        }
    }

    // ========================================================================
    // Apply keyboard transitions (only if focused)
    // ========================================================================
    if (hasKeyboardFocus)
    {
        for (const auto& transition : thisFrame.keyTransitions)
        {
            uint16_t scancode = transition.scancode;
            if (scancode >= 512) continue; // Safety check
            if (transition.pressed)
            {
                // Key pressed
                bool wasAlreadyDown = m_CurrentInput.keys[scancode];
                m_CurrentInput.keys[scancode] = true;

                if (transition.isRepeat)
                {
                    // Increment repeat count
                    m_CurrentInput.keyRepeatCount[scancode]++;
                }
                else if (!wasAlreadyDown)
                {
                    // Initial press
                    m_CurrentInput.keyRepeatCount[scancode] = 1;
                }

                // Map scancode to KeyCode
                KeyCode keyCode = ScancodeToKeyCode(scancode);

                // Emit event
                auto evt = std::make_shared<KeyPressedEvent>(
                    keyCode,
                    scancode,
                    transition.isRepeat,
                    IsShiftDown(),
                    IsCtrlDown(),
                    IsAltDown()
                );
                DispatchEvent(evt);
            }
            else
            {
                // Key released
                m_CurrentInput.keys[scancode] = false;
                m_CurrentInput.keyRepeatCount[scancode] = 0;

                // Map scancode to KeyCode
                KeyCode keyCode = ScancodeToKeyCode(scancode);

                // Emit event
                auto evt = std::make_shared<KeyReleasedEvent>(
                    keyCode,
                    scancode,
                    IsShiftDown(),
                    IsCtrlDown(),
                    IsAltDown()
                );
                DispatchEvent(evt);
            }
        }
    }
    else
        // No focus: keyboard state was already cleared by OnFocusLost()
        // Just ensure our InputState reflects that
        // (It should already be clear, but belt-and-suspenders)
        m_CurrentInput.Reset();

    // ========================================================================
    // Emit mouse wheel event if scrolling occurred
    // ========================================================================
    if (thisFrame.wheelDelta != 0.0f || thisFrame.hWheelDelta != 0.0f)
    {
        auto evt = std::make_shared<MouseWheelEvent>(
            thisFrame.wheelDelta,
            thisFrame.hWheelDelta,
            m_CurrentInput.mouseX,
            m_CurrentInput.mouseY,
            IsShiftDown(),
            IsCtrlDown(),
            IsAltDown()
        );
        DispatchEvent(evt);
    }

    // ========================================================================
    // Done: State updated, events emitted
    // ========================================================================
}

// ============================================================================
// Input Query Methods
// ============================================================================

template<WindowPlatformConcept PlatformT>
inline KeyCode WindowT<PlatformT>::ScancodeToKeyCode(uint16_t scancode) const
{
    return KeyMapping::ScancodeToKeyCode(scancode);
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsKeyDown(KeyCode key) const
{
    m_InputThreadChecker.AssertOnOwnerThread("Window::IsKeyDown");

    uint16_t scancode = KeyMapping::KeyCodeToScancode(key);
    if (scancode == 0) return false; // No mapping

    return m_CurrentInput.keys[scancode];
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::WasKeyJustPressed(KeyCode key) const
{
    m_InputThreadChecker.AssertOnOwnerThread("Window::WasKeyJustPressed");

    uint16_t scancode = KeyMapping::KeyCodeToScancode(key);
    if (scancode == 0) return false;

    bool isDown = m_CurrentInput.keys[scancode];
    bool wasDown = m_PreviousInput.keys[scancode];

    return isDown && !wasDown;
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::WasKeyJustReleased(KeyCode key) const
{
    m_InputThreadChecker.AssertOnOwnerThread("Window::WasKeyJustReleased");

    uint16_t scancode = KeyMapping::KeyCodeToScancode(key);
    if (scancode == 0) return false;

    bool isDown = m_CurrentInput.keys[scancode];
    bool wasDown = m_PreviousInput.keys[scancode];

    return !isDown && wasDown;
}

template<WindowPlatformConcept PlatformT>
inline uint16_t WindowT<PlatformT>::GetKeyRepeatCount(KeyCode key) const
{
    m_InputThreadChecker.AssertOnOwnerThread("Window::GetKeyRepeatCount");

    uint16_t scancode = KeyMapping::KeyCodeToScancode(key);
    if (scancode == 0) return 0;

    return m_CurrentInput.keyRepeatCount[scancode];
}

// ============================================================================
// Mouse Query Methods
// ============================================================================

template<WindowPlatformConcept PlatformT>
inline int32_t WindowT<PlatformT>::GetMouseX() const
{
    m_InputThreadChecker.AssertOnOwnerThread("Window::GetMouseX");
    return m_CurrentInput.mouseX;
}

template<WindowPlatformConcept PlatformT>
inline int32_t WindowT<PlatformT>::GetMouseY() const
{
    m_InputThreadChecker.AssertOnOwnerThread("Window::GetMouseY");
    return m_CurrentInput.mouseY;
}

template<WindowPlatformConcept PlatformT>
inline int32_t WindowT<PlatformT>::GetMouseDeltaX() const
{
    m_InputThreadChecker.AssertOnOwnerThread("Window::GetMouseDeltaX");
    return m_CurrentInput.mouseDeltaX;
}

template<WindowPlatformConcept PlatformT>
inline int32_t WindowT<PlatformT>::GetMouseDeltaY() const
{
    m_InputThreadChecker.AssertOnOwnerThread("Window::GetMouseDeltaY");
    return m_CurrentInput.mouseDeltaY;
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsMouseButtonDown(MouseButton button) const
{
    m_InputThreadChecker.AssertOnOwnerThread("Window::IsMouseButtonDown");
    return IsButtonSet(m_CurrentInput.mouseButtons, button);
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::WasMouseButtonJustPressed(MouseButton button) const
{
    m_InputThreadChecker.AssertOnOwnerThread("Window::WasMouseButtonJustPressed");

    bool isDown = IsButtonSet(m_CurrentInput.mouseButtons, button);
    bool wasDown = IsButtonSet(m_PreviousInput.mouseButtons, button);

    return isDown && !wasDown;
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::WasMouseButtonJustReleased(MouseButton button) const
{
    m_InputThreadChecker.AssertOnOwnerThread("Window::WasMouseButtonJustReleased");

    bool isDown = IsButtonSet(m_CurrentInput.mouseButtons, button);
    bool wasDown = IsButtonSet(m_PreviousInput.mouseButtons, button);

    return !isDown && wasDown;
}

template<WindowPlatformConcept PlatformT>
inline float WindowT<PlatformT>::GetMouseWheelDelta() const
{
    m_InputThreadChecker.AssertOnOwnerThread("Window::GetMouseWheelDelta");
    return m_CurrentInput.mouseWheelDelta;
}

template<WindowPlatformConcept PlatformT>
inline float WindowT<PlatformT>::GetMouseWheelHDelta() const
{
    m_InputThreadChecker.AssertOnOwnerThread("Window::GetMouseWheelHDelta");
    return m_CurrentInput.mouseWheelHDelta;
}


// ============================================================================
// Modifier Key Helpers
// ============================================================================

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsShiftDown() const
{
    return IsLeftShiftDown() || IsRightShiftDown();
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsCtrlDown() const
{
    return IsLeftCtrlDown() || IsRightCtrlDown();
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsAltDown() const
{
    return IsLeftAltDown() || IsRightAltDown();
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsSuperDown() const
{
    return IsLeftSuperDown() || IsRightSuperDown();
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsLeftShiftDown() const
{
    return IsKeyDown(KeyCode::LeftShift);
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsRightShiftDown() const
{
    return IsKeyDown(KeyCode::RightShift);
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsLeftCtrlDown() const
{
    return IsKeyDown(KeyCode::LeftCtrl);
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsRightCtrlDown() const
{
    return IsKeyDown(KeyCode::RightCtrl);
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsLeftAltDown() const
{
    return IsKeyDown(KeyCode::LeftAlt);
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsRightAltDown() const
{
    return IsKeyDown(KeyCode::RightAlt);
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsLeftSuperDown() const
{
    return IsKeyDown(KeyCode::LeftSuper);
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsRightSuperDown() const
{
    return IsKeyDown(KeyCode::RightSuper);
}