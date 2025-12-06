#pragma once

#ifdef __linux__

#include "Window/WindowContextPlatformFactory.h"
#include "Preprocessor/API.h"

class SOLARC_CORE_API WaylandWindowContextPlatformFactory : public WindowContextPlatformFactory
{
public:
    WaylandWindowContextPlatformFactory() = default;
    Components Create() const override;
};

#endif