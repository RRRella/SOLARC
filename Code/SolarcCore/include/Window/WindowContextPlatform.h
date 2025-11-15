#pragma once
#include "Event/WindowEvent.h"

class WindowPlatform;

using WindowEventDispatcher = std::function<void(std::shared_ptr<const WindowEvent>)>;

// Abstract backend for WindowContext (per-platform)
class WindowContextPlatform
{
public:
    virtual ~WindowContextPlatform() = default;

    void SetEventDispatcher(WindowEventDispatcher dispatcher)
    {
        std::lock_guard lock(m_DispatcherMutex);
        m_Dispatcher = std::move(dispatcher);
    }

    virtual void PollEvents() = 0;
    virtual void Shutdown() = 0;

    virtual std::shared_ptr<WindowPlatform> CreateWindowPlatform(
        const std::string& title, const int32_t& width, const int32_t& height) = 0;

    static std::unique_ptr<WindowContextPlatform> CreateWindowContextPlatform();

protected:
    WindowContextPlatform(){}

    void DispatchWindowEvent(std::shared_ptr<const WindowEvent> event)
    {
        std::lock_guard lock(m_DispatcherMutex);
        if (m_Dispatcher)
            m_Dispatcher(std::move(event));
    }

    WindowEventDispatcher m_Dispatcher;
    mutable std::mutex m_DispatcherMutex;

    // Allow test to access dispatcher for verification
    friend class MockWindowContextPlatform;
};

