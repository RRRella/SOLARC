#include "Window/WindowContext.h"
#include <algorithm>

WindowContext::WindowContext(std::unique_ptr<WindowContextPlatform> platform)
    : m_Platform(std::move(platform))
{
    if (!m_Platform)
        throw std::invalid_argument("WindowContextPlatform must not be null");

    m_Bus.RegisterProducer(this);
    m_Platform->SetEventDispatcher([this](std::shared_ptr<const WindowEvent> e) {
        DispatchEvent(e);
        });
}

WindowContext::~WindowContext()
{
    Shutdown();
    m_Bus.UnregisterProducer(this);
}

void WindowContext::OnDestroyWindow(Window* window)
{
    std::lock_guard lock(m_WindowsMutex);
    auto it = std::find_if(m_Windows.begin(), m_Windows.end(),
        [window](const auto& w) { return w.get() == window; });
    if (it != m_Windows.end())
        m_Windows.erase(it);
}

void WindowContext::PollEvents()
{
    m_Platform->PollEvents();

    // Communicate events from bus queue to windows
    m_Bus.Communicate();

    // Update all windows to process their queued events
    std::vector<std::shared_ptr<Window>> windows;
    {
        std::lock_guard lock(m_WindowsMutex);
        windows.assign(m_Windows.begin(), m_Windows.end());
    }

    for (auto& window : windows)
    {
        window->Update();
    }
}

void WindowContext::Shutdown()
{
    std::vector<std::shared_ptr<Window>> windowsToDestroy;
    {
        std::lock_guard lock(m_WindowsMutex);
        windowsToDestroy.assign(m_Windows.begin(), m_Windows.end());
        m_Windows.clear();
    }

    for (auto& w : windowsToDestroy)
        w->Destroy();

    if (m_Platform)
        m_Platform->Shutdown();
}