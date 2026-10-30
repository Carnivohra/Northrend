#include "Render/Mesh/data.hpp"

#include <limits>
#include <stdexcept>

namespace Northrend {

MeshData::MeshData(std::span<const std::byte> vertices, std::span<const std::byte> indices,
    const std::uint32_t indexCount, const MeshIndexType indexType) :
    vertexData(vertices), indexData(indices), indexCount(indexCount), indexType(indexType) {
    if (this->vertexData.empty() || this->indexData.empty() || this->indexCount == 0) {
        throw std::invalid_argument("Mesh data must not be empty");
    }

    const std::size_t indexSize = this->indexType == MeshIndexType::unsigned16 ? sizeof(std::uint16_t) :
        sizeof(std::uint32_t);

    if (this->indexCount > std::numeric_limits<std::size_t>::max() / indexSize ||
        this->indexData.size_bytes() != static_cast<std::size_t>(this->indexCount) * indexSize) {
        throw std::invalid_argument("Mesh index data has an invalid size");
    }
}

std::span<const std::byte> MeshData::vertices() const noexcept {
    return this->vertexData;
}

std::span<const std::byte> MeshData::indices() const noexcept {
    return this->indexData;
}

std::uint32_t MeshData::index_count() const noexcept {
    return this->indexCount;
}

MeshIndexType MeshData::index_type() const noexcept {
    return this->indexType;
}

}
