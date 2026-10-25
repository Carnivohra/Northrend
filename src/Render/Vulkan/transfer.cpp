#include "Render/Vulkan/transfer.hpp"

#include "Render/Vulkan/device.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace Northrend {

VulkanTransferContext::VulkanTransferContext(
    VulkanDevice& device)
    : device(device) {
    try {
        const VkCommandPoolCreateInfo poolInfo{
            .sType =
                VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex =
                this->device.graphics_family()
        };
        VkResult result = vkCreateCommandPool(
            this->device.handle(),
            &poolInfo,
            nullptr,
            &this->pool);

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create the Vulkan transfer "
                "command pool: " +
                std::to_string(result));
        }

        const VkCommandBufferAllocateInfo allocationInfo{
            .sType =
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = this->pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };
        result = vkAllocateCommandBuffers(
            this->device.handle(),
            &allocationInfo,
            &this->commandBuffer);

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to allocate the Vulkan transfer "
                "command buffer: " +
                std::to_string(result));
        }

        const VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };
        result = vkCreateFence(
            this->device.handle(),
            &fenceInfo,
            nullptr,
            &this->fence);

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create the Vulkan transfer fence: " +
                std::to_string(result));
        }
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanTransferContext::~VulkanTransferContext() {
    this->destroy();
}

void VulkanTransferContext::copy_buffer(
    VkBuffer source,
    VkBuffer destination,
    VkDeviceSize size) {
    VkResult result = vkResetFences(
        this->device.handle(),
        1,
        &this->fence);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to reset the Vulkan transfer fence: " +
            std::to_string(result));
    }

    result = vkResetCommandBuffer(this->commandBuffer, 0);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to reset the Vulkan transfer command buffer: " +
            std::to_string(result));
    }

    const VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };
    result = vkBeginCommandBuffer(
        this->commandBuffer,
        &beginInfo);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to begin the Vulkan transfer command buffer: " +
            std::to_string(result));
    }

    const VkBufferCopy region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    vkCmdCopyBuffer(
        this->commandBuffer,
        source,
        destination,
        1,
        &region);
    result = vkEndCommandBuffer(this->commandBuffer);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to end the Vulkan transfer command buffer: " +
            std::to_string(result));
    }

    const VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &this->commandBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };
    result = vkQueueSubmit(
        this->device.graphics_queue(),
        1,
        &submitInfo,
        this->fence);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to submit the Vulkan buffer transfer: " +
            std::to_string(result));
    }

    result = vkWaitForFences(
        this->device.handle(),
        1,
        &this->fence,
        VK_TRUE,
        std::numeric_limits<std::uint64_t>::max());

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to wait for the Vulkan buffer transfer: " +
            std::to_string(result));
    }
}

void VulkanTransferContext::destroy() noexcept {
    if (this->fence != VK_NULL_HANDLE) {
        vkDestroyFence(
            this->device.handle(),
            this->fence,
            nullptr);
        this->fence = VK_NULL_HANDLE;
    }

    this->commandBuffer = VK_NULL_HANDLE;

    if (this->pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(
            this->device.handle(),
            this->pool,
            nullptr);
        this->pool = VK_NULL_HANDLE;
    }
}

}
