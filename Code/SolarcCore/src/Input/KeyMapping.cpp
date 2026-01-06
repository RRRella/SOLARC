#include "Input/KeyMapping.h"
#include <array>
#include <algorithm>

namespace KeyMapping
{
    // ========================================================================
    // Scancode -> KeyCode Mapping Table
    // ========================================================================
    // 
    // This table maps PC keyboard scancodes (Set 1) to USB HID KeyCodes.
    // 
    // Scancodes 0-127: Standard keys
    // Scancodes 128-255: Extended keys (E0 prefix on Win32)
    // Scancodes 256-511: Reserved for platform-specific extensions
    // 
    // References:
    // - USB HID Usage Tables: https://usb.org/sites/default/files/hut1_4.pdf
    // - Win32 Scan Codes: https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
    // - Linux Scancodes: /usr/include/linux/input-event-codes.h
    // ========================================================================

    static constexpr std::array<KeyCode, 512> g_ScancodeToKeyCode = []()
    {
        std::array<KeyCode, 512> table{};
        
        // Initialize all to Unknown
        for (auto& entry : table)
        {
            entry = KeyCode::Unknown;
        }

        // ====================================================================
        // Standard keys (scancodes 0-127)
        // ====================================================================
        
        // Row 1: Escape and function keys
        table[0x01] = KeyCode::Escape;
        table[0x3B] = KeyCode::F1;
        table[0x3C] = KeyCode::F2;
        table[0x3D] = KeyCode::F3;
        table[0x3E] = KeyCode::F4;
        table[0x3F] = KeyCode::F5;
        table[0x40] = KeyCode::F6;
        table[0x41] = KeyCode::F7;
        table[0x42] = KeyCode::F8;
        table[0x43] = KeyCode::F9;
        table[0x44] = KeyCode::F10;
        table[0x57] = KeyCode::F11;
        table[0x58] = KeyCode::F12;
        
        // Row 2: Number row
        table[0x29] = KeyCode::Grave;      // ` ~
        table[0x02] = KeyCode::Num1;
        table[0x03] = KeyCode::Num2;
        table[0x04] = KeyCode::Num3;
        table[0x05] = KeyCode::Num4;
        table[0x06] = KeyCode::Num5;
        table[0x07] = KeyCode::Num6;
        table[0x08] = KeyCode::Num7;
        table[0x09] = KeyCode::Num8;
        table[0x0A] = KeyCode::Num9;
        table[0x0B] = KeyCode::Num0;
        table[0x0C] = KeyCode::Minus;      // - _
        table[0x0D] = KeyCode::Equals;     // = +
        table[0x0E] = KeyCode::Backspace;
        
        // Row 3: QWERTY row
        table[0x0F] = KeyCode::Tab;
        table[0x10] = KeyCode::Q;
        table[0x11] = KeyCode::W;
        table[0x12] = KeyCode::E;
        table[0x13] = KeyCode::R;
        table[0x14] = KeyCode::T;
        table[0x15] = KeyCode::Y;
        table[0x16] = KeyCode::U;
        table[0x17] = KeyCode::I;
        table[0x18] = KeyCode::O;
        table[0x19] = KeyCode::P;
        table[0x1A] = KeyCode::LeftBracket;  // [ {
        table[0x1B] = KeyCode::RightBracket; // ] }
        table[0x2B] = KeyCode::Backslash;    // \ |
        
        // Row 4: ASDF row
        table[0x3A] = KeyCode::CapsLock;
        table[0x1E] = KeyCode::A;
        table[0x1F] = KeyCode::S;
        table[0x20] = KeyCode::D;
        table[0x21] = KeyCode::F;
        table[0x22] = KeyCode::G;
        table[0x23] = KeyCode::H;
        table[0x24] = KeyCode::J;
        table[0x25] = KeyCode::K;
        table[0x26] = KeyCode::L;
        table[0x27] = KeyCode::Semicolon;    // ; :
        table[0x28] = KeyCode::Apostrophe;   // ' "
        table[0x1C] = KeyCode::Enter;
        
        // Row 5: ZXCV row
        table[0x2A] = KeyCode::LeftShift;
        table[0x2C] = KeyCode::Z;
        table[0x2D] = KeyCode::X;
        table[0x2E] = KeyCode::C;
        table[0x2F] = KeyCode::V;
        table[0x30] = KeyCode::B;
        table[0x31] = KeyCode::N;
        table[0x32] = KeyCode::M;
        table[0x33] = KeyCode::Comma;        // , <
        table[0x34] = KeyCode::Period;       // . >
        table[0x35] = KeyCode::Slash;        // / ?
        table[0x36] = KeyCode::RightShift;
        
        // Row 6: Bottom row
        table[0x1D] = KeyCode::LeftCtrl;
        table[0x38] = KeyCode::LeftAlt;
        table[0x39] = KeyCode::Space;
        
        // Navigation cluster
        table[0x46] = KeyCode::ScrollLock;
        table[0x45] = KeyCode::NumLock;     // Also Pause on some keyboards
        
        // ====================================================================
        // Extended keys (scancodes 128+, E0-prefixed on Win32)
        // ====================================================================
        // Win32: These have bit 8 set (0x100 | base_scancode)
        // Linux: These are in the 0x100-0x1FF range
        
        // Right modifiers (extended)
        table[0x11D] = KeyCode::RightCtrl;   // E0 1D
        table[0x138] = KeyCode::RightAlt;    // E0 38
        table[0x15B] = KeyCode::LeftSuper;   // E0 5B (Left Windows key)
        table[0x15C] = KeyCode::RightSuper;  // E0 5C (Right Windows key)
        table[0x15D] = KeyCode::Application; // E0 5D (Menu key)
        
        // Arrow keys (extended)
        table[0x148] = KeyCode::Up;          // E0 48
        table[0x14B] = KeyCode::Left;        // E0 4B
        table[0x150] = KeyCode::Down;        // E0 50
        table[0x14D] = KeyCode::Right;       // E0 4D
        
        // Navigation cluster (extended)
        table[0x152] = KeyCode::Insert;      // E0 52
        table[0x153] = KeyCode::Delete;      // E0 53
        table[0x147] = KeyCode::Home;        // E0 47
        table[0x14F] = KeyCode::End;         // E0 4F
        table[0x149] = KeyCode::PageUp;      // E0 49
        table[0x151] = KeyCode::PageDown;    // E0 51
        
        // Numpad (some keys overlap with non-extended versions)
        table[0x47] = KeyCode::NumPad7;      // Home on numpad
        table[0x48] = KeyCode::NumPad8;      // Up on numpad
        table[0x49] = KeyCode::NumPad9;      // PgUp on numpad
        table[0x4A] = KeyCode::NumPadMinus;
        table[0x4B] = KeyCode::NumPad4;      // Left on numpad
        table[0x4C] = KeyCode::NumPad5;
        table[0x4D] = KeyCode::NumPad6;      // Right on numpad
        table[0x4E] = KeyCode::NumPadPlus;
        table[0x4F] = KeyCode::NumPad1;      // End on numpad
        table[0x50] = KeyCode::NumPad2;      // Down on numpad
        table[0x51] = KeyCode::NumPad3;      // PgDn on numpad
        table[0x52] = KeyCode::NumPad0;      // Insert on numpad
        table[0x53] = KeyCode::NumPadPeriod; // Delete on numpad
        
        // Extended numpad keys
        table[0x135] = KeyCode::NumPadDivide;   // E0 35
        table[0x137] = KeyCode::PrintScreen;    // E0 37 (also * on numpad)
        table[0x11C] = KeyCode::NumPadEnter;    // E0 1C
        
        // Special keys
        table[0x137] = KeyCode::NumPadMultiply; // Note: conflicts with PrtScn
        
        // Media keys (platform-specific, may vary)
        // These are approximate mappings
        table[0x120] = KeyCode::Mute;           // E0 20
        table[0x12E] = KeyCode::VolumeDown;     // E0 2E
        table[0x130] = KeyCode::VolumeUp;       // E0 30
        table[0x122] = KeyCode::MediaPlay;      // E0 22
        table[0x124] = KeyCode::MediaStop;      // E0 24
        table[0x119] = KeyCode::MediaNext;      // E0 19
        table[0x110] = KeyCode::MediaPrev;      // E0 10

        return table;
    }();

    // ========================================================================
    // Public API Implementation
    // ========================================================================

    KeyCode ScancodeToKeyCode(uint16_t scancode)
    {
        if (scancode >= 512)
        {
            return KeyCode::Unknown;
        }
        return g_ScancodeToKeyCode[scancode];
    }

    uint16_t KeyCodeToScancode(KeyCode key)
    {
        // Reverse lookup: find first scancode that maps to this KeyCode
        uint16_t keyValue = static_cast<uint16_t>(key);
        
        for (uint16_t scancode = 0; scancode < 512; ++scancode)
        {
            if (g_ScancodeToKeyCode[scancode] == key)
            {
                return scancode;
            }
        }
        
        return 0; // Not found
    }

    bool HasScancodeMapping(KeyCode key)
    {
        return KeyCodeToScancode(key) != 0;
    }

} // namespace KeyMapping