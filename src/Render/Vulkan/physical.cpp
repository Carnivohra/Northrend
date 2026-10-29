#include "Render/Vulkan/physical.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace Northrend {

VulkanPhysicalDevice::VulkanPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
    std::uint32_t deviceCount = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (result != VK_SUCCESS || deviceCount == 0) {
        throw std::runtime_error("No Vulkan-capable graphics device was found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    result = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to enumerate Vulkan graphics devices: " + std::to_string(result));
    }

    std::uint32_t bestScore = 0;

    for (VkPhysicalDevice candidate : devices) {
        const VulkanQueueFamilies candidateFamilies = find_queue_families(candidate, surface);

        if (!candidateFamilies.complete() || !supports_swapchain_extension(candidate) ||
            !has_swapchain_support(candidate, surface)) {
            continue;
        }

        const std::uint32_t candidateScore = score(candidate);

        if (this->device == VK_NULL_HANDLE || candidateScore > bestScore) {
            this->device = candidate;
            this->families = candidateFamilies;
            bestScore = candidateScore;
        }
    }

    if (this->device == VK_NULL_HANDLE) {
        throw std::runtime_error("No Vulkan device supports graphics and presentation");
    }
}

VkPhysicalDevice VulkanPhysicalDevice::handle() const noexcept {
    return this->device;
}

const VulkanQueueFamilies& VulkanPhysicalDevice::queue_families() const noexcept {
    return this->families;
}

std::uint32_t VulkanPhysicalDevice::memory_type(std::uint32_t typeBits, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(this->device, &memoryProperties);

    for (std::uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        const bool supported = typeBits & (1U << index);
        const bool matches = (memoryProperties.memoryTypes[index].propertyFlags & properties) == properties;

        if (supported && matches) {
            return index;
        }
    }

    throw std::runtime_error("No compatible Vulkan memory type was found");
}

VkFormat VulkanPhysicalDevice::depth_format() const {
    constexpr std::array<VkFormat, 3> candidates {
        VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (VkFormat format : candidates) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(this->device, format, &properties);

        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }

    throw std::runtime_error("No supported Vulkan depth format was found");
}

VulkanQueueFamilies VulkanPhysicalDevice::find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());

    VulkanQueueFamilies families;

    for (std::uint32_t index = 0; index < count; ++index) {
        VkBool32 supportsPresent = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &supportsPresent);

        if ((properties[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supportsPresent) {
            families.graphics = index;
            families.present = index;
            return families;
        }

        if (!families.graphics && (properties[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            families.graphics = index;
        }

        if (!families.present && supportsPresent) {
            families.present = index;
        }
    }

    return families;
}

bool VulkanPhysicalDevice::supports_swapchain_extension(VkPhysicalDevice device) {
    std::uint32_t count = 0;

    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS) {
        return false;
    }

    std::vector<VkExtensionProperties> extensions(count);

    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()) != VK_SUCCESS) {
        return false;
    }

    return std::ranges::any_of(extensions, [](const VkExtensionProperties& extension) {
        return std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
    });
}

bool VulkanPhysicalDevice::has_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
    std::uint32_t formatCount = 0;
    std::uint32_t presentModeCount = 0;

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    return formatCount > 0 && presentModeCount > 0;
}

std::uint32_t VulkanPhysicalDevice::score(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    std::uint32_t deviceScore = properties.limits.maxImageDimension2D;

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        deviceScore += 100000;
    }

    return deviceScore;
}

}
