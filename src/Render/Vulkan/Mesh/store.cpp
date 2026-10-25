#include "Render/Vulkan/Mesh/store.hpp"

#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/mesh.hpp"

#include <limits>
#include <stdexcept>

namespace Northrend {

VulkanMeshStore::VulkanMeshStore(VulkanDevice& device) noexcept
    : device(device) {}

VulkanMeshStore::~VulkanMeshStore() = default;

void VulkanMeshStore::store(
    MeshHandle handle,
    std::span<const std::byte> vertices,
    std::span<const std::byte> indices,
    std::uint32_t indexCount,
    VkIndexType indexType) {
    if (!handle) {
        throw std::invalid_argument("Cannot store a Vulkan mesh without a handle");
    }

    const std::uint64_t rawIndex = handle.id() - 1;

    if (rawIndex >= std::numeric_limits<std::size_t>::max()) {
        throw std::overflow_error("Mesh handle exceeds the platform index range");
    }

    const std::size_t index = static_cast<std::size_t>(rawIndex);

    if (this->meshes.size() <= index) {
        this->meshes.resize(index + 1);
    }

    if (this->meshes[index]) {
        throw std::invalid_argument("A Vulkan mesh already exists for this handle");
    }

    this->meshes[index] = std::make_unique<VulkanMesh>(
        this->device,
        vertices,
        indices,
        indexCount,
        indexType);
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
