#pragma once
#include "Event/WindowEvent.h"
#include <functional>
#include <memory>
#include <mutex>
#include "Preprocessor/API.h"

class WindowPlatform;
class WindowPlatformFactory;

/**
 * Abstract backend for WindowContext (per-platform)
 */
class SOLARC_CORE_API WindowContextPlatform
{
public:
    virtual ~WindowContextPlatform() = default;

    virtual void PollEvents() = 0;
    virtual void Shutdown() = 0;

protected:
    WindowContextPlatform() {}
};