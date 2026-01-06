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
    // ========================================================================
    // Phase 1: Reset all windows' input accumulators
    // ========================================================================
    // This must happen BEFORE OS event processing so that input captured
    // during this frame starts from a clean slate.
    {
        std::lock_guard lock(m_WindowsMutex);
        for (auto& window : m_Windows)
        {
            if (window)
            {
                // Access platform via non-const method (we need to mutate it)
                // Note: This is safe because Window manages platform lifetime
                WindowPlatform* platform = window->GetPlatform();
                if (platform)
                {
                    platform->ResetThisFrameInput();
                }
            }
        }
    }

    // ========================================================================
    // Phase 2: Poll platform events (triggers OS callbacks)
    // ========================================================================
    // Win32: Calls DispatchMessage() which triggers WndProc
    // Wayland: Calls wl_display_dispatch_pending() which triggers listeners
    // 
    // During this phase, WindowPlatform::m_ThisFrameInput is populated
    // with key/mouse transitions and deltas.
    m_Platform.PollEvents();

    // ========================================================================
    // Phase 3: Update all windows (process input, emit events)
    // ========================================================================
    // Window::Update() will call UpdateInput() which reads m_ThisFrameInput
    // and updates InputState, then emits transition events.
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