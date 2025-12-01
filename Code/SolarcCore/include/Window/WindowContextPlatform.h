#pragma once
#include "Event/WindowEvent.h"
#include <functional>
#include <memory>
#include <mutex>

class WindowPlatform;
class WindowPlatformFactory;

/**
 * Abstract backend for WindowContext (per-platform)
 */
class WindowContextPlatform
{
public:
    virtual ~WindowContextPlatform() = default;

    virtual void PollEvents() = 0;
    virtual void Shutdown() = 0;

protected:
    WindowContextPlatform() {}
};