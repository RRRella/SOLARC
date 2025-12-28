#include "RHIIntegrationTestFixture.h"
#include <gtest/gtest.h>

using namespace Solarc;

// ============================================================================
// RHI Frame Cycle Integration Tests
// ============================================================================

class RHIFrameCycleIntegrationTest : public RHIPerTestIntegrationTestFixture
{
};

// ----------------------------------------------------------------------------
// Invalid Frame Cycle Death Tests
// ----------------------------------------------------------------------------

TEST_F(RHIFrameCycleIntegrationTest, ClearBeforeBeginFrame_Asserts)
{
    EXPECT_DEATH({
        RHI::Get().Clear(1.0f, 0.0f, 0.0f, 1.0f);
        }, "Clear called outside BeginFrame/EndFrame");
}

TEST_F(RHIFrameCycleIntegrationTest, EndFrameWithoutBegin_Asserts)
{
    EXPECT_DEATH({
        RHI::Get().EndFrame();
        }, "EndFrame called without BeginFrame");
}

TEST_F(RHIFrameCycleIntegrationTest, PresentBeforeEndFrame_Asserts)
{
    auto& rhi = RHI::Get();

    m_Window->Show();
    PumpWindowEvents(m_Window);

    rhi.BeginFrame();

    // Present without EndFrame should assert
    EXPECT_DEATH({
        rhi.Present();
        }, "Present called before EndFrame");

    rhi.EndFrame();
}

TEST_F(RHIFrameCycleIntegrationTest, BeginFrameTwiceWithoutEnd_Asserts)
{
    auto& rhi = RHI::Get();

    rhi.BeginFrame();

    // Second BeginFrame without EndFrame should assert
    EXPECT_DEATH({
        rhi.BeginFrame();
        }, "BeginFrame called twice without EndFrame");

    rhi.EndFrame();
}

// ----------------------------------------------------------------------------
// Valid Frame Cycle Tests
// ----------------------------------------------------------------------------

TEST_F(RHIFrameCycleIntegrationTest, CompleteFrameCycle_Succeeds)
{
    auto& rhi = RHI::Get();

    EXPECT_NO_THROW({
        rhi.BeginFrame();
        rhi.Clear(0.1f, 0.2f, 0.3f, 1.0f);
        rhi.EndFrame();
        rhi.Present();
        });
}

TEST_F(RHIFrameCycleIntegrationTest, MultipleFrameCycles_Succeed)
{
    // Run 10 complete frame cycles
    for (int i = 0; i < 10; ++i) {
        EXPECT_NO_THROW({
            RunFrameCycle();
            });
    }
}

TEST_F(RHIFrameCycleIntegrationTest, FrameCyclesWithDifferentClearColors_Succeed)
{
    // Red
    EXPECT_NO_THROW({
        RunFrameCycle(1.0f, 0.0f, 0.0f, 1.0f);
        });

    // Green
    EXPECT_NO_THROW({
        RunFrameCycle(0.0f, 1.0f, 0.0f, 1.0f);
        });

    // Blue
    EXPECT_NO_THROW({
        RunFrameCycle(0.0f, 0.0f, 1.0f, 1.0f);
        });

    // White
    EXPECT_NO_THROW({
        RunFrameCycle(1.0f, 1.0f, 1.0f, 1.0f);
        });

    // Black
    EXPECT_NO_THROW({
        RunFrameCycle(0.0f, 0.0f, 0.0f, 1.0f);
        });
}

TEST_F(RHIFrameCycleIntegrationTest, FrameCycleWithoutClear_Succeeds)
{
    auto& rhi = RHI::Get();

    // Valid to skip Clear (results in undefined render target contents, but API-wise legal)
    EXPECT_NO_THROW({
        rhi.BeginFrame();
    // No Clear call
    rhi.EndFrame();
    rhi.Present();
        });
}

TEST_F(RHIFrameCycleIntegrationTest, MultipleClearsInSameFrame_Succeeds)
{
    auto& rhi = RHI::Get();

    rhi.BeginFrame();

    // Multiple clears are legal (each overwrites the previous)
    EXPECT_NO_THROW({
        rhi.Clear(1.0f, 0.0f, 0.0f, 1.0f); // Red
        rhi.Clear(0.0f, 1.0f, 0.0f, 1.0f); // Green (overwrites)
        rhi.Clear(0.0f, 0.0f, 1.0f, 1.0f); // Blue (overwrites)
        });

    rhi.EndFrame();
    rhi.Present();
}

// ----------------------------------------------------------------------------
// Frame Index Tests
// ----------------------------------------------------------------------------

TEST_F(RHIFrameCycleIntegrationTest, FrameIndexIncrementsAfterEndFrame)
{
    auto& rhi = RHI::Get();

    uint32_t initialIndex = rhi.GetCurrentFrameIndex();

    rhi.BeginFrame();
    rhi.Clear(0.1f, 0.2f, 0.3f, 1.0f);
    rhi.EndFrame();

    uint32_t indexAfterEnd = rhi.GetCurrentFrameIndex();
    EXPECT_EQ(indexAfterEnd, initialIndex + 1);

    rhi.Present();

    // Index should not change during Present
    EXPECT_EQ(rhi.GetCurrentFrameIndex(), indexAfterEnd);
}

TEST_F(RHIFrameCycleIntegrationTest, FrameIndexIncrementsCorrectlyOverMultipleFrames)
{
    auto& rhi = RHI::Get();

    uint32_t expectedIndex = rhi.GetCurrentFrameIndex();

    for (int i = 0; i < 5; ++i) {
        rhi.BeginFrame();
        rhi.Clear(0.1f, 0.2f, 0.3f, 1.0f);
        rhi.EndFrame();

        expectedIndex++;
        EXPECT_EQ(rhi.GetCurrentFrameIndex(), expectedIndex);

        rhi.Present();
    }
}

// ----------------------------------------------------------------------------
// Edge Case Tests
// ----------------------------------------------------------------------------

TEST_F(RHIFrameCycleIntegrationTest, BeginFrameAfterPresent_Succeeds)
{
    auto& rhi = RHI::Get();

    // Complete first frame
    RunFrameCycle();

    // Begin second frame should work
    EXPECT_NO_THROW({
        rhi.BeginFrame();
        rhi.EndFrame();
        rhi.Present();
        });
}

TEST_F(RHIFrameCycleIntegrationTest, RapidFrameCycles_Stable)
{
    // Run 50 frames rapidly to stress-test synchronization
    for (int i = 0; i < 50; ++i) {
        EXPECT_NO_THROW({
            RunFrameCycle();
            });
    }
}