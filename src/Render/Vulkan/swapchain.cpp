#include "Render/Vulkan/swapchain.hpp"

#include "Backend/Window/window.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/frame.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace Northrend::VulkanSwapchainSelection {

VkSurfaceFormatKHR choose_format(const std::vector<VkSurfaceFormatKHR>& formats) {
    if (formats.size() == 1 && formats.front().format == VK_FORMAT_UNDEFINED) {
        return VkSurfaceFormatKHR{
            .format = VK_FORMAT_B8G8R8A8_SRGB,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        };
    }

    for (const VkSurfaceFormatKHR& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return formats.front();
}

VkExtent2D choose_extent(
    const VkSurfaceCapabilitiesKHR& capabilities,
    const WindowSize& windowSize) {
    if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    return VkExtent2D{
        .width = std::clamp(
            windowSize.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width),
        .height = std::clamp(
            windowSize.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height)
    };
}

VkCompositeAlphaFlagBitsKHR choose_composite_alpha(VkCompositeAlphaFlagsKHR supported) {
    constexpr std::array<VkCompositeAlphaFlagBitsKHR, 4> choices{
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
    };

    for (VkCompositeAlphaFlagBitsKHR choice : choices) {
        if (supported & choice) {
            return choice;
        }
    }

    return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

}

namespace Northrend {

VulkanSwapchain::VulkanSwapchain(
    VulkanDevice& device,
    VkSurfaceKHR surface,
    const Window& window,
    const VkClearColorValue& clearColor)
    : device(device), surface(surface), window(window), clearColor(clearColor) {
    try {
        this->create_command_pool();
        this->create_frame_resources();
        this->create_swapchain_resources();
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanSwapchain::~VulkanSwapchain() {
    this->device.wait_idle();
    this->destroy();
}

void VulkanSwapchain::render() {
    const WindowSize windowSize = this->window.size();

    if (windowSize.width == 0 || windowSize.height == 0) {
        return;
    }

    if (this->swapchain == VK_NULL_HANDLE) {
        this->recreate();

        if (this->swapchain == VK_NULL_HANDLE) {
            return;
        }
    }

    VulkanFrame& frame = *this->frames[this->currentFrame];
    const VkFence frameFence = frame.fence();
    VkResult result = vkWaitForFences(
        this->device.handle(),
        1,
        &frameFence,
        VK_TRUE,
        std::numeric_limits<std::uint64_t>::max());

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to wait for the Vulkan frame fence: " + std::to_string(result));
    }

    std::uint32_t imageIndex = 0;
    result = vkAcquireNextImageKHR(
        this->device.handle(),
        this->swapchain,
        std::numeric_limits<std::uint64_t>::max(),
        frame.image_available(),
        VK_NULL_HANDLE,
        &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        this->recreate();
        return;
    }

    const bool recreateAfterPresent = result == VK_SUBOPTIMAL_KHR;

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error(
            "Failed to acquire a Vulkan swapchain image: " + std::to_string(result));
    }

    if (this->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        result = vkWaitForFences(
            this->device.handle(),
            1,
            &this->imagesInFlight[imageIndex],
            VK_TRUE,
            std::numeric_limits<std::uint64_t>::max());

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to wait for a Vulkan swapchain image: " + std::to_string(result));
        }
    }

    result = vkResetFences(this->device.handle(), 1, &frameFence);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to reset the Vulkan frame fence: " + std::to_string(result));
    }

    this->imagesInFlight[imageIndex] = frameFence;

    const VkSemaphore waitSemaphores[] = { frame.image_available() };
    const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    const VkSemaphore signalSemaphores[] = { frame.render_finished() };
    const VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &this->commandBuffers[imageIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };

    result = vkQueueSubmit(this->device.graphics_queue(), 1, &submitInfo, frameFence);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to submit the Vulkan frame: " + std::to_string(result));
    }

    const VkSwapchainKHR swapchains[] = { this->swapchain };
    const VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &imageIndex,
        .pResults = nullptr
    };

    result = vkQueuePresentKHR(this->device.present_queue(), &presentInfo);
    this->currentFrame = (this->currentFrame + 1) % FramesInFlight;

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || recreateAfterPresent) {
        this->recreate();
        return;
    }

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to present the Vulkan frame: " + std::to_string(result));
    }
}

void VulkanSwapchain::suspend() noexcept {
    this->device.wait_idle();
}

void VulkanSwapchain::create_command_pool() {
    const VkCommandPoolCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = this->device.graphics_family()
    };
    const VkResult result = vkCreateCommandPool(
        this->device.handle(),
        &createInfo,
        nullptr,
        &this->commandPool);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create the Vulkan command pool: " + std::to_string(result));
    }
}

void VulkanSwapchain::create_frame_resources() {
    this->frames.reserve(FramesInFlight);

    for (std::size_t index = 0; index < FramesInFlight; ++index) {
        this->frames.push_back(std::make_unique<VulkanFrame>(this->device.handle()));
    }
}

void VulkanSwapchain::create_swapchain_resources() {
    const WindowSize windowSize = this->window.size();

    if (windowSize.width == 0 || windowSize.height == 0) {
        return;
    }

    VkSurfaceCapabilitiesKHR capabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        this->device.physical(),
        this->surface,
        &capabilities);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to query Vulkan surface capabilities: " + std::to_string(result));
    }

    std::uint32_t formatCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        this->device.physical(),
        this->surface,
        &formatCount,
        nullptr);

    if (result != VK_SUCCESS || formatCount == 0) {
        throw std::runtime_error("The Vulkan surface does not provide a usable image format");
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        this->device.physical(),
        this->surface,
        &formatCount,
        formats.data());

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to query Vulkan surface formats: " + std::to_string(result));
    }

    const VkSurfaceFormatKHR surfaceFormat = VulkanSwapchainSelection::choose_format(formats);
    this->imageFormat = surfaceFormat.format;
    this->extent = VulkanSwapchainSelection::choose_extent(capabilities, windowSize);

    std::uint32_t imageCount = std::max(capabilities.minImageCount, 3U);

    if (capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    const std::uint32_t familyIndices[] = {
        this->device.graphics_family(),
        this->device.present_family()
    };
    const bool separateQueues = familyIndices[0] != familyIndices[1];
    const VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = this->surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = this->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = separateQueues ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = separateQueues ? 2U : 0U,
        .pQueueFamilyIndices = separateQueues ? familyIndices : nullptr,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VulkanSwapchainSelection::choose_composite_alpha(
            capabilities.supportedCompositeAlpha),
        .presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    result = vkCreateSwapchainKHR(
        this->device.handle(),
        &createInfo,
        nullptr,
        &this->swapchain);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create the Vulkan swapchain: " + std::to_string(result));
    }

    result = vkGetSwapchainImagesKHR(
        this->device.handle(),
        this->swapchain,
        &imageCount,
        nullptr);

    if (result != VK_SUCCESS || imageCount == 0) {
        throw std::runtime_error("The Vulkan swapchain did not provide any images");
    }

    this->images.resize(imageCount);
    result = vkGetSwapchainImagesKHR(
        this->device.handle(),
        this->swapchain,
        &imageCount,
        this->images.data());

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to get Vulkan swapchain images: " + std::to_string(result));
    }

    this->imagesInFlight.assign(this->images.size(), VK_NULL_HANDLE);
    this->create_image_views();
    this->create_render_pass();
    this->create_framebuffers();
    this->create_command_buffers();
}

void VulkanSwapchain::create_image_views() {
    this->imageViews.reserve(this->images.size());

    for (VkImage image : this->images) {
        const VkImageViewCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = this->imageFormat,
            .components = VkComponentMapping{
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        VkImageView imageView = VK_NULL_HANDLE;
        const VkResult result = vkCreateImageView(
            this->device.handle(),
            &createInfo,
            nullptr,
            &imageView);

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create a Vulkan swapchain image view: " + std::to_string(result));
        }

        this->imageViews.push_back(imageView);
    }
}

void VulkanSwapchain::create_render_pass() {
    const VkAttachmentDescription colorAttachment{
        .flags = 0,
        .format = this->imageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };
    const VkAttachmentReference colorReference{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    const VkSubpassDescription subpass{
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorReference,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr
    };
    const VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0
    };
    const VkRenderPassCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };
    const VkResult result = vkCreateRenderPass(
        this->device.handle(),
        &createInfo,
        nullptr,
        &this->renderPass);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create the Vulkan render pass: " + std::to_string(result));
    }
}

void VulkanSwapchain::create_framebuffers() {
    this->framebuffers.reserve(this->imageViews.size());

    for (VkImageView imageView : this->imageViews) {
        const VkImageView attachments[] = { imageView };
        const VkFramebufferCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = this->renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = this->extent.width,
            .height = this->extent.height,
            .layers = 1
        };
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        const VkResult result = vkCreateFramebuffer(
            this->device.handle(),
            &createInfo,
            nullptr,
            &framebuffer);

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create a Vulkan framebuffer: " + std::to_string(result));
        }

        this->framebuffers.push_back(framebuffer);
    }
}

void VulkanSwapchain::create_command_buffers() {
    this->commandBuffers.resize(this->images.size());

    const VkCommandBufferAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = this->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<std::uint32_t>(this->commandBuffers.size())
    };
    const VkResult result = vkAllocateCommandBuffers(
        this->device.handle(),
        &allocateInfo,
        this->commandBuffers.data());

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to allocate Vulkan command buffers: " + std::to_string(result));
    }

    for (std::size_t imageIndex = 0; imageIndex < this->commandBuffers.size(); ++imageIndex) {
        this->record_commands(static_cast<std::uint32_t>(imageIndex));
    }
}

void VulkanSwapchain::record_commands(std::uint32_t imageIndex) {
    VkCommandBuffer commandBuffer = this->commandBuffers[imageIndex];
    const VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr
    };

    VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to begin a Vulkan command buffer: " + std::to_string(result));
    }

    VkClearValue clearValue{};
    clearValue.color = this->clearColor;

    const VkRenderPassBeginInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = this->renderPass,
        .framebuffer = this->framebuffers[imageIndex],
        .renderArea = VkRect2D{
            .offset = VkOffset2D{ .x = 0, .y = 0 },
            .extent = this->extent
        },
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(commandBuffer);

    result = vkEndCommandBuffer(commandBuffer);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to record a Vulkan command buffer: " + std::to_string(result));
    }
}

void VulkanSwapchain::recreate() {
    const WindowSize windowSize = this->window.size();

    if (windowSize.width == 0 || windowSize.height == 0) {
        return;
    }

    this->device.wait_idle();
    this->destroy_swapchain_resources();
    this->create_swapchain_resources();
    this->currentFrame = 0;
}

void VulkanSwapchain::destroy_swapchain_resources() noexcept {
    if (!this->commandBuffers.empty()) {
        vkFreeCommandBuffers(
            this->device.handle(),
            this->commandPool,
            static_cast<std::uint32_t>(this->commandBuffers.size()),
            this->commandBuffers.data());
        this->commandBuffers.clear();
    }

    for (VkFramebuffer framebuffer : this->framebuffers) {
        vkDestroyFramebuffer(this->device.handle(), framebuffer, nullptr);
    }
    this->framebuffers.clear();

    if (this->renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(this->device.handle(), this->renderPass, nullptr);
        this->renderPass = VK_NULL_HANDLE;
    }

    for (VkImageView imageView : this->imageViews) {
        vkDestroyImageView(this->device.handle(), imageView, nullptr);
    }
    this->imageViews.clear();
    this->images.clear();
    this->imagesInFlight.clear();

    if (this->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(this->device.handle(), this->swapchain, nullptr);
        this->swapchain = VK_NULL_HANDLE;
    }
}

void VulkanSwapchain::destroy() noexcept {
    this->destroy_swapchain_resources();
    this->frames.clear();

    if (this->commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(this->device.handle(), this->commandPool, nullptr);
        this->commandPool = VK_NULL_HANDLE;
    }
}

}
