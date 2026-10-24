#pragma once

#include <volk.h>

#include <cstdint>

namespace Northrend {

class VulkanDevice final {
public:
    VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
    ~VulkanDevice();

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

    VkPhysicalDevice physical() const noexcept;
    VkDevice handle() const noexcept;
    VkQueue graphics_queue() const noexcept;
    VkQueue present_queue() const noexcept;
    std::uint32_t graphics_family() const noexcept;
    std::uint32_t present_family() const noexcept;
    std::uint32_t memory_type(
        std::uint32_t typeBits,
        VkMemoryPropertyFlags properties) const;

    void copy_buffer(
        VkBuffer source,
        VkBuffer destination,
        VkDeviceSize size);
    void wait_idle() const noexcept;

private:
    void create_transfer_context();

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkCommandPool transferPool = VK_NULL_HANDLE;
    VkCommandBuffer transferCommandBuffer = VK_NULL_HANDLE;
    VkFence transferFence = VK_NULL_HANDLE;
    std::uint32_t graphicsFamily = 0;
    std::uint32_t presentFamily = 0;
};

}
