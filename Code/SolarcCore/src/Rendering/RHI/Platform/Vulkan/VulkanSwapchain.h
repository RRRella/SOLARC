#pragma once

#ifdef SOLARC_RENDERER_VULKAN

#include "VulkanDevice.h"
#include "Rendering/RHI/RHIResult.h"
#include "Window/Window.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>


    /**
     * Vulkan Swapchain Wrapper
     *
     * Manages swapchain, surface, and image views for presenting to window.
     * Handles resize and format selection.
     */
    class VulkanSwapchain
    {
    public:
        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2; // Double-buffered
        static constexpr VkFormat PREFERRED_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
        static constexpr VkColorSpaceKHR PREFERRED_COLOR_SPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        /**
         * Create swapchain for given window
         * param device: Vulkan device
         * param window: Target window
         * throws std::runtime_error if creation fails
         */
        VulkanSwapchain(VulkanDevice* device, std::shared_ptr<Window> window);
        ~VulkanSwapchain();

        // Non-copyable, non-movable
        VulkanSwapchain(const VulkanSwapchain&) = delete;
        VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;
        VulkanSwapchain(VulkanSwapchain&&) = delete;
        VulkanSwapchain& operator=(VulkanSwapchain&&) = delete;

        /**
         * Acquire the next swapchain image
         * return RHIResult indicating success or failure
         */
        RHIResult AcquireNextImage();

        /**
         * Advance frame
         */
        void AdvanceFrame() {
            m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        /**
         * Get the current swapchain image index
         * return Index of current image (only valid after AcquireNextImage)
         */
        uint32_t GetCurrentImageIndex() const { return m_CurrentImageIndex; }

        /**
         * Get the current swapchain image
         * return VkImage handle
         */
        VkImage GetCurrentImage() const;

        /**
         * Get the current image view
         * return VkImageView handle
         */
        VkImageView GetCurrentImageView() const;

        /**
         * Get the current framebuffer
         * return VkFramebuffer handle
         */
        VkFramebuffer GetCurrentFramebuffer() const;

        /**
         * Get the render pass
         * return VkRenderPass handle
         */
        VkRenderPass GetRenderPass() const { return m_RenderPass; }

        /**
         * Get image available semaphore for current frame
         * return VkSemaphore handle
         */
        VkSemaphore GetImageAvailableSemaphore() const
        {
            return m_ImageAvailableSemaphores[m_CurrentFrame];
        }


        /**
         * Get render finished semaphore
         * return VkSemaphore handle
         */
        VkSemaphore GetRenderFinishedSemaphoreForImage() const 
        {
            return m_RenderFinishedSemaphores[m_CurrentImageIndex];
        }

        /**
         * Present the current frame
         * param renderFinishedSemaphore: Semaphore to wait on before presenting
         * param vsync: If true, use FIFO (VSync). If false, use MAILBOX or IMMEDIATE
         * return RHIResult indicating success or failure
         */
        RHIResult Present(bool vsync);

        /**
         * Resize swapchain to new dimensions
         * param width: New width (must be > 0)
         * param height: New height (must be > 0)
         * return RHIResult indicating success or failure
         */
        RHIResult Resize(uint32_t width, uint32_t height);

        /**
        * Call this before using swapchain
        */
        RHIResult Initialize();

        /**
         * Get current swapchain dimensions
         */
        uint32_t GetWidth() const { return m_SwapchainExtent.width; }
        uint32_t GetHeight() const { return m_SwapchainExtent.height; }

        /**
         * Get swapchain format
         */
        VkFormat GetFormat() const { return m_SwapchainImageFormat; }

    private:

        bool m_IsInitialized = false;


        void CreateSurface();
        void CreateSwapchain();
        void CreateImageViews();
        void CreateRenderPass();
        void CreateFramebuffers();
        void CreateSyncObjects();

        void CleanupSwapchain();

        void ReleaseBackBuffers();
        void RecreateBackBuffers();


        // Helper methods
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool vsync);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        VulkanDevice* m_Device; // Non-owning
        std::weak_ptr<Window> m_Window;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;

        std::vector<VkImage> m_SwapchainImages;
        std::vector<VkImageView> m_SwapchainImageViews;
        std::vector<VkFramebuffer> m_Framebuffers;

        VkFormat m_SwapchainImageFormat;
        VkExtent2D m_SwapchainExtent;
        VkRenderPass m_RenderPass = VK_NULL_HANDLE;

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores; // size = m_SwapchainImages.size()

        uint32_t m_CurrentFrame = 0;
        uint32_t m_CurrentImageIndex = 0;
};

#endif // SOLARC_RENDERER_VULKAN