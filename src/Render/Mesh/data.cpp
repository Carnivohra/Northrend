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
    vertexData(vertices), meshLevels(levels.begin(), levels.end()), indexType(indexType) {
    if (this->vertexData.empty() || this->meshLevels.empty()) {
        throw std::invalid_argument("Mesh data must not be empty");
    }

    const std::size_t indexSize =
        this->indexType == MeshIndexType::unsigned16 ? sizeof(std::uint16_t) : sizeof(std::uint32_t);

    for (const MeshLevel& level : this->meshLevels) {
        if (level.indices.empty() || level.indexCount == 0 ||
            level.indexCount > std::numeric_limits<std::size_t>::max() / indexSize ||
            level.indices.size_bytes() != static_cast<std::size_t>(level.indexCount) * indexSize) {
            throw std::invalid_argument("Mesh level index data has an invalid size");
        }
    }
}

std::span<const std::byte> MeshData::vertices() const noexcept {
    return this->vertexData;
}

std::span<const MeshLevel> MeshData::levels() const noexcept {
    return this->meshLevels;
}

MeshIndexType MeshData::index_type() const noexcept {
    return this->indexType;
}

}
