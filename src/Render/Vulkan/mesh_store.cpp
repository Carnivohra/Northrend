#include "Render/Vulkan/mesh_store.hpp"

#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/mesh.hpp"

#include <limits>
#include <stdexcept>

namespace Northrend {

VulkanMeshStore::VulkanMeshStore(VulkanDevice& device) noexcept
    : device(device) {}

VulkanMeshStore::~VulkanMeshStore() = default;

MeshHandle VulkanMeshStore::create(
    std::span<const std::byte> vertices,
    std::span<const std::byte> indices,
    std::uint32_t indexCount,
    VkIndexType indexType) {
    if (this->meshes.size() == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Vulkan mesh handle space is exhausted");
    }

    auto mesh = std::make_unique<VulkanMesh>(
        this->device,
        vertices,
        indices,
        indexCount,
        indexType);
    this->meshes.push_back(std::move(mesh));

    return MeshHandle(static_cast<std::uint64_t>(this->meshes.size()));
}

const VulkanMesh* VulkanMeshStore::find(MeshHandle handle) const noexcept {
    if (!handle) {
        return nullptr;
    }

    const std::uint64_t index = handle.id() - 1;

    if (index >= this->meshes.size()) {
        return nullptr;
    }

    return this->meshes[static_cast<std::size_t>(index)].get();
}

}
