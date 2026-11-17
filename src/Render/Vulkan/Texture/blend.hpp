#pragma once

#include "Render/Texture/blend.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace Northrend {

class VulkanBuffer;
class VulkanDevice;
class VulkanTexture;
class VulkanTextureStore;

class VulkanTextureBlend final {
public:
    VulkanTextureBlend(VulkanDevice& device, VulkanTextureStore& textures, std::size_t batchCount);
    ~VulkanTextureBlend();

    VulkanTextureBlend(const VulkanTextureBlend&) = delete;
    VulkanTextureBlend& operator=(const VulkanTextureBlend&) = delete;

    void store(TextureHandle handle, const TextureBlendData& data);
    void cancel(TextureHandle handle) noexcept;
    [[nodiscard]] VkCommandBuffer flush(std::size_t batch, std::uint64_t submissionSerial);
    void collect(std::uint64_t completedSerial) noexcept;
    [[nodiscard]] bool pending() const noexcept;
    [[nodiscard]] bool ready(TextureHandle handle) const noexcept;

private:
    struct Job final {
        std::unique_ptr<VulkanBuffer> cells;
        TextureHandle handle;
        VkDescriptorSet descriptor = VK_NULL_HANDLE;
        VkImage output = VK_NULL_HANDLE;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t layers = 0;
        std::uint32_t mipCount = 0;
        std::uint32_t columns = 0;
        std::uint32_t rows = 0;
        std::uint32_t cellSize = 0;
        float textureScale = 1.0F;
    };

    void create_layout();
    void create_descriptors();
    void create_pipeline();
    void create_commands(std::size_t batchCount);
    void write_descriptor(Job& job, std::span<const TextureHandle> sources, TextureHandle alpha, VkImageView output);
    void record(VkCommandBuffer commandBuffer, const Job& job) const;
    void destroy() noexcept;
#ifdef NORTHREND_GPU_PROFILING
    void report();
#endif

    static constexpr std::size_t MaximumTextures = 64;
    static constexpr std::size_t MaximumJobs = 4096;
    static constexpr std::size_t JobsPerFrame = 1;

    VulkanDevice& device;
    VulkanTextureStore& textures;
    std::deque<Job> jobs;
    std::vector<std::pair<std::uint64_t, Job>> retired;
    std::vector<VkCommandBuffer> commandBuffers;
    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;
    VkSampler alphaSampler = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
#ifdef NORTHREND_GPU_PROFILING
    std::uint64_t submittedJobs = 0;
    std::uint64_t completedJobs = 0;
    std::uint64_t cancelledJobs = 0;
#endif
};

}
