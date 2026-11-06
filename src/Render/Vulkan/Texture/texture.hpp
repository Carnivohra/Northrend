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
        std::uint32_t layers, std::size_t mipCount, bool array);
    ~VulkanTexture();

    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;

    [[nodiscard]] VkImage handle() const noexcept;
    [[nodiscard]] VkImageView view() const noexcept;

private:
    [[nodiscard]] static VkFormat vulkan_format(TextureFormat format) noexcept;
    void destroy() noexcept;

    VulkanDevice& device;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
};

}
