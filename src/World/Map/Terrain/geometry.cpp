#include "World/Map/Terrain/geometry.hpp"

#include "Asset/Format/adt.hpp"

#include <algorithm>
#include <limits>

namespace Northrend {

static_assert(TerrainGeometry::VertexCount - 1 <= std::numeric_limits<std::uint16_t>::max());

TerrainGeometry::TerrainGeometry(const Adt& adt) {
    this->terrainVertices.reserve(VertexCount);

    for (std::size_t level = 0; level < LevelCount; ++level) {
        const std::size_t step = std::size_t {1} << level;
        const std::size_t cells = CellsPerChunk / step;
        const std::size_t indicesPerCell = level == 0 ? 12 : 6;
        this->terrainIndices[level].reserve(Adt::ChunkCount * cells * cells * indicesPerCell);
    }

    for (std::size_t chunkY = 0; chunkY < Adt::Size; ++chunkY) {
        for (std::size_t chunkX = 0; chunkX < Adt::Size; ++chunkX) {
            this->append_vertices(adt, chunkX, chunkY);

            for (std::size_t level = 0; level < LevelCount; ++level) {
                this->append_indices(adt, chunkX, chunkY, level);
            }
        }
    }
}

std::span<const MeshVertex> TerrainGeometry::vertices() const noexcept {
    return this->terrainVertices;
}

std::span<const std::uint16_t> TerrainGeometry::indices(const std::size_t level) const {
    return this->terrainIndices.at(level);
}

void TerrainGeometry::append_vertices(const Adt& adt, const std::size_t chunkX, const std::size_t chunkY) {
    const float baseX = adt.position_x(chunkX, chunkY);
    const float baseY = adt.position_y(chunkX, chunkY);

    for (std::size_t row = 0; row <= CellsPerChunk; ++row) {
        const float x = baseX - static_cast<float>(row) * CellSize;
        const std::size_t rowOffset = row * (CellsPerChunk * 2 + 1);

        for (std::size_t column = 0; column <= CellsPerChunk; ++column) {
            const std::size_t vertex = rowOffset + column;
            const std::array<std::int8_t, 3> normal = adt.normal(chunkX, chunkY, vertex);
            this->terrainVertices.push_back(MeshVertex {
                .position =
                    Vector3 {
                        .x = x,
                        .y = baseY - static_cast<float>(column) * CellSize,
                        .z = adt.height(chunkX, chunkY, vertex)
                    },
                .normal = {normal[0], normal[1], normal[2], 0}
            });
        }

        if (row == CellsPerChunk) {
            continue;
        }

        for (std::size_t column = 0; column < CellsPerChunk; ++column) {
            const std::size_t vertex = rowOffset + CellsPerChunk + 1 + column;
            const std::array<std::int8_t, 3> normal = adt.normal(chunkX, chunkY, vertex);
            this->terrainVertices.push_back(MeshVertex {
                .position =
                    Vector3 {
                        .x = x - HalfCellSize,
                        .y = baseY - static_cast<float>(column) * CellSize - HalfCellSize,
                        .z = adt.height(chunkX, chunkY, vertex)
                    },
                .normal = {normal[0], normal[1], normal[2], 0}
            });
        }
    }
}

void TerrainGeometry::append_indices(
    const Adt& adt, const std::size_t chunkX, const std::size_t chunkY, const std::size_t level) {
    const std::size_t step = std::size_t {1} << level;
    const std::size_t chunk = chunkY * Adt::Size + chunkX;
    const std::uint16_t chunkOffset = static_cast<std::uint16_t>(chunk * VerticesPerChunk);
    const std::uint32_t holes = adt.holes(chunkX, chunkY);

    for (std::size_t row = 0; row < CellsPerChunk; row += step) {
        const std::uint16_t rowOffset = static_cast<std::uint16_t>(chunkOffset + row * 17);

        for (std::size_t column = 0; column < CellsPerChunk; column += step) {
            if (level == 0) {
                if (!has_hole(holes, row, column, 1)) {
                    append_cell(this->terrainIndices[level], chunkOffset, row, column);
                }

                continue;
            }

            if (has_hole(holes, row, column, step)) {
                for (std::size_t fineRow = row; fineRow < row + step; ++fineRow) {
                    for (std::size_t fineColumn = column; fineColumn < column + step; ++fineColumn) {
                        if (!has_hole(holes, fineRow, fineColumn, 1)) {
                            append_cell(this->terrainIndices[level], chunkOffset, fineRow, fineColumn);
                        }
                    }
                }

                continue;
            }

            const std::uint16_t topLeft = static_cast<std::uint16_t>(rowOffset + column);
            const std::uint16_t topRight = static_cast<std::uint16_t>(topLeft + step);
            const std::uint16_t bottomLeft = static_cast<std::uint16_t>(topLeft + step * 17);
            const std::uint16_t bottomRight = static_cast<std::uint16_t>(bottomLeft + step);

            this->terrainIndices[level].insert(
                this->terrainIndices[level].end(), {topLeft, bottomLeft, bottomRight, topLeft, bottomRight, topRight});
        }
    }
}

void TerrainGeometry::append_cell(std::vector<std::uint16_t>& indices, const std::uint16_t chunkOffset,
    const std::size_t row, const std::size_t column) {
    const std::uint16_t topLeft = static_cast<std::uint16_t>(chunkOffset + row * 17 + column);
    const std::uint16_t topRight = static_cast<std::uint16_t>(topLeft + 1);
    const std::uint16_t center = static_cast<std::uint16_t>(topLeft + 9);
    const std::uint16_t bottomLeft = static_cast<std::uint16_t>(topLeft + 17);
    const std::uint16_t bottomRight = static_cast<std::uint16_t>(topLeft + 18);

    indices.insert(indices.end(),
        {center, topLeft, bottomLeft, center, topRight, topLeft, center, bottomRight, topRight, center, bottomLeft,
            bottomRight});
}

bool TerrainGeometry::has_hole(
    const std::uint32_t holes, const std::size_t row, const std::size_t column, const std::size_t step) {
    const std::size_t lastRow = std::min(row + step, CellsPerChunk);
    const std::size_t lastColumn = std::min(column + step, CellsPerChunk);

    for (std::size_t cellRow = row; cellRow < lastRow; ++cellRow) {
        for (std::size_t cellColumn = column; cellColumn < lastColumn; ++cellColumn) {
            const std::size_t hole = (cellRow / 2) * 4 + cellColumn / 2;

            if ((holes & (std::uint32_t {1} << hole)) != 0) {
                return true;
            }
        }
    }

    return false;
}

}
