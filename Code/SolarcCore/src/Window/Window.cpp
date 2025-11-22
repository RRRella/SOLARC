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
    {
        SOLARC_ERROR("Window: Platform cannot be null");
        throw std::invalid_argument("Window platform must not be null");
    }

    SOLARC_WINDOW_TRACE("Window created: '{}'", m_Platform->GetTitle());
}

Window::~Window()
{
    SOLARC_WINDOW_TRACE("Window destructor: '{}'",
        m_Platform ? m_Platform->GetTitle() : "null");
    Destroy();
}

void Window::Destroy()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Destroyed) return;

    m_Destroyed = true;

    SOLARC_WINDOW_INFO("Destroying window: '{}'",
        m_Platform ? m_Platform->GetTitle() : "null");

    if (m_OnDestroy)
    {
        m_OnDestroy(this);
    }
}

void Window::Show()
{
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Show();
        SOLARC_WINDOW_DEBUG("Window shown: '{}'", m_Platform->GetTitle());
    }
}

void Window::Hide()
{
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Hide();
        SOLARC_WINDOW_DEBUG("Window hidden: '{}'", m_Platform->GetTitle());
    }
}

bool Window::IsVisible() const
{
    return m_Platform && !m_Destroyed ? m_Platform->IsVisible() : false;
}

bool Window::IsClosed() const
{
    std::lock_guard lock(m_DestroyMutex);
    return m_Destroyed;
}

void Window::Update()
{
    // Process all queued events
    ProcessEvents();
}

void Window::OnEvent(const std::shared_ptr<const WindowEvent>& e)
{
    switch (e->GetWindowEventType())
    {
    case WindowEvent::TYPE::CLOSE:
        SOLARC_WINDOW_INFO("Window close event received: '{}'", GetTitle());
        Destroy();
        break;

    case WindowEvent::TYPE::SHOWN:
        SOLARC_WINDOW_TRACE("Window shown event: '{}'", GetTitle());
        break;

    case WindowEvent::TYPE::HIDDEN:
        SOLARC_WINDOW_TRACE("Window hidden event: '{}'", GetTitle());
        break;

    default:
        SOLARC_WINDOW_TRACE("Window generic event: '{}'", GetTitle());
        break;
    }
}