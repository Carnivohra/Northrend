#include "Render/Vulkan/Texture/texture.hpp"

#include "Render/Vulkan/device.hpp"

#ifdef NORTHREND_GPU_PROFILING
#include <chrono>
#endif

#include <array>
#include <limits>
#include <stdexcept>
#include <string>

namespace Northrend {

VulkanTexture::VulkanTexture(VulkanDevice& device, const TextureFormat format, const std::uint32_t width,
    const std::uint32_t height, const std::uint32_t layers, const std::size_t mipCount, const bool array,
    const VkImageUsageFlags usage) :
    device(device) {
    if (width == 0 || height == 0 || layers == 0 || mipCount == 0 ||
        mipCount > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("Vulkan texture dimensions are invalid");
    }

    const VkFormat vkFormat = vulkan_format(format);
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(this->device.physical(), vkFormat, &properties);

    if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) == 0) {
        throw std::runtime_error("Vulkan device does not support the requested sampled texture format");
    }

    if ((usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0 &&
        (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) == 0) {
        throw std::runtime_error("Vulkan device does not support the requested storage texture format");
    }

    const std::array<std::uint32_t, 2> queueFamilies {this->device.graphics_family(), this->device.transfer_family()};
    const bool shared = this->device.dedicated_transfer_queue();
    const VkImageCreateInfo imageInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = vkFormat,
        .extent = VkExtent3D {.width = width, .height = height, .depth = 1},
        .mipLevels = static_cast<std::uint32_t>(mipCount),
        .arrayLayers = layers,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = shared ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = shared ? static_cast<std::uint32_t>(queueFamilies.size()) : 0,
        .pQueueFamilyIndices = shared ? queueFamilies.data() : nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
#ifdef NORTHREND_GPU_PROFILING
    const auto imageStart = std::chrono::steady_clock::now();
#endif
    VkResult result = vkCreateImage(this->device.handle(), &imageInfo, nullptr, &this->image);
#ifdef NORTHREND_GPU_PROFILING
    this->imageCreationTime =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - imageStart).count();
#endif

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan texture image: " + std::to_string(result));
    }

    try {
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(this->device.handle(), this->image, &requirements);
#ifdef NORTHREND_GPU_PROFILING
        this->allocationSize = requirements.size;
#endif

        const VkMemoryAllocateInfo allocationInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = requirements.size,
            .memoryTypeIndex =
                this->device.memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        };
#ifdef NORTHREND_GPU_PROFILING
        const auto allocationStart = std::chrono::steady_clock::now();
#endif
        result = vkAllocateMemory(this->device.handle(), &allocationInfo, nullptr, &this->memory);
#ifdef NORTHREND_GPU_PROFILING
        this->memoryAllocationTime =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - allocationStart).count();
#endif

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate Vulkan texture memory: " + std::to_string(result));
        }

        result = vkBindImageMemory(this->device.handle(), this->image, this->memory, 0);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to bind Vulkan texture memory: " + std::to_string(result));
        }

        const VkImageViewCreateInfo viewInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = this->image,
            .viewType = array ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
            .format = vkFormat,
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
                .levelCount = static_cast<std::uint32_t>(mipCount),
                .baseArrayLayer = 0,
                .layerCount = layers
            }
        };
        result = vkCreateImageView(this->device.handle(), &viewInfo, nullptr, &this->imageView);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan texture view: " + std::to_string(result));
        }
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanTexture::~VulkanTexture() {
    this->destroy();
}

VkImage VulkanTexture::handle() const noexcept {
    return this->image;
}

VkImageView VulkanTexture::view() const noexcept {
    return this->imageView;
}

#ifdef NORTHREND_GPU_PROFILING
VkDeviceSize VulkanTexture::allocation_size() const noexcept {
    return this->allocationSize;
}

double VulkanTexture::image_creation_time() const noexcept {
    return this->imageCreationTime;
}

double VulkanTexture::memory_allocation_time() const noexcept {
    return this->memoryAllocationTime;
}
#endif

VkFormat VulkanTexture::vulkan_format(const TextureFormat format) noexcept {
    switch (format) {
    case TextureFormat::bc1Srgb:
        return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    case TextureFormat::bc2Srgb:
        return VK_FORMAT_BC2_SRGB_BLOCK;
    case TextureFormat::bc3Srgb:
        return VK_FORMAT_BC3_SRGB_BLOCK;
    case TextureFormat::r8Unorm:
        return VK_FORMAT_R8_UNORM;
    case TextureFormat::rgba8Unorm:
        return VK_FORMAT_R8G8B8A8_UNORM;
    }

    return VK_FORMAT_UNDEFINED;
}

void VulkanTexture::destroy() noexcept {
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
