#pragma once
#include "WindowPlatform.h"
#include <memory>
#include <string>

/**
 * Abstract factory for creating platform-specific window implementations
 */
class WindowPlatformFactory
{
public:
    virtual ~WindowPlatformFactory() = default;

    /**
     * Create a platform-specific window
     * param title: Window title
     * param width: Window width
     * param height: Window height
     * return Unique pointer to platform window
     */
    virtual std::unique_ptr<WindowPlatform> Create(
        const std::string& title,
        int32_t width,
        int32_t height) = 0;
};