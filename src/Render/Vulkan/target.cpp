#include "Render/Vulkan/target.hpp"

#include "Render/Vulkan/depth.hpp"
#include "Render/Vulkan/device.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace Northrend {

VulkanRenderTarget::VulkanRenderTarget(
    VulkanDevice& device, std::span<const VkImage> images, VkFormat imageFormat, VkExtent2D extent) :
    device(device), imageFormat(imageFormat), targetExtent(extent) {
    try {
        this->create_image_views(images);
        this->create_depth_images(images.size());
        this->create_render_pass();
        this->create_framebuffers();
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanRenderTarget::~VulkanRenderTarget() {
    this->destroy();
}

VkRenderPass VulkanRenderTarget::render_pass() const noexcept {
    return this->renderPass;
}

VkFramebuffer VulkanRenderTarget::framebuffer(std::size_t imageIndex) const noexcept {
    return this->framebuffers[imageIndex];
}

VkExtent2D VulkanRenderTarget::extent() const noexcept {
    return this->targetExtent;
}

void VulkanRenderTarget::create_image_views(std::span<const VkImage> images) {
    this->imageViews.reserve(images.size());

    for (VkImage image : images) {
        const VkImageViewCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = this->imageFormat,
            .components =
                VkComponentMapping {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY
                },
            .subresourceRange = VkImageSubresourceRange {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        VkImageView imageView = VK_NULL_HANDLE;
        const VkResult result = vkCreateImageView(this->device.handle(), &createInfo, nullptr, &imageView);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create a Vulkan swapchain image view: " + std::to_string(result));
        }

        this->imageViews.push_back(imageView);
    }
}

void VulkanRenderTarget::create_depth_images(std::size_t imageCount) {
    this->depthFormat = this->device.depth_format();
    this->depthImages.reserve(imageCount);

    for (std::size_t index = 0; index < imageCount; ++index) {
        this->depthImages.push_back(
            std::make_unique<VulkanDepthImage>(this->device, this->targetExtent, this->depthFormat));
    }
}

void VulkanRenderTarget::create_render_pass() {
    const std::array<VkAttachmentDescription, 2> attachments {
        VkAttachmentDescription {
            .flags = 0,
            .format = this->imageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        },
        VkAttachmentDescription {
            .flags = 0,
            .format = this->depthFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        }
    };
    const VkAttachmentReference colorReference {.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    const VkAttachmentReference depthReference {
        .attachment = 1, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    const VkSubpassDescription subpass {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorReference,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = &depthReference,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr
    };
    const VkSubpassDependency dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0
    };
    const VkRenderPassCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };
    const VkResult result = vkCreateRenderPass(this->device.handle(), &createInfo, nullptr, &this->renderPass);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan render pass: " + std::to_string(result));
    }
}

void VulkanRenderTarget::create_framebuffers() {
    this->framebuffers.reserve(this->imageViews.size());

    for (std::size_t index = 0; index < this->imageViews.size(); ++index) {
        const std::array<VkImageView, 2> attachments {this->imageViews[index], this->depthImages[index]->view()};
        const VkFramebufferCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = this->renderPass,
            .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = this->targetExtent.width,
            .height = this->targetExtent.height,
            .layers = 1
        };
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        const VkResult result = vkCreateFramebuffer(this->device.handle(), &createInfo, nullptr, &framebuffer);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create a Vulkan framebuffer: " + std::to_string(result));
        }

        this->framebuffers.push_back(framebuffer);
    }
}

void VulkanRenderTarget::destroy() noexcept {
    for (VkFramebuffer framebuffer : this->framebuffers) {
        vkDestroyFramebuffer(this->device.handle(), framebuffer, nullptr);
    }
    this->framebuffers.clear();

    if (this->renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(this->device.handle(), this->renderPass, nullptr);
        this->renderPass = VK_NULL_HANDLE;
    }

    this->depthImages.clear();
    this->depthFormat = VK_FORMAT_UNDEFINED;

    for (VkImageView imageView : this->imageViews) {
        vkDestroyImageView(this->device.handle(), imageView, nullptr);
    }
    this->imageViews.clear();
}

}
