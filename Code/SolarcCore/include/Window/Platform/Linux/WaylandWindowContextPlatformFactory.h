#pragma once

#ifdef linux

#include "Window/WindowContextPlatformFactory.h"

class WaylandWindowContextPlatformFactory : public WindowContextPlatformFactory
{
public:
    WaylandWindowContextPlatformFactory() = default;

    PlatformComponents CreateComponents() override;
};

#endif