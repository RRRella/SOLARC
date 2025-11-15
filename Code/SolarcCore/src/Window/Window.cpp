#include "Window/Window.h"
#include <stdexcept>

Window::Window(
    std::shared_ptr<WindowPlatform> platform,
    Window* parent,
    std::function<void(Window*)> onDestroy)
    : m_Platform(std::move(platform))
    , m_Parent(parent)
    , m_OnDestroy(std::move(onDestroy))
{
    if (!m_Platform)
        throw std::invalid_argument("Window platform must not be null");
}

Window::~Window()
{
    Destroy();
}

void Window::Destroy()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Destroyed) return;
    m_Destroyed = true;

    if (m_OnDestroy)
        m_OnDestroy(this);
}

void Window::Show()
{
    if (m_Platform) m_Platform->Show();
}

void Window::Hide()
{
    if (m_Platform) m_Platform->Hide();
}

bool Window::IsVisible() const
{
    return m_Platform ? m_Platform->IsVisible() : false;
}

void Window::Update()
{
    // Process all events in the queue
    std::optional<std::shared_ptr<const WindowEvent>> event;
    while (event = m_EventQueue.TryNext())
    {
        OnEvent(event.value());
    }
}

void Window::OnEvent(const std::shared_ptr<const WindowEvent>& e)
{
    if (e->GetWindowEventType() == WindowEvent::TYPE::CLOSE)
    {
        Destroy();
    }
}