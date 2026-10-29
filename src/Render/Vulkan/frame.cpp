#include "Render/Vulkan/frame.hpp"

#include <stdexcept>
#include <string>

namespace Northrend {

VulkanFrame::VulkanFrame(VkDevice device) : device(device) {
    const VkSemaphoreCreateInfo semaphoreInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = 0
    };
    const VkFenceCreateInfo fenceInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = nullptr, .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    VkResult result = vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->imageAvailable);

    if (result == VK_SUCCESS) {
        result = vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->renderFinished);
    }

    if (result == VK_SUCCESS) {
        result = vkCreateFence(this->device, &fenceInfo, nullptr, &this->renderFence);
    }

    if (result != VK_SUCCESS) {
        this->destroy();
        throw std::runtime_error("Failed to create Vulkan frame synchronization: " + std::to_string(result));
    }
}

VulkanFrame::~VulkanFrame() {
    this->destroy();
}

VkSemaphore VulkanFrame::image_available() const noexcept {
    return this->imageAvailable;
}

VkSemaphore VulkanFrame::render_finished() const noexcept {
    return this->renderFinished;
}

VkFence VulkanFrame::fence() const noexcept {
    return this->renderFence;
}

void VulkanFrame::destroy() noexcept {
    if (this->renderFence != VK_NULL_HANDLE) {
        vkDestroyFence(this->device, this->renderFence, nullptr);
        this->renderFence = VK_NULL_HANDLE;
    }

    if (this->renderFinished != VK_NULL_HANDLE) {
        vkDestroySemaphore(this->device, this->renderFinished, nullptr);
        this->renderFinished = VK_NULL_HANDLE;
    }

    if (this->imageAvailable != VK_NULL_HANDLE) {
        vkDestroySemaphore(this->device, this->imageAvailable, nullptr);
        this->imageAvailable = VK_NULL_HANDLE;
    }
}

}
