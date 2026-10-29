#pragma once

#include <volk.h>

#include <memory>

namespace Northrend {

struct RenderFrame;
class VulkanDevice;
class VulkanInstance;
class VulkanMeshStore;
class VulkanSurface;
class VulkanSwapchain;
class Window;

class VulkanRendererImplementation final {
public:
    explicit VulkanRendererImplementation(const Window& window);
    ~VulkanRendererImplementation();

    VulkanRendererImplementation(const VulkanRendererImplementation&) = delete;
    VulkanRendererImplementation& operator=(const VulkanRendererImplementation&) = delete;

    void frame(const RenderFrame& frame);
    void suspend() noexcept;

private:
    static constexpr VkClearColorValue ClearColor {.float32 = {0.015F, 0.035F, 0.080F, 1.000F}};

    std::unique_ptr<VulkanInstance> instance;
    std::unique_ptr<VulkanSurface> surface;
    std::unique_ptr<VulkanDevice> device;
    std::unique_ptr<VulkanMeshStore> meshes;
    std::unique_ptr<VulkanSwapchain> swapchain;
};

}
