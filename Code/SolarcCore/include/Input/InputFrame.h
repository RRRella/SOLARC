#pragma once
#include <vector>
#include <cstdint>
#include "Input/KeyCode.h"
#include "Input/MouseButton.h"

struct InputFrame
{
   /**
   * Represents a keyboard key state transition captured this frame.
   *
   * These are accumulated during OS event processing (WndProc/Wayland callbacks)
   * and consumed by Window::UpdateInput() once per frame.
   */
   struct KeyTransition
   {
       uint16_t scancode;  // Platform scancode (0-511)
       bool pressed;       // true = key pressed, false = key released
       bool isRepeat;      // true = this is a key repeat event
 
       KeyTransition(uint16_t sc, bool p, bool repeat = false)
           : scancode(sc), pressed(p), isRepeat(repeat)
       {
       }
   };

   /**
   * Represents a mouse button state transition captured this frame.
   */
   struct MouseButtonTransition
   {
       MouseButton button;  // Which button changed state
       bool pressed;        // true = button pressed, false = button released
 
       MouseButtonTransition(MouseButton btn, bool p)
           : button(btn), pressed(p)
       {
       }
   };

   /**
   * Per-frame input accumulator.
   *
   * This structure holds all raw input captured during the current frame's
   * event processing phase (between PollEvents start and Window::Update).
   *
   * Lifecycle:
   * 1. Reset to zero at start of WindowContext::PollEvents()
   * 2. Accumulated during OS event callbacks (WndProc / Wayland listeners)
   * 3. Consumed once by Window::UpdateInput()
   * 4. Repeat next frame
   *
   * Thread safety: Main thread only (enforced by WindowContext::ThreadChecker)
   */

    // Mouse motion (accumulated deltas)
    int32_t mouseDeltaX = 0;
    int32_t mouseDeltaY = 0;

    // Mouse position (last reported absolute position in client coords)
    int32_t mouseX = 0;
    int32_t mouseY = 0;
    // Mouse wheel (accumulated scroll deltas)
    float wheelDelta = 0.0f;
    float hWheelDelta = 0.0f;
    // Keyboard transitions (presses/releases that occurred this frame)
    std::vector<KeyTransition> keyTransitions;
    // Mouse button transitions
    std::vector<MouseButtonTransition> mouseButtonTransitions;
    /**
     * Reset all fields to zero/empty.
     * Called at the start of each frame by WindowContext::PollEvents().
     */
    void Reset()
    {
        mouseDeltaX = 0;
        mouseDeltaY = 0;
        // Note: mouseX/mouseY are NOT reset (they persist frame-to-frame)
        wheelDelta = 0.0f;
        hWheelDelta = 0.0f;
        keyTransitions.clear();
        mouseButtonTransitions.clear();
    }
};