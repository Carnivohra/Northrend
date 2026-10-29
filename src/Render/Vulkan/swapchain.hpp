#pragma once

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace Northrend {

struct RenderFrame;
class VulkanCommandContext;
class VulkanDevice;
class VulkanFrame;
class VulkanMeshStore;
class VulkanMeshPipeline;
class VulkanRenderTarget;
class Window;

class VulkanSwapchain final {
public:
    VulkanSwapchain(VulkanDevice& device, VulkanMeshStore& meshes, VkSurfaceKHR surface, const Window& window,
        const VkClearColorValue& clearColor);
    ~VulkanSwapchain();

    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    void render(const RenderFrame& frame);
    void suspend() noexcept;

private:
    static constexpr std::size_t FramesInFlight = 2;

    [[nodiscard]] static VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& formats);
    [[nodiscard]] static VkExtent2D choose_surface_extent(
        const VkSurfaceCapabilitiesKHR& capabilities, int width, int height);
    [[nodiscard]] static VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& presentModes);
    [[nodiscard]] static VkCompositeAlphaFlagBitsKHR choose_composite_alpha(VkCompositeAlphaFlagsKHR supported);

    void create_frame_resources();
    void create_swapchain_resources();
    void recreate();
    void destroy_swapchain_resources() noexcept;
    void destroy() noexcept;

    VulkanDevice& device;
    VulkanMeshStore& meshes;
    VkSurfaceKHR surface;
    const Window& window;
    VkClearColorValue clearColor;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat imageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D extent {};
    std::vector<VkImage> images;
    std::vector<std::unique_ptr<VulkanFrame>> frames;
    std::vector<VkFence> imagesInFlight;
    std::unique_ptr<VulkanRenderTarget> target;
    std::unique_ptr<VulkanMeshPipeline> pipeline;
    std::unique_ptr<VulkanCommandContext> commands;
    std::size_t currentFrame = 0;
};

}
