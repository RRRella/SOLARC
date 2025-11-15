#include "SolarcApp.h"
#include "Utility/CompileTimeUtil.h"
#include "Utility/FileSystemUtil.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

void PrintUsage(const char* exeName)
{
    std::cout << "Usage: " << exeName << " [OPTIONS] [PROJECT_FILE]\n"
        << "Options:\n"
        << "  --help, -h       Show this help message\n"
        << "  --config PATH    Specify config file (default: ./Data/config.toml)\n"
        << "\n"
        << "PROJECT_FILE must end with '.solarcproj' and exist on disk.\n";
}

bool IsValidProjectFile(const std::string& path)
{
    if (!fs::exists(path) || !fs::is_regular_file(path))
        return false;
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".solarcproj";
}

int main(int argc, char** argv)
{
    try
    {
        std::string configPath = GetExeDir() + "\\Data\\config.toml";
        std::string projectPath;

        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--help" || arg == "-h")
            {
                PrintUsage(argv[0]);
                return EXIT_SUCCESS;
            }
            else if (arg == "--config")
            {
                if (++i >= argc)
                {
                    std::cerr << "Error: --config requires a path.\n";
                    return EXIT_FAILURE;
                }
                configPath = argv[i];
            }
            else if (arg.starts_with("--"))
            {
                std::cerr << "Error: Unknown option '" << arg << "'\n";
                PrintUsage(argv[0]);
                return EXIT_FAILURE;
            }
            else
            {
                if (!projectPath.empty())
                {
                    std::cerr << "Error: Only one project file allowed.\n";
                    return EXIT_FAILURE;
                }
                projectPath = arg;
            }
        }

        if (!projectPath.empty() && !IsValidProjectFile(projectPath))
        {
            std::cerr << "Error: Invalid project file: " << projectPath << "\n";
            return EXIT_FAILURE;
        }

        SolarcApp::Initialize(configPath);
        auto& app = SolarcApp::Get();

        if (!projectPath.empty())
        {
            app.SetInitialProject(fs::canonical(projectPath).string());
        }

        app.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "Unknown fatal error.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}