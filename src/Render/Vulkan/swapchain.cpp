#include "Render/Vulkan/swapchain.hpp"

#include "Backend/Window/window.hpp"
#include "Render/Vulkan/commands.hpp"
#include "Render/Vulkan/Mesh/store.hpp"
#include "Render/Vulkan/target.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/frame.hpp"
#include "Render/Vulkan/Pipeline/mesh.hpp"
#include "Render/frame.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace Northrend {

VulkanSwapchain::VulkanSwapchain(
    VulkanDevice& device,
    VulkanMeshStore& meshes,
    VkSurfaceKHR surface,
    const Window& window,
    const VkClearColorValue& clearColor)
    : device(device),
      meshes(meshes),
      surface(surface),
      window(window),
      clearColor(clearColor) {
    try {
        this->commands =
            std::make_unique<VulkanCommandContext>(this->device);
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

void VulkanSwapchain::render(const RenderFrame& renderFrame) {
    const int width = this->window.width();
    const int height = this->window.height();

    if (width <= 0 || height <= 0) {
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
            "Failed to wait for the Vulkan frame fence: " +
            std::to_string(result));
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

    const bool recreateAfterPresent =
        result == VK_SUBOPTIMAL_KHR;

    if (result != VK_SUCCESS &&
        result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error(
            "Failed to acquire a Vulkan swapchain image: " +
            std::to_string(result));
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
                "Failed to wait for a Vulkan swapchain image: " +
                std::to_string(result));
        }
    }

    this->pipeline->update_camera(
        imageIndex,
        renderFrame.camera,
        renderFrame.view,
        renderFrame.cameraRevision,
        this->extent);
    this->commands->record(
        imageIndex,
        renderFrame,
        *this->target,
        *this->pipeline,
        this->meshes,
        this->clearColor);

    result = vkResetFences(
        this->device.handle(),
        1,
        &frameFence);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to reset the Vulkan frame fence: " +
            std::to_string(result));
    }

    this->imagesInFlight[imageIndex] = frameFence;
    const VkCommandBuffer commandBuffer =
        this->commands->handle(imageIndex);
    const VkSemaphore waitSemaphores[]{
        frame.image_available()
    };
    const VkPipelineStageFlags waitStages[]{
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    const VkSemaphore signalSemaphores[]{
        frame.render_finished()
    };
    const VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };

    result = vkQueueSubmit(
        this->device.graphics_queue(),
        1,
        &submitInfo,
        frameFence);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to submit the Vulkan frame: " +
            std::to_string(result));
    }

    const VkSwapchainKHR swapchains[]{ this->swapchain };
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

    result = vkQueuePresentKHR(
        this->device.present_queue(),
        &presentInfo);
    this->currentFrame =
        (this->currentFrame + 1) % FramesInFlight;

    if (result == VK_ERROR_OUT_OF_DATE_KHR ||
        result == VK_SUBOPTIMAL_KHR ||
        recreateAfterPresent) {
        this->recreate();
        return;
    }

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to present the Vulkan frame: " +
            std::to_string(result));
    }
}

void VulkanSwapchain::suspend() noexcept {
    this->device.wait_idle();
}

void VulkanSwapchain::create_frame_resources() {
    this->frames.reserve(FramesInFlight);

    for (std::size_t index = 0;
         index < FramesInFlight;
         ++index) {
        this->frames.push_back(
            std::make_unique<VulkanFrame>(
                this->device.handle()));
    }
}

void VulkanSwapchain::create_swapchain_resources() {
    const int width = this->window.width();
    const int height = this->window.height();

    if (width <= 0 || height <= 0) {
        return;
    }

    VkSurfaceCapabilitiesKHR capabilities;
    VkResult result =
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            this->device.physical(),
            this->surface,
            &capabilities);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to query Vulkan surface capabilities: " +
            std::to_string(result));
    }

    std::uint32_t formatCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        this->device.physical(),
        this->surface,
        &formatCount,
        nullptr);

    if (result != VK_SUCCESS || formatCount == 0) {
        throw std::runtime_error(
            "The Vulkan surface does not provide a usable "
            "image format");
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        this->device.physical(),
        this->surface,
        &formatCount,
        formats.data());

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to query Vulkan surface formats: " +
            std::to_string(result));
    }

    std::uint32_t presentModeCount = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        this->device.physical(),
        this->surface,
        &presentModeCount,
        nullptr);

    if (result != VK_SUCCESS || presentModeCount == 0) {
        throw std::runtime_error(
            "The Vulkan surface does not provide a usable "
            "present mode");
    }

    std::vector<VkPresentModeKHR> presentModes(
        presentModeCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        this->device.physical(),
        this->surface,
        &presentModeCount,
        presentModes.data());

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to query Vulkan present modes: " +
            std::to_string(result));
    }

    const VkSurfaceFormatKHR surfaceFormat =
        choose_surface_format(formats);
    const VkPresentModeKHR presentMode =
        choose_present_mode(presentModes);
    this->imageFormat = surfaceFormat.format;
    this->extent = choose_surface_extent(
        capabilities,
        width,
        height);

    std::uint32_t imageCount =
        std::max(capabilities.minImageCount, 3U);

    if (capabilities.maxImageCount > 0) {
        imageCount = std::min(
            imageCount,
            capabilities.maxImageCount);
    }

    const std::uint32_t familyIndices[]{
        this->device.graphics_family(),
        this->device.present_family()
    };
    const bool separateQueues =
        familyIndices[0] != familyIndices[1];
    const VkSwapchainCreateInfoKHR createInfo{
        .sType =
            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = this->surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = this->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = separateQueues
            ? VK_SHARING_MODE_CONCURRENT
            : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = separateQueues ? 2U : 0U,
        .pQueueFamilyIndices =
            separateQueues ? familyIndices : nullptr,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = choose_composite_alpha(
            capabilities.supportedCompositeAlpha),
        .presentMode = presentMode,
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
            "Failed to create the Vulkan swapchain: " +
            std::to_string(result));
    }

    result = vkGetSwapchainImagesKHR(
        this->device.handle(),
        this->swapchain,
        &imageCount,
        nullptr);

    if (result != VK_SUCCESS || imageCount == 0) {
        throw std::runtime_error(
            "The Vulkan swapchain did not provide any images");
    }

    this->images.resize(imageCount);
    result = vkGetSwapchainImagesKHR(
        this->device.handle(),
        this->swapchain,
        &imageCount,
        this->images.data());

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to get Vulkan swapchain images: " +
            std::to_string(result));
    }

    this->imagesInFlight.assign(
        this->images.size(),
        VK_NULL_HANDLE);
    this->target = std::make_unique<VulkanRenderTarget>(
        this->device,
        std::span<const VkImage>(this->images),
        this->imageFormat,
        this->extent);
    this->pipeline = std::make_unique<VulkanMeshPipeline>(
        this->device,
        this->target->render_pass(),
        this->extent,
        this->images.size());
    this->commands->allocate(this->images.size());
}

void VulkanSwapchain::recreate() {
    const int width = this->window.width();
    const int height = this->window.height();

    if (width <= 0 || height <= 0) {
        return;
    }

    this->device.wait_idle();
    this->destroy_swapchain_resources();
    this->create_swapchain_resources();
    this->currentFrame = 0;
}

void VulkanSwapchain::destroy_swapchain_resources() noexcept {
    if (this->commands) {
        this->commands->release();
    }

    this->pipeline.reset();
    this->target.reset();
    this->images.clear();
    this->imagesInFlight.clear();

    if (this->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(
            this->device.handle(),
            this->swapchain,
            nullptr);
        this->swapchain = VK_NULL_HANDLE;
    }
}

void VulkanSwapchain::destroy() noexcept {
    this->destroy_swapchain_resources();
    this->frames.clear();
    this->commands.reset();
}

VkSurfaceFormatKHR VulkanSwapchain::choose_surface_format(
    const std::vector<VkSurfaceFormatKHR>& formats) {
    if (formats.size() == 1 &&
        formats.front().format == VK_FORMAT_UNDEFINED) {
        return VkSurfaceFormatKHR{
            .format = VK_FORMAT_B8G8R8A8_SRGB,
            .colorSpace =
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        };
    }

    for (const VkSurfaceFormatKHR& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return formats.front();
}

VkExtent2D VulkanSwapchain::choose_surface_extent(
    const VkSurfaceCapabilitiesKHR& capabilities,
    int width,
    int height) {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<std::uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    return VkExtent2D{
        .width = std::clamp(
            static_cast<std::uint32_t>(width),
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width),
        .height = std::clamp(
            static_cast<std::uint32_t>(height),
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height)
    };
}

VkPresentModeKHR VulkanSwapchain::choose_present_mode(
    const std::vector<VkPresentModeKHR>& presentModes) {
    for (VkPresentModeKHR presentMode : presentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkCompositeAlphaFlagBitsKHR
VulkanSwapchain::choose_composite_alpha(
    VkCompositeAlphaFlagsKHR supported) {
    constexpr std::array<
        VkCompositeAlphaFlagBitsKHR,
        4> choices{
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
