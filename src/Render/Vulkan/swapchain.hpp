#pragma once

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace Northrend {

class VulkanDevice;
class VulkanFrame;
class Window;

class VulkanSwapchain final {
public:
    VulkanSwapchain(
        VulkanDevice& device,
        VkSurfaceKHR surface,
        const Window& window,
        const VkClearColorValue& clearColor);
    ~VulkanSwapchain();

    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    void render();
    void suspend() noexcept;

private:
    static constexpr std::size_t FramesInFlight = 2;

    void create_command_pool();
    void create_frame_resources();
    void create_swapchain_resources();
    void create_image_views();
    void create_render_pass();
    void create_framebuffers();
    void create_command_buffers();
    void record_commands(std::uint32_t imageIndex);
    void recreate();
    void destroy_swapchain_resources() noexcept;
    void destroy() noexcept;

    VulkanDevice& device;
    VkSurfaceKHR surface;
    const Window& window;
    VkClearColorValue clearColor;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat imageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<std::unique_ptr<VulkanFrame>> frames;
    std::vector<VkFence> imagesInFlight;
    std::size_t currentFrame = 0;
};

}
