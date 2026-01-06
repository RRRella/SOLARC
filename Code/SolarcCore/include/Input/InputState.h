#pragma once
#include "Input/KeyCode.h"
#include "Input/MouseButton.h"
#include <array>
#include <cstring>  // For memset

/**
 * Per-frame input state snapshot.
 * 
 * This structure holds the complete input state for a single window
 * at a specific point in time (one frame). It is designed for:
 * - Fast polling queries (IsKeyDown, GetMouseDelta)
 * - Transition detection (comparing current vs previous frame)
 * - Frame coherence (all values updated atomically per frame)
 * 
 * Internal representation uses scancodes (0-511) for keyboard state,
 * but public APIs expose KeyCode for platform independence.
 * 
 * Thread Safety: This structure is NOT thread-safe. All access must
 * occur on the main thread, enforced by ThreadChecker in Window.
 */
struct InputState
{
    // ========================================================================
    // Keyboard State
    // ========================================================================
    
    /**
     * Per-key state indexed by scancode (0-511).
     * 
     * Scancodes are used internally because:
     * - They represent physical key positions (layout-independent)
     * - They match platform APIs (Win32 scancode, Wayland XKB code)
     * - They support full keyboard range (USB HID allows 0-511)
     * 
     * true = key is currently held down
     * false = key is released
     * 
     * Note: Public APIs use KeyCode enum, not scancodes directly.
     */
    std::array<bool, 512> keys;
    
    /**
     * Per-key repeat count indexed by scancode (0-511).
     * 
     * Value semantics:
     * - 0     = Key is not pressed
     * - 1     = First press (initial WM_KEYDOWN / key event)
     * - 2+    = Key repeat (subsequent WM_KEYDOWN with repeat bit set)
     * 
     * Reset to 0 on key release.
     * Incremented on each repeat event.
     * 
     * Used for:
     * - Text input (implement key repeat behavior)
     * - Distinguishing initial press from held key
     * - Filtering repeats in game logic
     */
    std::array<uint16_t, 512> keyRepeatCount;

    // ========================================================================
    // Mouse State
    // ========================================================================
    
    /**
     * Mouse cursor position in logical client-area coordinates.
     * 
     * Coordinate space:
     * - Origin (0, 0) = top-left corner of window client area
     * - Positive X = right
     * - Positive Y = down
     * - Units = logical pixels (DPI-scaled)
     * 
     * Excludes window borders, title bar, etc. (client area only).
     * 
     * Note: Values can be negative or exceed window dimensions if
     * mouse moves outside window while captured.
     */
    int32_t mouseX;
    int32_t mouseY;
    
    /**
     * Mouse movement delta accumulated this frame (logical pixels).
     * 
     * Represents cumulative mouse motion since last frame:
     * - Sum of all WM_MOUSEMOVE deltas (Win32)
     * - Sum of all wl_pointer::motion relative events (Wayland)
     * 
     * Reset to (0, 0) at the start of each frame in
     * WindowContext::PollEvents() before OS event processing.
     * 
     * Used for:
     * - Camera rotation (FPS games)
     * - Viewport panning (editors)
     * - Any continuous mouse-driven motion
     * 
     * Note: This is NOT the same as (currentPos - previousPos), which
     * would miss motion when cursor is clamped to screen edges.
     */
    int32_t mouseDeltaX;
    int32_t mouseDeltaY;
    
    /**
     * Mouse button state as a bitmask.
     * 
     * Bit layout:
     * - Bit 0 (0x01): Left button
     * - Bit 1 (0x02): Right button
     * - Bit 2 (0x04): Middle button
     * - Bit 3 (0x08): X1 button (Back)
     * - Bit 4 (0x10): X2 button (Forward)
     * - Bits 5-7: Reserved (always 0)
     * 
     * Use MouseButtonToBit() and IsButtonSet() helpers for safe access.
     * 
     * Example:
     *   bool leftDown = IsButtonSet(mouseButtons, MouseButton::Left);
     */
    uint8_t mouseButtons;
    
    /**
     * Mouse wheel delta accumulated this frame (vertical scroll).
     * 
     * Accumulated from all WM_MOUSEWHEEL / wl_pointer::axis events.
     * 
     * Positive values = scroll up (away from user)
     * Negative values = scroll down (toward user)
     * 
     * Units: Notches (typical scroll wheel: 120 units per notch on Win32)
     * Some mice (pixel-precise scrolling) may report fractional values.
     * 
     * Reset to 0.0f at start of each frame.
     */
    float mouseWheelDelta;
    
    /**
     * Mouse wheel delta accumulated this frame (horizontal scroll).
     * 
     * Similar to mouseWheelDelta, but for horizontal tilt wheels.
     * 
     * Positive values = scroll right
     * Negative values = scroll left
     * 
     * Note: Not all mice support horizontal scrolling.
     * Reset to 0.0f at start of each frame.
     */
    float mouseWheelHDelta;

    // ========================================================================
    // Constructors
    // ========================================================================
    
    /**
     * Default constructor - zero-initialize all state.
     */
    InputState()
    {
        Reset();
    }
    
    /**
     * Reset all input state to default (no input).
     * Called at construction and when clearing state (e.g., focus loss).
     */
    void Reset()
    {
        keys.fill(false);
        keyRepeatCount.fill(0);
        mouseX = 0;
        mouseY = 0;
        mouseDeltaX = 0;
        mouseDeltaY = 0;
        mouseButtons = 0;
        mouseWheelDelta = 0.0f;
        mouseWheelHDelta = 0.0f;
    }
    
    // ========================================================================
    // Helper Methods (Optional - for internal use)
    // ========================================================================
    
    /**
     * Check if a key is down by scancode (internal use).
     * Prefer public Window::IsKeyDown(KeyCode) in application code.
     */
    bool IsKeyScanCodeDown(uint16_t scancode) const
    {
        return scancode < 512 && keys[scancode];
    }
    
    /**
     * Check if a mouse button is down.
     */
    bool IsMouseButtonDown(MouseButton button) const
    {
        return IsButtonSet(mouseButtons, button);
    }
};