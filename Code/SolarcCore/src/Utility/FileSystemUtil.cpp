#include "Utility/FileSystemUtil.h"
#include <windows.h>
#include <filesystem>


std::string GetExeDir()
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path().string();
}