#include <string>
#include <filesystem>
#include "Utility/FileSystemUtil.h"

#if defined(_WIN32)
    #include <windows.h> 
    #include <limits.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <limits.h>
    #include <sys/types.h>
#endif

std::string SOLARC_CORE_API GetExeDir()
{
#if defined(_WIN32)
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path().string();

#elif defined(__linux__)
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count == -1) return {};
    return std::filesystem::path(std::string(result, count)).parent_path().string();

#else
    static_assert(false, "Unsupported platform");
#endif
}
