#pragma once
#include "Window.h"
#include "Event/EventQueue.h"

std::unique_ptr<WindowPlatformFactory> SOLARC_CORE_API GetWindowPlatformFactory() noexcept;

template <typename To, typename From>
std::unique_ptr<To> static_unique_ptr_cast(std::unique_ptr<From>&& from) {
    return std::unique_ptr<To>(static_cast<To*>(from.release()));
}

