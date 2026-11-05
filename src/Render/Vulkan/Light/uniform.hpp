#pragma once

#include "Render/Vulkan/Light/buffer.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Northrend {

struct DirectionalLight;
class VulkanDevice;

class VulkanLightUniform final {
public:
    VulkanLightUniform(VulkanDevice& device, std::size_t imageCount);
    ~VulkanLightUniform();

    VulkanLightUniform(const VulkanLightUniform&) = delete;
    VulkanLightUniform& operator=(const VulkanLightUniform&) = delete;

    void update(std::size_t imageIndex, const DirectionalLight& light);
    void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, std::size_t imageIndex) const noexcept;

    [[nodiscard]] VkDescriptorSetLayout descriptor_layout() const noexcept;

private:
    void create_layout();
    void create_buffers(std::size_t imageCount);
    void create_descriptors();
    void destroy() noexcept;

    VulkanDevice& device;
    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VulkanLightBuffer> buffers;
    std::vector<VkDescriptorSet> descriptorSets;
};

}
