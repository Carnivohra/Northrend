#include "Render/Vulkan/buffer.hpp"

#include "Render/Vulkan/device.hpp"

#include <array>
#include <cstring>
#include <stdexcept>
#include <string>

namespace Northrend {

VulkanBuffer::VulkanBuffer(
    VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) :
    device(device), byteSize(size) {
    if (this->byteSize == 0) {
        throw std::invalid_argument("Vulkan buffer size must be positive");
    }

    const std::array<std::uint32_t, 2> queueFamilies {
        this->device.graphics_family(), this->device.transfer_family()
    };
    const bool shared = (usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) != 0 && this->device.dedicated_transfer_queue();
    const VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = this->byteSize,
        .usage = usage,
        .sharingMode = shared ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = shared ? static_cast<std::uint32_t>(queueFamilies.size()) : 0,
        .pQueueFamilyIndices = shared ? queueFamilies.data() : nullptr
    };
    VkResult result = vkCreateBuffer(this->device.handle(), &bufferInfo, nullptr, &this->buffer);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan buffer: " + std::to_string(result));
    }

    try {
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(this->device.handle(), this->buffer, &requirements);

        const VkMemoryAllocateInfo allocationInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = requirements.size,
            .memoryTypeIndex = this->device.memory_type(requirements.memoryTypeBits, properties)
        };
        result = vkAllocateMemory(this->device.handle(), &allocationInfo, nullptr, &this->memory);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate Vulkan buffer memory: " + std::to_string(result));
        }

        result = vkBindBufferMemory(this->device.handle(), this->buffer, this->memory, 0);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to bind Vulkan buffer memory: " + std::to_string(result));
        }
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanBuffer::~VulkanBuffer() {
    this->destroy();
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

    const VkResult result = vkMapMemory(this->device.handle(), this->memory, 0, this->byteSize, 0, &this->mapped);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to map Vulkan buffer memory: " + std::to_string(result));
    }

    return this->mapped;
}

void VulkanBuffer::write(const std::span<const std::byte> data, const VkDeviceSize offset) {
    if (offset > this->byteSize || data.size_bytes() > this->byteSize - offset) {
        throw std::out_of_range("Vulkan buffer upload exceeds its capacity");
    }

    auto* destination = static_cast<std::byte*>(this->map()) + offset;
    std::memcpy(destination, data.data(), data.size_bytes());
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
