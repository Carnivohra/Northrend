#pragma once

#include <volk.h>

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace Northrend {

class VulkanDepthImage;
class VulkanDevice;

class VulkanRenderTarget final {
public:
    VulkanRenderTarget(VulkanDevice& device, std::span<const VkImage> images, VkFormat imageFormat, VkExtent2D extent);
    ~VulkanRenderTarget();

    VulkanRenderTarget(const VulkanRenderTarget&) = delete;
    VulkanRenderTarget& operator=(const VulkanRenderTarget&) = delete;

    [[nodiscard]] VkRenderPass render_pass() const noexcept;
    [[nodiscard]] VkFramebuffer framebuffer(std::size_t imageIndex) const noexcept;
    [[nodiscard]] VkExtent2D extent() const noexcept;

private:
    void create_image_views(std::span<const VkImage> images);
    void create_depth_images(std::size_t imageCount);
    void create_render_pass();
    void create_framebuffers();
    void destroy() noexcept;

    VulkanDevice& device;
    VkFormat imageFormat;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D targetExtent;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkImageView> imageViews;
    std::vector<std::unique_ptr<VulkanDepthImage>> depthImages;
    std::vector<VkFramebuffer> framebuffers;
};

}
