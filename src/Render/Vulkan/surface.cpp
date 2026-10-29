#include "Render/Vulkan/surface.hpp"

#include "Render/Vulkan/instance.hpp"
#include "Render/Vulkan/Platform/Surface/creation.hpp"

namespace Northrend {

VulkanSurface::VulkanSurface(VulkanInstance& instance, const NativeWindowHandle& windowHandle) :
    instance(instance), surface(VulkanPlatform::create_surface(instance.handle(), windowHandle)) {}

VulkanSurface::~VulkanSurface() {
    if (this->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(this->instance.handle(), this->surface, nullptr);
    }
}

VkSurfaceKHR VulkanSurface::handle() const noexcept {
    return this->surface;
}

}
