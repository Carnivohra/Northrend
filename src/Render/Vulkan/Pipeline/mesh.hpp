#pragma once

#include "Math/matrix.hpp"
#include "Render/Vulkan/Camera/context.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace Northrend {

struct Camera;
class VulkanDevice;

class VulkanMeshPipeline final {
public:
    VulkanMeshPipeline(VulkanDevice& device, VkRenderPass renderPass, VkExtent2D extent, std::size_t imageCount);
    ~VulkanMeshPipeline();

    VulkanMeshPipeline(const VulkanMeshPipeline&) = delete;
    VulkanMeshPipeline& operator=(const VulkanMeshPipeline&) = delete;

    void update_camera(
        std::size_t imageIndex, const Camera& camera, const Matrix4& view, std::uint64_t revision, VkExtent2D extent);
    void bind(VkCommandBuffer commandBuffer, std::size_t imageIndex) const noexcept;
    void push_transform(VkCommandBuffer commandBuffer, const Matrix4& transform) const noexcept;

private:
    [[nodiscard]] VkShaderModule create_shader_module(std::span<const unsigned char> code) const;

    void create_layout();
    void create_graphics_pipeline(VkRenderPass renderPass, VkExtent2D extent);
    void destroy() noexcept;

    VulkanDevice& device;
    VulkanCameraContext camera;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

}
