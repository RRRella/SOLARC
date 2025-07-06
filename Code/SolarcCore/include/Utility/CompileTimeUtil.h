#pragma once
#include "Window.h"

std::unique_ptr<WindowFactory> SOLARC_CORE_API GetWindowFactory() noexcept;

void SOLARC_CORE_API InitilizeCompileTimeCode();
