#pragma once
#include <cstdint>

/**
 * Platform-agnostic keyboard key codes based on USB HID Usage IDs.
 * 
 * This enumeration uses USB HID Usage Table values (Section 10: Keyboard/Keypad)
 * as the underlying numeric representation. This ensures:
 * - Layout independence (keys identified by physical position, not character)
 * - Cross-platform consistency (same values on Win32, Wayland, macOS, etc.)
 * - Industry standard alignment (matches SDL, GLFW, Web APIs)
 * 
 * Internal note: InputState tracks scancodes (0-511) for state arrays,
 * but public APIs (events, queries) use this KeyCode enum.
 * 
 * References:
 * - USB HID Usage Tables v1.4: https://usb.org/sites/default/files/hut1_4.pdf
 * - Section 10: Keyboard/Keypad Page (0x07)
 */
enum class KeyCode : uint16_t
{
    Unknown = 0x00,

    // ========================================================================
    // Letters (USB HID 0x04 - 0x1D)
    // ========================================================================
    A = 0x04, B = 0x05, C = 0x06, D = 0x07,
    E = 0x08, F = 0x09, G = 0x0A, H = 0x0B,
    I = 0x0C, J = 0x0D, K = 0x0E, L = 0x0F,
    M = 0x10, N = 0x11, O = 0x12, P = 0x13,
    Q = 0x14, R = 0x15, S = 0x16, T = 0x17,
    U = 0x18, V = 0x19, W = 0x1A, X = 0x1B,
    Y = 0x1C, Z = 0x1D,

    // ========================================================================
    // Numbers (top row) (USB HID 0x1E - 0x27)
    // ========================================================================
    Num1 = 0x1E, Num2 = 0x1F, Num3 = 0x20, Num4 = 0x21,
    Num5 = 0x22, Num6 = 0x23, Num7 = 0x24, Num8 = 0x25,
    Num9 = 0x26, Num0 = 0x27,

    // ========================================================================
    // Special keys (USB HID 0x28 - 0x38)
    // ========================================================================
    Enter      = 0x28,
    Escape     = 0x29,
    Backspace  = 0x2A,
    Tab        = 0x2B,
    Space      = 0x2C,
    Minus      = 0x2D,  // - and _
    Equals     = 0x2E,  // = and +
    LeftBracket  = 0x2F,  // [ and {
    RightBracket = 0x30,  // ] and }
    Backslash    = 0x31,  // \ and |
    NonUSHash    = 0x32,  // Non-US # and ~
    Semicolon    = 0x33,  // ; and :
    Apostrophe   = 0x34,  // ' and "
    Grave        = 0x35,  // ` and ~
    Comma        = 0x36,  // , and <
    Period       = 0x37,  // . and >
    Slash        = 0x38,  // / and ?

    // ========================================================================
    // Function keys (USB HID 0x39 - 0x45)
    // ========================================================================
    CapsLock   = 0x39,
    F1  = 0x3A, F2  = 0x3B, F3  = 0x3C, F4  = 0x3D,
    F5  = 0x3E, F6  = 0x3F, F7  = 0x40, F8  = 0x41,
    F9  = 0x42, F10 = 0x43, F11 = 0x44, F12 = 0x45,

    // ========================================================================
    // System keys (USB HID 0x46 - 0x4E)
    // ========================================================================
    PrintScreen = 0x46,
    ScrollLock  = 0x47,
    Pause       = 0x48,
    Insert      = 0x49,
    Home        = 0x4A,
    PageUp      = 0x4B,
    Delete      = 0x4C,
    End         = 0x4D,
    PageDown    = 0x4E,

    // ========================================================================
    // Arrow keys (USB HID 0x4F - 0x52)
    // ========================================================================
    Right = 0x4F,
    Left  = 0x50,
    Down  = 0x51,
    Up    = 0x52,

    // ========================================================================
    // Keypad (USB HID 0x53 - 0x63)
    // ========================================================================
    NumLock       = 0x53,
    NumPadDivide  = 0x54,
    NumPadMultiply = 0x55,
    NumPadMinus   = 0x56,
    NumPadPlus    = 0x57,
    NumPadEnter   = 0x58,
    NumPad1 = 0x59, NumPad2 = 0x5A, NumPad3 = 0x5B,
    NumPad4 = 0x5C, NumPad5 = 0x5D, NumPad6 = 0x5E,
    NumPad7 = 0x5F, NumPad8 = 0x60, NumPad9 = 0x61,
    NumPad0 = 0x62,
    NumPadPeriod = 0x63,
    
    // ========================================================================
    // Additional keys (USB HID 0x64 - 0x65)
    // ========================================================================
    NonUSBackslash = 0x64,  // Non-US \ and |
    Application    = 0x65,  // Context menu key

    // ========================================================================
    // Modifier keys (USB HID 0xE0 - 0xE7)
    // Note: USB HID distinguishes left/right modifiers
    // ========================================================================
    LeftCtrl   = 0xE0,
    LeftShift  = 0xE1,
    LeftAlt    = 0xE2,
    LeftSuper  = 0xE3,  // Windows key / Command key
    RightCtrl  = 0xE4,
    RightShift = 0xE5,
    RightAlt   = 0xE6,
    RightSuper = 0xE7,

    // ========================================================================
    // Extended function keys (non-USB standard, but commonly mapped)
    // Using values beyond standard USB HID range for disambiguation
    // ========================================================================
    F13 = 0x68, F14 = 0x69, F15 = 0x6A, F16 = 0x6B,
    F17 = 0x6C, F18 = 0x6D, F19 = 0x6E, F20 = 0x6F,
    F21 = 0x70, F22 = 0x71, F23 = 0x72, F24 = 0x73,

    // ========================================================================
    // Media keys (non-USB HID, extended mappings)
    // These are often mapped inconsistently across platforms
    // Using high values to avoid conflicts
    // ========================================================================
    Mute       = 0x7F,
    VolumeUp   = 0x80,
    VolumeDown = 0x81,
    MediaPlay  = 0x82,
    MediaStop  = 0x83,
    MediaNext  = 0x84,
    MediaPrev  = 0x85,

    // ========================================================================
    // Sentinel value (max scancode range)
    // ========================================================================
    MaxValue = 0x1FF  // 511 (scancode range is 0-511)
};

/**
 * Check if a KeyCode represents a modifier key
 */
inline constexpr bool IsModifierKey(KeyCode key)
{
    uint16_t code = static_cast<uint16_t>(key);
    return (code >= 0xE0 && code <= 0xE7);
}

/**
 * Check if a KeyCode represents a function key (F1-F24)
 */
inline constexpr bool IsFunctionKey(KeyCode key)
{
    uint16_t code = static_cast<uint16_t>(key);
    return (code >= 0x3A && code <= 0x45) ||  // F1-F12
           (code >= 0x68 && code <= 0x73);    // F13-F24
}

/**
 * Check if a KeyCode represents a numpad key
 */
inline constexpr bool IsNumPadKey(KeyCode key)
{
    uint16_t code = static_cast<uint16_t>(key);
    return (code >= 0x53 && code <= 0x63);
}

/**
 * Get string representation of KeyCode (for debugging)
 * Returns "Unknown" for unrecognized codes
 */
const char* KeyCodeToString(KeyCode key);