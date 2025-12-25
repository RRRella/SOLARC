#ifdef SOLARC_RENDERER_VULKAN

#include "VulkanSwapchain.h"
#include "Logging/LogMacros.h"
#include <stdexcept>
#include <algorithm>
#include <array>

#ifdef WIN32
#include <vulkan/vulkan_win32.h>
#elif __linux__
#include <vulkan/vulkan_wayland.h>
#endif

    VulkanSwapchain::VulkanSwapchain(VulkanDevice* device, std::shared_ptr<Window> window)
        : m_Device(device)
        , m_Window(window)
    {
        SOLARC_ASSERT(device != nullptr, "Device cannot be null");
        SOLARC_ASSERT(window != nullptr, "Window cannot be null");
        m_SwapchainExtent.width = static_cast<uint32_t>(window->GetWidth());
        m_SwapchainExtent.height = static_cast<uint32_t>(window->GetHeight());
        SOLARC_RENDER_DEBUG("Vulkan swapchain wrapper created (deferred init)");
    }

    RHIResult VulkanSwapchain::Initialize()
    {
        if (m_IsInitialized) {
            return RHIResult(RHIStatus::SUCCESS);
        }

        auto window = m_Window.lock();
        if (!window || !window->IsVisible() || window->IsMinimized() ||
            window->GetWidth() <= 0 || window->GetHeight() <= 0) {
            return RHIResult(RHIStatus::SWAPCHAIN_OUT_OF_DATE, "Window not ready for rendering");
        }

        try {
            SOLARC_RENDER_INFO("Creating Vulkan swapchain ({}x{})",
                m_SwapchainExtent.width, m_SwapchainExtent.height);
            CreateSurface();
            CreateSwapchain();
            CreateImageViews();
            CreateRenderPass();
            CreateFramebuffers();
            CreateSyncObjects();
            m_IsInitialized = true;
            SOLARC_RENDER_INFO("Vulkan swapchain initialized successfully");
            return RHIResult(RHIStatus::SUCCESS);
        }
        catch (const std::exception& e) {
            SOLARC_RENDER_ERROR("Swapchain initialization failed: {}", e.what());
            return RHIResult(RHIStatus::INITIALIZATION_FAILED, e.what());
        }
    }

    VulkanSwapchain::~VulkanSwapchain()
    {
        SOLARC_RENDER_TRACE("Destroying Vulkan swapchain");

        VkDevice device = m_Device->GetDevice();

        // Wait for device to be idle
        vkDeviceWaitIdle(device);

        // Destroy sync objects
        for (auto semaphore : m_ImageAvailableSemaphores) {
            vkDestroySemaphore(device, semaphore, nullptr);
        }

        for (auto s : m_RenderFinishedSemaphores)
            vkDestroySemaphore(device, s, nullptr);

        CleanupSwapchain();

        if (m_Surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_Device->GetInstance(), m_Surface, nullptr);
        }

        SOLARC_RENDER_TRACE("Vulkan swapchain destroyed");
    }

    void VulkanSwapchain::CreateSurface()
    {
        SOLARC_RENDER_DEBUG("Creating Vulkan surface");

        auto window = m_Window.lock();

        SOLARC_ASSERT(!window->IsMinimized() || window->IsVisible() , "Creating Vulkan swapchain with hidden/minimized window – may cause failures.");

        if (!window) {
            throw std::runtime_error("Window expired during swapchain creation");
        }

#ifdef WIN32
        VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = static_cast<HWND>(window->GetPlatform()->GetWin32Handle());
        createInfo.hinstance = GetModuleHandle(nullptr);

        VkResult result = vkCreateWin32SurfaceKHR(
            m_Device->GetInstance(),
            &createInfo,
            nullptr,
            &m_Surface
        );

#elif __linux__
        VkWaylandSurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        createInfo.display = window->GetPlatform()->GetWaylandDisplay();
        createInfo.surface = window->GetPlatform()->GetWaylandSurface();

        VkResult result = vkCreateWaylandSurfaceKHR(
            m_Device->GetInstance(),
            &createInfo,
            nullptr,
            &m_Surface
        );
#endif

        if (result != VK_SUCCESS) {
            auto rhiResult = ToRHIResult(result, "vkCreateSurfaceKHR");
            SOLARC_RENDER_ERROR("Failed to create Vulkan surface: {}", rhiResult.GetResultMessage());
            throw std::runtime_error("Failed to create Vulkan surface");
        }

        // Verify that the physical device supports presentation on this surface
        VkBool32 supported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            m_Device->GetPhysicalDevice(),
            m_Device->GetPresentQueueFamilyIndex(),
            m_Surface,
            &supported
        );

        if (!supported) {
            throw std::runtime_error("Physical device does not support presentation on this surface");
        }

        SOLARC_RENDER_DEBUG("Vulkan surface created");
    }

    void VulkanSwapchain::CreateSwapchain()
    {
        SOLARC_RENDER_DEBUG("Creating Vulkan swapchain");

        VkPhysicalDevice physicalDevice = m_Device->GetPhysicalDevice();

        // Query surface capabilities
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &capabilities);
        // Query surface formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, formats.data());

        // Query present modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, presentModes.data());

        // Choose settings
        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(presentModes, true); // Default to VSync
        VkExtent2D extent = ChooseSwapExtent(capabilities);

        m_SwapchainImageFormat = surfaceFormat.format;
        m_SwapchainExtent = extent;

        // Image count (prefer triple buffering if available)
        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        SOLARC_RENDER_DEBUG("Swapchain image count: {}", imageCount);

        // Create swapchain
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_Surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = {
            m_Device->GetGraphicsQueueFamilyIndex(),
            m_Device->GetPresentQueueFamilyIndex()
        };

        if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VkResult result = vkCreateSwapchainKHR(m_Device->GetDevice(), &createInfo, nullptr, &m_Swapchain);
        if (result != VK_SUCCESS) {
            auto rhiResult = ToRHIResult(result, "vkCreateSwapchainKHR");
            SOLARC_RENDER_ERROR("Failed to create swapchain: {}", rhiResult.GetResultMessage());
            throw std::runtime_error("Failed to create Vulkan swapchain");
        }

        // Retrieve swapchain images
        vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &imageCount, nullptr);
        m_SwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &imageCount, m_SwapchainImages.data());

        SOLARC_RENDER_DEBUG("Vulkan swapchain created with {} images", imageCount);
    }

    void VulkanSwapchain::CreateImageViews()
    {
        SOLARC_RENDER_DEBUG("Creating swapchain image views");

        m_SwapchainImageViews.resize(m_SwapchainImages.size());

        for (size_t i = 0; i < m_SwapchainImages.size(); ++i) {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = m_SwapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_SwapchainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkResult result = vkCreateImageView(m_Device->GetDevice(), &createInfo, nullptr, &m_SwapchainImageViews[i]);
            if (result != VK_SUCCESS) {
                auto rhiResult = ToRHIResult(result, "vkCreateImageView");
                SOLARC_RENDER_ERROR("Failed to create image view {}: {}", i, rhiResult.GetResultMessage());
                throw std::runtime_error("Failed to create swapchain image views");
            }
        }

        SOLARC_RENDER_DEBUG("Created {} image views", m_SwapchainImageViews.size());
    }

    void VulkanSwapchain::CreateRenderPass()
    {
        SOLARC_RENDER_DEBUG("Creating render pass");

        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = m_SwapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear on load
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        // Subpass dependency for layout transitions
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VkResult result = vkCreateRenderPass(m_Device->GetDevice(), &renderPassInfo, nullptr, &m_RenderPass);
        if (result != VK_SUCCESS) {
            auto rhiResult = ToRHIResult(result, "vkCreateRenderPass");
            SOLARC_RENDER_ERROR("Failed to create render pass: {}", rhiResult.GetResultMessage());
            throw std::runtime_error("Failed to create Vulkan render pass");
        }

        SOLARC_RENDER_DEBUG("Render pass created");
    }

    void VulkanSwapchain::CreateFramebuffers()
    {
        SOLARC_RENDER_DEBUG("Creating framebuffers");

        m_Framebuffers.resize(m_SwapchainImageViews.size());

        for (size_t i = 0; i < m_SwapchainImageViews.size(); ++i) {
            VkImageView attachments[] = { m_SwapchainImageViews[i] };

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_RenderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_SwapchainExtent.width;
            framebufferInfo.height = m_SwapchainExtent.height;
            framebufferInfo.layers = 1;
            VkResult result = vkCreateFramebuffer(m_Device->GetDevice(), &framebufferInfo, nullptr, &m_Framebuffers[i]);
            if (result != VK_SUCCESS) {
                auto rhiResult = ToRHIResult(result, "vkCreateFramebuffer");
                SOLARC_RENDER_ERROR("Failed to create framebuffer {}: {}", i, rhiResult.GetResultMessage());
                throw std::runtime_error("Failed to create framebuffers");
            }
        }

        SOLARC_RENDER_DEBUG("Created {} framebuffers", m_Framebuffers.size());
    }

    void VulkanSwapchain::CreateSyncObjects()
    {
        SOLARC_RENDER_DEBUG("Creating synchronization objects");

        // Create image available semaphores (per frame-in-flight)
        m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            VkResult result = vkCreateSemaphore(
                m_Device->GetDevice(), &semaphoreInfo, nullptr,
                &m_ImageAvailableSemaphores[i]
            );
            if (result != VK_SUCCESS) {
                auto rhiResult = ToRHIResult(result, "vkCreateSemaphore (image available)");
                SOLARC_RENDER_ERROR("Failed to create image available semaphore: {}", rhiResult.GetResultMessage());
                throw std::runtime_error("Failed to create synchronization objects");
            }
        }

        // Create render finished semaphores (PER SWAPCHAIN IMAGE, not per frame!)
        m_RenderFinishedSemaphores.resize(m_SwapchainImages.size());
        for (size_t i = 0; i < m_RenderFinishedSemaphores.size(); ++i) {
            VkResult result = vkCreateSemaphore(
                m_Device->GetDevice(), &semaphoreInfo, nullptr,
                &m_RenderFinishedSemaphores[i]
            );
            if (result != VK_SUCCESS) {
                auto rhiResult = ToRHIResult(result, "vkCreateSemaphore (render finished)");
                SOLARC_RENDER_ERROR("Failed to create render finished semaphore: {}", rhiResult.GetResultMessage());
                throw std::runtime_error("Failed to create synchronization objects");
            }
        }

        SOLARC_RENDER_DEBUG("Synchronization objects created");
    }

    void VulkanSwapchain::CleanupSwapchain()
    {
        VkDevice device = m_Device->GetDevice();

        for (auto framebuffer : m_Framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        m_Framebuffers.clear();

        if (m_RenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }

        for (auto imageView : m_SwapchainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        m_SwapchainImageViews.clear();

        if (m_Swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
        }

        m_SwapchainImages.clear();
    }

    RHIResult VulkanSwapchain::AcquireNextImage()
    {
        VkSemaphore imageAvailableSemaphore = m_ImageAvailableSemaphores[m_CurrentFrame];

        VkResult result = vkAcquireNextImageKHR(
            m_Device->GetDevice(),
            m_Swapchain,
            UINT64_MAX, // Timeout
            imageAvailableSemaphore,
            VK_NULL_HANDLE,
            &m_CurrentImageIndex
        );

        // Treat SURFACE_LOST as OUT_OF_DATE (requires recreate)
        if (result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_SUBOPTIMAL_KHR ||
            result == VK_ERROR_SURFACE_LOST_KHR) {
            return ToRHIResult(result, "vkAcquireNextImageKHR");
        }
        else if (result != VK_SUCCESS) {
            auto rhiResult = ToRHIResult(result, "vkAcquireNextImageKHR");
            SOLARC_RENDER_ERROR("Failed to acquire swapchain image: {}", rhiResult.GetResultMessage());
            return rhiResult;
        }
        return RHIResult(RHIStatus::SUCCESS);
    }

    RHIResult VulkanSwapchain::Present(bool vsync)
    {
        VkSemaphore renderFinishedSemaphore = m_RenderFinishedSemaphores[m_CurrentImageIndex];

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphore; // indexed by image
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &m_CurrentImageIndex;

        VkResult result = vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            return ToRHIResult(result, "vkQueuePresentKHR");
        else if (result != VK_SUCCESS) {
            auto rhiResult = ToRHIResult(result, "vkQueuePresentKHR");
            SOLARC_RENDER_ERROR("Failed to present swapchain image: {}", rhiResult.GetResultMessage());
            return rhiResult;
        }

    #ifdef __linux__
        auto window = m_Window.lock();
        if (window) {
            wl_surface* surface = window->GetPlatform()->GetWaylandSurface();

            // Request next frame callback
            m_FrameCallback = wl_surface_frame(surface);
            wl_callback_add_listener(m_FrameCallback, &s_FrameListener, this);

            // Commit to apply buffer
            wl_surface_commit(surface);
            wl_display_flush(WindowContextPlatform::Get().GetDisplay());
        }
    #endif

        return RHIResult(RHIStatus::SUCCESS);
    }
#ifdef __linux
    const wl_callback_listener VulkanSwapchain::s_FrameListener = {
        .done = frame_done_callback
    };

    void VulkanSwapchain::frame_done_callback(void* data, wl_callback* cb, uint32_t time) {
        auto* swapchain = static_cast<VulkanSwapchain*>(data);
        wl_callback_destroy(cb);
        swapchain->m_FrameCallback = nullptr;
        swapchain->m_WaitingForFrame = false;
    }
#endif

    RHIResult VulkanSwapchain::Resize(uint32_t width, uint32_t height)
    {
        SOLARC_ASSERT(width > 0 && height > 0, "Invalid swapchain dimensions");

        if (width == m_SwapchainExtent.width && height == m_SwapchainExtent.height)
            return RHIResult(RHIStatus::SUCCESS);

        SOLARC_RENDER_INFO("Resizing swapchain: {}x{} -> {}x{}",
            m_SwapchainExtent.width, m_SwapchainExtent.height, width, height);

        m_SwapchainExtent = { width, height };

        if (m_IsInitialized) {
            ReleaseBackBuffers();
            RecreateBackBuffers();
        }

        return RHIResult(RHIStatus::SUCCESS);
    }

    void VulkanSwapchain::ReleaseBackBuffers()
    {
        SOLARC_RENDER_TRACE("Releasing swapchain back buffers");
        vkDeviceWaitIdle(m_Device->GetDevice());
        CleanupSwapchain();
    }

    void VulkanSwapchain::RecreateBackBuffers()
    {
        SOLARC_RENDER_TRACE("Recreating swapchain back buffers");

        CreateSwapchain();
        CreateImageViews();
        CreateRenderPass();
        CreateFramebuffers();

        SOLARC_RENDER_DEBUG("Swapchain back buffers recreated");
    }

    VkImage VulkanSwapchain::GetCurrentImage() const
    {
        return m_SwapchainImages[m_CurrentImageIndex];
    }

    VkImageView VulkanSwapchain::GetCurrentImageView() const
    {
        return m_SwapchainImageViews[m_CurrentImageIndex];
    }

    VkFramebuffer VulkanSwapchain::GetCurrentFramebuffer() const
    {
        return m_Framebuffers[m_CurrentImageIndex];
    }

    VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        // Prefer BGRA8_UNORM with SRGB color space
        for (const auto& format : availableFormats) {
            if (format.format == PREFERRED_FORMAT && format.colorSpace == PREFERRED_COLOR_SPACE) {
                return format;
            }
        }

        // Fallback to first available
        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool vsync)
    {
        if (vsync) {
            // VSync: Use FIFO (guaranteed to be available)
            return VK_PRESENT_MODE_FIFO_KHR;
        }
        else {
            // No VSync: Prefer MAILBOX, fallback to IMMEDIATE, then FIFO
            for (const auto& mode : availablePresentModes) {
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return mode;
                }
            }

            for (const auto& mode : availablePresentModes) {
                if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                    return mode;
                }
            }

            return VK_PRESENT_MODE_FIFO_KHR; // Fallback
        }
    }

    VkExtent2D VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }
        else {
            VkExtent2D actualExtent = m_SwapchainExtent;

            actualExtent.width = std::clamp(actualExtent.width,
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height,
                capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

#endif // SOLARC_RENDERER_VULKAN