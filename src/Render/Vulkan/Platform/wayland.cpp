#include "Render/Vulkan/Platform/wayland.hpp"

#include <stdexcept>
#include <string>

namespace Northrend::VulkanPlatform {

VkSurfaceKHR create_wayland_surface(VkInstance instance, const WaylandWindowHandle& handle) {
    const VkWaylandSurfaceCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .display = static_cast<wl_display*>(handle.display),
        .surface = static_cast<wl_surface*>(handle.surface)
    };

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const VkResult result = vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &surface);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create Vulkan Wayland surface: " + std::to_string(result));
    }

    return surface;
}

}
