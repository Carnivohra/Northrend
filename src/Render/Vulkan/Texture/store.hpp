#pragma once

#include "Render/Texture/data.hpp"
#include "Render/Texture/texture.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace Northrend {

class VulkanDevice;
class VulkanTexture;

class VulkanTextureStore final {
public:
    explicit VulkanTextureStore(VulkanDevice& device) noexcept;
    ~VulkanTextureStore();

    VulkanTextureStore(const VulkanTextureStore&) = delete;
    VulkanTextureStore& operator=(const VulkanTextureStore&) = delete;

    void store(TextureHandle handle, const TextureData& data);
    void erase(TextureHandle handle, std::uint64_t retirementSerial);
    void erase(std::span<const TextureHandle> handles, std::uint64_t retirementSerial);
    void collect(std::uint64_t completedSerial) noexcept;
    [[nodiscard]] const VulkanTexture* find(TextureHandle handle) const noexcept;

private:
    static constexpr VkDeviceSize UploadAlignment = 16;

    VulkanDevice& device;
    std::vector<std::unique_ptr<VulkanTexture>> textures;
    std::vector<std::pair<std::uint64_t, std::unique_ptr<VulkanTexture>>> retired;
};

}
