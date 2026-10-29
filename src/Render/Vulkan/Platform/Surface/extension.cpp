#include "Render/Vulkan/Platform/Surface/extension.hpp"

#include <volk.h>

#include <stdexcept>
#include <variant>

namespace Northrend::VulkanPlatform {

const char* surface_extension(const NativeWindowHandle& handle) {
    if (std::holds_alternative<WaylandWindowHandle>(handle)) {
        return VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
    }

    if (std::holds_alternative<X11WindowHandle>(handle)) {
        return VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
    }

    throw std::runtime_error("Unsupported Linux window handle for Vulkan");
}

}
