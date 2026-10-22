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

    void wait_idle() const noexcept;

private:
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    std::uint32_t graphicsFamily = 0;
    std::uint32_t presentFamily = 0;
};

}
