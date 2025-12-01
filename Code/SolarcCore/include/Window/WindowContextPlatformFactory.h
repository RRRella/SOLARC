#pragma once
#include "WindowContextPlatform.h"
#include "WindowPlatformFactory.h"
#include <memory>
#include <utility>

/**
 * Abstract factory for creating platform-specific window context implementations
 */
class WindowContextPlatformFactory
{
public:
    struct Components
    {
        std::unique_ptr<WindowContextPlatform> context;
        std::unique_ptr<WindowPlatformFactory> windowFactory;
    };

    virtual ~WindowContextPlatformFactory() = default;

    /**
     * Create both platform context and window factory atomically.
     * This is required for platforms where window creation depends on context state
     * (e.g., Wayland requires shared wl_display/compositor).
     */
    virtual Components Create() const = 0;
};