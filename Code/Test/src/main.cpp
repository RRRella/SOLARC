#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "Logging/Log.h"

int main(int argc, char** argv)
{

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

    Log::SetAllLevels(LogLevel::Off);

    // Initialize Google Test/Mock ONLY
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}