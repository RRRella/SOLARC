#include "SolarcApp.h"
#include "Logging/Log.h"
#include "Logging/LogMacros.h"
#include "Utility/FileSystemUtil.h"
#include <filesystem>
#include <cstdlib>
#include <iostream> // Only for --help output (before logging init)

namespace fs = std::filesystem;

// ============================================================================
// Version Info
// ============================================================================

constexpr const char* SOLARC_VERSION = "0.1.0-alpha";

// ============================================================================
// Command Line Argument Parsing
// ============================================================================

struct CommandLineArgs
{
    std::string configPath;
    std::string projectPath;
    bool showHelp = false;
    bool showVersion = false;
};

// NOTE: PrintUsage and PrintVersion use std::cout because they're called
// BEFORE logging is initialized. This is intentional.
void PrintUsage(const char* exeName)
{
    std::cout << "Solarc Engine v" << SOLARC_VERSION << "\n\n"
        << "Usage: " << exeName << " [OPTIONS] [PROJECT_FILE]\n\n"
        << "Options:\n"
        << "  --help, -h          Show this help message\n"
        << "  --version, -v       Show version information\n"
        << "  --config PATH       Specify config file (default: ./Data/config.toml)\n\n"
        << "Arguments:\n"
        << "  PROJECT_FILE        Path to .solarcproj file to open on startup\n\n"
        << "Examples:\n"
        << "  " << exeName << "                         # Start without project\n"
        << "  " << exeName << " --config custom.toml    # Use custom config\n"
        << "  " << exeName << " myproject.solarcproj    # Open specific project\n";
}

void PrintVersion()
{
    std::cout << "Solarc Engine v" << SOLARC_VERSION << "\n";
}

bool IsValidProjectFile(const std::string& path)
{
    if (!fs::exists(path) || !fs::is_regular_file(path))
        return false;

    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".solarcproj";
}

// Returns false on error, sets appropriate flags on success
bool ParseCommandLine(int argc, char** argv, CommandLineArgs& args, std::string& errorMsg)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h")
        {
            args.showHelp = true;
            return true;
        }
        else if (arg == "--version" || arg == "-v")
        {
            args.showVersion = true;
            return true;
        }
        else if (arg == "--config")
        {
            if (++i >= argc)
            {
                errorMsg = "Error: --config requires a path argument";
                return false;
            }
            args.configPath = argv[i];
        }
        else if (arg.starts_with("--"))
        {
            errorMsg = "Error: Unknown option '" + arg + "'";
            return false;
        }
        else
        {
            // Positional argument - assume it's a project file
            if (!args.projectPath.empty())
            {
                errorMsg = "Error: Multiple project files specified. Only one is allowed.";
                return false;
            }
            args.projectPath = arg;
        }
    }

    return true;
}

bool ValidateArguments(const CommandLineArgs& args, std::string& errorMsg)
{
    // Validate config path if specified
    if (!args.configPath.empty() && !fs::exists(args.configPath))
    {
        errorMsg = "Error: Config file not found: " + args.configPath; return false;
    }

    // Validate project path if specified
    if (!args.projectPath.empty() && !IsValidProjectFile(args.projectPath))
    {
        errorMsg = "Error: Invalid or missing project file: " + args.projectPath +
            "\nProject files must have .solarcproj extension and exist on disk.";
        return false;
    }

    return true;
}

// ============================================================================
// Entry Point
// ============================================================================

int main(int argc, char** argv)
{
    // ========================================================================
    // Phase 1: Command Line Parsing (before logging)
    // ========================================================================

    CommandLineArgs args;
    args.configPath = GetExeDir() + "/Data/config.toml"; // Default config path

    std::string errorMsg;
    if (!ParseCommandLine(argc, argv, args, errorMsg))
    {
        // Pre-logging error - must use stdout/stderr
        std::cerr << errorMsg << "\n\n";
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    // Handle --help and --version (exit before logging init)
    if (args.showHelp)
    {
        PrintUsage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (args.showVersion)
    {
        PrintVersion();
        return EXIT_SUCCESS;
    }

    if (!ValidateArguments(args, errorMsg))
    {
        // Pre-logging error - must use stderr
        std::cerr << errorMsg << "\n";
        return EXIT_FAILURE;
    }

    // ========================================================================
    // Phase 2: Initialize Logging (from this point, NO iostream!)
    // ========================================================================

    try
    {
        Log::Initialize(
            "logs/solarc.log",
            LogLevel::Info,      // Console: Info and above
            LogLevel::Trace,     // File: Everything
            1024 * 1024 * 5,             // 5MB max file size
            3                             // Keep 3 backup files
        );
    }
    catch (const std::exception& e)
    {
        // Logging init failed - this is the ONLY acceptable stderr use
        std::cerr << "FATAL: Failed to initialize logging system: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    // From this point forward, ALL output goes through logging
    SOLARC_INFO("=================================================");
    SOLARC_INFO("Solarc Engine v{}", SOLARC_VERSION);
    SOLARC_INFO("=================================================");
    SOLARC_INFO("Executable: {}", argv[0]);
    SOLARC_INFO("Working directory: {}", fs::current_path().string());
    SOLARC_INFO("Config file: {}", args.configPath);

    if (!args.projectPath.empty())
    {
        SOLARC_INFO("Initial project: {}", args.projectPath);
    }
    else
    {
        SOLARC_INFO("No initial project specified");
    }

    // ========================================================================
    // Phase 3: Main Application Execution
    // ========================================================================

    int exitCode = EXIT_SUCCESS;

    try
    {
        // Initialize application
        SOLARC_INFO("Initializing Solarc application...");
        SolarcApp::Initialize(args.configPath);
        auto& app = SolarcApp::Get();

        // Set initial project if provided
        if (!args.projectPath.empty())
        {
            try
            {
                std::string canonicalPath = fs::canonical(args.projectPath).string();
                app.SetInitialProject(canonicalPath); SOLARC_APP_INFO("Set initial project: {}", canonicalPath);
            }
            catch (const fs::filesystem_error& e)
            {
                SOLARC_APP_ERROR("Failed to resolve project path '{}': {}",
                    args.projectPath, e.what());
                throw;
            }
        }

        // Run the application
        SOLARC_INFO("Starting main application loop...");
        app.Run();

        SOLARC_INFO("Application exited normally");
    }
    catch (const std::exception& e)
    {
        SOLARC_CRITICAL("=================================================");
        SOLARC_CRITICAL("FATAL EXCEPTION");
        SOLARC_CRITICAL("=================================================");
        SOLARC_CRITICAL("Exception: {}", e.what());
        SOLARC_CRITICAL("Type: {}", typeid(e).name());
        SOLARC_CRITICAL("Location: main.cpp::main()");
        SOLARC_CRITICAL("=================================================");

        // Ensure logs are written before exit
        Log::FlushAll();

        exitCode = EXIT_FAILURE;
    }
    catch (...)
    {
        SOLARC_CRITICAL("=================================================");
        SOLARC_CRITICAL("FATAL UNKNOWN EXCEPTION");
        SOLARC_CRITICAL("=================================================");
        SOLARC_CRITICAL("An unknown exception was caught");
        SOLARC_CRITICAL("Location: main.cpp::main()");
        SOLARC_CRITICAL("=================================================");

        Log::FlushAll();

        exitCode = EXIT_FAILURE;
    }

    // ========================================================================
    // Phase 4: Shutdown
    // ========================================================================

    SOLARC_INFO("=================================================");
    SOLARC_INFO("Solarc Engine Shutdown (Exit Code: {})", exitCode);
    SOLARC_INFO("=================================================");

    Log::Shutdown();

    return exitCode;
}