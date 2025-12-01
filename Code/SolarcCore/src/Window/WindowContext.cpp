#include "Window/WindowContext.h"
#include <algorithm>

WindowContext::WindowContext(std::unique_ptr<WindowContextPlatformFactory> factory)
{
    if (!factory)
    {
        SOLARC_WINDOW_ERROR("WindowContextPlatformFactory cannot be null");
        throw std::invalid_argument("WindowContextPlatformFactory must not be null");
    }

    // ✅ Atomic creation — safe for Wayland
    auto components = factory->Create();
    m_Platform = std::move(components.context);
    m_WindowFactory = std::move(components.windowFactory);

    if (!m_Platform)
    {
        SOLARC_WINDOW_ERROR("Failed to create WindowContextPlatform");
        throw std::runtime_error("Failed to create WindowContextPlatform");
    }

    if (!m_WindowFactory)
    {
        SOLARC_WINDOW_ERROR("Failed to create WindowPlatformFactory");
        throw std::runtime_error("Failed to create WindowPlatformFactory");
    }

    SOLARC_WINDOW_INFO("WindowContext initialized");
}

WindowContext::~WindowContext()
{
    SOLARC_WINDOW_INFO("WindowContext destructor");
    Shutdown();
}

void WindowContext::OnDestroyWindow(Window* window)
{
    std::lock_guard lock(m_WindowsMutex);

    auto it = std::find_if(m_Windows.begin(), m_Windows.end(),
        [window](const auto& w) { return w.get() == window; });

    if (it != m_Windows.end())
    {
        SOLARC_WINDOW_DEBUG("Removing window from tracking: '{}'", (*it)->GetTitle());
        
        m_Windows.erase(it);
    }
}

void WindowContext::PollEvents()
{
    m_ThreadChecker.AssertOnOwnerThread("WindowContext::PollEvents");

    if (m_Shutdown) return;

    // Poll platform events
    m_Platform->PollEvents();

    // Update all windows to process their queued events
    std::vector<std::shared_ptr<Window>> windows;
    {
        std::lock_guard lock(m_WindowsMutex);
        windows.assign(m_Windows.begin(), m_Windows.end());
    }

    for (auto& window : windows)
    {
        if (window && window->IsVisible())
        {
            window->Update();
        }
    }
}

void WindowContext::Shutdown()
{
    if (m_Shutdown) return;

    SOLARC_WINDOW_INFO("WindowContext shutting down...");
    m_Shutdown = true;

    std::vector<std::shared_ptr<Window>> windowsToDestroy;
    {
        std::lock_guard lock(m_WindowsMutex);
        windowsToDestroy.assign(m_Windows.begin(), m_Windows.end());
    }

    SOLARC_WINDOW_INFO("Destroying {} window(s)", windowsToDestroy.size());
    for (auto& w : windowsToDestroy)
    {
        if (w)
        {
            w->Destroy();
        }
    }

    if (m_Platform)
    {
        m_Platform->Shutdown();
    }

    SOLARC_WINDOW_INFO("WindowContext shutdown complete");
}