#pragma once
#include "WindowContextPlatform.h"
#include "WindowPlatformFactory.h"
#include <memory>

/**
 * Components created by platform factory
 */
struct PlatformComponents
{
    std::unique_ptr<WindowContextPlatform> context;
    std::unique_ptr<WindowPlatformFactory> windowFactory;
};

/**
 * Abstract factory for creating platform-specific window context implementations
 */
class WindowContextPlatformFactory
{
public:
    virtual ~WindowContextPlatformFactory() = default;

    /**
     * Create both platform context and window factory atomically
     * This ensures correct initialization order and lifetime dependencies
     * return Platform components with guaranteed valid pointers
     */
    virtual PlatformComponents CreateComponents() = 0;
};