#pragma once

#include "Render/Texture/format.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>

namespace Northrend {

class VulkanDevice;

class VulkanTexture final {
public:
    VulkanTexture(VulkanDevice& device, TextureFormat format, std::uint32_t width, std::uint32_t height,
        std::uint32_t layers, std::size_t mipCount, bool array,
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    ~VulkanTexture();

    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;

    [[nodiscard]] VkImage handle() const noexcept;
    [[nodiscard]] VkImageView view() const noexcept;
#ifdef NORTHREND_GPU_PROFILING
    [[nodiscard]] VkDeviceSize allocation_size() const noexcept;
    [[nodiscard]] double image_creation_time() const noexcept;
    [[nodiscard]] double memory_allocation_time() const noexcept;
#endif

private:
    [[nodiscard]] static VkFormat vulkan_format(TextureFormat format) noexcept;
    void destroy() noexcept;

    VulkanDevice& device;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
#ifdef NORTHREND_GPU_PROFILING
    VkDeviceSize allocationSize = 0;
    double imageCreationTime = 0.0;
    double memoryAllocationTime = 0.0;
#endif
};

}
