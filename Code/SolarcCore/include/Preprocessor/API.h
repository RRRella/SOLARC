// Detect platform
#if defined(_WIN32) || defined(_WIN64)
    #ifdef SOLARC_CORE_STATIC
        #define SOLARC_CORE_API
    #else
        #ifdef SOLARC_CORE_BUILD
            #define SOLARC_CORE_API __declspec(dllexport)
        #else
            #define SOLARC_CORE_API __declspec(dllimport)
        #endif
    #endif
#else
    // Linux / macOS
    #ifdef SOLARC_CORE_STATIC
        #define SOLARC_CORE_API
    #else
        #ifdef SOLARC_CORE_BUILD
            #define SOLARC_CORE_API __attribute__((visibility("default")))
        #else
            #define SOLARC_CORE_API
        #endif
    #endif
#endif
