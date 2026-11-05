#include "Render/Vulkan/vulkan.hpp"

#include "Backend/Window/window.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/instance.hpp"
#include "Render/Vulkan/Material/store.hpp"
#include "Render/Vulkan/Mesh/store.hpp"
#include "Render/Vulkan/Platform/Surface/extension.hpp"
#include "Render/Vulkan/surface.hpp"
#include "Render/Vulkan/swapchain.hpp"
#include "Render/Vulkan/Texture/store.hpp"

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
    this->textures = std::make_unique<VulkanTextureStore>(*this->device);
    this->materials = std::make_unique<VulkanMaterialStore>(*this->device, *this->textures);
    this->swapchain = std::make_unique<VulkanSwapchain>(
        *this->device, *this->meshes, *this->materials, this->surface->handle(), window, ClearColor);
}

VulkanRenderer::~VulkanRenderer() = default;

void VulkanRenderer::store_mesh(const MeshHandle handle, const MeshData& data) {
    this->meshes->store(handle, data);
}

void VulkanRenderer::remove_mesh(const MeshHandle handle) {
    this->meshes->erase(handle);
}

void VulkanRenderer::store_material(const MaterialHandle handle, const MaterialData& data) {
    this->materials->store(handle, data);
}

void VulkanRenderer::remove_material(const MaterialHandle handle) {
    this->materials->erase(handle);
}

void VulkanRenderer::store_texture(const TextureHandle handle, const TextureData& data) {
    this->textures->store(handle, data);
}

void VulkanRenderer::remove_texture(const TextureHandle handle) {
    this->textures->erase(handle);
}

void VulkanRenderer::remove_textures(const std::span<const TextureHandle> handles) {
    this->textures->erase(handles);
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
