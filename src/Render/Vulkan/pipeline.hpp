#pragma once

#include "Math/matrix.hpp"
#include "Render/Vulkan/buffer.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace Northrend {

struct RenderCamera;
class VulkanDevice;

class VulkanPipeline final {
public:
    VulkanPipeline(
        VulkanDevice& device,
        VkRenderPass renderPass,
        VkExtent2D extent,
        std::size_t imageCount);
    ~VulkanPipeline();

    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;

    void update_camera(
        std::size_t imageIndex,
        const RenderCamera& camera,
        std::uint64_t revision,
        VkExtent2D extent);
    void bind(VkCommandBuffer commandBuffer, std::size_t imageIndex) const noexcept;
    void push_transform(
        VkCommandBuffer commandBuffer,
        const Matrix4& transform) const noexcept;

private:
    struct CameraBuffer final {
        std::unique_ptr<VulkanBuffer> buffer;
        void* mapped = nullptr;
        std::uint64_t uploadRevision = 0;
    };

    void create_descriptor_layout();
    void create_camera_buffers(std::size_t imageCount);
    void create_descriptors();
    void create_layout();
    void create_graphics_pipeline(VkRenderPass renderPass, VkExtent2D extent);
    void destroy() noexcept;

    VulkanDevice& device;
    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    std::vector<CameraBuffer> cameraBuffers;
    std::vector<VkDescriptorSet> descriptorSets;
    Matrix4 cameraMatrix;
    VkExtent2D cameraExtent{};
    std::uint64_t cameraSourceRevision = 0;
    std::uint64_t cameraUploadRevision = 0;
    bool cameraInitialized = false;
};

}
