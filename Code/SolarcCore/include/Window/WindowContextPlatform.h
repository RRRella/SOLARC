#pragma once
#include "Event/WindowEvent.h"
#include <functional>
#include <memory>
#include <mutex>

class WindowPlatform;
class WindowPlatformFactory;

using WindowEventDispatcher = std::function<void(std::shared_ptr<const WindowEvent>)>;

/**
 * Abstract backend for WindowContext (per-platform)
 */
class WindowContextPlatform
{
public:
    virtual ~WindowContextPlatform() = default;

    void SetEventDispatcher(WindowEventDispatcher dispatcher)
    {
        std::lock_guard lock(m_DispatcherMutex);
        m_Dispatcher = std::move(dispatcher);
    }

    /**
     * Register a window with the platform for event routing
     * param window: Window platform to register
     * note: Called after window creation to associate platform-specific handles
     */
    virtual void RegisterWindow(WindowPlatform* window) = 0;

    /**
     * Unregister a window from the platform
     * param window: Window platform to unregister
     * note: Called before window destruction to clean up associations
     */
    virtual void UnregisterWindow(WindowPlatform* window) = 0;

    virtual void PollEvents() = 0;
    virtual void Shutdown() = 0;

protected:
    WindowContextPlatform() {}

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