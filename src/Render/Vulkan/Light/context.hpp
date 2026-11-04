#pragma once

#include "Render/Vulkan/Light/buffer.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Northrend {

struct DirectionalLight;
class VulkanDevice;

class VulkanLightContext final {
public:
    VulkanLightContext(VulkanDevice& device, std::size_t imageCount);
    ~VulkanLightContext();

    VulkanLightContext(const VulkanLightContext&) = delete;
    VulkanLightContext& operator=(const VulkanLightContext&) = delete;

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
