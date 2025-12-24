#include "RHIIntegrationTestFixture.h"
#include <gtest/gtest.h>

using namespace Solarc;

// ============================================================================
// RHI Window State Integration Tests
// ============================================================================

class RHIWindowStateIntegrationTest : public RHIPerTestIntegrationTestFixture
{

};


// ----------------------------------------------------------------------------
// Minimized Window Tests
// ----------------------------------------------------------------------------

TEST_F(RHIWindowStateIntegrationTest, MinimizedWindow_FrameCycleIsSafe)
{
    auto& rhi = RHI::Get();

    // Get platform and minimize
    m_Window->Minimize();
    m_Window->Update();

    // Frame cycle while minimized should be safe (no-op internally)
    EXPECT_NO_THROW({
        rhi.BeginFrame();
        rhi.EndFrame();
        rhi.Present();
        });
}

TEST_F(RHIWindowStateIntegrationTest, RestoreFromMinimize_ResumesRendering)
{
    auto& rhi = RHI::Get();

    // Normal frame
    EXPECT_NO_THROW({
        RunFrameCycle(1.0f, 0.0f, 0.0f, 1.0f);
        });

    // Minimize
    m_Window->Minimize();
    m_Window->Update();

    // Frame while minimized
    EXPECT_NO_THROW({
        rhi.BeginFrame();
        rhi.EndFrame();
        rhi.Present();
        });

    // Restore
    m_Window->Restore();
    m_Window->Update();

    // Normal frame should work again
    EXPECT_NO_THROW({
        RunFrameCycle(0.0f, 1.0f, 0.0f, 1.0f);
        });
}

TEST_F(RHIWindowStateIntegrationTest, MultipleMinimizeRestoreCycles_Stable)
{
    auto& rhi = RHI::Get();

    for (int i = 0; i < 3; ++i) {
        // Normal frame
        EXPECT_NO_THROW({
            RunFrameCycle();
            });

        // Minimize
        m_Window->Minimize();
        m_Window->Update();

        EXPECT_NO_THROW({
            rhi.BeginFrame();
            rhi.EndFrame();
            rhi.Present();
            });

        // Restore
        m_Window->Restore();
        m_Window->Update();
    }

    // Final frame
    EXPECT_NO_THROW({
        RunFrameCycle(1.0f, 1.0f, 1.0f, 1.0f);
        });
}

// ----------------------------------------------------------------------------
// Window Visibility Tests
// ----------------------------------------------------------------------------

TEST_F(RHIWindowStateIntegrationTest, HiddenWindow_FrameCycleSucceeds)
{
    auto& rhi = RHI::Get();

    m_Window->Hide();
    m_Window->Update();

    // Window starts hidden in fixture
    EXPECT_FALSE(m_Window->IsVisible());

    // Frame cycle should still work with hidden window
    EXPECT_NO_THROW({
        RunFrameCycle();
        });
}

TEST_F(RHIWindowStateIntegrationTest, ShowAndHideWindow_RenderingContinues)
{
    auto& rhi = RHI::Get();

    // Show window
    m_Window->Show();
    m_Window->Update();

    EXPECT_TRUE(m_Window->IsVisible());

    EXPECT_NO_THROW({
        RunFrameCycle();
        });

    // Hide window
    m_Window->Hide();
    m_Window->Update();

    EXPECT_FALSE(m_Window->IsVisible());

    EXPECT_NO_THROW({
        RunFrameCycle();
        });
}