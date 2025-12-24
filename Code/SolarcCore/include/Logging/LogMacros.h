#pragma once
#include "Logging/Log.h"

// ============================================================================
// Core Logger Macros (Most commonly used)
// ============================================================================

#define SOLARC_TRACE(...)       ::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define SOLARC_DEBUG(...)       ::Log::GetCoreLogger()->debug(__VA_ARGS__)
#define SOLARC_INFO(...)        ::Log::GetCoreLogger()->info(__VA_ARGS__)
#define SOLARC_WARN(...)        ::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define SOLARC_ERROR(...)       ::Log::GetCoreLogger()->error(__VA_ARGS__)
#define SOLARC_CRITICAL(...)    ::Log::GetCoreLogger()->critical(__VA_ARGS__)

// ============================================================================
// Category-Specific Macros
// ============================================================================

// Rendering
#define SOLARC_RENDER_TRACE(...)    ::Log::GetLogger(::LogCategory::Rendering)->trace(__VA_ARGS__)
#define SOLARC_RENDER_DEBUG(...)    ::Log::GetLogger(::LogCategory::Rendering)->debug(__VA_ARGS__)
#define SOLARC_RENDER_INFO(...)     ::Log::GetLogger(::LogCategory::Rendering)->info(__VA_ARGS__)
#define SOLARC_RENDER_WARN(...)     ::Log::GetLogger(::LogCategory::Rendering)->warn(__VA_ARGS__)
#define SOLARC_RENDER_ERROR(...)    ::Log::GetLogger(::LogCategory::Rendering)->error(__VA_ARGS__)

// Assets
#define SOLARC_ASSET_TRACE(...)     ::Log::GetLogger(::LogCategory::Assets)->trace(__VA_ARGS__)
#define SOLARC_ASSET_DEBUG(...)     ::Log::GetLogger(::LogCategory::Assets)->debug(__VA_ARGS__)
#define SOLARC_ASSET_INFO(...)      ::Log::GetLogger(::LogCategory::Assets)->info(__VA_ARGS__)
#define SOLARC_ASSET_WARN(...)      ::Log::GetLogger(::LogCategory::Assets)->warn(__VA_ARGS__)
#define SOLARC_ASSET_ERROR(...)     ::Log::GetLogger(::LogCategory::Assets)->error(__VA_ARGS__)

// Window
#define SOLARC_WINDOW_TRACE(...)    ::Log::GetLogger(::LogCategory::Window)->trace(__VA_ARGS__)
#define SOLARC_WINDOW_DEBUG(...)    ::Log::GetLogger(::LogCategory::Window)->debug(__VA_ARGS__)
#define SOLARC_WINDOW_INFO(...)     ::Log::GetLogger(::LogCategory::Window)->info(__VA_ARGS__)
#define SOLARC_WINDOW_WARN(...)     ::Log::GetLogger(::LogCategory::Window)->warn(__VA_ARGS__)
#define SOLARC_WINDOW_ERROR(...)    ::Log::GetLogger(::LogCategory::Window)->error(__VA_ARGS__)

// Physics
#define SOLARC_PHYSICS_TRACE(...)   ::Log::GetLogger(::LogCategory::Physics)->trace(__VA_ARGS__)
#define SOLARC_PHYSICS_DEBUG(...)   ::Log::GetLogger(::LogCategory::Physics)->debug(__VA_ARGS__)
#define SOLARC_PHYSICS_INFO(...)    ::Log::GetLogger(::LogCategory::Physics)->info(__VA_ARGS__)
#define SOLARC_PHYSICS_WARN(...)    ::Log::GetLogger(::LogCategory::Physics)->warn(__VA_ARGS__)
#define SOLARC_PHYSICS_ERROR(...)   ::Log::GetLogger(::LogCategory::Physics)->error(__VA_ARGS__)

// Animation
#define SOLARC_ANIM_TRACE(...)      ::Log::GetLogger(::LogCategory::Animation)->trace(__VA_ARGS__)
#define SOLARC_ANIM_DEBUG(...)      ::Log::GetLogger(::LogCategory::Animation)->debug(__VA_ARGS__)
#define SOLARC_ANIM_INFO(...)       ::Log::GetLogger(::LogCategory::Animation)->info(__VA_ARGS__)
#define SOLARC_ANIM_WARN(...)       ::Log::GetLogger(::LogCategory::Animation)->warn(__VA_ARGS__)
#define SOLARC_ANIM_ERROR(...)      ::Log::GetLogger(::LogCategory::Animation)->error(__VA_ARGS__)

// Job System
#define SOLARC_JOB_TRACE(...)       ::Log::GetLogger(::LogCategory::JobSystem)->trace(__VA_ARGS__)
#define SOLARC_JOB_DEBUG(...)       ::Log::GetLogger(::LogCategory::JobSystem)->debug(__VA_ARGS__)
#define SOLARC_JOB_INFO(...)        ::Log::GetLogger(::LogCategory::JobSystem)->info(__VA_ARGS__)
#define SOLARC_JOB_WARN(...)        ::Log::GetLogger(::LogCategory::JobSystem)->warn(__VA_ARGS__)
#define SOLARC_JOB_ERROR(...)       ::Log::GetLogger(::LogCategory::JobSystem)->error(__VA_ARGS__)

// Application
#define SOLARC_APP_TRACE(...)       ::Log::GetLogger(::LogCategory::App)->trace(__VA_ARGS__)
#define SOLARC_APP_DEBUG(...)       ::Log::GetLogger(::LogCategory::App)->debug(__VA_ARGS__)
#define SOLARC_APP_INFO(...)        ::Log::GetLogger(::LogCategory::App)->info(__VA_ARGS__)
#define SOLARC_APP_WARN(...)        ::Log::GetLogger(::LogCategory::App)->warn(__VA_ARGS__)
#define SOLARC_APP_ERROR(...)       ::Log::GetLogger(::LogCategory::App)->error(__VA_ARGS__)


// ============================================================================
// Assertions (unchanged, always active in debug builds)
// ============================================================================

#ifdef SOLARC_DEBUG_BUILD
#define SOLARC_ASSERT(condition, ...)                                       \
    do {                                                                    \
        if (!(condition)) {                                                 \
            /* 1. Log to the normal logger*/               \
            SOLARC_CRITICAL("Assertion failed: {}", #condition);            \
            SOLARC_CRITICAL(__VA_ARGS__);                                   \
            SOLARC_CRITICAL("File: {}, Line: {}", __FILE__, __LINE__);      \
            ::Log::FlushAll();                                              \
                                                                            \
            /* 2. FORCE output to stderr so EXPECT_DEATH can see it */      \
            /* fmt::print allows using the same format args as spdlog */    \
            fmt::print(stderr, __VA_ARGS__);                                \
            fmt::print(stderr, "\n");                                       \
                                                                            \
            std::abort();                                                   \
        }                                                                   \
    } while(0)
#else
#define SOLARC_ASSERT(condition, ...) (void)0
#endif

// ============================================================================
// Performance Logging (Scoped Timer)
// ============================================================================

namespace Solarc {

    class ScopedTimer
    {
    public:
        explicit ScopedTimer(const char* name)
            : m_Name(name)
            , m_Start(std::chrono::high_resolution_clock::now())
        {
        }

        ~ScopedTimer()
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_Start);
            SOLARC_TRACE("[PERF] {} took {:.3f}ms", m_Name, duration.count() / 1000.0);
        }

    private:
        const char* m_Name;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
    };

} // namespace Solarc

#define SOLARC_PROFILE_SCOPE(name)      ::Solarc::ScopedTimer _timer##__LINE__(name)
#define SOLARC_PROFILE_FUNCTION()       SOLARC_PROFILE_SCOPE(__FUNCTION__)

// ============================================================================
// Scoped Category Logger (No-op when logging disabled)
// ============================================================================

namespace Solarc {

    class ScopedLogCategory
    {
    public:
        explicit ScopedLogCategory(LogCategory) {}
        ~ScopedLogCategory() {}

        std::shared_ptr<spdlog::logger> GetLogger() const
        {
            return Log::GetLogger(m_Category);
        }

    private:
        LogCategory m_Category;
    };

} // namespace Solarc

#define SOLARC_LOG_CATEGORY(category) \
    ::Solarc::ScopedLogCategory _scoped_log_##__LINE__(category)
