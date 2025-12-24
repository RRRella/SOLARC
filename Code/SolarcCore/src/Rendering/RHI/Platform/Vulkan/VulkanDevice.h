#pragma once

#ifdef SOLARC_RENDERER_VULKAN

#include "Rendering/RHI/RHIResult.h"
#include "Window/Window.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>

/**
 * Vulkan Device Wrapper
 *
 * Manages Vulkan instance, physical device, and logical device creation.
 * Enables validation layers in debug builds.
 * Prefers discrete GPU over integrated.
 */
class VulkanDevice
{
public:
    /**
     * Create Vulkan device
     * throws std::runtime_error if device creation fails
     */
    VulkanDevice();
    ~VulkanDevice();

    // Non-copyable, non-movable
    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;
    VulkanDevice(VulkanDevice&&) = delete;
    VulkanDevice& operator=(VulkanDevice&&) = delete;

    /**
     * Get the Vulkan instance
     * return VkInstance handle
     */
    VkInstance GetInstance() const { return m_Instance; }

    /**
     * Get the physical device (GPU)
     * return VkPhysicalDevice handle
     */
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

    /**
     * Get the logical device
     * return VkDevice handle
     */
    VkDevice GetDevice() const { return m_Device; }

    /**
     * Get the graphics queue family index
     * return Queue family index for graphics operations
     */
    uint32_t GetGraphicsQueueFamilyIndex() const { return m_GraphicsQueueFamilyIndex; }

    /**
     * Get the graphics queue
     * return VkQueue handle for graphics operations
     */
    VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }

    /**
     * Get the present queue family index
     * return Queue family index for presentation (may be same as graphics)
     */
    uint32_t GetPresentQueueFamilyIndex() const { return m_PresentQueueFamilyIndex; }

    /**
     * Get the present queue
     * return VkQueue handle for presentation
     */
    VkQueue GetPresentQueue() const { return m_PresentQueue; }

private:
    void CreateInstance();
    void SetupDebugMessenger();
    void SelectPhysicalDevice();
    void CreateLogicalDevice();
    void GetQueues();

    // Helper methods
    bool CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions();
    bool IsDeviceSuitable(VkPhysicalDevice device);
    uint32_t RateDeviceSuitability(VkPhysicalDevice device);
    bool FindQueueFamilies(VkPhysicalDevice device, uint32_t& graphicsFamily, uint32_t& presentFamily);

    // Debug callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;

    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;

    uint32_t m_GraphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t m_PresentQueueFamilyIndex = UINT32_MAX;

#ifdef SOLARC_DEBUG_BUILD
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

    // Validation layers
    const std::vector<const char*> m_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
#endif

    // Required device extensions
    const std::vector<const char*> m_DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};

#endif // SOLARC_RENDERER_VULKAN