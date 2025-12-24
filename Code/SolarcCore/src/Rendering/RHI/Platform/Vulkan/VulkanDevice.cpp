#ifdef SOLARC_RENDERER_VULKAN

#include "VulkanDevice.h"
#include "Logging/LogMacros.h"
#include <stdexcept>
#include <set>
#include <algorithm>


    VulkanDevice::VulkanDevice()
    {
        SOLARC_RENDER_INFO("Creating Vulkan device...");

        CreateInstance();
        SetupDebugMessenger();
        SelectPhysicalDevice();
        CreateLogicalDevice();
        GetQueues();

        SOLARC_RENDER_INFO("Vulkan device created successfully");
    }

    VulkanDevice::~VulkanDevice()
    {
        SOLARC_RENDER_TRACE("Destroying Vulkan device");

        if (m_Device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(m_Device);
            vkDestroyDevice(m_Device, nullptr);
        }

#ifdef SOLARC_DEBUG
        if (m_DebugMessenger != VK_NULL_HANDLE) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                m_Instance, "vkDestroyDebugUtilsMessengerEXT"
            );
            if (func != nullptr) {
                func(m_Instance, m_DebugMessenger, nullptr);
            }
        }
#endif

        if (m_Instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_Instance, nullptr);
        }

        SOLARC_RENDER_TRACE("Vulkan device destroyed");
    }

    void VulkanDevice::CreateInstance()
    {
        SOLARC_RENDER_DEBUG("Creating Vulkan instance");

#ifdef SOLARC_DEBUG
        if (!CheckValidationLayerSupport()) {
            SOLARC_RENDER_WARN("Validation layers requested but not available");
        }
#endif

        // Application info
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Solarc Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Solarc";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3; // Target Vulkan 1.3

        // Instance create info
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Get required extensions
        auto extensions = GetRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // Enable validation layers in debug
#ifdef SOLARC_DEBUG
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

        // Debug messenger for instance creation/destruction
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = DebugCallback;

        createInfo.pNext = &debugCreateInfo;
#else
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
#endif

        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
        if (result != VK_SUCCESS) {
            auto rhiResult = ToRHIResult(result, "vkCreateInstance");
            SOLARC_RENDER_ERROR("Failed to create Vulkan instance: {}", rhiResult.GetResultMessage());
            throw std::runtime_error("Failed to create Vulkan instance");
        }

        SOLARC_RENDER_DEBUG("Vulkan instance created");
    }

    void VulkanDevice::SetupDebugMessenger()
    {
#ifdef SOLARC_DEBUG
        SOLARC_RENDER_DEBUG("Setting up Vulkan debug messenger");

        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_Instance, "vkCreateDebugUtilsMessengerEXT"
        );

        if (func != nullptr) {
            VkResult result = func(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
            if (result != VK_SUCCESS) {
                SOLARC_RENDER_WARN("Failed to set up debug messenger");
            }
            else {
                SOLARC_RENDER_DEBUG("Debug messenger created");
            }
        }
        else {
            SOLARC_RENDER_WARN("Debug messenger extension not available");
        }
#endif
    }

    void VulkanDevice::SelectPhysicalDevice()
    {
        SOLARC_RENDER_DEBUG("Selecting physical device");

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            SOLARC_RENDER_ERROR("Failed to find GPUs with Vulkan support");
            throw std::runtime_error("No Vulkan-capable GPU found");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

        // Rate each device and pick the best one
        uint32_t bestScore = 0;
        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

        for (const auto& device : devices) {
            if (IsDeviceSuitable(device)) {
                uint32_t score = RateDeviceSuitability(device);
                if (score > bestScore) {
                    bestScore = score;
                    bestDevice = device;
                }
            }
        }

        if (bestDevice == VK_NULL_HANDLE) {
            SOLARC_RENDER_ERROR("Failed to find suitable GPU");
            throw std::runtime_error("No suitable Vulkan GPU found");
        }

        m_PhysicalDevice = bestDevice;

        // Log selected device
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);

        SOLARC_RENDER_INFO("Selected GPU: {} (Vulkan {}.{}.{})",
            deviceProperties.deviceName,
            VK_VERSION_MAJOR(deviceProperties.apiVersion),
            VK_VERSION_MINOR(deviceProperties.apiVersion),
            VK_VERSION_PATCH(deviceProperties.apiVersion)
        );
    }

    void VulkanDevice::CreateLogicalDevice()
    {
        SOLARC_RENDER_DEBUG("Creating logical device");

        // Find queue families
        uint32_t graphicsFamily, presentFamily;
        if (!FindQueueFamilies(m_PhysicalDevice, graphicsFamily, presentFamily)) {
            throw std::runtime_error("Failed to find required queue families");
        }

        m_GraphicsQueueFamilyIndex = graphicsFamily;
        m_PresentQueueFamilyIndex = presentFamily;

        // Create queue create infos
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { graphicsFamily, presentFamily };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Device features (for Vulkan 1.3, we'll enable required features)
        VkPhysicalDeviceFeatures deviceFeatures = {};
        // Enable features as needed (currently none required for Phase 1)
        // Vulkan 1.3 features
        VkPhysicalDeviceVulkan13Features vulkan13Features = {};
        vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        vulkan13Features.dynamicRendering = VK_TRUE; // Enable dynamic rendering
        vulkan13Features.synchronization2 = VK_TRUE; // Enable synchronization2

        // Device create info
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &vulkan13Features;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        // Extensions
        createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

        // Validation layers (deprecated in newer Vulkan, but kept for compatibility)
#ifdef SOLARC_DEBUG
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
#else
        createInfo.enabledLayerCount = 0;
#endif

        VkResult result = vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device);
        if (result != VK_SUCCESS) {
            auto rhiResult = ToRHIResult(result, "vkCreateDevice");
            SOLARC_RENDER_ERROR("Failed to create logical device: {}", rhiResult.GetResultMessage());
            throw std::runtime_error("Failed to create Vulkan logical device");
        }

        SOLARC_RENDER_DEBUG("Logical device created");
    }

    void VulkanDevice::GetQueues()
    {
        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamilyIndex, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_PresentQueueFamilyIndex, 0, &m_PresentQueue);

        SOLARC_RENDER_DEBUG("Graphics queue family index: {}", m_GraphicsQueueFamilyIndex);
        SOLARC_RENDER_DEBUG("Present queue family index: {}", m_PresentQueueFamilyIndex);
    }

    bool VulkanDevice::CheckValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

#ifdef SOLARC_DEBUG
        for (const char* layerName : m_ValidationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }
#endif

        return true;
    }

    std::vector<const char*> VulkanDevice::GetRequiredExtensions()
    {
        std::vector<const char*> extensions;

        // Surface extensions
#ifdef _WIN32
        extensions.push_back("VK_KHR_surface");
        extensions.push_back("VK_KHR_win32_surface");
#elif __linux__
        extensions.push_back("VK_KHR_surface");
        extensions.push_back("VK_KHR_wayland_surface");
#endif

        // Debug extensions
#ifdef SOLARC_DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        return extensions;
    }

    bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device)
    {
        // Check queue families
        uint32_t graphicsFamily, presentFamily;
        if (!FindQueueFamilies(device, graphicsFamily, presentFamily)) {
            return false;
        }

        // Check device extensions
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    uint32_t VulkanDevice::RateDeviceSuitability(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        uint32_t score = 0;

        // Discrete GPUs have a significant performance advantage
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        // Maximum possible size of textures affects graphics quality
        score += deviceProperties.limits.maxImageDimension2D;

        return score;
    }

    bool VulkanDevice::FindQueueFamilies(VkPhysicalDevice device, uint32_t& graphicsFamily, uint32_t& presentFamily)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        bool graphicsFound = false;
        bool presentFound = false;

        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            // Check for graphics support
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamily = i;
                graphicsFound = true;
            }

            // For now, assume present is supported on graphics queue
            // (Will be validated properly in swapchain creation with actual surface)
            presentFamily = graphicsFamily;
            presentFound = graphicsFound;

            if (graphicsFound && presentFound) {
                break;
            }
        }

        return graphicsFound && presentFound;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDevice::DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        // Route Vulkan validation messages to our logging system
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            SOLARC_RENDER_ERROR("[Vulkan Validation] {}", pCallbackData->pMessage);
        }
        else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            SOLARC_RENDER_WARN("[Vulkan Validation] {}", pCallbackData->pMessage);
        }
        else {
            SOLARC_RENDER_DEBUG("[Vulkan Validation] {}", pCallbackData->pMessage);
        }

        return VK_FALSE; // Don't abort on validation errors
    }


#endif // SOLARC_RENDERER_VULKAN