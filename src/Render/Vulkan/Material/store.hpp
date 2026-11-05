#pragma once

#include "Render/Material/data.hpp"
#include "Render/Material/material.hpp"

#include <volk.h>

#include <cstddef>
#include <vector>

namespace Northrend {

class VulkanDevice;
class VulkanTextureStore;

class VulkanMaterialStore final {
public:
    static constexpr std::size_t MaximumTextures = 64;

    VulkanMaterialStore(VulkanDevice& device, const VulkanTextureStore& textures);
    ~VulkanMaterialStore();

    VulkanMaterialStore(const VulkanMaterialStore&) = delete;
    VulkanMaterialStore& operator=(const VulkanMaterialStore&) = delete;

    void store(MaterialHandle handle, const MaterialData& data);
    void erase(MaterialHandle handle) noexcept;
    void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, MaterialHandle handle) const noexcept;
    [[nodiscard]] VkDescriptorSetLayout descriptor_layout() const noexcept;

private:
    void destroy() noexcept;

    static constexpr std::size_t MaximumMaterials = 4096;

    VulkanDevice& device;
    const VulkanTextureStore& textures;
    std::vector<VkDescriptorSet> materials;
    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
};

}
