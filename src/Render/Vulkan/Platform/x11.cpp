#include "Render/Vulkan/Platform/x11.hpp"

#include <stdexcept>
#include <string>

namespace Northrend::VulkanPlatform {

VkSurfaceKHR create_x11_surface(VkInstance instance, const X11WindowHandle& handle) {
    const VkXlibSurfaceCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .dpy = static_cast<Display*>(handle.display),
        .window = static_cast<::Window>(handle.window)
    };

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const VkResult result = vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create Vulkan X11 surface: " + std::to_string(result));
    }

    return surface;
}

}
