#pragma once
#include "Event/Event.h"
#include <cstdint>

/**
 * @brief Base class for all input events
 */
class InputEvent : public Event
{
public:
    enum class TYPE
    {
        KEY_PRESSED,
        KEY_RELEASED,
        MOUSE_MOVED,
        MOUSE_BUTTON_PRESSED,
        MOUSE_BUTTON_RELEASED,
        MOUSE_WHEEL_SCROLLED
    };

    InputEvent(TYPE type)
        : Event(Event::TYPE::STUB_EVENT) // Or create INPUT_EVENT type
        , m_InputEventType(type)
    {
    }

    TYPE GetInputEventType() const { return m_InputEventType; }

private:
    TYPE m_InputEventType;
};

// ============================================================================
// Keyboard Events
// ============================================================================

enum class KeyCode : uint16_t
{
    // Letters
    A = 0x41, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Numbers
    Num0 = 0x30, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // Function keys
    F1 = 0x70, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // Special keys
    Space = 0x20,
    Enter = 0x0D,
    Escape = 0x1B,
    Tab = 0x09,
    Backspace = 0x08,
    Delete = 0x2E,

    // Arrow keys
    Left = 0x25,
    Up = 0x26,
    Right = 0x27,
    Down = 0x28,

    // Modifiers
    Shift = 0x10,
    Control = 0x11,
    Alt = 0x12,

    Unknown = 0x00
};

class KeyPressedEvent : public InputEvent
{
public:
    KeyPressedEvent(KeyCode key, bool isRepeat = false)
        : InputEvent(TYPE::KEY_PRESSED)
        , m_KeyCode(key)
        , m_IsRepeat(isRepeat)
    {
    }

    KeyCode GetKey() const { return m_KeyCode; }
    bool IsRepeat() const { return m_IsRepeat; }

private:
    KeyCode m_KeyCode;
    bool m_IsRepeat;
};

class KeyReleasedEvent : public InputEvent
{
public:
    KeyReleasedEvent(KeyCode key)
        : InputEvent(TYPE::KEY_RELEASED)
        , m_KeyCode(key)
    {
    }

    KeyCode GetKey() const { return m_KeyCode; }

private:
    KeyCode m_KeyCode;
};

// ============================================================================
// Mouse Events
// ============================================================================

enum class MouseButton : uint8_t
{
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4
};

class MouseMovedEvent : public InputEvent
{
public:
    MouseMovedEvent(float x, float y, float deltaX, float deltaY)
        : InputEvent(TYPE::MOUSE_MOVED)
        , m_X(x), m_Y(y)
        , m_DeltaX(deltaX), m_DeltaY(deltaY)
    {
    }

    float GetX() const { return m_X; }
    float GetY() const { return m_Y; }
    float GetDeltaX() const { return m_DeltaX; }
    float GetDeltaY() const { return m_DeltaY; }

private:
    float m_X, m_Y;
    float m_DeltaX, m_DeltaY;
};

class MouseButtonPressedEvent : public InputEvent
{
public:
    MouseButtonPressedEvent(MouseButton button, float x, float y)
        : InputEvent(TYPE::MOUSE_BUTTON_PRESSED)
        , m_Button(button)
        , m_X(x), m_Y(y)
    {
    }

    MouseButton GetButton() const { return m_Button; }
    float GetX() const { return m_X; }
    float GetY() const { return m_Y; }

private:
    MouseButton m_Button;
    float m_X, m_Y;
};

class MouseButtonReleasedEvent : public InputEvent
{
public:
    MouseButtonReleasedEvent(MouseButton button, float x, float y)
        : InputEvent(TYPE::MOUSE_BUTTON_RELEASED)
        , m_Button(button)
        , m_X(x), m_Y(y)
    {
    }

    MouseButton GetButton() const { return m_Button; }
    float GetX() const { return m_X; }
    float GetY() const { return m_Y; }

private:
    MouseButton m_Button;
    float m_X, m_Y;
};

class MouseWheelScrolledEvent : public InputEvent
{
public:
    MouseWheelScrolledEvent(float deltaX, float deltaY)
        : InputEvent(TYPE::MOUSE_WHEEL_SCROLLED)
        , m_DeltaX(deltaX), m_DeltaY(deltaY)
    {
    }

    float GetDeltaX() const { return m_DeltaX; }
    float GetDeltaY() const { return m_DeltaY; }

private:
    float m_DeltaX, m_DeltaY;
};