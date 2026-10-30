#pragma once

#include "Math/vector.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Northrend {

class Adt;

class TerrainGeometry final {
public:
    static constexpr std::size_t VertexCount = 16 * 16 * 145;
    static constexpr std::size_t MaximumIndexCount = 16 * 16 * 8 * 8 * 12;

    explicit TerrainGeometry(const Adt& adt);

    [[nodiscard]] std::span<const Vector3> vertices() const noexcept;
    [[nodiscard]] std::span<const std::uint16_t> indices() const noexcept;

private:
    static constexpr std::size_t CellsPerChunk = 8;
    static constexpr std::size_t VerticesPerChunk = 145;
    static constexpr float CellSize = 4.1666665F;
    static constexpr float HalfCellSize = CellSize * 0.5F;

    void append_vertices(const Adt& adt, std::size_t chunkX, std::size_t chunkY);
    void append_indices(const Adt& adt, std::size_t chunkX, std::size_t chunkY);

    std::vector<Vector3> terrainVertices;
    std::vector<std::uint16_t> terrainIndices;
};

}
