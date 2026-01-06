#include "RHIIntegrationTestFixture.h"
#include <gtest/gtest.h>

using namespace Solarc;

// ============================================================================
// RHI Lifecycle Integration Tests
// ============================================================================

// Note: Most tests inherit from RHIIntegrationTestFixture which handles
// initialization. These tests cover edge cases and death conditions.

// ----------------------------------------------------------------------------
// Pre-Initialization Death Tests
// ----------------------------------------------------------------------------

TEST(RHILifecycleDeathTest, GetBeforeInitialize_Asserts)
{
    // Ensure RHI is not initialized from previous test
    if (RHI::IsInitialized()) {
        RHI::Shutdown();
    }

    EXPECT_DEATH({
        Log::SetAllLevels(LogLevel::Critical);
        RHI::Get();
        }, "RHI not initialized");
}

TEST(RHILifecycleDeathTest, InitializeWithNullWindow_Asserts)
{
    // Ensure clean state
    if (RHI::IsInitialized()) {
        RHI::Shutdown();
    }

    EXPECT_DEATH({
        RHI::Initialize(nullptr);
        }, "Window cannot be null");
}

// ----------------------------------------------------------------------------
// Initialization Tests
// ----------------------------------------------------------------------------

class RHILifecycleIntegrationTest : public RHIPerTestIntegrationTestFixture {};

TEST_F(RHILifecycleIntegrationTest, RHIIsInitializedAfterSetup)
{
    EXPECT_TRUE(RHI::IsInitialized());
}

TEST_F(RHILifecycleIntegrationTest, GetReturnsValidInstance)
{
    EXPECT_NO_THROW({
        RHI & rhi = RHI::Get();
        (void)rhi; // Suppress unused warning
        });
}

TEST_F(RHILifecycleIntegrationTest, InitializeTwice_IsIdempotent)
{
    EXPECT_TRUE(RHI::IsInitialized());

    // Second initialization should be safe (logs warning but doesn't crash)
    EXPECT_NO_THROW({
        RHI::Initialize(m_Window);
        });

    EXPECT_TRUE(RHI::IsInitialized());
}

// ----------------------------------------------------------------------------
// Shutdown Tests
// ----------------------------------------------------------------------------

TEST_F(RHILifecycleIntegrationTest, Shutdown_MarksAsUninitialized)
{
    EXPECT_TRUE(RHI::IsInitialized());

    RHI::Shutdown();

    EXPECT_FALSE(RHI::IsInitialized());
}

TEST_F(RHILifecycleIntegrationTest, ShutdownTwice_IsIdempotent)
{
    RHI::Shutdown();
    EXPECT_FALSE(RHI::IsInitialized());

    // Second shutdown should be safe
    EXPECT_NO_THROW({
        RHI::Shutdown();
        });

    EXPECT_FALSE(RHI::IsInitialized());
}

TEST(RHILifecycleStandaloneTest, ShutdownWithoutInitialize_IsSafe)
{
    // Ensure clean state
    if (RHI::IsInitialized()) {
        RHI::Shutdown();
    }

    // Shutdown without Initialize should not crash
    EXPECT_NO_THROW({
        RHI::Shutdown();
        });

    EXPECT_FALSE(RHI::IsInitialized());
}

// ----------------------------------------------------------------------------
// State Query Tests
// ----------------------------------------------------------------------------

TEST_F(RHILifecycleIntegrationTest, IsInitialized_ReflectsStateAccurately)
{
    // Initially true (from fixture Setup)
    EXPECT_TRUE(RHI::IsInitialized());

    // After shutdown
    RHI::Shutdown();
    EXPECT_FALSE(RHI::IsInitialized());

    // After re-initialization
    RHI::Initialize(m_Window);
    EXPECT_TRUE(RHI::IsInitialized());
}

TEST(RHILifecycleDeathTest, GetAfterShutdown_Asserts)
{
    // Create and immediately shutdown
    auto& windowCtx = WindowContext::Get();
    auto window = windowCtx.CreateWindow("Temp", 800, 600);
    window->Hide();
#ifdef _WIN32
        WindowContext::Get().PollEvents(); // triggers WndProc / Wayland callbacks
#elif defined(__linux__)
        wl_display_roundtrip(WindowContextPlatform::Get().GetDisplay()); 
        window->Update();
#endif

    RHI::Initialize(window);
    RHI::Shutdown();

    // Get() should assert after shutdown
    EXPECT_DEATH({
        RHI::Get();
        }, "RHI not initialized");

    // Cleanup
    window->Destroy();
}