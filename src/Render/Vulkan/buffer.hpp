#pragma once

#include <volk.h>

#include <cstddef>
#include <span>

namespace Northrend {

class VulkanDevice;

class VulkanBuffer final {
public:
    VulkanBuffer(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    ~VulkanBuffer();

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    [[nodiscard]] VkBuffer handle() const noexcept;
    [[nodiscard]] VkDeviceSize size() const noexcept;
    [[nodiscard]] void* map();
    void write(std::span<const std::byte> data, VkDeviceSize offset = 0);

private:
    void destroy() noexcept;

    VulkanDevice& device;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize byteSize = 0;
    void* mapped = nullptr;
};

}
