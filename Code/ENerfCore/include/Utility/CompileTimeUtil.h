#pragma once
#include "Window.h"

std::unique_ptr<WindowFactory> GetWindowFactory() noexcept;

void InitilizeCompileTimeCode();
