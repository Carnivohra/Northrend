#pragma once

#include "Math/matrix.hpp"
#include "Render/Vulkan/Camera/buffer.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Northrend {

struct RenderCamera;
class VulkanDevice;

class VulkanCameraContext final {
public:
    VulkanCameraContext(
        VulkanDevice& device,
        std::size_t imageCount);
    ~VulkanCameraContext();

    VulkanCameraContext(const VulkanCameraContext&) = delete;
    VulkanCameraContext& operator=(
        const VulkanCameraContext&) = delete;

    void update(
        std::size_t imageIndex,
        const RenderCamera& camera,
        std::uint64_t revision,
        VkExtent2D extent);
    void bind(
        VkCommandBuffer commandBuffer,
        VkPipelineLayout pipelineLayout,
        std::size_t imageIndex) const noexcept;

    [[nodiscard]] VkDescriptorSetLayout
        descriptor_layout() const noexcept;

private:
    void create_layout();
    void create_buffers(std::size_t imageCount);
    void create_descriptors();
    void destroy() noexcept;

    VulkanDevice& device;
    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VulkanCameraBuffer> buffers;
    std::vector<VkDescriptorSet> descriptorSets;
    Matrix4 matrix;
    VkExtent2D sourceExtent{};
    std::uint64_t sourceRevision = 0;
    std::uint64_t uploadRevision = 0;
    bool initialized = false;
};

}
