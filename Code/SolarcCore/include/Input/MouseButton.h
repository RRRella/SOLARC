#pragma once
#include <cstdint>

/**
 * Mouse button enumeration.
 * 
 * Supports the 5 standard mouse buttons found on modern mice:
 * - Left/Right/Middle (standard 3-button mouse)
 * - X1/X2 (side buttons, typically "Back" and "Forward")
 * 
 * Values chosen to match Win32 button indices and common conventions.
 * Stored as bitmask in InputState (bit 0 = Left, bit 1 = Right, etc.)
 * 
 * Note: Mice with 6+ buttons typically expose extra buttons as keyboard
 * events (e.g., XButton3 → Keyboard F13), not as mouse buttons.
 */
enum class MouseButton : uint8_t
{
    Left   = 0,  // Primary button (typically left for right-handed users)
    Right  = 1,  // Secondary button (context menu)
    Middle = 2,  // Middle button / scroll wheel click
    X1     = 3,  // Extra button 1 (typically "Back" in browsers)
    X2     = 4,  // Extra button 2 (typically "Forward" in browsers)

    Count  = 5   // Total number of supported buttons
};

/**
 * Convert MouseButton to bitmask value
 * Example: MouseButtonToBit(MouseButton::Left) = 0x01
 */
inline constexpr uint8_t MouseButtonToBit(MouseButton button)
{
    return static_cast<uint8_t>(1 << static_cast<uint8_t>(button));
}

/**
 * Check if a button is set in a bitmask
 * Example: IsButtonSet(mask, MouseButton::Left)
 */
inline constexpr bool IsButtonSet(uint8_t mask, MouseButton button)
{
    return (mask & MouseButtonToBit(button)) != 0;
}

/**
 * Set a button in a bitmask (returns new mask)
 */
inline constexpr uint8_t SetButton(uint8_t mask, MouseButton button)
{
    return mask | MouseButtonToBit(button);
}

/**
 * Clear a button in a bitmask (returns new mask)
 */
inline constexpr uint8_t ClearButton(uint8_t mask, MouseButton button)
{
    return mask & ~MouseButtonToBit(button);
}

/**
 * Get string representation of MouseButton (for debugging)
 */
inline const char* MouseButtonToString(MouseButton button)
{
    switch (button)
    {
        case MouseButton::Left:   return "Left";
        case MouseButton::Right:  return "Right";
        case MouseButton::Middle: return "Middle";
        case MouseButton::X1:     return "X1";
        case MouseButton::X2:     return "X2";
        default:                  return "Unknown";
    }
}