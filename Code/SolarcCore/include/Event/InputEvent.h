#pragma once
#include "Event/WindowEvent.h"
#include "Input/KeyCode.h"
#include "Input/MouseButton.h"
#include <cstdint>

/**
 * ============================================================================
 * Input Event Usage Examples
 * ============================================================================
 * 
 * Example 1: Listen for key press (game logic)
 * -----------------------------------------------
 * class PlayerController : public EventListener<WindowEvent> {
 *     void OnEvent(const std::shared_ptr<const WindowEvent>& e) override {
 *         if (e->GetWindowEventType() == WindowEvent::TYPE::KEY_PRESSED) {
 *             auto keyEvent = std::static_pointer_cast<const KeyPressedEvent>(e);
 *             
 *             // Jump on Space (ignore repeats)
 *             if (keyEvent->GetKeyCode() == KeyCode::Space && !keyEvent->IsRepeat()) {
 *                 m_Player->Jump();
 *             }
 *             
 *             // Toggle menu on Escape
 *             if (keyEvent->GetKeyCode() == KeyCode::Escape && !keyEvent->IsRepeat()) {
 *                 m_Menu->Toggle();
 *             }
 *         }
 *     }
 * };
 * 
 * Example 2: Listen for mouse clicks (UI)
 * -----------------------------------------
 * class Button : public EventListener<WindowEvent> {
 *     void OnEvent(const std::shared_ptr<const WindowEvent>& e) override {
 *         if (e->GetWindowEventType() == WindowEvent::TYPE::MOUSE_BUTTON_DOWN) {
 *             auto mouseEvent = std::static_pointer_cast<const MouseButtonDownEvent>(e);
 *             
 *             if (mouseEvent->GetButton() == MouseButton::Left) {
 *                 if (m_Bounds.Contains(mouseEvent->GetX(), mouseEvent->GetY())) {
 *                     OnClicked();
 *                 }
 *             }
 *         }
 *     }
 * };
 * 
 * Example 3: Modifier-aware hotkeys
 * -----------------------------------
 * void OnEvent(const std::shared_ptr<const WindowEvent>& e) override {
 *     if (e->GetWindowEventType() == WindowEvent::TYPE::KEY_PRESSED) {
 *         auto keyEvent = std::static_pointer_cast<const KeyPressedEvent>(e);
 *         
 *         // Ctrl+S = Save
 *         if (keyEvent->GetKeyCode() == KeyCode::S && 
 *             keyEvent->IsCtrlDown() && 
 *             !keyEvent->IsRepeat()) {
 *             Save();
 *         }
 *         
 *         // Ctrl+Shift+S = Save As
 *         if (keyEvent->GetKeyCode() == KeyCode::S && 
 *             keyEvent->IsCtrlDown() && 
 *             keyEvent->IsShiftDown() && 
 *             !keyEvent->IsRepeat()) {
 *             SaveAs();
 *         }
 *     }
 * }
 * 
 * Example 4: Mouse wheel zoom
 * -----------------------------
 * void OnEvent(const std::shared_ptr<const WindowEvent>& e) override {
 *     if (e->GetWindowEventType() == WindowEvent::TYPE::MOUSE_WHEEL) {
 *         auto wheelEvent = std::static_pointer_cast<const MouseWheelEvent>(e);
 *         
 *         float zoomFactor = wheelEvent->GetDeltaVertical() / 120.0f;
 *         m_Camera->Zoom(zoomFactor);
 *     }
 * }
 * 
 * ============================================================================
 * When to Use Events vs Polling
 * ============================================================================
 * 
 * Use EVENTS for:
 * - Discrete actions (button clicks, menu shortcuts)
 * - Toggle behavior (pause/unpause)
 * - One-shot reactions (play sound on key press)
 * - UI interactions (which widget was clicked?)
 * 
 * Use POLLING (Window::IsKeyDown, GetMouseDelta) for:
 * - Continuous input (WASD movement, camera rotation)
 * - Frame-by-frame state (is player holding jump?)
 * - Physics-driven logic (apply force while key held)
 * - Low-latency reactions (FPS aiming)
 * 
 * Events have ~1 frame delay. Polling is immediate.
 * ============================================================================
 */

/**
 * Base class for all input-related events.
 * 
 * Input events derive from WindowEvent because:
 * - Input is window-scoped (each window has independent input state)
 * - They flow through the same EventBus as window lifecycle events
 * - They follow the same producer/listener pattern
 * 
 * Design principle: Events represent TRANSITIONS, not continuous state.
 * - KeyPressedEvent = key just transitioned to pressed
 * - KeyReleasedEvent = key just transitioned to released
 * - No "KeyHeldEvent" or "MouseMoveEvent" (use polling for continuous state)
 * 
 * All input events include modifier key state at the time of the event.
 */

// ============================================================================
// Keyboard Events
// ============================================================================

/**
 * Fired when a key transitions from released to pressed.
 * 
 * Includes both the initial press and key repeats (if OS/compositor sends them).
 * Use IsRepeat() to distinguish:
 * - IsRepeat() == false: Initial press (user just pushed the key down)
 * - IsRepeat() == true: Key repeat (key is held, OS sent another press event)
 * 
 * For game logic that wants "press once" behavior (toggle menu, jump, etc.),
 * check !IsRepeat(). For text input that needs repeats, use all events.
 * 
 * Event carries both KeyCode (platform-agnostic) and raw scancode (for debugging).
 */

class WindowInputEvent : public WindowEvent
{
public:

    enum class TYPE
    {
        KEY_PRESSED,
        KEY_RELEASED,
        MOUSE_BUTTON_DOWN,
        MOUSE_BUTTON_UP,
        MOUSE_WHEEL
    };

    WindowInputEvent(TYPE t)
        : WindowEvent(WindowEvent::TYPE::INPUT)
        , m_WindowInputEventType(t)
    {
    }

    TYPE GetWindowInputEventType() const { return m_WindowInputEventType; }

protected:
    TYPE m_WindowInputEventType;
};

class KeyPressedEvent : public WindowInputEvent
{
public:
    KeyPressedEvent(
        KeyCode keyCode,
        uint16_t scancode,
        bool isRepeat,
        bool shift,
        bool ctrl,
        bool alt)
        : WindowInputEvent(WindowInputEvent::TYPE::KEY_PRESSED)
        , m_KeyCode(keyCode)
        , m_Scancode(scancode)
        , m_IsRepeat(isRepeat)
        , m_Shift(shift)
        , m_Ctrl(ctrl)
        , m_Alt(alt)
    {
    }

    /**
     * Get platform-agnostic key identifier (USB HID based).
     * Use this for game logic, action mappings, etc.
     */
    KeyCode GetKeyCode() const { return m_KeyCode; }

    /**
     * Get raw platform scancode (for debugging/advanced use).
     * - Win32: Physical scancode from lParam (0-511)
     * - Wayland: XKB keycode
     * 
     * Most application code should use GetKeyCode() instead.
     */
    uint16_t GetScancode() const { return m_Scancode; }

    /**
     * Check if this is a key repeat event (key was already held).
     * 
     * false = Initial press (user just pushed key)
     * true  = Repeat event (key held, OS sent another press)
     * 
     * Game logic typically ignores repeats:
     *   if (e.GetKeyCode() == KeyCode::Space && !e.IsRepeat()) {
     *       Jump();
     *   }
     * 
     * Text input uses repeats for character repetition.
     */
    bool IsRepeat() const { return m_IsRepeat; }

    // Modifier key state at time of event
    bool IsShiftDown() const { return m_Shift; }
    bool IsCtrlDown() const { return m_Ctrl; }
    bool IsAltDown() const { return m_Alt; }

private:
    KeyCode m_KeyCode;
    uint16_t m_Scancode;
    bool m_IsRepeat;
    bool m_Shift;
    bool m_Ctrl;
    bool m_Alt;
};

/**
 * Fired when a key transitions from pressed to released.
 * 
 * Always represents a state change (key was down, now it's up).
 * Never sent for keys that were never pressed.
 * 
 * Used for:
 * - Detecting key release for toggle actions
 * - Implementing "charge-up" mechanics (measure time between press and release)
 * - Ensuring keys don't stay "stuck" after focus loss
 */
class KeyReleasedEvent : public WindowInputEvent
{
public:
    KeyReleasedEvent(
        KeyCode keyCode,
        uint16_t scancode,
        bool shift,
        bool ctrl,
        bool alt)
        : WindowInputEvent(WindowInputEvent::TYPE::KEY_RELEASED)
        , m_KeyCode(keyCode)
        , m_Scancode(scancode)
        , m_Shift(shift)
        , m_Ctrl(ctrl)
        , m_Alt(alt)
    {
    }

    KeyCode GetKeyCode() const { return m_KeyCode; }
    uint16_t GetScancode() const { return m_Scancode; }

    // Modifier key state at time of release
    bool IsShiftDown() const { return m_Shift; }
    bool IsCtrlDown() const { return m_Ctrl; }
    bool IsAltDown() const { return m_Alt; }

private:
    KeyCode m_KeyCode;
    uint16_t m_Scancode;
    bool m_Shift;
    bool m_Ctrl;
    bool m_Alt;
};

// ============================================================================
// Mouse Button Events
// ============================================================================

/**
 * Fired when a mouse button transitions from released to pressed.
 * 
 * Includes mouse position at the time of the press (in client coordinates).
 * Position is useful for:
 * - UI click detection (which button was clicked?)
 * - World-space interaction (raycast from click position)
 * - Drag-start tracking (record position on press, compute drag distance)
 * 
 * Coordinates are in logical client-area pixels (DPI-scaled).
 */
class MouseButtonDownEvent : public WindowInputEvent
{
public:
    MouseButtonDownEvent(
        MouseButton button,
        int32_t x,
        int32_t y,
        bool shift,
        bool ctrl,
        bool alt)
        : WindowInputEvent(WindowInputEvent::TYPE::MOUSE_BUTTON_DOWN)
        , m_Button(button)
        , m_X(x)
        , m_Y(y)
        , m_Shift(shift)
        , m_Ctrl(ctrl)
        , m_Alt(alt)
    {
    }

    /**
     * Which button was pressed.
     */
    MouseButton GetButton() const { return m_Button; }

    /**
     * Mouse position in client-area coordinates (logical pixels).
     * Origin (0,0) = top-left of window client area.
     */
    int32_t GetX() const { return m_X; }
    int32_t GetY() const { return m_Y; }

    // Modifier key state at time of click
    bool IsShiftDown() const { return m_Shift; }
    bool IsCtrlDown() const { return m_Ctrl; }
    bool IsAltDown() const { return m_Alt; }

private:
    MouseButton m_Button;
    int32_t m_X;
    int32_t m_Y;
    bool m_Shift;
    bool m_Ctrl;
    bool m_Alt;
};

/**
 * Fired when a mouse button transitions from pressed to released.
 * 
 * Position represents where the mouse was when button was released.
 * Useful for:
 * - Detecting clicks (press + release in same location)
 * - Drag-end tracking (compare release position to press position)
 * - Context menus (show on right-click release)
 */
class MouseButtonUpEvent : public WindowInputEvent
{
public:
    MouseButtonUpEvent(
        MouseButton button,
        int32_t x,
        int32_t y,
        bool shift,
        bool ctrl,
        bool alt)
        : WindowInputEvent(WindowInputEvent::TYPE::MOUSE_BUTTON_UP)
        , m_Button(button)
        , m_X(x)
        , m_Y(y)
        , m_Shift(shift)
        , m_Ctrl(ctrl)
        , m_Alt(alt)
    {
    }

    MouseButton GetButton() const { return m_Button; }
    
    int32_t GetX() const { return m_X; }
    int32_t GetY() const { return m_Y; }

    bool IsShiftDown() const { return m_Shift; }
    bool IsCtrlDown() const { return m_Ctrl; }
    bool IsAltDown() const { return m_Alt; }

private:
    MouseButton m_Button;
    int32_t m_X;
    int32_t m_Y;
    bool m_Shift;
    bool m_Ctrl;
    bool m_Alt;
};

// ============================================================================
// Mouse Wheel Event
// ============================================================================

/**
 * Fired when mouse wheel is scrolled (vertical or horizontal).
 * 
 * Unlike keyboard/mouse button events, wheel scrolling is inherently
 * delta-based (no "wheel down" state). This event represents discrete
 * scroll actions within a frame.
 * 
 * Note: For continuous scrolling (e.g., during a frame where user scrolls
 * multiple notches), only ONE event is emitted with accumulated delta.
 * This matches the "transition only" design principle.
 */
class MouseWheelEvent : public WindowInputEvent
{
public:
    MouseWheelEvent(
        float deltaVertical,
        float deltaHorizontal,
        int32_t x,
        int32_t y,
        bool shift,
        bool ctrl,
        bool alt)
        : WindowInputEvent(WindowInputEvent::TYPE::MOUSE_WHEEL)
        , m_DeltaVertical(deltaVertical)
        , m_DeltaHorizontal(deltaHorizontal)
        , m_X(x)
        , m_Y(y)
        , m_Shift(shift)
        , m_Ctrl(ctrl)
        , m_Alt(alt)
    {
    }

    /**
     * Vertical scroll delta (positive = up, negative = down).
     * Units: Platform-specific (typically 120 per notch on Win32).
     */
    float GetDeltaVertical() const { return m_DeltaVertical; }

    /**
     * Horizontal scroll delta (positive = right, negative = left).
     * Only non-zero if mouse supports horizontal scrolling.
     */
    float GetDeltaHorizontal() const { return m_DeltaHorizontal; }

    /**
     * Mouse position where scroll occurred (client coordinates).
     */
    int32_t GetX() const { return m_X; }
    int32_t GetY() const { return m_Y; }

    bool IsShiftDown() const { return m_Shift; }
    bool IsCtrlDown() const { return m_Ctrl; }
    bool IsAltDown() const { return m_Alt; }

private:
    float m_DeltaVertical;
    float m_DeltaHorizontal;
    int32_t m_X;
    int32_t m_Y;
    bool m_Shift;
    bool m_Ctrl;
    bool m_Alt;
};