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
#include <tuple>

namespace Northrend {

VulkanDrawBatch::VulkanDrawBatch(VulkanDevice& device) noexcept : device(device) {}

VulkanDrawBatch::~VulkanDrawBatch() = default;

void VulkanDrawBatch::allocate(const std::size_t count) {
    this->stores.clear();
    this->stores.resize(count);
}

void VulkanDrawBatch::release() noexcept {
    this->stores.clear();
    this->prepared = {};
}

void VulkanDrawBatch::update(const std::size_t imageIndex, const RenderFrame& frame, const VulkanMeshStore& meshes,
    const VulkanMaterialStore& materials) {
    Store& target = this->stores.at(imageIndex);
    const std::uint64_t meshRevision = meshes.revision();
    const bool prepareInstances = !this->prepared.initialized ||
        this->prepared.instanceRevision != frame.instancesRevision || this->prepared.meshRevision != meshRevision;
    const bool prepareVisibility = prepareInstances || this->prepared.visibilityRevision != frame.visibilityRevision;
    const bool updateTransformed = !target.transformedInitialized ||
        target.instanceRevision != frame.instancesRevision || target.visibilityRevision != frame.visibilityRevision ||
        target.transformedMeshRevision != meshRevision;
    bool updateWorld = !target.worldInitialized || target.sectionsRevision != frame.sectionsRevision ||
        target.worldMeshRevision != meshRevision || (this->prepared.hasWorldInstances &&
            (target.worldInstanceRevision != frame.instancesRevision ||
                target.worldVisibilityRevision != frame.visibilityRevision));

    if (!prepareInstances && !prepareVisibility && !updateTransformed && !updateWorld) {
        return;
    }

#ifdef NORTHREND_GPU_PROFILING
    static Timing rebuildTiming;
    const auto rebuildStart = std::chrono::steady_clock::now();
#endif

    if (prepareInstances) {
        this->prepared.unsigned16.clear();
        this->prepared.unsigned32.clear();
        this->prepared.unsigned16.resize(meshes.page_count());
        this->prepared.unsigned32.resize(meshes.page_count());
        this->prepared.hasWorldInstances = false;

        for (std::size_t index = 0; index < frame.instances.size(); ++index) {
            const RenderInstance& instance = frame.instances[index];
            const VulkanMesh* mesh = meshes.find(instance.mesh);

            if (!mesh) {
                continue;
            }

            if (instance.worldSpace) {
                this->prepared.hasWorldInstances = true;
                continue;
            }

            auto& pending =
                mesh->index_type() == VK_INDEX_TYPE_UINT16 ? this->prepared.unsigned16 : this->prepared.unsigned32;
            VulkanDraw draw = mesh->draw(0, instance.section, instance.level);
            draw.material = instance.material;
            const MaterialProperties properties = materials.properties(instance.material);
            pending[mesh->page()].push_back(VulkanInstanceDraw {
                .draw = draw,
                .transform = instance.transform,
                .blendMode = properties.blendMode,
                .visibilityIndex = index,
                .doubleSided = (properties.flags & 0x4U) != 0
            });
        }

        for (std::size_t page = 0; page < meshes.page_count(); ++page) {
            sort_instances(this->prepared.unsigned16[page]);
            sort_instances(this->prepared.unsigned32[page]);
        }

        this->prepared.instanceRevision = frame.instancesRevision;
        this->prepared.meshRevision = meshRevision;
        this->prepared.initialized = true;
    }

    updateWorld = updateWorld || (this->prepared.hasWorldInstances &&
        (target.worldInstanceRevision != frame.instancesRevision ||
            target.worldVisibilityRevision != frame.visibilityRevision));

    if (prepareVisibility) {
        this->prepared.visibleUnsigned16.clear();
        this->prepared.visibleUnsigned32.clear();
        this->prepared.visibleUnsigned16.resize(meshes.page_count());
        this->prepared.visibleUnsigned32.resize(meshes.page_count());
        this->prepared.transforms.clear();
        this->prepared.transforms.reserve(frame.instances.size());

        for (std::size_t page = 0; page < meshes.page_count(); ++page) {
            build_instances(this->prepared.unsigned16[page], frame.instanceVisibility,
                this->prepared.visibleUnsigned16[page], this->prepared.transforms);
            build_instances(this->prepared.unsigned32[page], frame.instanceVisibility,
                this->prepared.visibleUnsigned32[page], this->prepared.transforms);
        }

        this->prepared.visibilityRevision = frame.visibilityRevision;
    }

    if (updateWorld) {
        target.worldUnsigned16.clear();
        target.worldUnsigned32.clear();
        target.worldUnsigned16Groups.clear();
        target.worldUnsigned32Groups.clear();
        target.worldUnsigned16.resize(meshes.page_count());
        target.worldUnsigned32.resize(meshes.page_count());
        target.worldUnsigned16Groups.resize(meshes.page_count());
        target.worldUnsigned32Groups.resize(meshes.page_count());
        target.worldDraws = false;

        for (std::size_t index = 0; index < frame.instances.size(); ++index) {
            const RenderInstance& instance = frame.instances[index];

            if (!instance.worldSpace ||
                (index < frame.instanceVisibility.size() && frame.instanceVisibility[index] == 0)) {
                continue;
            }

            const VulkanMesh* mesh = meshes.find(instance.mesh);

            if (!mesh) {
                continue;
            }

            auto& pages = mesh->index_type() == VK_INDEX_TYPE_UINT16 ? target.worldUnsigned16 : target.worldUnsigned32;
            VulkanDraw draw = mesh->draw(0, instance.section, instance.level);
            draw.detailLevel = instance.level;
            draw.material = instance.material;
            pages[mesh->page()].push_back(draw);
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

        target.sectionsRevision = frame.sectionsRevision;
        target.worldInstanceRevision = frame.instancesRevision;
        target.worldVisibilityRevision = frame.visibilityRevision;
        target.worldMeshRevision = meshRevision;
        target.worldInitialized = true;
    }

    if (updateTransformed) {
        target.transformedDraws = !this->prepared.transforms.empty();
        const VkDeviceSize required = this->prepared.transforms.size() * sizeof(Matrix4);

        if (required != 0 && (!target.transformBuffer || target.transformBuffer->size() < required)) {
            const VkDeviceSize byteCapacity =
                capacity(target.transformBuffer ? target.transformBuffer->size() : 0, required);
            target.transformBuffer = std::make_unique<VulkanBuffer>(this->device, byteCapacity,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }

        if (required != 0) {
            const VulkanTransferSegment segment {
                .data = std::as_bytes(std::span<const Matrix4>(this->prepared.transforms)),
                .offset = 0
            };
            this->device.upload_buffer(target.transformBuffer->handle(), 0, required, std::span(&segment, 1));
        }

        target.instanceRevision = frame.instancesRevision;
        target.visibilityRevision = frame.visibilityRevision;
        target.transformedMeshRevision = meshRevision;
        target.transformedInitialized = true;
    }

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

#ifdef NORTHREND_GPU_PROFILING
    static Interval transformedInterval;

    if (transformedInterval.elapsed()) {
        std::uint64_t calls = 0;
        std::uint64_t triangles = 0;
        std::uint64_t materialSwitches = 0;
        std::uint64_t pipelineSwitches = 0;
        std::uint64_t cullingSwitches = 0;
        MaterialHandle activeMaterial;
        std::uint32_t activePipeline = std::numeric_limits<std::uint32_t>::max();
        bool activeDoubleSided = false;

        for (std::size_t page = 0; page < meshes.page_count(); ++page) {
            const std::vector<VulkanDraw>* groups[] {
                &this->prepared.visibleUnsigned16[page], &this->prepared.visibleUnsigned32[page]
            };

            for (const std::vector<VulkanDraw>* group : groups) {
                for (const VulkanDraw& draw : *group) {
                    triangles += static_cast<std::uint64_t>(draw.indexCount) * draw.instanceCount / 3;

                    ++calls;

                    if (draw.material != activeMaterial) {
                        activeMaterial = draw.material;
                        ++materialSwitches;
                    }

                    const MaterialProperties properties = materials.properties(draw.material);
                    const bool doubleSided = (properties.flags & 0x4U) != 0;
                    const std::uint32_t pipeline = properties.blendMode;

                    if (pipeline != activePipeline) {
                        activePipeline = pipeline;
                        ++pipelineSwitches;
                    }

                    if (doubleSided != activeDoubleSided) {
                        activeDoubleSided = doubleSided;
                        ++cullingSwitches;
                    }
                }
            }
        }

        std::cout << "WMO draw: calls=" << calls << " triangles=" << triangles
                  << " materials=" << materialSwitches << " pipelines=" << pipelineSwitches
                  << " culling=" << cullingSwitches << '\n';
    }
#endif

    constexpr VkDeviceSize offsets[] {0, 0};
    std::uint32_t activeBlendMode = std::numeric_limits<std::uint32_t>::max();
    bool activeDoubleSided = false;

    for (std::size_t page = 0; page < meshes.page_count(); ++page) {
        const auto& unsigned16 = this->prepared.visibleUnsigned16[page];
        const auto& unsigned32 = this->prepared.visibleUnsigned32[page];

        if (unsigned16.empty() && unsigned32.empty()) {
            continue;
        }

        const VkBuffer vertexBuffers[] {meshes.buffer(page), source.transformBuffer->handle()};
        vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);

        if (!unsigned16.empty()) {
            vkCmdBindIndexBuffer(commandBuffer, vertexBuffers[0], 0, VK_INDEX_TYPE_UINT16);

            draw_transformed_group(
                commandBuffer, imageIndex, unsigned16, materials, pipeline, activeBlendMode, activeDoubleSided);
        }

        if (!unsigned32.empty()) {
            vkCmdBindIndexBuffer(commandBuffer, vertexBuffers[0], 0, VK_INDEX_TYPE_UINT32);
            draw_transformed_group(
                commandBuffer, imageIndex, unsigned32, materials, pipeline, activeBlendMode, activeDoubleSided);
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
            .instanceCount = draw.instanceCount,
            .firstIndex = draw.firstIndex,
            .vertexOffset = draw.vertexOffset,
            .firstInstance = draw.firstInstance
        });
        ++groups.back().commandCount;
    }
}

void VulkanDrawBatch::sort_instances(std::vector<VulkanInstanceDraw>& instances) {
    std::stable_sort(instances.begin(), instances.end(), [](const VulkanInstanceDraw& left,
                                                            const VulkanInstanceDraw& right) {
        const bool leftOpaque = left.blendMode < 2;
        const bool rightOpaque = right.blendMode < 2;

        if (leftOpaque != rightOpaque) {
            return leftOpaque;
        }

        if (!leftOpaque) {
            return false;
        }

        return std::tie(left.blendMode, left.doubleSided, left.draw.material, left.draw.vertexOffset,
                   left.draw.firstIndex, left.draw.indexCount, left.draw.detailLevel) <
            std::tie(right.blendMode, right.doubleSided, right.draw.material, right.draw.vertexOffset,
                right.draw.firstIndex, right.draw.indexCount, right.draw.detailLevel);
    });
}

void VulkanDrawBatch::build_instances(const std::vector<VulkanInstanceDraw>& instances,
    const std::span<const std::uint8_t> visibility, std::vector<VulkanDraw>& draws,
    std::vector<Matrix4>& transforms) {
    draws.clear();

    for (const VulkanInstanceDraw& instance : instances) {
        if (instance.visibilityIndex < visibility.size() && visibility[instance.visibilityIndex] == 0) {
            continue;
        }

        if (transforms.size() >= std::numeric_limits<std::uint32_t>::max()) {
            throw std::overflow_error("Vulkan draw batch exceeds the instance index range");
        }

        const std::uint32_t firstInstance = static_cast<std::uint32_t>(transforms.size());
        transforms.push_back(instance.transform);
        const VulkanDraw& source = instance.draw;
        const bool instanced = instance.blendMode < 2 && !draws.empty() && draws.back().material == source.material &&
            draws.back().vertexOffset == source.vertexOffset && draws.back().firstIndex == source.firstIndex &&
            draws.back().indexCount == source.indexCount && draws.back().detailLevel == source.detailLevel &&
            draws.back().firstInstance + draws.back().instanceCount == firstInstance &&
            draws.back().instanceCount != std::numeric_limits<std::uint32_t>::max();

        if (instanced) {
            ++draws.back().instanceCount;
            continue;
        }

        VulkanDraw draw = source;
        draw.firstInstance = firstInstance;
        draw.instanceCount = 1;
        draws.push_back(draw);
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
    std::uint32_t& activeBlendMode, bool& activeDoubleSided) noexcept {
    const VkPipelineLayout pipelineLayout = pipeline.pipeline_layout();

    for (const VulkanDraw& draw : draws) {
        const MaterialProperties properties = materials.properties(draw.material);
        const bool doubleSided = (properties.flags & 0x4U) != 0;

        if (properties.blendMode != activeBlendMode) {
            pipeline.bind_transformed(commandBuffer, imageIndex, properties.blendMode, doubleSided);
            activeBlendMode = properties.blendMode;
            activeDoubleSided = doubleSided;
        } else if (doubleSided != activeDoubleSided) {
            if (pipeline.dynamic_culling()) {
                pipeline.set_double_sided(commandBuffer, doubleSided);
            } else {
                pipeline.bind_transformed(commandBuffer, imageIndex, properties.blendMode, doubleSided);
            }

            activeDoubleSided = doubleSided;
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
        vkCmdDrawIndexed(commandBuffer, draw.indexCount, draw.instanceCount, draw.firstIndex, draw.vertexOffset,
            draw.firstInstance);
    }
}

}
