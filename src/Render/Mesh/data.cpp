#include "Render/Mesh/data.hpp"

#include <array>
#include <limits>
#include <stdexcept>

namespace Northrend {

MeshData::MeshData(std::span<const std::byte> vertices, std::span<const std::byte> indices,
    const std::uint32_t indexCount, const MeshIndexType indexType) :
    MeshData(vertices, std::array {MeshLevel {.indices = indices, .indexCount = indexCount}}, indexType) {}

MeshData::MeshData(
    std::span<const std::byte> vertices, const std::span<const MeshLevel> levels, const MeshIndexType indexType) :
    MeshData(vertices, levels, levels.size(), indexType) {}

MeshData::MeshData(std::span<const std::byte> vertices, const std::span<const MeshLevel> levels,
    const std::size_t levelsPerSection, const MeshIndexType indexType) :
    vertexData(vertices), meshLevels(levels.begin(), levels.end()), levelsPerSection(levelsPerSection),
    indexType(indexType) {
    if (this->vertexData.empty() || this->meshLevels.empty() || this->levelsPerSection == 0 ||
        this->meshLevels.size() % this->levelsPerSection != 0) {
        throw std::invalid_argument("Mesh data must not be empty");
    }

    const std::size_t indexSize =
        this->indexType == MeshIndexType::unsigned16 ? sizeof(std::uint16_t) : sizeof(std::uint32_t);

    bool hasIndices = false;

    for (const MeshLevel& level : this->meshLevels) {
        if (level.indexCount > std::numeric_limits<std::size_t>::max() / indexSize ||
            level.indices.size_bytes() != static_cast<std::size_t>(level.indexCount) * indexSize) {
            throw std::invalid_argument("Mesh level index data has an invalid size");
        }

        hasIndices = hasIndices || level.indexCount != 0;
    }

    if (!hasIndices) {
        throw std::invalid_argument("Mesh data contains no indices");
    }
}

std::span<const std::byte> MeshData::vertices() const noexcept {
    return this->vertexData;
}

std::span<const MeshLevel> MeshData::levels() const noexcept {
    return this->meshLevels;
}

std::size_t MeshData::section_count() const noexcept {
    return this->meshLevels.size() / this->levelsPerSection;
}

std::size_t MeshData::level_count() const noexcept {
    return this->levelsPerSection;
}

MeshIndexType MeshData::index_type() const noexcept {
    return this->indexType;
}

}
