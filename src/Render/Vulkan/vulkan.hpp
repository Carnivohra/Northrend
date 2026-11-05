#pragma once

#include "Render/renderer.hpp"

#include <memory>

namespace Northrend {

class Window;
class VulkanDevice;
class VulkanInstance;
class VulkanMaterialStore;
class VulkanMeshStore;
class VulkanSurface;
class VulkanSwapchain;
class VulkanTextureStore;

class VulkanRenderer final : public Renderer {
public:
    explicit VulkanRenderer(const Window& window);
    ~VulkanRenderer() override;

    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;

    void store_mesh(MeshHandle handle, const MeshData& data) override;
    void remove_mesh(MeshHandle handle) override;
    void store_material(MaterialHandle handle, const MaterialData& data) override;
    void remove_material(MaterialHandle handle) override;
    void store_texture(TextureHandle handle, const TextureData& data) override;
    void remove_texture(TextureHandle handle) override;
    void remove_textures(std::span<const TextureHandle> handles) override;
    void frame(const RenderFrame& frame) override;
    void suspend() noexcept override;

private:
    std::unique_ptr<VulkanInstance> instance;
    std::unique_ptr<VulkanSurface> surface;
    std::unique_ptr<VulkanDevice> device;
    std::unique_ptr<VulkanMeshStore> meshes;
    std::unique_ptr<VulkanTextureStore> textures;
    std::unique_ptr<VulkanMaterialStore> materials;
    std::unique_ptr<VulkanSwapchain> swapchain;
};

}
