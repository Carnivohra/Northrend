#pragma once

#include "Render/Texture/data.hpp"
#include "Render/Texture/texture.hpp"

#include <volk.h>

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace Northrend {

class VulkanBuffer;
class VulkanDevice;
class VulkanTexture;

class VulkanTextureStore final {
public:
    explicit VulkanTextureStore(VulkanDevice& device) noexcept;
    ~VulkanTextureStore();

    VulkanTextureStore(const VulkanTextureStore&) = delete;
    VulkanTextureStore& operator=(const VulkanTextureStore&) = delete;

    void store(TextureHandle handle, const TextureData& data);
    void erase(TextureHandle handle) noexcept;
    void erase(std::span<const TextureHandle> handles) noexcept;
    [[nodiscard]] const VulkanTexture* find(TextureHandle handle) const noexcept;

private:
    void grow_staging(VkDeviceSize required);

    static constexpr VkDeviceSize InitialStagingCapacity = 4 * 1024 * 1024;
    static constexpr VkDeviceSize UploadAlignment = 16;

    VulkanDevice& device;
    std::unique_ptr<VulkanBuffer> staging;
    std::vector<std::unique_ptr<VulkanTexture>> textures;
};

}
