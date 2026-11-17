#include "Render/Vulkan/Draw/batch.hpp"

#include "Render/Vulkan/Mesh/mesh.hpp"
#include "Render/Vulkan/Mesh/store.hpp"
#include "Render/Vulkan/Material/store.hpp"
#include "Render/Vulkan/Material/constants.hpp"
#include "Render/Vulkan/Mesh/pipeline.hpp"
#include "Render/Vulkan/buffer.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/Transfer/segment.hpp"
#include "Render/frame.hpp"

#ifdef NORTHREND_GPU_PROFILING
#include "Debug/interval.hpp"
#include "Debug/timing.hpp"

#include <chrono>
#include <iostream>
#endif

#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <stdexcept>

namespace Northrend {

VulkanDrawBatch::VulkanDrawBatch(VulkanDevice& device) noexcept : device(device) {}

VulkanDrawBatch::~VulkanDrawBatch() = default;

void VulkanDrawBatch::allocate(const std::size_t count) {
    this->stores.clear();
    this->stores.resize(count);
}

void VulkanDrawBatch::release() noexcept {
    this->stores.clear();
}

void VulkanDrawBatch::update(const std::size_t imageIndex, const RenderFrame& frame, const VulkanMeshStore& meshes) {
    Store& target = this->stores.at(imageIndex);
    const std::uint64_t meshRevision = meshes.revision();

    if (target.initialized && target.instanceRevision == frame.instancesRevision &&
        target.meshRevision == meshRevision) {
        return;
    }

#ifdef NORTHREND_GPU_PROFILING
    static Timing rebuildTiming;
    const auto rebuildStart = std::chrono::steady_clock::now();
#endif

    std::vector<Matrix4> transforms;
    transforms.reserve(frame.instances.size());
    target.worldUnsigned16.resize(meshes.page_count());
    target.worldUnsigned32.resize(meshes.page_count());
    target.worldUnsigned16Groups.resize(meshes.page_count());
    target.worldUnsigned32Groups.resize(meshes.page_count());
    target.transformedUnsigned16.resize(meshes.page_count());
    target.transformedUnsigned32.resize(meshes.page_count());
    target.worldDraws = false;
    target.transformedDraws = false;

    for (std::size_t page = 0; page < meshes.page_count(); ++page) {
        target.worldUnsigned16[page].clear();
        target.worldUnsigned32[page].clear();
        target.worldUnsigned16Groups[page].clear();
        target.worldUnsigned32Groups[page].clear();
        target.transformedUnsigned16[page].clear();
        target.transformedUnsigned32[page].clear();
    }

    for (const RenderInstance& instance : frame.instances) {
        const VulkanMesh* mesh = meshes.find(instance.mesh);

        if (!mesh) {
            continue;
        }

        if (instance.worldSpace) {
            auto& pages = mesh->index_type() == VK_INDEX_TYPE_UINT16 ? target.worldUnsigned16 : target.worldUnsigned32;
            VulkanDraw draw = mesh->draw(0, instance.section, instance.level);
            draw.detailLevel = instance.level;
            draw.material = instance.material;
            pages[mesh->page()].push_back(draw);
            continue;
        }

        if (transforms.size() >= std::numeric_limits<std::uint32_t>::max()) {
            throw std::overflow_error("Vulkan draw batch exceeds the instance index range");
        }

        const std::uint32_t firstInstance = static_cast<std::uint32_t>(transforms.size());
        transforms.push_back(instance.transform);
        auto& pages =
            mesh->index_type() == VK_INDEX_TYPE_UINT16 ? target.transformedUnsigned16 : target.transformedUnsigned32;
        auto& commands = pages[mesh->page()];
        VulkanDraw draw = mesh->draw(firstInstance, instance.section, instance.level);
        draw.material = instance.material;
        commands.push_back(draw);
        target.transformedDraws = true;
    }

    for (const RenderSections& sections : frame.sections) {
        const VulkanMesh* mesh = meshes.find(sections.mesh);

        if (!mesh) {
            continue;
        }

        auto& pages = mesh->index_type() == VK_INDEX_TYPE_UINT16 ? target.worldUnsigned16 : target.worldUnsigned32;
        auto& draws = pages[mesh->page()];

        for (std::size_t section = 0; section < sections.levels.size(); ++section) {
            if ((sections.visibility & (std::uint16_t {1} << section)) == 0) {
                continue;
            }

            VulkanDraw draw = mesh->draw(0, section, sections.levels[section]);
            draw.detailLevel = sections.levels[section];
            draw.material = sections.material;
            draws.push_back(draw);
        }
    }

    std::vector<VkDrawIndexedIndirectCommand> indirectCommands;

    for (std::size_t page = 0; page < meshes.page_count(); ++page) {
        build_groups(target.worldUnsigned16[page], target.worldUnsigned16Groups[page], indirectCommands);
        build_groups(target.worldUnsigned32[page], target.worldUnsigned32Groups[page], indirectCommands);
    }

#ifdef NORTHREND_GPU_PROFILING
    target.worldCommands = indirectCommands.size();
    target.worldTriangles = 0;
    target.worldDrawCalls = 0;

    for (const VkDrawIndexedIndirectCommand& command : indirectCommands) {
        target.worldTriangles += static_cast<std::uint64_t>(command.indexCount) * command.instanceCount / 3;
    }

    for (std::size_t page = 0; page < meshes.page_count(); ++page) {
        const std::size_t groupCount =
            target.worldUnsigned16Groups[page].size() + target.worldUnsigned32Groups[page].size();
        const std::size_t commandCount = target.worldUnsigned16[page].size() + target.worldUnsigned32[page].size();
        target.worldDrawCalls += this->device.multi_draw_indirect() ? groupCount : commandCount;
    }
#endif

    const VkDeviceSize indirectSize = indirectCommands.size() * sizeof(VkDrawIndexedIndirectCommand);

    if (indirectSize != 0 && (!target.indirectBuffer || target.indirectBuffer->size() < indirectSize)) {
        const VkDeviceSize byteCapacity =
            capacity(target.indirectBuffer ? target.indirectBuffer->size() : 0, indirectSize);
        target.indirectBuffer = std::make_unique<VulkanBuffer>(this->device, byteCapacity,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    if (indirectSize != 0) {
        const VulkanTransferSegment segment {
            .data = std::as_bytes(std::span<const VkDrawIndexedIndirectCommand>(indirectCommands)),
            .offset = 0
        };
        this->device.upload_buffer(target.indirectBuffer->handle(), 0, indirectSize, std::span(&segment, 1));
        target.worldDraws = true;
    }

    const VkDeviceSize required = transforms.size() * sizeof(Matrix4);

    if (required != 0 && (!target.transformBuffer || target.transformBuffer->size() < required)) {
        const VkDeviceSize byteCapacity =
            capacity(target.transformBuffer ? target.transformBuffer->size() : 0, required);
        target.transformBuffer = std::make_unique<VulkanBuffer>(this->device, byteCapacity,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    if (required != 0) {
        const VulkanTransferSegment segment {
            .data = std::as_bytes(std::span<const Matrix4>(transforms)),
            .offset = 0
        };
        this->device.upload_buffer(target.transformBuffer->handle(), 0, required, std::span(&segment, 1));
    }

    target.instanceRevision = frame.instancesRevision;
    target.meshRevision = meshRevision;
    target.initialized = true;

#ifdef NORTHREND_GPU_PROFILING
    const auto rebuildEnd = std::chrono::steady_clock::now();
    const double milliseconds = std::chrono::duration<double, std::milli>(rebuildEnd - rebuildStart).count();

    if (const auto summary = rebuildTiming.record(milliseconds)) {
        Timing::write(std::cout, "CPU draw batch rebuild", *summary);
    }
#endif
}

bool VulkanDrawBatch::has_world(const std::size_t imageIndex) const noexcept {
    return this->stores[imageIndex].worldDraws;
}

bool VulkanDrawBatch::has_transformed(const std::size_t imageIndex) const noexcept {
    return this->stores[imageIndex].transformedDraws;
}

void VulkanDrawBatch::draw_world(const VkCommandBuffer commandBuffer, const std::size_t imageIndex,
    const VulkanMeshStore& meshes, const VulkanMaterialStore& materials,
    const VkPipelineLayout pipelineLayout) const noexcept {
    const Store& source = this->stores[imageIndex];

#ifdef NORTHREND_GPU_PROFILING
    static Interval drawInterval;

    if (drawInterval.elapsed()) {
        std::cout << "Draw: calls=" << source.worldDrawCalls << " commands=" << source.worldCommands
                  << " triangles=" << source.worldTriangles << '\n';
    }
#endif

    if (!source.worldDraws || !source.indirectBuffer) {
        return;
    }

    constexpr VkDeviceSize offset = 0;

    for (std::size_t page = 0; page < meshes.page_count(); ++page) {
        const auto& unsigned16 = source.worldUnsigned16[page];
        const auto& unsigned32 = source.worldUnsigned32[page];

        if (unsigned16.empty() && unsigned32.empty()) {
            continue;
        }

        const VkBuffer meshBuffer = meshes.buffer(page);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &meshBuffer, &offset);

        if (!unsigned16.empty()) {
            vkCmdBindIndexBuffer(commandBuffer, meshBuffer, 0, VK_INDEX_TYPE_UINT16);
            this->draw_world_groups(commandBuffer, source.worldUnsigned16Groups[page],
                source.indirectBuffer->handle(), materials, pipelineLayout);
        }

        if (!unsigned32.empty()) {
            vkCmdBindIndexBuffer(commandBuffer, meshBuffer, 0, VK_INDEX_TYPE_UINT32);
            this->draw_world_groups(commandBuffer, source.worldUnsigned32Groups[page],
                source.indirectBuffer->handle(), materials, pipelineLayout);
        }
    }
}

void VulkanDrawBatch::draw_transformed(const VkCommandBuffer commandBuffer, const std::size_t imageIndex,
    const VulkanMeshStore& meshes, const VulkanMaterialStore& materials,
    VulkanMeshPipeline& pipeline) const noexcept {
    const Store& source = this->stores[imageIndex];

    if (!source.transformedDraws || !source.transformBuffer) {
        return;
    }

    constexpr VkDeviceSize offsets[] {0, 0};
    std::uint32_t activeBlendMode = std::numeric_limits<std::uint32_t>::max();

    for (std::size_t page = 0; page < meshes.page_count(); ++page) {
        const auto& unsigned16 = source.transformedUnsigned16[page];
        const auto& unsigned32 = source.transformedUnsigned32[page];

        if (unsigned16.empty() && unsigned32.empty()) {
            continue;
        }

        const VkBuffer vertexBuffers[] {meshes.buffer(page), source.transformBuffer->handle()};
        vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);

        if (!unsigned16.empty()) {
            vkCmdBindIndexBuffer(commandBuffer, vertexBuffers[0], 0, VK_INDEX_TYPE_UINT16);
            draw_transformed_group(
                commandBuffer, imageIndex, unsigned16, materials, pipeline, activeBlendMode);
        }

        if (!unsigned32.empty()) {
            vkCmdBindIndexBuffer(commandBuffer, vertexBuffers[0], 0, VK_INDEX_TYPE_UINT32);
            draw_transformed_group(
                commandBuffer, imageIndex, unsigned32, materials, pipeline, activeBlendMode);
        }
    }
}

VkDeviceSize VulkanDrawBatch::capacity(const VkDeviceSize current, const VkDeviceSize required) {
    VkDeviceSize result = current == 0 ? InitialCapacity : current;

    while (result < required) {
        if (result > std::numeric_limits<VkDeviceSize>::max() / 2) {
            return required;
        }

        result *= 2;
    }

    return result;
}

void VulkanDrawBatch::build_groups(std::vector<VulkanDraw>& draws, std::vector<WorldGroup>& groups,
    std::vector<VkDrawIndexedIndirectCommand>& commands) {
    std::ranges::sort(draws, [](const VulkanDraw& left, const VulkanDraw& right) {
        if (left.material.id() != right.material.id()) {
            return left.material.id() < right.material.id();
        }

        return left.detailLevel < right.detailLevel;
    });

    for (const VulkanDraw& draw : draws) {
        if (draw.indexCount == 0) {
            continue;
        }

        if (commands.size() >= std::numeric_limits<std::uint32_t>::max()) {
            throw std::overflow_error("Vulkan indirect draw count exceeds the supported range");
        }

        if (groups.empty() || groups.back().material != draw.material ||
            groups.back().detailLevel != draw.detailLevel) {
            groups.push_back(WorldGroup {
                .material = draw.material,
                .firstCommand = static_cast<std::uint32_t>(commands.size()),
                .commandCount = 0,
                .detailLevel = static_cast<std::uint8_t>(draw.detailLevel)
            });
        }

        commands.push_back(VkDrawIndexedIndirectCommand {
            .indexCount = draw.indexCount,
            .instanceCount = 1,
            .firstIndex = draw.firstIndex,
            .vertexOffset = draw.vertexOffset,
            .firstInstance = draw.firstInstance
        });
        ++groups.back().commandCount;
    }
}

void VulkanDrawBatch::draw_world_groups(const VkCommandBuffer commandBuffer,
    const std::vector<WorldGroup>& groups, const VkBuffer indirectBuffer, const VulkanMaterialStore& materials,
    const VkPipelineLayout pipelineLayout) const noexcept {
    static constexpr std::array<float, 3> TextureMipBias {0.0F, 0.75F, 1.5F};
    static constexpr std::array<float, 3> AlphaMipBias {0.0F, 1.0F, 2.0F};
    static constexpr std::uint32_t CommandStride = sizeof(VkDrawIndexedIndirectCommand);

    for (const WorldGroup& group : groups) {
        const std::size_t level = std::min<std::size_t>(group.detailLevel, TextureMipBias.size() - 1);
        const std::array<float, 2> detail {TextureMipBias[level], AlphaMipBias[level]};
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(detail), detail.data());
        materials.bind(commandBuffer, pipelineLayout, group.material);
        VkDeviceSize offset = static_cast<VkDeviceSize>(group.firstCommand) * CommandStride;

        if (this->device.multi_draw_indirect()) {
            vkCmdDrawIndexedIndirect(commandBuffer, indirectBuffer, offset, group.commandCount, CommandStride);
            continue;
        }

        for (std::uint32_t command = 0; command < group.commandCount; ++command) {
            vkCmdDrawIndexedIndirect(commandBuffer, indirectBuffer, offset, 1, CommandStride);
            offset += CommandStride;
        }
    }
}

void VulkanDrawBatch::draw_transformed_group(const VkCommandBuffer commandBuffer, const std::size_t imageIndex,
    const std::vector<VulkanDraw>& draws, const VulkanMaterialStore& materials, VulkanMeshPipeline& pipeline,
    std::uint32_t& activeBlendMode) noexcept {
    const VkPipelineLayout pipelineLayout = pipeline.pipeline_layout();

    for (const VulkanDraw& draw : draws) {
        const MaterialProperties properties = materials.properties(draw.material);

        if (properties.blendMode != activeBlendMode) {
            pipeline.bind_transformed(commandBuffer, imageIndex, properties.blendMode);
            activeBlendMode = properties.blendMode;
        }

        const VulkanMaterialConstants constants {
            .shader = properties.shader,
            .blendMode = properties.blendMode,
            .flags = properties.flags,
            .textureCount = properties.textureCount
        };
        vkCmdPushConstants(
            commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants), &constants);
        materials.bind(commandBuffer, pipelineLayout, draw.material);
        vkCmdDrawIndexed(commandBuffer, draw.indexCount, 1, draw.firstIndex, draw.vertexOffset, draw.firstInstance);
    }
}

}
