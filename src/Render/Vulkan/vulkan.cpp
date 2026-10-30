#include "Render/Vulkan/vulkan.hpp"

#include "Backend/Window/window.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/instance.hpp"
#include "Render/Vulkan/Mesh/store.hpp"
#include "Render/Vulkan/Platform/Surface/extension.hpp"
#include "Render/Vulkan/surface.hpp"
#include "Render/Vulkan/swapchain.hpp"

#include <volk.h>

#include <memory>

namespace Northrend {

VulkanRenderer::VulkanRenderer(const Window& window) {
    constexpr VkClearColorValue ClearColor {.float32 = {0.015F, 0.035F, 0.080F, 1.000F}};
    const NativeWindowHandle nativeHandle = window.native_handle();
    const char* platformExtension = VulkanPlatform::surface_extension(nativeHandle);

    this->instance = std::make_unique<VulkanInstance>(platformExtension);
    this->surface = std::make_unique<VulkanSurface>(*this->instance, nativeHandle);
    this->device = std::make_unique<VulkanDevice>(this->instance->handle(), this->surface->handle());
    this->meshes = std::make_unique<VulkanMeshStore>(*this->device);
    this->swapchain = std::make_unique<VulkanSwapchain>(
        *this->device, *this->meshes, this->surface->handle(), window, ClearColor);
}

VulkanRenderer::~VulkanRenderer() = default;

void VulkanRenderer::store_mesh(const MeshHandle handle, const MeshData& data) {
    this->meshes->store(handle, data);
}

void VulkanRenderer::remove_mesh(const MeshHandle handle) {
    this->meshes->erase(handle);
}

void VulkanRenderer::frame(const RenderFrame& frame) {
    this->swapchain->render(frame);
}

void VulkanRenderer::suspend() noexcept {
    if (this->swapchain) {
        this->swapchain->suspend();
    }
}

}
