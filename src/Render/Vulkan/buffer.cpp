#include "Render/Vulkan/buffer.hpp"

#include "Render/Vulkan/device.hpp"

#include <cstring>
#include <stdexcept>
#include <string>

namespace Northrend {

VulkanBuffer::VulkanBuffer(
    VulkanDevice& device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties)
    : device(device), byteSize(size) {
    if (this->byteSize == 0) {
        throw std::invalid_argument("Vulkan buffer size must be positive");
    }

    const VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = this->byteSize,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };
    VkResult result = vkCreateBuffer(
        this->device.handle(),
        &bufferInfo,
        nullptr,
        &this->buffer);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create Vulkan buffer: " + std::to_string(result));
    }

    try {
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(
            this->device.handle(),
            this->buffer,
            &requirements);

        const VkMemoryAllocateInfo allocationInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = requirements.size,
            .memoryTypeIndex = this->device.memory_type(
                requirements.memoryTypeBits,
                properties)
        };
        result = vkAllocateMemory(
            this->device.handle(),
            &allocationInfo,
            nullptr,
            &this->memory);

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to allocate Vulkan buffer memory: " +
                std::to_string(result));
        }

        result = vkBindBufferMemory(
            this->device.handle(),
            this->buffer,
            this->memory,
            0);

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to bind Vulkan buffer memory: " +
                std::to_string(result));
        }
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanBuffer::~VulkanBuffer() {
    this->destroy();
}

std::unique_ptr<VulkanBuffer> VulkanBuffer::upload(
    VulkanDevice& device,
    std::span<const std::byte> data,
    VkBufferUsageFlags usage) {
    if (data.empty()) {
        throw std::invalid_argument("Vulkan upload data must not be empty");
    }

    VulkanBuffer staging(
        device,
        data.size_bytes(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    staging.write(data);

    auto destination = std::make_unique<VulkanBuffer>(
        device,
        data.size_bytes(),
        usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    device.copy_buffer(
        staging.handle(),
        destination->handle(),
        data.size_bytes());
    return destination;
}

VkBuffer VulkanBuffer::handle() const noexcept {
    return this->buffer;
}

VkDeviceSize VulkanBuffer::size() const noexcept {
    return this->byteSize;
}

void* VulkanBuffer::map() {
    if (this->mapped != nullptr) {
        return this->mapped;
    }

    const VkResult result = vkMapMemory(
        this->device.handle(),
        this->memory,
        0,
        this->byteSize,
        0,
        &this->mapped);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to map Vulkan buffer memory: " +
            std::to_string(result));
    }

    return this->mapped;
}

void VulkanBuffer::write(std::span<const std::byte> data) {
    if (data.size_bytes() > this->byteSize) {
        throw std::out_of_range("Vulkan buffer upload exceeds its capacity");
    }

    std::memcpy(this->map(), data.data(), data.size_bytes());
}

void VulkanBuffer::destroy() noexcept {
    if (this->mapped != nullptr) {
        vkUnmapMemory(this->device.handle(), this->memory);
        this->mapped = nullptr;
    }

    if (this->buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(this->device.handle(), this->buffer, nullptr);
        this->buffer = VK_NULL_HANDLE;
    }

    if (this->memory != VK_NULL_HANDLE) {
        vkFreeMemory(this->device.handle(), this->memory, nullptr);
        this->memory = VK_NULL_HANDLE;
    }
}

}
