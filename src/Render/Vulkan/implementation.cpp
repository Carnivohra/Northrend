#include "Render/Vulkan/implementation.hpp"

#include "Backend/Window/window.hpp"
#include "Render/Vulkan/Platform/Surface/extension.hpp"
#include "Render/Vulkan/instance.hpp"
#include "Render/Vulkan/Mesh/store.hpp"
#include "Render/Vulkan/surface.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/swapchain.hpp"

#include <memory>

namespace Northrend {

VulkanRendererImplementation::VulkanRendererImplementation(const Window& window) {
    const NativeWindowHandle nativeHandle = window.native_handle();
    const char* platformExtension = VulkanPlatform::surface_extension(nativeHandle);

    this->instance = std::make_unique<VulkanInstance>(platformExtension);
    this->surface = std::make_unique<VulkanSurface>(*this->instance, nativeHandle);
    this->device = std::make_unique<VulkanDevice>(this->instance->handle(), this->surface->handle());
    this->meshes = std::make_unique<VulkanMeshStore>(*this->device);
    this->swapchain =
        std::make_unique<VulkanSwapchain>(*this->device, *this->meshes, this->surface->handle(), window, ClearColor);
}

VulkanRendererImplementation::~VulkanRendererImplementation() = default;

void VulkanRendererImplementation::frame(const RenderFrame& frame) {
    this->swapchain->render(frame);
}

void VulkanRendererImplementation::suspend() noexcept {
    if (this->swapchain) {
        this->swapchain->suspend();
    }
}

}
