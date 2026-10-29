#include "Render/Vulkan/depth.hpp"

#include "Render/Vulkan/device.hpp"

#include <stdexcept>
#include <string>

namespace Northrend {

VulkanDepthImage::VulkanDepthImage(VulkanDevice& device, VkExtent2D extent, VkFormat format) : device(device) {
    const VkImageCreateInfo imageInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = VkExtent3D {.width = extent.width, .height = extent.height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkResult result = vkCreateImage(this->device.handle(), &imageInfo, nullptr, &this->image);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a Vulkan depth image: " + std::to_string(result));
    }

    try {
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(this->device.handle(), this->image, &requirements);

        const VkMemoryAllocateInfo allocationInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = requirements.size,
            .memoryTypeIndex =
                this->device.memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        };
        result = vkAllocateMemory(this->device.handle(), &allocationInfo, nullptr, &this->memory);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate Vulkan depth memory: " + std::to_string(result));
        }

        result = vkBindImageMemory(this->device.handle(), this->image, this->memory, 0);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to bind Vulkan depth memory: " + std::to_string(result));
        }

        const VkImageViewCreateInfo viewInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = this->image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .components =
                VkComponentMapping {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY
                },
            .subresourceRange = VkImageSubresourceRange {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        result = vkCreateImageView(this->device.handle(), &viewInfo, nullptr, &this->imageView);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create a Vulkan depth image view: " + std::to_string(result));
        }
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanDepthImage::~VulkanDepthImage() {
    this->destroy();
}

VkImageView VulkanDepthImage::view() const noexcept {
    return this->imageView;
}

void VulkanDepthImage::destroy() noexcept {
    if (this->imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(this->device.handle(), this->imageView, nullptr);
        this->imageView = VK_NULL_HANDLE;
    }

    if (this->image != VK_NULL_HANDLE) {
        vkDestroyImage(this->device.handle(), this->image, nullptr);
        this->image = VK_NULL_HANDLE;
    }

    if (this->memory != VK_NULL_HANDLE) {
        vkFreeMemory(this->device.handle(), this->memory, nullptr);
        this->memory = VK_NULL_HANDLE;
    }
}

}
