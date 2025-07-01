#pragma once
#include "Preprocessor/API.h"

#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
#include <windows.h>

static_assert(sizeof(_GUID) == 128 / CHAR_BIT, "GUID");

// Specialize std::hash
namespace std {
    template<> struct ENERF_CORE_API hash<UUID>
    {
        size_t operator()(const UUID& uuid) const noexcept {
            const std::uint64_t* ptr = reinterpret_cast<const std::uint64_t*>(&uuid);
            std::hash<std::uint64_t> hash;
            return hash(ptr[0]) ^ hash(ptr[1]);
        }
    };
}

ENERF_CORE_API inline void GeenerateUUID(UUID& uuid)
{
    UuidCreate(&uuid);
}