#pragma once

#ifdef __linux__

#include "Window/WindowContextPlatformFactory.h"

class WaylandWindowContextPlatformFactory : public WindowContextPlatformFactory
{
public:
    WaylandWindowContextPlatformFactory() = default;
    Components Create() const override;
};

#endif