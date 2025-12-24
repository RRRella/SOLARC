#include "RHIIntegrationTestFixture.h"
#include <gtest/gtest.h>

using namespace Solarc;

// ============================================================================
// RHI VSync Integration Tests
// ============================================================================

class RHIVSyncIntegrationTest : public RHIPerTestIntegrationTestFixture
{

};

// ----------------------------------------------------------------------------
// VSync State Tests
// ----------------------------------------------------------------------------

TEST_F(RHIVSyncIntegrationTest, DefaultVSyncIsEnabled)
{
    auto& rhi = RHI::Get();
    EXPECT_TRUE(rhi.GetVSync());
}

TEST_F(RHIVSyncIntegrationTest, SetVSyncToFalse_UpdatesState)
{
    auto& rhi = RHI::Get();

    rhi.SetVSync(false);
    EXPECT_FALSE(rhi.GetVSync());
}

TEST_F(RHIVSyncIntegrationTest, SetVSyncToTrue_UpdatesState)
{
    auto& rhi = RHI::Get();

    rhi.SetVSync(false);
    EXPECT_FALSE(rhi.GetVSync());

    rhi.SetVSync(true);
    EXPECT_TRUE(rhi.GetVSync());
}

TEST_F(RHIVSyncIntegrationTest, ToggleVSyncMultipleTimes_Stable)
{
    auto& rhi = RHI::Get();

    for (int i = 0; i < 10; ++i) {
        bool enabled = (i % 2 == 0);
        rhi.SetVSync(enabled);
        EXPECT_EQ(rhi.GetVSync(), enabled);
    }
}

// ----------------------------------------------------------------------------
// VSync Idempotency Tests
// ----------------------------------------------------------------------------

TEST_F(RHIVSyncIntegrationTest, SetVSyncToSameValue_IsIdempotent)
{
    auto& rhi = RHI::Get();

    // Set to true multiple times
    rhi.SetVSync(true);
    rhi.SetVSync(true);
    rhi.SetVSync(true);
    EXPECT_TRUE(rhi.GetVSync());

    // Set to false multiple times
    rhi.SetVSync(false);
    rhi.SetVSync(false);
    rhi.SetVSync(false);
    EXPECT_FALSE(rhi.GetVSync());
}

// ----------------------------------------------------------------------------
// VSync with Frame Cycles
// ----------------------------------------------------------------------------

TEST_F(RHIVSyncIntegrationTest, ChangeVSyncBetweenFrames_Succeeds)
{
    auto& rhi = RHI::Get();

    // Frame with VSync on
    rhi.SetVSync(true);
    EXPECT_NO_THROW({
        RunFrameCycle();
        });
    EXPECT_TRUE(rhi.GetVSync());

    // Frame with VSync off
    rhi.SetVSync(false);
    EXPECT_NO_THROW({
        RunFrameCycle();
        });
    EXPECT_FALSE(rhi.GetVSync());

    // Frame with VSync on again
    rhi.SetVSync(true);
    EXPECT_NO_THROW({
        RunFrameCycle();
        });
    EXPECT_TRUE(rhi.GetVSync());
}

TEST_F(RHIVSyncIntegrationTest, ChangeVSyncDuringFrame_TakesEffect)
{
    auto& rhi = RHI::Get();

    rhi.BeginFrame();

    // Change VSync mid-frame
    rhi.SetVSync(false);
    EXPECT_FALSE(rhi.GetVSync());

    rhi.Clear(0.1f, 0.2f, 0.3f, 1.0f);
    rhi.EndFrame();

    // Present should use the new VSync setting
    EXPECT_NO_THROW({
        rhi.Present();
        });

    EXPECT_FALSE(rhi.GetVSync());
}

TEST_F(RHIVSyncIntegrationTest, VSyncStatePersistsAcrossFrames)
{
    auto& rhi = RHI::Get();

    rhi.SetVSync(false);

    // Run multiple frames without changing VSync
    for (int i = 0; i < 5; ++i) {
        RunFrameCycle();
        EXPECT_FALSE(rhi.GetVSync());
    }
}

TEST_F(RHIVSyncIntegrationTest, AlternateVSyncEachFrame_Stable)
{
    auto& rhi = RHI::Get();

    for (int i = 0; i < 10; ++i) {
        bool vsync = (i % 2 == 0);
        rhi.SetVSync(vsync);

        EXPECT_NO_THROW({
            RunFrameCycle();
            });

        EXPECT_EQ(rhi.GetVSync(), vsync);
    }
}