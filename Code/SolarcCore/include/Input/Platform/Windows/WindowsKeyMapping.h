#pragma once

#ifdef _WIN32

#include "Input/KeyCode.h"
#include <cstdint>
#include <windows.h>

/**
 * Win32-specific key mapping utilities.
 * 
 * Handles Win32 virtual key codes (VK_*) and scancode extraction.
 */
namespace Win32KeyMapping
{
    /**
     * Extract scancode from Win32 lParam.
     * 
     * Win32 WM_KEYDOWN/WM_KEYUP lParam format:
     * - Bits 0-15:  Repeat count
     * - Bits 16-23: Scancode
     * - Bit 24:     Extended key flag (E0 prefix)
     * - Bits 25-28: Reserved
     * - Bit 29:     Context code (Alt state)
     * - Bit 30:     Previous key state
     * - Bit 31:     Transition state
     * 
     * param lParam: lParam from WM_KEYDOWN/WM_KEYUP
     * return Scancode (0-511), with bit 8 set for extended keys
     * 
     * Example:
     *   case WM_KEYDOWN:
     *       uint16_t scancode = ExtractScancode(lParam);
     *       KeyCode key = KeyMapping::ScancodeToKeyCode(scancode);
     *       break;
     */
    inline uint16_t ExtractScancode(LPARAM lParam)
    {
        // Extract base scancode (bits 16-23)
        uint16_t scancode = static_cast<uint16_t>((lParam >> 16) & 0xFF);
        
        // Check extended key flag (bit 24)
        bool extended = (lParam & (1 << 24)) != 0;
        
        if (extended)
        {
            // Set bit 8 to mark as extended (matches our mapping table)
            scancode |= 0x100;
        }
        
        return scancode;
    }

    /**
     * Check if a key message is a repeat (key was already down).
     * 
     * param lParam: lParam from WM_KEYDOWN
     * return true if this is a repeat, false if initial press
     */
    inline bool IsKeyRepeat(LPARAM lParam)
    {
        // Bit 30: Previous key state (1 = was down)
        return (lParam & (1 << 30)) != 0;
    }

    /**
     * Get repeat count from lParam.
     * 
     * param lParam: lParam from WM_KEYDOWN
     * return Number of times key was pressed (usually 1)
     */
    inline uint16_t GetRepeatCount(LPARAM lParam)
    {
        return static_cast<uint16_t>(lParam & 0xFFFF);
    }

} // namespace Win32KeyMapping

#endif // _WIN32