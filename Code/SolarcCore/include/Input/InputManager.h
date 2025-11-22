#pragma once
#include "Event/EventProducer.h"
#include "Event/InputEvent.h"
#include <unordered_map>
#include <array>

/**
 * Manages input state and generates input events
 *
 * Responsibilities:
 * - Track keyboard/mouse state
 * - Generate input events
 * - Provide query interface for input state
 *
 * Usage:
 * - Call PollInput() once per frame
 * - Register listeners to receive input events
 * - Query IsKeyPressed(), GetMousePosition(), etc. for immediate state
 */
class InputManager : public EventProducer<InputEvent>
{
public:
    InputManager();

    /**
     * Poll input and generate events
     * note: Should be called once per frame from main thread
     */
    void PollInput();

    /**
     * Check if a key is currently pressed
     * param key: Key code to check
     * return true if pressed, false otherwise
     */
    bool IsKeyPressed(KeyCode key) const;

    /**
     * Check if a mouse button is currently pressed
     * param button: Mouse button to check
     * return true if pressed, false otherwise
     */
    bool IsMouseButtonPressed(MouseButton button) const;

    /**
     * Get current mouse position
     * param x: Output: X position
     * param y: Output: Y position
     */
    void GetMousePosition(float& x, float& y) const;

    /**
     * Get mouse delta since last frame
     * param deltaX: Output: X delta
     * param deltaY: Output: Y delta
     */
    void GetMouseDelta(float& deltaX, float& deltaY) const;

    // Platform-specific methods (called by platform layer)
    void OnKeyPressed(KeyCode key, bool isRepeat = false);
    void OnKeyReleased(KeyCode key);
    void OnMouseMoved(float x, float y);
    void OnMouseButtonPressed(MouseButton button);
    void OnMouseButtonReleased(MouseButton button);
    void OnMouseWheelScrolled(float deltaX, float deltaY);

private:
    std::unordered_map<KeyCode, bool> m_KeyStates;
    std::array<bool, 5> m_MouseButtonStates;
    float m_MouseX = 0.0f;
    float m_MouseY = 0.0f;
    float m_LastMouseX = 0.0f;
    float m_LastMouseY = 0.0f;
    float m_MouseDeltaX = 0.0f;
    float m_MouseDeltaY = 0.0f;
};