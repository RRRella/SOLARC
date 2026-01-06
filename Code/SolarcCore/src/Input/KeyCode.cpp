#include "Input/KeyCode.h"

const char* KeyCodeToString(KeyCode key)
{
    switch (key)
    {
        // Letters
        case KeyCode::A: return "A";
        case KeyCode::B: return "B";
        case KeyCode::C: return "C";
        case KeyCode::D: return "D";
        case KeyCode::E: return "E";
        case KeyCode::F: return "F";
        case KeyCode::G: return "G";
        case KeyCode::H: return "H";
        case KeyCode::I: return "I";
        case KeyCode::J: return "J";
        case KeyCode::K: return "K";
        case KeyCode::L: return "L";
        case KeyCode::M: return "M";
        case KeyCode::N: return "N";
        case KeyCode::O: return "O";
        case KeyCode::P: return "P";
        case KeyCode::Q: return "Q";
        case KeyCode::R: return "R";
        case KeyCode::S: return "S";
        case KeyCode::T: return "T";
        case KeyCode::U: return "U";
        case KeyCode::V: return "V";
        case KeyCode::W: return "W";
        case KeyCode::X: return "X";
        case KeyCode::Y: return "Y";
        case KeyCode::Z: return "Z";
        
        // Numbers
        case KeyCode::Num1: return "1";
        case KeyCode::Num2: return "2";
        case KeyCode::Num3: return "3";
        case KeyCode::Num4: return "4";
        case KeyCode::Num5: return "5";
        case KeyCode::Num6: return "6";
        case KeyCode::Num7: return "7";
        case KeyCode::Num8: return "8";
        case KeyCode::Num9: return "9";
        case KeyCode::Num0: return "0";
        
        // Special keys
        case KeyCode::Enter: return "Enter";
        case KeyCode::Escape: return "Escape";
        case KeyCode::Backspace: return "Backspace";
        case KeyCode::Tab: return "Tab";
        case KeyCode::Space: return "Space";
        
        // Function keys
        case KeyCode::F1: return "F1";
        case KeyCode::F2: return "F2";
        case KeyCode::F3: return "F3";
        case KeyCode::F4: return "F4";
        case KeyCode::F5: return "F5";
        case KeyCode::F6: return "F6";
        case KeyCode::F7: return "F7";
        case KeyCode::F8: return "F8";
        case KeyCode::F9: return "F9";
        case KeyCode::F10: return "F10";
        case KeyCode::F11: return "F11";
        case KeyCode::F12: return "F12";
        
        // Arrows
        case KeyCode::Left: return "Left";
        case KeyCode::Right: return "Right";
        case KeyCode::Up: return "Up";
        case KeyCode::Down: return "Down";
        
        // Modifiers
        case KeyCode::LeftShift: return "LeftShift";
        case KeyCode::RightShift: return "RightShift";
        case KeyCode::LeftCtrl: return "LeftCtrl";
        case KeyCode::RightCtrl: return "RightCtrl";
        case KeyCode::LeftAlt: return "LeftAlt";
        case KeyCode::RightAlt: return "RightAlt";
        case KeyCode::LeftSuper: return "LeftSuper";
        case KeyCode::RightSuper: return "RightSuper";
        
        // Add more as needed...
        
        default: return "Unknown";
    }
}