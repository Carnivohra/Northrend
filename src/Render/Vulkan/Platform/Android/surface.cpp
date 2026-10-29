#include "Render/Vulkan/Platform/Surface/creation.hpp"

#include <android/native_window.h>

#include <stdexcept>
#include <string>

namespace Northrend::VulkanPlatform {

VkSurfaceKHR create_surface(VkInstance instance, const NativeWindowHandle& handle) {
    const VkAndroidSurfaceCreateInfoKHR createInfo {
        .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .window = static_cast<ANativeWindow*>(handle.window)
    };

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const VkResult result = vkCreateAndroidSurfaceKHR(instance, &createInfo, nullptr, &surface);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan Android surface: " + std::to_string(result));
    }

    return surface;
}

}
