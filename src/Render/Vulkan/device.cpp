#include "Render/Vulkan/device.hpp"

#include "Render/Vulkan/transfer.hpp"

#include <cstdint>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace Northrend {

VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface) : physicalDevice(instance, surface) {
    const VulkanQueueFamilies& families = this->physicalDevice.queue_families();
    const std::uint32_t graphicsFamily = *families.graphics;
    const std::uint32_t presentFamily = *families.present;
    const std::uint32_t transferFamily = *families.transfer;
    const float queuePriority = 1.0F;
    const std::set<std::uint32_t> uniqueFamilies {graphicsFamily, presentFamily, transferFamily};
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    queueInfos.reserve(uniqueFamilies.size());

    for (std::uint32_t family : uniqueFamilies) {
        queueInfos.push_back(VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = family,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        });
    }

    VkPhysicalDeviceDescriptorIndexingFeatures supportedIndexing {};
    supportedIndexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    VkPhysicalDeviceFeatures2 supportedFeatures {};
    supportedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    supportedFeatures.pNext = &supportedIndexing;
    vkGetPhysicalDeviceFeatures2(this->physicalDevice.handle(), &supportedFeatures);

    if (supportedIndexing.shaderSampledImageArrayNonUniformIndexing != VK_TRUE) {
        throw std::runtime_error("Vulkan device does not support non-uniform texture indexing");
    }

    VkPhysicalDeviceDescriptorIndexingFeatures enabledIndexing {};
    enabledIndexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    enabledIndexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    const char* extensions[] {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkPhysicalDeviceFeatures features {};
    features.multiDrawIndirect = supportedFeatures.features.multiDrawIndirect;
    const VkDeviceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabledIndexing,
        .flags = 0,
        .queueCreateInfoCount = static_cast<std::uint32_t>(queueInfos.size()),
        .pQueueCreateInfos = queueInfos.data(),
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = extensions,
        .pEnabledFeatures = &features
    };
    const VkResult result = vkCreateDevice(this->physicalDevice.handle(), &createInfo, nullptr, &this->device);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan device: " + std::to_string(result));
    }

    volkLoadDevice(this->device);
    vkGetDeviceQueue(this->device, graphicsFamily, 0, &this->graphicsQueue);
    vkGetDeviceQueue(this->device, presentFamily, 0, &this->presentQueue);
    vkGetDeviceQueue(this->device, transferFamily, 0, &this->transferQueue);
    this->supportsMultiDrawIndirect = features.multiDrawIndirect == VK_TRUE;
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

VkQueue VulkanDevice::transfer_queue() const noexcept {
    return this->transferQueue;
}

std::uint32_t VulkanDevice::graphics_family() const noexcept {
    return *this->physicalDevice.queue_families().graphics;
}

std::uint32_t VulkanDevice::present_family() const noexcept {
    return *this->physicalDevice.queue_families().present;
}

std::uint32_t VulkanDevice::transfer_family() const noexcept {
    return *this->physicalDevice.queue_families().transfer;
}

bool VulkanDevice::dedicated_transfer_queue() const noexcept {
    return this->transfer_family() != this->graphics_family();
}

bool VulkanDevice::multi_draw_indirect() const noexcept {
    return this->supportsMultiDrawIndirect;
}

VkFormat VulkanDevice::depth_format() const {
    return this->physicalDevice.depth_format();
}

std::uint32_t VulkanDevice::memory_type(std::uint32_t typeBits, VkMemoryPropertyFlags properties) const {
    return this->physicalDevice.memory_type(typeBits, properties);
}

void VulkanDevice::prepare_transfers(const std::size_t batchCount) {
    if (!this->transfer) {
        this->transfer = std::make_unique<VulkanTransferContext>(*this, batchCount);
    }
}

void VulkanDevice::upload_buffer(const VkBuffer destination, const VkDeviceSize destinationOffset,
    const VkDeviceSize size, const std::span<const VulkanTransferSegment> segments) {
    if (!this->transfer) {
        throw std::logic_error("Vulkan transfers have not been prepared");
    }

    this->transfer->queue_buffer(destination, destinationOffset, size, segments);
}

void VulkanDevice::upload_texture(const VkImage destination, const std::span<const VkBufferImageCopy> regions,
    const std::uint32_t mipLevels, const std::uint32_t layers, const VkDeviceSize size,
    const std::span<const VulkanTransferSegment> segments) {
    if (!this->transfer) {
        throw std::logic_error("Vulkan transfers have not been prepared");
    }

    this->transfer->queue_texture(destination, regions, mipLevels, layers, size, segments);
}

VkCommandBuffer VulkanDevice::flush_transfers(const std::size_t batch, const std::uint64_t submissionSerial) {
    return this->transfer ? this->transfer->flush(batch, submissionSerial) : VK_NULL_HANDLE;
}

void VulkanDevice::collect_transfers(const std::uint64_t completedSerial) noexcept {
    if (this->transfer) {
        this->transfer->collect(completedSerial);
    }
}

bool VulkanDevice::transfers_pending() const noexcept {
    return this->transfer && this->transfer->pending();
}

void VulkanDevice::wait_idle() const noexcept {
    if (this->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(this->device);
    }
}

}
