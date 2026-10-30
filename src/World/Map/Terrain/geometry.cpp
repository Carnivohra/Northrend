#include "World/Map/Terrain/geometry.hpp"

#include "Asset/adt.hpp"

#include <limits>

namespace Northrend {

static_assert(TerrainGeometry::VertexCount - 1 <= std::numeric_limits<std::uint16_t>::max());

TerrainGeometry::TerrainGeometry(const Adt& adt) {
    this->terrainVertices.reserve(VertexCount);
    this->terrainIndices.reserve(MaximumIndexCount);

    for (std::size_t chunkY = 0; chunkY < Adt::Size; ++chunkY) {
        for (std::size_t chunkX = 0; chunkX < Adt::Size; ++chunkX) {
            this->append_vertices(adt, chunkX, chunkY);
            this->append_indices(adt, chunkX, chunkY);
        }
    }
}

std::span<const Vector3> TerrainGeometry::vertices() const noexcept {
    return this->terrainVertices;
}

std::span<const std::uint16_t> TerrainGeometry::indices() const noexcept {
    return this->terrainIndices;
}

void TerrainGeometry::append_vertices(const Adt& adt, const std::size_t chunkX, const std::size_t chunkY) {
    const float baseX = adt.position_x(chunkX, chunkY);
    const float baseY = adt.position_y(chunkX, chunkY);

    for (std::size_t row = 0; row <= CellsPerChunk; ++row) {
        const float x = baseX - static_cast<float>(row) * CellSize;
        const std::size_t rowOffset = row * (CellsPerChunk * 2 + 1);

        for (std::size_t column = 0; column <= CellsPerChunk; ++column) {
            this->terrainVertices.push_back(Vector3 {
                .x = x,
                .y = baseY - static_cast<float>(column) * CellSize,
                .z = adt.height(chunkX, chunkY, rowOffset + column)
            });
        }

        if (row == CellsPerChunk) {
            continue;
        }

        for (std::size_t column = 0; column < CellsPerChunk; ++column) {
            this->terrainVertices.push_back(Vector3 {
                .x = x - HalfCellSize,
                .y = baseY - static_cast<float>(column) * CellSize - HalfCellSize,
                .z = adt.height(chunkX, chunkY, rowOffset + CellsPerChunk + 1 + column)
            });
        }
    }
}

void TerrainGeometry::append_indices(const Adt& adt, const std::size_t chunkX, const std::size_t chunkY) {
    const std::uint32_t holes = adt.holes(chunkX, chunkY);
    const std::size_t chunk = chunkY * Adt::Size + chunkX;
    const std::uint16_t chunkOffset = static_cast<std::uint16_t>(chunk * VerticesPerChunk);

    for (std::size_t row = 0; row < CellsPerChunk; ++row) {
        const std::uint16_t rowOffset = static_cast<std::uint16_t>(chunkOffset + row * 17);

        for (std::size_t column = 0; column < CellsPerChunk; ++column) {
            const std::size_t hole = (row / 2) * 4 + column / 2;

            if ((holes & (std::uint32_t {1} << hole)) != 0) {
                continue;
            }

            const std::uint16_t topLeft = static_cast<std::uint16_t>(rowOffset + column);
            const std::uint16_t topRight = static_cast<std::uint16_t>(topLeft + 1);
            const std::uint16_t center = static_cast<std::uint16_t>(topLeft + 9);
            const std::uint16_t bottomLeft = static_cast<std::uint16_t>(topLeft + 17);
            const std::uint16_t bottomRight = static_cast<std::uint16_t>(topLeft + 18);

            this->terrainIndices.insert(this->terrainIndices.end(), {
                center, topLeft, bottomLeft,
                center, topRight, topLeft,
                center, bottomRight, topRight,
                center, bottomLeft, bottomRight
            });
        }
    }
}

}
