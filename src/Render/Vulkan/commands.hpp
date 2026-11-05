#pragma once

#include "Render/Vulkan/Draw/batch.hpp"

#ifdef NORTHREND_GPU_PROFILING
#include "Render/Vulkan/timer.hpp"
#endif

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#ifdef NORTHREND_GPU_PROFILING
#include <optional>
#endif

namespace Northrend {

struct RenderFrame;
class VulkanDevice;
class VulkanMeshStore;
class VulkanMeshPipeline;
class VulkanRenderTarget;
class VulkanMaterialStore;

class VulkanCommandContext final {
public:
    explicit VulkanCommandContext(VulkanDevice& device);
    ~VulkanCommandContext();

    VulkanCommandContext(const VulkanCommandContext&) = delete;
    VulkanCommandContext& operator=(const VulkanCommandContext&) = delete;

    void allocate(std::size_t count);
    void release() noexcept;

    void record(std::size_t imageIndex, const RenderFrame& frame, const VulkanRenderTarget& target,
        VulkanMeshPipeline& pipeline, const VulkanMeshStore& meshes, const VulkanMaterialStore& materials,
        const VkClearColorValue& clearColor);

#ifdef NORTHREND_GPU_PROFILING
    void submitted(std::size_t imageIndex) noexcept;
#endif

    [[nodiscard]] VkCommandBuffer handle(std::size_t imageIndex) const noexcept;

#ifdef NORTHREND_GPU_PROFILING
    [[nodiscard]] std::optional<double> gpu_time(std::size_t imageIndex);
#endif

private:
    VulkanDevice& device;
    VulkanDrawBatch batch;

#ifdef NORTHREND_GPU_PROFILING
    VulkanGpuTimer timer;
#endif

    VkCommandPool pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> buffers;
    std::vector<std::uint64_t> recordedInstanceRevisions;
    std::vector<std::uint64_t> recordedMeshRevisions;
    std::vector<std::uint8_t> recorded;
};

}
