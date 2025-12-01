#pragma once
#ifdef _WIN32
#include "Window/WindowContextPlatformFactory.h"

class WindowsWindowContextPlatformFactory : public WindowContextPlatformFactory
{
public:
    Components Create() const override;
};
#endif