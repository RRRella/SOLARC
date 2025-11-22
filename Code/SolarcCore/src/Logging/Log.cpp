#include "Logging/Log.h"
#include "spdlog/async.h"
#include <iostream>

void Log::Initialize(
    const std::string& logFilePath,
    LogLevel consoleLevel,
    LogLevel fileLevel,
    size_t maxFileSize,
    size_t maxFiles)
{
    if (s_Initialized)
    {
        std::cerr << "Log system already initialized!" << std::endl;
        return;
    }

    try
    {
        // Create console sink (colored output)
        s_ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        s_ConsoleSink->set_level(ToSpdlogLevel(consoleLevel));
        s_ConsoleSink->set_pattern(s_LogPattern);

        // Create file sink
        s_FileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logFilePath, maxFileSize, maxFiles
        );
        
        s_FileSink->set_level(ToSpdlogLevel(fileLevel));
        s_FileSink->set_pattern(s_LogPattern);

        // Create category loggers
        s_CoreLogger = CreateCategoryLogger("CORE", LogLevel::Trace);
        
        s_CategoryLoggers[LogCategory::Core] = s_CoreLogger;
        s_CategoryLoggers[LogCategory::Rendering] = CreateCategoryLogger("RENDER", LogLevel::Trace);
        s_CategoryLoggers[LogCategory::Assets] = CreateCategoryLogger("ASSETS", LogLevel::Trace);
        s_CategoryLoggers[LogCategory::Window] = CreateCategoryLogger("WINDOW", LogLevel::Trace);
        s_CategoryLoggers[LogCategory::Physics] = CreateCategoryLogger("PHYSICS", LogLevel::Trace);
        s_CategoryLoggers[LogCategory::Animation] = CreateCategoryLogger("ANIM", LogLevel::Trace);
        s_CategoryLoggers[LogCategory::Audio] = CreateCategoryLogger("AUDIO", LogLevel::Trace);
        s_CategoryLoggers[LogCategory::Scripting] = CreateCategoryLogger("SCRIPT", LogLevel::Trace);
        s_CategoryLoggers[LogCategory::Network] = CreateCategoryLogger("NET", LogLevel::Trace);
        s_CategoryLoggers[LogCategory::JobSystem] = CreateCategoryLogger("JOBS", LogLevel::Trace);
        s_CategoryLoggers[LogCategory::App] = CreateCategoryLogger("APP", LogLevel::Trace);

        // Set spdlog to flush on warning or higher
        spdlog::flush_on(spdlog::level::warn);

        s_Initialized = true;
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}

void Log::Shutdown()
{
    if (!s_Initialized)
        return;

    s_CoreLogger->info("Shutting down logging system");
    
    FlushAll();
    
    // Clear all loggers
    s_CategoryLoggers.clear();
    s_CoreLogger.reset();
    
    // Clear sinks
    s_ConsoleSink.reset();
    s_FileSink.reset();
    
    // Shutdown spdlog
    spdlog::shutdown();
    
    s_Initialized = false;
}

std::shared_ptr<spdlog::logger> Log::GetCoreLogger()
{
    if (!s_Initialized)
    {
        std::cerr << "Log system not initialized! Call Log::Initialize() first." << std::endl;
        return nullptr;
    }
    return s_CoreLogger;
}

std::shared_ptr<spdlog::logger> Log::GetLogger(LogCategory category)
{
    if (!s_Initialized)
    {
        std::cerr << "Log system not initialized! Call Log::Initialize() first." << std::endl;
        return nullptr;
    }

    auto it = s_CategoryLoggers.find(category);
    if (it != s_CategoryLoggers.end())
        return it->second;

    // Fallback to core logger
    return s_CoreLogger;
}

std::shared_ptr<spdlog::logger> Log::CreateLogger(
    const std::string& name,
    LogLevel level)
{
    if (!s_Initialized)
    {
        std::cerr << "Log system not initialized! Call Log::Initialize() first." << std::endl;
        return nullptr;
    }

    return CreateCategoryLogger(name, level);
}void Log::SetLevel(LogCategory category, LogLevel level)
{
    if (!s_Initialized)
        return;

    auto logger = GetLogger(category);
    if (logger)
    {
        logger->set_level(ToSpdlogLevel(level));
    }
}

void Log::SetAllLevels(LogLevel level)
{
    if (!s_Initialized)
        return;

    auto spdLevel = ToSpdlogLevel(level);
    
    for (auto& [cat, logger] : s_CategoryLoggers)
    {
        logger->set_level(spdLevel);
    }
    
    if (s_ConsoleSink)
        s_ConsoleSink->set_level(spdLevel);
}

void Log::FlushAll()
{
    if (!s_Initialized)
        return;

    for (auto& [cat, logger] : s_CategoryLoggers)
    {
        logger->flush();
    }
}

void Log::EnableConsole(LogCategory category, bool enable)
{
    if (!s_Initialized)
        return;

    auto logger = GetLogger(category);
    if (logger)
    {
        // This is a simplified version - you'd need to recreate the logger
        // with/without the console sink to fully implement this
        if (enable)
            logger->set_level(spdlog::level::trace);
        else
            logger->set_level(spdlog::level::off);
    }
}

void Log::SetPattern(const std::string& pattern)
{
    s_LogPattern = pattern;
    
    if (!s_Initialized)
        return;

    if (s_ConsoleSink)
        s_ConsoleSink->set_pattern(pattern);
    
    if (s_FileSink)
        s_FileSink->set_pattern(pattern);
    
    for (auto& [cat, logger] : s_CategoryLoggers)
    {
        logger->set_pattern(pattern);
    }
}

std::shared_ptr<spdlog::logger> Log::CreateCategoryLogger(
    const std::string& name,
    LogLevel level)
{
    spdlog::sinks_init_list sinks = {s_ConsoleSink, s_FileSink};
    
    auto logger = std::make_shared<spdlog::logger>(name, sinks);
    logger->set_level(ToSpdlogLevel(level));
    
    // Register with spdlog
    spdlog::register_logger(logger);
    
    return logger;
}

spdlog::level::level_enum Log::ToSpdlogLevel(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Trace:    return spdlog::level::trace;
        case LogLevel::Debug:    return spdlog::level::debug;
        case LogLevel::Info:     return spdlog::level::info;
        case LogLevel::Warning:  return spdlog::level::warn;
        case LogLevel::Error:    return spdlog::level::err;
        case LogLevel::Critical: return spdlog::level::critical;
        case LogLevel::Off:      return spdlog::level::off;
        default:                 return spdlog::level::info;
    }
}

const char* Log::CategoryToString(LogCategory category)
{
    switch (category)
    {
        case LogCategory::Core:       return "CORE";
        case LogCategory::Rendering:  return "RENDER";
        case LogCategory::Assets:     return "ASSETS";
        case LogCategory::Window:     return "WINDOW";
        case LogCategory::Physics:    return "PHYSICS";
        case LogCategory::Animation:  return "ANIM";
        case LogCategory::Audio:      return "AUDIO";
        case LogCategory::Scripting:  return "SCRIPT";
        case LogCategory::Network:    return "NET";
        case LogCategory::JobSystem:  return "JOBS";
        case LogCategory::App:        return "APP";
        case LogCategory::Custom:     return "CUSTOM";
        default:                      return "UNKNOWN";
    }
}