#pragma once

#include "Render/Vulkan/physical.hpp"

#include <volk.h>

#include <cstdint>
#include <memory>

namespace Northrend {

class VulkanTransferContext;

class VulkanDevice final {
public:
    VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
    ~VulkanDevice();

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

    [[nodiscard]] VkPhysicalDevice physical() const noexcept;
    [[nodiscard]] VkDevice handle() const noexcept;
    [[nodiscard]] VkQueue graphics_queue() const noexcept;
    [[nodiscard]] VkQueue present_queue() const noexcept;
    [[nodiscard]] std::uint32_t graphics_family() const noexcept;
    [[nodiscard]] std::uint32_t present_family() const noexcept;
    [[nodiscard]] VkFormat depth_format() const;
    [[nodiscard]] std::uint32_t memory_type(std::uint32_t typeBits, VkMemoryPropertyFlags properties) const;

    void copy_buffer(VkBuffer source, VkBuffer destination, VkDeviceSize size);
    void wait_idle() const noexcept;

private:
    VulkanPhysicalDevice physicalDevice;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    std::unique_ptr<VulkanTransferContext> transfer;
};

}
