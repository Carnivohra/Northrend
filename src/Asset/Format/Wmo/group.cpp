#include "Asset/Format/Wmo/group.hpp"

#include "Asset/Chunk/id.hpp"
#include "Asset/Chunk/reader.hpp"
#include "Asset/data.hpp"

#include <bit>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace Northrend {

WmoGroup::WmoGroup(std::shared_ptr<const AssetData> data) : data(std::move(data)) {
    static constexpr ChunkId VersionId {'R', 'E', 'V', 'M'};

    if (this->data == nullptr || this->data->empty()) {
        throw std::invalid_argument("WMO group data is empty");
    }

    ChunkReader reader(this->data->bytes());
    bool versionFound = false;

    while (const std::optional<Chunk> chunk = reader.next()) {
        if (chunk->id() != VersionId) {
            continue;
        }

        if (versionFound || chunk->size() != sizeof(std::uint32_t)) {
            throw std::runtime_error("WMO group version chunk is invalid");
        }

        this->fileVersion = read_uint32(chunk->bytes(), 0);
        versionFound = true;
    }

    if (!versionFound) {
        throw std::runtime_error("WMO group is missing its format version");
    }

    switch (this->fileVersion) {
    case 17:
        this->parse_version17();
        break;
    default:
        throw std::runtime_error("Unsupported WMO group version: " + std::to_string(this->fileVersion));
    }

    this->data.reset();
}

std::uint32_t WmoGroup::version() const noexcept {
    return this->fileVersion;
}

const WmoGroup::Header& WmoGroup::header() const noexcept {
    return this->groupHeader;
}

std::span<const WmoGroup::Polygon> WmoGroup::polygons() const noexcept {
    return this->polygonData;
}

std::span<const std::uint16_t> WmoGroup::indices() const noexcept {
    return this->indexData;
}

std::span<const Vector3> WmoGroup::vertices() const noexcept {
    return this->vertexData;
}

std::span<const Vector3> WmoGroup::normals() const noexcept {
    return this->normalData;
}

std::size_t WmoGroup::texture_coordinate_set_count() const noexcept {
    return this->textureCoordinateData.size();
}

std::span<const WmoGroup::TextureCoordinate> WmoGroup::texture_coordinates(const std::size_t set) const {
    return this->textureCoordinateData.at(set);
}

std::span<const WmoGroup::Batch> WmoGroup::batches() const noexcept {
    return this->batchData;
}

std::int16_t WmoGroup::read_int16(const std::span<const std::byte> bytes, const std::size_t offset) {
    return std::bit_cast<std::int16_t>(read_uint16(bytes, offset));
}

std::uint16_t WmoGroup::read_uint16(const std::span<const std::byte> bytes, const std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint16_t)) {
        throw std::out_of_range("WMO group value exceeds the available data");
    }

    return std::to_integer<std::uint16_t>(bytes[offset]) |
        std::to_integer<std::uint16_t>(bytes[offset + 1]) << 8U;
}

std::uint32_t WmoGroup::read_uint32(const std::span<const std::byte> bytes, const std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint32_t)) {
        throw std::out_of_range("WMO group value exceeds the available data");
    }

    return std::to_integer<std::uint32_t>(bytes[offset]) | std::to_integer<std::uint32_t>(bytes[offset + 1]) << 8U |
        std::to_integer<std::uint32_t>(bytes[offset + 2]) << 16U |
        std::to_integer<std::uint32_t>(bytes[offset + 3]) << 24U;
}

float WmoGroup::read_float(const std::span<const std::byte> bytes, const std::size_t offset) {
    return std::bit_cast<float>(read_uint32(bytes, offset));
}

Vector3 WmoGroup::read_vector(const std::span<const std::byte> bytes, const std::size_t offset) {
    return Vector3 {
        .x = read_float(bytes, offset),
        .y = read_float(bytes, offset + sizeof(float)),
        .z = read_float(bytes, offset + sizeof(float) * 2)
    };
}

}
