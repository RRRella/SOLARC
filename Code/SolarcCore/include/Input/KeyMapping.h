#pragma once
#include "Input/KeyCode.h"
#include <cstdint>
#include <array>

/**
 * Platform-agnostic key mapping utilities.
 * 
 * Provides bidirectional mapping between:
 * - Scancodes (0-511) <-> KeyCode (USB HID based)
 * - Platform virtual keys (VK_*, XKB) <-> KeyCode
 * 
 * Scancodes represent physical key positions (layout-independent).
 * KeyCodes represent logical key identities (USB HID standard).
 * 
 * Thread safety: All functions are read-only and thread-safe.
 */
namespace KeyMapping
{
    /**
     * Map scancode to KeyCode.
     * 
     * param scancode: Platform scancode (0-511)
     * return KeyCode, or KeyCode::Unknown if unmapped
     * 
     * Note: This uses a hardcoded table built at compile-time.
     * The mapping is based on standard PC keyboard scancodes (Set 1).
     */
    KeyCode ScancodeToKeyCode(uint16_t scancode);

    /**
     * Map KeyCode to scancode.
     * 
     * param key: KeyCode to map
     * return Scancode (0-511), or 0 if unmapped
     * 
     * Note: This performs a reverse lookup. For performance-critical
     * code that needs repeated lookups, consider caching the result.
     */
    uint16_t KeyCodeToScancode(KeyCode key);

    /**
     * Check if a KeyCode has a valid scancode mapping.
     */
    bool HasScancodeMapping(KeyCode key);

} // namespace KeyMapping