#pragma once
#ifdef _WIN32
#include "Window/WindowContextPlatformFactory.h"
#include "Preprocessor/API.h"

class SOLARC_CORE_API WindowsWindowContextPlatformFactory : public WindowContextPlatformFactory
{
public:
    Components Create() const override;
};
#endif