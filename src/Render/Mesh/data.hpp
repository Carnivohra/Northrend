#pragma once

#include "Render/Mesh/level.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace Northrend {

class MeshData final {
public:
    MeshData(std::span<const std::byte> vertices, std::span<const std::byte> indices, std::uint32_t indexCount,
        MeshIndexType indexType);
    MeshData(std::span<const std::byte> vertices, std::span<const MeshLevel> levels, MeshIndexType indexType);

    [[nodiscard]] std::span<const std::byte> vertices() const noexcept;
    [[nodiscard]] std::span<const MeshLevel> levels() const noexcept;
    [[nodiscard]] MeshIndexType index_type() const noexcept;

private:
    std::span<const std::byte> vertexData;
    std::vector<MeshLevel> meshLevels;
    MeshIndexType indexType;
};

}
