#include "Render/Vulkan/device.hpp"

#include "Render/Vulkan/transfer.hpp"

#include <cstdint>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace Northrend {

VulkanDevice::VulkanDevice(
    VkInstance instance,
    VkSurfaceKHR surface)
    : physicalDevice(instance, surface) {
    const VulkanQueueFamilies& families =
        this->physicalDevice.queue_families();
    const std::uint32_t graphicsFamily = *families.graphics;
    const std::uint32_t presentFamily = *families.present;
    const float queuePriority = 1.0F;
    const std::set<std::uint32_t> uniqueFamilies{
        graphicsFamily,
        presentFamily
    };
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    queueInfos.reserve(uniqueFamilies.size());

    for (std::uint32_t family : uniqueFamilies) {
        queueInfos.push_back(VkDeviceQueueCreateInfo{
            .sType =
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = family,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        });
    }

    const char* extensions[]{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    const VkPhysicalDeviceFeatures features{};
    const VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount =
            static_cast<std::uint32_t>(queueInfos.size()),
        .pQueueCreateInfos = queueInfos.data(),
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = extensions,
        .pEnabledFeatures = &features
    };
    const VkResult result = vkCreateDevice(
        this->physicalDevice.handle(),
        &createInfo,
        nullptr,
        &this->device);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create the Vulkan device: " +
            std::to_string(result));
    }

    volkLoadDevice(this->device);
    vkGetDeviceQueue(
        this->device,
        graphicsFamily,
        0,
        &this->graphicsQueue);
    vkGetDeviceQueue(
        this->device,
        presentFamily,
        0,
        &this->presentQueue);
}

VulkanDevice::~VulkanDevice() {
    this->transfer.reset();

    if (this->device != VK_NULL_HANDLE) {
        vkDestroyDevice(this->device, nullptr);
    }
}

VkPhysicalDevice VulkanDevice::physical() const noexcept {
    return this->physicalDevice.handle();
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

std::uint32_t
VulkanDevice::graphics_family() const noexcept {
    return *this->physicalDevice.queue_families().graphics;
}

std::uint32_t
VulkanDevice::present_family() const noexcept {
    return *this->physicalDevice.queue_families().present;
}

VkFormat VulkanDevice::depth_format() const {
    return this->physicalDevice.depth_format();
}

std::uint32_t VulkanDevice::memory_type(
    std::uint32_t typeBits,
    VkMemoryPropertyFlags properties) const {
    return this->physicalDevice.memory_type(
        typeBits,
        properties);
}

void VulkanDevice::copy_buffer(
    VkBuffer source,
    VkBuffer destination,
    VkDeviceSize size) {
    if (!this->transfer) {
        this->transfer =
            std::make_unique<VulkanTransferContext>(*this);
    }

    this->transfer->copy_buffer(
        source,
        destination,
        size);
}

void VulkanDevice::wait_idle() const noexcept {
    if (this->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(this->device);
    }
}

}
