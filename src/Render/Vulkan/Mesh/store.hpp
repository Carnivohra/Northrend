#pragma once

#include "Render/Mesh/mesh.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace Northrend {

class VulkanDevice;
class VulkanMesh;

class VulkanMeshStore final {
public:
    explicit VulkanMeshStore(VulkanDevice& device) noexcept;
    ~VulkanMeshStore();

    VulkanMeshStore(const VulkanMeshStore&) = delete;
    VulkanMeshStore& operator=(const VulkanMeshStore&) = delete;

    void store(MeshHandle handle, std::span<const std::byte> vertices, std::span<const std::byte> indices,
        std::uint32_t indexCount, VkIndexType indexType);
    [[nodiscard]] const VulkanMesh* find(MeshHandle handle) const noexcept;
    [[nodiscard]] std::uint64_t revision() const noexcept;

private:
    VulkanDevice& device;
    std::vector<std::unique_ptr<VulkanMesh>> meshes;
    std::uint64_t currentRevision = 0;
};

}
