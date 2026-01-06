#pragma once

#ifdef __linux__

#include "Input/KeyCode.h"
#include <cstdint>

/**
 * Wayland/XKB key mapping utilities.
 * 
 * Wayland uses XKB (X Keyboard Extension) keycodes, which are
 * typically scancode + 8 (legacy X11 offset).
 */
namespace WaylandKeyMapping
{
    /**
     * Convert XKB keycode to our internal scancode.
     * 
     * XKB keycodes = Linux scancode + 8 (X11 legacy offset)
     * We store Linux scancodes internally (0-511 range).
     * 
     * param xkbKeycode: Keycode from wl_keyboard::key event
     * return Internal scancode (0-511)
     * 
     * Example:
     *   void keyboard_key(void* data, wl_keyboard* kbd, uint32_t serial,
     *                     uint32_t time, uint32_t key, uint32_t state) {
     *       uint16_t scancode = XKBKeyToScancode(key);
     *       KeyCode keyCode = KeyMapping::ScancodeToKeyCode(scancode);
     *   }
     */
    inline uint16_t XKBKeyToScancode(uint32_t xkbKeycode)
    {
        // XKB codes are offset by 8 from Linux scancodes
        if (xkbKeycode < 8)
        {
            return 0; // Invalid
        }
        
        uint16_t scancode = static_cast<uint16_t>(xkbKeycode - 8);
        
        // Clamp to valid range
        if (scancode >= 512)
        {
            return 0;
        }
        
        return scancode;
    }

    /**
     * Convert internal scancode to XKB keycode.
     * 
     * param scancode: Internal scancode (0-511)
     * return XKB keycode
     */
    inline uint32_t ScancodeToXKBKey(uint16_t scancode)
    {
        if (scancode >= 512)
        {
            return 0;
        }
        return static_cast<uint32_t>(scancode + 8);
    }

} // namespace WaylandKeyMapping

#endif // __linux__