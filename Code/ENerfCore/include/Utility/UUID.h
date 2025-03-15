#pragma once
#include "Preprocessor/API.h"

#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
#include <windows.h>

ENERF_CORE_API inline void GeenerateUUID(UUID& uuid)
{
    UuidCreate(&uuid);
}