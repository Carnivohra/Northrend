#include "Render/Vulkan/Platform/surface.hpp"

#include "Render/Vulkan/Platform/wayland.hpp"
#include "Render/Vulkan/Platform/x11.hpp"

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

VkSurfaceKHR create_surface(VkInstance instance, const NativeWindowHandle& handle) {
    if (const auto* wayland = std::get_if<WaylandWindowHandle>(&handle)) {
        return create_wayland_surface(instance, *wayland);
    }

    if (const auto* x11 = std::get_if<X11WindowHandle>(&handle)) {
        return create_x11_surface(instance, *x11);
    }

    throw std::runtime_error("Unsupported Linux window handle for Vulkan");
}

}
