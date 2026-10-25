#pragma once

#include "Render/Vulkan/Queue/families.hpp"

#include <volk.h>

#include <cstdint>

namespace Northrend {

class VulkanPhysicalDevice final {
public:
    VulkanPhysicalDevice(
        VkInstance instance,
        VkSurfaceKHR surface);

    [[nodiscard]] VkPhysicalDevice handle() const noexcept;
    [[nodiscard]] const VulkanQueueFamilies&
        queue_families() const noexcept;
    [[nodiscard]] std::uint32_t memory_type(
        std::uint32_t typeBits,
        VkMemoryPropertyFlags properties) const;
    [[nodiscard]] VkFormat depth_format() const;

private:
    [[nodiscard]] static VulkanQueueFamilies
        find_queue_families(
            VkPhysicalDevice device,
            VkSurfaceKHR surface);
    [[nodiscard]] static bool supports_swapchain_extension(
        VkPhysicalDevice device);
    [[nodiscard]] static bool has_swapchain_support(
        VkPhysicalDevice device,
        VkSurfaceKHR surface);
    [[nodiscard]] static std::uint32_t score(
        VkPhysicalDevice device);

    VkPhysicalDevice device = VK_NULL_HANDLE;
    VulkanQueueFamilies families;
};

}
