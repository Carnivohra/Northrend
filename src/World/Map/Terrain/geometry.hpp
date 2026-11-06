#pragma once

#include "Render/Mesh/vertex.hpp"

#include <array>
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
    static constexpr std::size_t LevelCount = 3;

    explicit TerrainGeometry(const Adt& adt);

    [[nodiscard]] std::span<const MeshVertex> vertices() const noexcept;
    [[nodiscard]] std::span<const std::uint16_t> indices(std::size_t level) const;

private:
    static constexpr std::size_t CellsPerChunk = 8;
    static constexpr std::size_t VerticesPerChunk = 145;
    static constexpr float CellSize = 4.1666665F;
    static constexpr float HalfCellSize = CellSize * 0.5F;

    void append_vertices(const Adt& adt, std::size_t chunkX, std::size_t chunkY);
    void append_indices(const Adt& adt, std::size_t chunkX, std::size_t chunkY, std::size_t level);
    static void append_cell(
        std::vector<std::uint16_t>& indices, std::uint16_t chunkOffset, std::size_t row, std::size_t column);
    [[nodiscard]] static bool has_hole(std::uint32_t holes, std::size_t row, std::size_t column, std::size_t step);

    std::vector<MeshVertex> terrainVertices;
    std::array<std::vector<std::uint16_t>, LevelCount> terrainIndices;
};

}
