#pragma once
#include "Window.h"

template <typename To, typename From>
std::unique_ptr<To> static_unique_ptr_cast(std::unique_ptr<From>&& from) {
    return std::unique_ptr<To>(static_cast<To*>(from.release()));
}
