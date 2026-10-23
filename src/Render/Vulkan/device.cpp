#include "Render/Vulkan/device.hpp"

#include <algorithm>
#include <cstring>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace Northrend::VulkanDeviceSelection {

constexpr const char* SwapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

struct QueueFamilies {
    std::optional<std::uint32_t> graphics;
    std::optional<std::uint32_t> present;

    bool complete() const noexcept {
        return this->graphics.has_value() && this->present.has_value();
    }
};

QueueFamilies find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());

    QueueFamilies families;

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

bool supports_swapchain_extension(VkPhysicalDevice device) {
    std::uint32_t count = 0;

    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS) {
        return false;
    }

    std::vector<VkExtensionProperties> extensions(count);

    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()) != VK_SUCCESS) {
        return false;
    }

    return std::ranges::any_of(extensions, [](const VkExtensionProperties& extension) {
        return std::strcmp(extension.extensionName, SwapchainExtension) == 0;
    });
}

bool has_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
    std::uint32_t formatCount = 0;
    std::uint32_t presentModeCount = 0;

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    return formatCount > 0 && presentModeCount > 0;
}

std::uint32_t score_device(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    std::uint32_t score = properties.limits.maxImageDimension2D;

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 100000;
    }

    return score;
}

}

namespace Northrend {

VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface) {
    std::uint32_t deviceCount = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (result != VK_SUCCESS || deviceCount == 0) {
        throw std::runtime_error("No Vulkan-capable graphics device was found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    result = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to enumerate Vulkan graphics devices: " + std::to_string(result));
    }

    std::uint32_t bestScore = 0;
    VulkanDeviceSelection::QueueFamilies selectedFamilies;

    for (VkPhysicalDevice candidate : devices) {
        const auto families = VulkanDeviceSelection::find_queue_families(candidate, surface);

        if (!families.complete() ||
            !VulkanDeviceSelection::supports_swapchain_extension(candidate) ||
            !VulkanDeviceSelection::has_swapchain_support(candidate, surface)) {
            continue;
        }

        const std::uint32_t score = VulkanDeviceSelection::score_device(candidate);

        if (this->physicalDevice == VK_NULL_HANDLE || score > bestScore) {
            this->physicalDevice = candidate;
            selectedFamilies = families;
            bestScore = score;
        }
    }

    if (this->physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("No Vulkan device supports graphics and presentation");
    }

    this->graphicsFamily = *selectedFamilies.graphics;
    this->presentFamily = *selectedFamilies.present;

    const float queuePriority = 1.0F;
    const std::set<std::uint32_t> uniqueFamilies{
        this->graphicsFamily,
        this->presentFamily
    };
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    queueInfos.reserve(uniqueFamilies.size());

    for (std::uint32_t family : uniqueFamilies) {
        queueInfos.push_back(VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = family,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        });
    }

    const char* extensions[] = {
        VulkanDeviceSelection::SwapchainExtension
    };
    const VkPhysicalDeviceFeatures features{};
    const VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = static_cast<std::uint32_t>(queueInfos.size()),
        .pQueueCreateInfos = queueInfos.data(),
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = extensions,
        .pEnabledFeatures = &features
    };

    result = vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create the Vulkan device: " + std::to_string(result));
    }

    volkLoadDevice(this->device);
    vkGetDeviceQueue(this->device, this->graphicsFamily, 0, &this->graphicsQueue);
    vkGetDeviceQueue(this->device, this->presentFamily, 0, &this->presentQueue);
}

VulkanDevice::~VulkanDevice() {
    if (this->device != VK_NULL_HANDLE) {
        vkDestroyDevice(this->device, nullptr);
    }
}

VkPhysicalDevice VulkanDevice::physical() const noexcept {
    return this->physicalDevice;
}

VkDevice VulkanDevice::handle() const noexcept {
    return this->device;
}

VkQueue VulkanDevice::graphics_queue() const noexcept {
    return this->graphicsQueue;
}

VkQueue VulkanDevice::present_queue() const noexcept {
    return this->presentQueue;
}

std::uint32_t VulkanDevice::graphics_family() const noexcept {
    return this->graphicsFamily;
}

std::uint32_t VulkanDevice::present_family() const noexcept {
    return this->presentFamily;
}

std::uint32_t VulkanDevice::memory_type(
    std::uint32_t typeBits,
    VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &memoryProperties);

    for (std::uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        const bool supported = typeBits & (1U << index);
        const bool matches =
            (memoryProperties.memoryTypes[index].propertyFlags & properties) == properties;

        if (supported && matches) {
            return index;
        }
    }

    throw std::runtime_error("No compatible Vulkan memory type was found");
}

void VulkanDevice::wait_idle() const noexcept {
    if (this->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(this->device);
    }
}

}
