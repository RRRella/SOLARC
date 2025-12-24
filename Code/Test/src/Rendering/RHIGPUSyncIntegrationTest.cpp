#include "RHIIntegrationTestFixture.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace Solarc;

// ============================================================================
// RHI GPU Synchronization Integration Tests
// ============================================================================

class RHIGPUSyncIntegrationTest : public RHIPerTestIntegrationTestFixture
{

};

// ----------------------------------------------------------------------------
// WaitForGPU Tests
// ----------------------------------------------------------------------------

TEST_F(RHIGPUSyncIntegrationTest, WaitForGPU_AfterFrameCompletes)
{
    auto& rhi = RHI::Get();

    // Render a frame
    RunFrameCycle();

    // Wait for GPU should complete without hanging
    EXPECT_NO_THROW({
        rhi.WaitForGPU();
        });
}

TEST_F(RHIGPUSyncIntegrationTest, WaitForGPU_MultipleCallsAreSafe)
{
    auto& rhi = RHI::Get();

    RunFrameCycle();

    // Multiple waits should be safe
    EXPECT_NO_THROW({
        rhi.WaitForGPU();
        rhi.WaitForGPU();
        rhi.WaitForGPU();
        });
}

TEST_F(RHIGPUSyncIntegrationTest, WaitForGPU_AfterMultipleFrames)
{
    auto& rhi = RHI::Get();

    // Render multiple frames
    for (int i = 0; i < 10; ++i) {
        RunFrameCycle();
    }

    // Wait should ensure all frames are complete
    EXPECT_NO_THROW({
        rhi.WaitForGPU();
        });
}

TEST_F(RHIGPUSyncIntegrationTest, WaitForGPU_BeforeShutdown)
{
    auto& rhi = RHI::Get();

    // Render frames
    for (int i = 0; i < 5; ++i) {
        RunFrameCycle();
    }

    // Wait before shutdown
    EXPECT_NO_THROW({
        rhi.WaitForGPU();
        });

    // Shutdown should be clean
    EXPECT_NO_THROW({
        RHI::Shutdown();
        });
}

// ----------------------------------------------------------------------------
// Thread Safety Tests
// ----------------------------------------------------------------------------

TEST_F(RHIGPUSyncIntegrationTest, WaitForGPU_CalledFromMultipleThreads)
{
    auto& rhi = RHI::Get();

    // Render a frame
    RunFrameCycle();

    // Spawn multiple threads that all call WaitForGPU
    constexpr int numThreads = 4;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{ 0 };

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&]() {
            try {
                rhi.WaitForGPU();
                successCount++;
            }
            catch (...) {
                // Should not throw
            }
            });
    }

    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }

    // All threads should succeed
    EXPECT_EQ(successCount, numThreads);
}