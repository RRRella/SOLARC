#include "Window/WindowContext.h"
#include <algorithm>
#include <memory>

WindowContext& WindowContext::Get()
{
    static WindowContext instance;
    return instance;
}

WindowContext::WindowContext()
{
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
    m_Platform.PollEvents();

    // Update all windows to process their queued events
    std::vector<std::shared_ptr<Window>> windows;
    {
        std::lock_guard lock(m_WindowsMutex);
        windows.assign(m_Windows.begin(), m_Windows.end());
    }

    for (auto& window : windows)
    {
        if (window)
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

    m_Platform.Shutdown();

    SOLARC_WINDOW_INFO("WindowContext shutdown complete");
}