#pragma once
#include "Preprocessor/API.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include <memory>
#include <string>
#include <unordered_map>


// Log levels (matches spdlog levels)
enum class LogLevel
{
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Critical = 5,
    Off = 6
};

// Log categories for filtering
enum class LogCategory
{
    Core,           // Core engine systems
    Rendering,      // Renderer, GPU, shaders
    Assets,         // Asset loading, resource management
    Window,         // Window, input, events
    Physics,        // Physics simulation
    Animation,      // Animation system
    Audio,          // Audio system
    Scripting,      // Scripting/gameplay
    Network,        // Network/multiplayer
    JobSystem,      // Job system, threading
    App,            // Application-level
    Custom          // User-defined
};

class SOLARC_CORE_API Log
{
public:
    // Initialize the logging system
    // Creates both console and file loggers
    static void Initialize(
        const std::string& logFilePath = "solarc.log",
        LogLevel consoleLevel = LogLevel::Info,
        LogLevel fileLevel = LogLevel::Trace,
        size_t maxFileSize = 1024 * 1024 * 5,  // 5MB
        size_t maxFiles = 3
    );

    // Shutdown logging system
    static void Shutdown();

    // Get loggers by category
    static std::shared_ptr<spdlog::logger> GetCoreLogger();
    static std::shared_ptr<spdlog::logger> GetLogger(LogCategory category);

    // Create a custom logger for specific subsystems
    static std::shared_ptr<spdlog::logger> CreateLogger(
        const std::string& name,
        LogLevel level = LogLevel::Info
    );

    // Set log level for a category
    static void SetLevel(LogCategory category, LogLevel level);
    static void SetAllLevels(LogLevel level);

    // Flush all loggers (useful before crash or shutdown)
    static void FlushAll();

    // Enable/disable console output for a category
    static void EnableConsole(LogCategory category, bool enable);

    // Set custom log pattern
    // Default: "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v"
    static void SetPattern(const std::string& pattern);

    // Check if logging is initialized
    static bool IsInitialized() { return s_Initialized; }

private:
    static std::shared_ptr<spdlog::logger> CreateCategoryLogger(
        const std::string& name,
        LogLevel level
    );

    static spdlog::level::level_enum ToSpdlogLevel(LogLevel level);
    static const char* CategoryToString(LogCategory category);

    inline static bool s_Initialized = false;
    inline static std::string s_LogPattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v";

    // Sinks (shared across loggers)
    inline static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> s_ConsoleSink = nullptr;
    inline static std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> s_FileSink = nullptr;

    // Category loggers
    inline static std::unordered_map<LogCategory, std::shared_ptr<spdlog::logger>> s_CategoryLoggers;

    // Core logger (default)
    inline static std::shared_ptr<spdlog::logger> s_CoreLogger = nullptr;
};