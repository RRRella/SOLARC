#pragma once

#ifdef SOLARC_RENDERER_VULKAN

#include "VulkanDevice.h"
#include "Rendering/RHI/RHIResult.h"
#include <vulkan/vulkan.h>
#include <array>
#include <vector>

    /**
     * Vulkan Command Context
     *
     * Manages command pools, command buffers, and synchronization for GPU command submission.
     * Implements frame pacing with fences to prevent CPU from getting ahead of GPU.
     */
    class VulkanCommandContext
    {
    public:
        static constexpr uint32_t FRAMES_IN_FLIGHT = 2; // Double-buffered

        /**
         * Create command context for device
         * param device: Vulkan device
         * throws std::runtime_error if creation fails
         */
        explicit VulkanCommandContext(VulkanDevice* device);
        ~VulkanCommandContext();

        // Non-copyable, non-movable
        VulkanCommandContext(const VulkanCommandContext&) = delete;
        VulkanCommandContext& operator=(const VulkanCommandContext&) = delete;
        VulkanCommandContext(VulkanCommandContext&&) = delete;
        VulkanCommandContext& operator=(VulkanCommandContext&&) = delete;

        /**
         * Get the current command buffer
         * return VkCommandBuffer handle
         */
        VkCommandBuffer GetCommandBuffer() const;

        /**
         * Get the render finished semaphore for current frame
         * return VkSemaphore handle
         */
        VkSemaphore GetRenderFinishedSemaphore() const;

        /**
         * Get the present semaphore (for Present to wait on)
         * return VkSemaphore handle
         */
        VkSemaphore GetPresentSemaphore() const { return GetRenderFinishedSemaphore(); }

        /**
         * Begin a new frame
         * param imageAvailableSemaphore: Semaphore to wait for (from swapchain)
         * note: Waits for the fence from FRAMES_IN_FLIGHT ago
         * note: Resets command buffer for current frame
         */
        void BeginFrame(VkSemaphore imageAvailableSemaphore);

        /**
         * Begin render pass
         * param framebuffer: Framebuffer to render to
         * param renderPass: Render pass to use
         * param width: Render area width
         * param height: Render area height
         * param clearColor: RGBA clear color (4 floats)
         */
        void BeginRenderPass(
            VkFramebuffer framebuffer,
            VkRenderPass renderPass,
            uint32_t width,
            uint32_t height,
            const float clearColor[4]
        );

        /**
         * End render pass
         */
        void EndRenderPass();

        /**
         * End the current frame
         * note: Ends command buffer recording and submits to queue
         * note: Signals the fence and render finished semaphore for this frame
         */
        void EndFrame(VkSemaphore renderFinishedSemaphore);

        /**
         * Wait for the current frame's fence to be signaled.
         * note: Ensures previous use of this frame's resources (including semaphores) is complete.
         */
        void WaitForCurrentFrame();


        /**
         * Wait for all frames to complete on GPU
         * note: Blocks until GPU is idle
         * note: Used before resize or shutdown
         */
        void WaitForGPU();

        /**
         * Get current frame index (for debugging)
         */
        uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

    private:
        struct FrameResources
        {
            VkCommandPool commandPool;
            VkCommandBuffer commandBuffer;
            VkFence inFlightFence;
            VkSemaphore imageAvailableSemaphore; // Stored from BeginFrame
        };

        void CreateCommandPools();
        void CreateCommandBuffers();
        void CreateSyncObjects();
        void WaitForFrame(uint32_t frameIndex);

        VulkanDevice* m_Device; // Non-owning
        std::array<FrameResources, FRAMES_IN_FLIGHT> m_FrameResources;
        uint32_t m_CurrentFrameIndex = 0;
    };

#endif // SOLARC_RENDERER_VULKAN