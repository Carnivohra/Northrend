#include "Render/Vulkan/Platform/Surface/creation.hpp"

#include "Render/Vulkan/Platform/wayland.hpp"
#include "Render/Vulkan/Platform/x11.hpp"

#include <stdexcept>
#include <variant>

namespace Northrend::VulkanPlatform {

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
