#include "Input/InputManager.h"
#include "Logging/LogMacros.h"

InputManager::InputManager()
{
    m_MouseButtonStates.fill(false);
    SOLARC_INFO("InputManager initialized");
}

void InputManager::PollInput()
{
    // Calculate mouse delta
    m_MouseDeltaX = m_MouseX - m_LastMouseX;
    m_MouseDeltaY = m_MouseY - m_LastMouseY;
    m_LastMouseX = m_MouseX;
    m_LastMouseY = m_MouseY;

    // Platform-specific input polling would happen here
    // For now, this is just a placeholder
}

bool InputManager::IsKeyPressed(KeyCode key) const
{
    auto it = m_KeyStates.find(key);
    return it != m_KeyStates.end() && it->second;
}

bool InputManager::IsMouseButtonPressed(MouseButton button) const
{
    size_t index = static_cast<size_t>(button);
    return index < m_MouseButtonStates.size() && m_MouseButtonStates[index];
}

void InputManager::GetMousePosition(float& x, float& y) const
{
    x = m_MouseX;
    y = m_MouseY;
}

void InputManager::GetMouseDelta(float& deltaX, float& deltaY) const
{
    deltaX = m_MouseDeltaX;
    deltaY = m_MouseDeltaY;
}

void InputManager::OnKeyPressed(KeyCode key, bool isRepeat)
{
    m_KeyStates[key] = true;

    auto event = std::make_shared<KeyPressedEvent>(key, isRepeat);
    DispatchEvent(event);
}

void InputManager::OnKeyReleased(KeyCode key)
{
    m_KeyStates[key] = false;

    auto event = std::make_shared<KeyReleasedEvent>(key);
    DispatchEvent(event);
}

void InputManager::OnMouseMoved(float x, float y)
{
    float deltaX = x - m_MouseX;
    float deltaY = y - m_MouseY;

    m_MouseX = x;
    m_MouseY = y;

    auto event = std::make_shared<MouseMovedEvent>(x, y, deltaX, deltaY);
    DispatchEvent(event);
}

void InputManager::OnMouseButtonPressed(MouseButton button)
{
    size_t index = static_cast<size_t>(button);
    if (index < m_MouseButtonStates.size())
    {
        m_MouseButtonStates[index] = true;
    }

    auto event = std::make_shared<MouseButtonPressedEvent>(button, m_MouseX, m_MouseY);
    DispatchEvent(event);
}

void InputManager::OnMouseButtonReleased(MouseButton button)
{
    size_t index = static_cast<size_t>(button);
    if (index < m_MouseButtonStates.size())
    {
        m_MouseButtonStates[index] = false;
    }

    auto event = std::make_shared<MouseButtonReleasedEvent>(button, m_MouseX, m_MouseY);
    DispatchEvent(event);
}

void InputManager::OnMouseWheelScrolled(float deltaX, float deltaY)
{
    auto event = std::make_shared<MouseWheelScrolledEvent>(deltaX, deltaY);
    DispatchEvent(event);
}