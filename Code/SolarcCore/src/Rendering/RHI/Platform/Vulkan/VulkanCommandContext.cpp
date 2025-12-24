#ifdef SOLARC_RENDERER_VULKAN

#include "VulkanCommandContext.h"
#include "Logging/LogMacros.h"
#include <stdexcept>
#include <array>

VulkanCommandContext::VulkanCommandContext(VulkanDevice* device)
    : m_Device(device)
{
    SOLARC_ASSERT(device != nullptr, "Device cannot be null");

    SOLARC_RENDER_INFO("Creating Vulkan command context");

    CreateCommandPools();
    CreateCommandBuffers();
    CreateSyncObjects();

    SOLARC_RENDER_INFO("Vulkan command context created successfully");
}

VulkanCommandContext::~VulkanCommandContext()
{
    SOLARC_RENDER_TRACE("Destroying Vulkan command context");

    VkDevice device = m_Device->GetDevice();

    // Wait for all GPU work to complete before destroying resources
    WaitForGPU();

    // Clean up frame resources
    for (auto& frame : m_FrameResources) {
        if (frame.inFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(device, frame.inFlightFence, nullptr);
        }
        if (frame.commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, frame.commandPool, nullptr);
        }
    }

    SOLARC_RENDER_TRACE("Vulkan command context destroyed");
}

void VulkanCommandContext::CreateCommandPools()
{
    SOLARC_RENDER_DEBUG("Creating command pools");

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_Device->GetGraphicsQueueFamilyIndex();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        VkResult result = vkCreateCommandPool(
            m_Device->GetDevice(),
            &poolInfo,
            nullptr,
            &m_FrameResources[i].commandPool
        );

        if (result != VK_SUCCESS) {
            auto rhiResult = ToRHIResult(result, "vkCreateCommandPool");
            SOLARC_RENDER_ERROR("Failed to create command pool for frame {}: {}",
                i, rhiResult.GetResultMessage());
            throw std::runtime_error("Failed to create command pool");
        }

        SOLARC_RENDER_TRACE("Created command pool for frame {}", i);
    }

    SOLARC_RENDER_DEBUG("Command pools created");
}

void VulkanCommandContext::CreateCommandBuffers()
{
    SOLARC_RENDER_DEBUG("Creating command buffers");

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_FrameResources[i].commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkResult result = vkAllocateCommandBuffers(
            m_Device->GetDevice(),
            &allocInfo,
            &m_FrameResources[i].commandBuffer
        );

        if (result != VK_SUCCESS) {
            auto rhiResult = ToRHIResult(result, "vkAllocateCommandBuffers");
            SOLARC_RENDER_ERROR("Failed to allocate command buffer for frame {}: {}",
                i, rhiResult.GetResultMessage());
            throw std::runtime_error("Failed to allocate command buffers");
        }

        SOLARC_RENDER_TRACE("Created command buffer for frame {}", i);
    }

    SOLARC_RENDER_DEBUG("Command buffers created");
}

void VulkanCommandContext::CreateSyncObjects()
{
    SOLARC_RENDER_DEBUG("Creating synchronization objects");

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // First frame doesn't wait

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        VkResult result = vkCreateFence(
            m_Device->GetDevice(),
            &fenceInfo,
            nullptr,
            &m_FrameResources[i].inFlightFence
        );

        if (result != VK_SUCCESS) {
            auto rhiResult = ToRHIResult(result, "vkCreateFence");
            SOLARC_RENDER_ERROR("Failed to create fence for frame {}: {}",
                i, rhiResult.GetResultMessage());
            throw std::runtime_error("Failed to create synchronization objects");
        }

        m_FrameResources[i].imageAvailableSemaphore = VK_NULL_HANDLE;

        SOLARC_RENDER_TRACE("Created sync objects for frame {}", i);
    }

    SOLARC_RENDER_DEBUG("Synchronization objects created");
}

VkCommandBuffer VulkanCommandContext::GetCommandBuffer() const
{
    return m_FrameResources[m_CurrentFrameIndex].commandBuffer;
}

void VulkanCommandContext::BeginFrame(VkSemaphore imageAvailableSemaphore)
{
    auto& frame = m_FrameResources[m_CurrentFrameIndex];

    // Store the image available semaphore for this frame (owned by swapchain)
    frame.imageAvailableSemaphore = imageAvailableSemaphore;

    // Wait for this frame's previous submission to complete
    WaitForFrame(m_CurrentFrameIndex);

    // Reset fence for this frame
    vkResetFences(m_Device->GetDevice(), 1, &frame.inFlightFence);

    // Reset and begin command buffer
    vkResetCommandBuffer(frame.commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    VkResult result = vkBeginCommandBuffer(frame.commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        auto rhiResult = ToRHIResult(result, "vkBeginCommandBuffer");
        SOLARC_RENDER_ERROR("Failed to begin command buffer: {}", rhiResult.GetResultMessage());
        throw std::runtime_error("Failed to begin command buffer");
    }

    SOLARC_RENDER_TRACE("Frame {} began", m_CurrentFrameIndex);
}

void VulkanCommandContext::BeginRenderPass(
    VkFramebuffer framebuffer,
    VkRenderPass renderPass,
    uint32_t width,
    uint32_t height,
    const float clearColor[4])
{
    SOLARC_ASSERT(clearColor != nullptr, "Clear color cannot be null");

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { width, height };

    VkClearValue clearValue = {};
    clearValue.color = { {clearColor[0], clearColor[1], clearColor[2], clearColor[3]} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(GetCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(GetCommandBuffer(), 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { width, height };
    vkCmdSetScissor(GetCommandBuffer(), 0, 1, &scissor);
}

void VulkanCommandContext::EndRenderPass()
{
    vkCmdEndRenderPass(GetCommandBuffer());
}

// Accept renderFinishedSemaphore from swapchain (indexed by image, not frame)
void VulkanCommandContext::EndFrame(VkSemaphore renderFinishedSemaphore)
{
    auto& frame = m_FrameResources[m_CurrentFrameIndex];

    VkResult result = vkEndCommandBuffer(frame.commandBuffer);
    if (result != VK_SUCCESS) {
        auto rhiResult = ToRHIResult(result, "vkEndCommandBuffer");
        SOLARC_RENDER_ERROR("Failed to end command buffer: {}", rhiResult.GetResultMessage());
        throw std::runtime_error("Failed to end command buffer");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { frame.imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame.commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore; // per-swapchain-image semaphore

    // Fence is already reset in BeginFrame; no need to reset again
    result = vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, frame.inFlightFence);

    if (result != VK_SUCCESS) {
        auto rhiResult = ToRHIResult(result, "vkQueueSubmit");
        SOLARC_RENDER_ERROR("Failed to submit command buffer: {}", rhiResult.GetResultMessage());
        throw std::runtime_error("Failed to submit command buffer");
    }

    SOLARC_RENDER_TRACE("Frame {} ended", m_CurrentFrameIndex);

    m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % FRAMES_IN_FLIGHT;
}

void VulkanCommandContext::WaitForCurrentFrame()
{
    WaitForFrame(m_CurrentFrameIndex);
}

void VulkanCommandContext::WaitForFrame(uint32_t frameIndex)
{
    SOLARC_ASSERT(frameIndex < FRAMES_IN_FLIGHT, "Invalid frame index");
    vkWaitForFences(m_Device->GetDevice(), 1, &m_FrameResources[frameIndex].inFlightFence, VK_TRUE, UINT64_MAX);
}

void VulkanCommandContext::WaitForGPU()
{
    SOLARC_RENDER_DEBUG("Waiting for GPU to complete all work...");
    vkDeviceWaitIdle(m_Device->GetDevice());
    SOLARC_RENDER_DEBUG("GPU idle");
}

#endif // SOLARC_RENDERER_VULKAN