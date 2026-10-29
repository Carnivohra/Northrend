#include "Render/Vulkan/instance.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Northrend {

VulkanInstance::VulkanInstance(const char* platformExtension) {
    const VkResult initializationResult = volkInitialize();

    if (initializationResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to initialize the Vulkan loader: " + std::to_string(initializationResult));
    }

    if (!has_extension(VK_KHR_SURFACE_EXTENSION_NAME)) {
        throw std::runtime_error("Vulkan does not support the required surface extension");
    }

    if (!has_extension(platformExtension)) {
        throw std::runtime_error(
            std::string("Vulkan does not support the required platform extension: ") + platformExtension);
    }

    std::vector<const char*> extensions {VK_KHR_SURFACE_EXTENSION_NAME, platformExtension};
    std::vector<const char*> layers;
    bool validationEnabled = false;

#if defined(NORTHREND_VULKAN_VALIDATION)
    validationEnabled = has_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) && has_layer(ValidationLayer);

    if (validationEnabled) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        layers.push_back(ValidationLayer);
    }
#endif

    const std::uint32_t apiVersion = std::min(volkGetInstanceVersion(), static_cast<std::uint32_t>(VK_API_VERSION_1_3));
    const VkApplicationInfo applicationInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Northrend",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 1),
        .pEngineName = "Northrend",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 1),
        .apiVersion = apiVersion
    };
    VkDebugUtilsMessengerCreateInfoEXT debugInfo = debug_info();
    const VkInstanceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = validationEnabled ? &debugInfo : nullptr,
        .flags = 0,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    const VkResult instanceResult = vkCreateInstance(&createInfo, nullptr, &this->instance);

    if (instanceResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan instance: " + std::to_string(instanceResult));
    }

    volkLoadInstance(this->instance);

    if (validationEnabled) {
        const VkResult debugResult =
            vkCreateDebugUtilsMessengerEXT(this->instance, &debugInfo, nullptr, &this->debugMessenger);

        if (debugResult != VK_SUCCESS) {
            vkDestroyInstance(this->instance, nullptr);
            this->instance = VK_NULL_HANDLE;
            throw std::runtime_error("Failed to create the Vulkan debug messenger: " + std::to_string(debugResult));
        }
    }
}

VulkanInstance::~VulkanInstance() {
    if (this->debugMessenger != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);
    }

    if (this->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(this->instance, nullptr);
    }
}

VkInstance VulkanInstance::handle() const noexcept {
    return this->instance;
}

bool VulkanInstance::has_extension(const char* name) {
    std::uint32_t count = 0;

    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS) {
        return false;
    }

    std::vector<VkExtensionProperties> extensions(count);

    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()) != VK_SUCCESS) {
        return false;
    }

    return std::ranges::any_of(extensions,
        [name](const VkExtensionProperties& extension) { return std::strcmp(extension.extensionName, name) == 0; });
}

bool VulkanInstance::has_layer(const char* name) {
    std::uint32_t count = 0;

    if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS) {
        return false;
    }

    std::vector<VkLayerProperties> layers(count);

    if (vkEnumerateInstanceLayerProperties(&count, layers.data()) != VK_SUCCESS) {
        return false;
    }

    return std::ranges::any_of(
        layers, [name](const VkLayerProperties& layer) { return std::strcmp(layer.layerName, name) == 0; });
}

VkDebugUtilsMessengerCreateInfoEXT VulkanInstance::debug_info() noexcept {
    return VkDebugUtilsMessengerCreateInfoEXT {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData = nullptr
    };
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void*) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "Vulkan: " << callbackData->pMessage << '\n';
    }

    return VK_FALSE;
}

}
