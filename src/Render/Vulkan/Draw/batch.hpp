#pragma once

#include "Render/Vulkan/Draw/draw.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace Northrend {

struct RenderFrame;
class VulkanBuffer;
class VulkanDevice;
class VulkanMeshStore;
class VulkanMaterialStore;
class VulkanMeshPipeline;

class VulkanDrawBatch final {
public:
    explicit VulkanDrawBatch(VulkanDevice& device) noexcept;
    ~VulkanDrawBatch();

    VulkanDrawBatch(const VulkanDrawBatch&) = delete;
    VulkanDrawBatch& operator=(const VulkanDrawBatch&) = delete;

    void allocate(std::size_t count);
    void release() noexcept;
    void update(std::size_t imageIndex, const RenderFrame& frame, const VulkanMeshStore& meshes);
    [[nodiscard]] bool has_world(std::size_t imageIndex) const noexcept;
    [[nodiscard]] bool has_transformed(std::size_t imageIndex) const noexcept;
    void draw_world(VkCommandBuffer commandBuffer, std::size_t imageIndex, const VulkanMeshStore& meshes,
        const VulkanMaterialStore& materials, VkPipelineLayout pipelineLayout) const noexcept;
    void draw_transformed(VkCommandBuffer commandBuffer, std::size_t imageIndex, const VulkanMeshStore& meshes,
        const VulkanMaterialStore& materials, VulkanMeshPipeline& pipeline) const noexcept;

private:
    struct WorldGroup final {
        MaterialHandle material;
        std::uint32_t firstCommand;
        std::uint32_t commandCount;
        std::uint8_t detailLevel;
    };

    struct Store final {
        std::unique_ptr<VulkanBuffer> transformBuffer;
        std::unique_ptr<VulkanBuffer> indirectBuffer;
        std::vector<std::vector<VulkanDraw>> worldUnsigned16;
        std::vector<std::vector<VulkanDraw>> worldUnsigned32;
        std::vector<std::vector<WorldGroup>> worldUnsigned16Groups;
        std::vector<std::vector<WorldGroup>> worldUnsigned32Groups;
        std::vector<std::vector<VulkanDraw>> transformedUnsigned16;
        std::vector<std::vector<VulkanDraw>> transformedUnsigned32;
        std::uint64_t instanceRevision = 0;
        std::uint64_t meshRevision = 0;
        bool worldDraws = false;
        bool transformedDraws = false;
        bool initialized = false;
#ifdef NORTHREND_GPU_PROFILING
        std::uint64_t worldDrawCalls = 0;
        std::uint64_t worldCommands = 0;
        std::uint64_t worldTriangles = 0;
#endif
    };

    static constexpr VkDeviceSize InitialCapacity = 64ULL * 1024ULL;

    [[nodiscard]] static VkDeviceSize capacity(VkDeviceSize current, VkDeviceSize required);
    static void build_groups(std::vector<VulkanDraw>& draws, std::vector<WorldGroup>& groups,
        std::vector<VkDrawIndexedIndirectCommand>& commands);
    void draw_world_groups(VkCommandBuffer commandBuffer, const std::vector<WorldGroup>& groups,
        VkBuffer indirectBuffer, const VulkanMaterialStore& materials, VkPipelineLayout pipelineLayout) const noexcept;
    static void draw_transformed_group(VkCommandBuffer commandBuffer, std::size_t imageIndex,
        const std::vector<VulkanDraw>& draws, const VulkanMaterialStore& materials, VulkanMeshPipeline& pipeline,
        std::uint32_t& activeBlendMode) noexcept;

    VulkanDevice& device;
    std::vector<Store> stores;
};

}
