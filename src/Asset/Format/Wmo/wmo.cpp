#include "Asset/Format/Wmo/wmo.hpp"

#include "Asset/Chunk/id.hpp"
#include "Asset/Chunk/reader.hpp"
#include "Asset/data.hpp"

#include <bit>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace Northrend {

Wmo::Wmo(std::shared_ptr<const AssetData> data) : data(std::move(data)) {
    static constexpr ChunkId VersionId {'R', 'E', 'V', 'M'};

    if (this->data == nullptr || this->data->empty()) {
        throw std::invalid_argument("WMO data is empty");
    }

    ChunkReader reader(this->data->bytes());
    bool versionFound = false;

    while (const std::optional<Chunk> chunk = reader.next()) {
        if (chunk->id() != VersionId) {
            continue;
        }

        if (versionFound || chunk->size() != sizeof(std::uint32_t)) {
            throw std::runtime_error("WMO version chunk is invalid");
        }

        this->fileVersion = read_uint32(chunk->bytes(), 0);
        versionFound = true;
    }

    if (!versionFound) {
        throw std::runtime_error("WMO is missing its format version");
    }

    switch (this->fileVersion) {
    case 17:
        this->parse_version17();
        break;
    default:
        throw std::runtime_error("Unsupported WMO version: " + std::to_string(this->fileVersion));
    }
}

std::uint32_t Wmo::version() const noexcept {
    return this->fileVersion;
}

const Wmo::Header& Wmo::header() const noexcept {
    return this->rootHeader;
}

const Wmo::Material& Wmo::material(const std::size_t index) const {
    return this->materials.at(index);
}

const Wmo::Group& Wmo::group(const std::size_t index) const {
    return this->groups.at(index);
}

std::string_view Wmo::read_string(
    const std::span<const std::byte> bytes, const std::uint32_t offset, const bool emptyAllowed) {
    if (offset >= bytes.size()) {
        throw std::runtime_error("WMO string offset exceeds its string block");
    }

    std::size_t end = offset;

    while (end < bytes.size() && bytes[end] != std::byte {0}) {
        ++end;
    }

    if (end == bytes.size()) {
        throw std::runtime_error("WMO string is not terminated");
    }

    if (!emptyAllowed && end == offset) {
        throw std::runtime_error("WMO contains an empty required string");
    }

    return {reinterpret_cast<const char*>(bytes.data() + offset), end - offset};
}

std::uint16_t Wmo::read_uint16(const std::span<const std::byte> bytes, const std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint16_t)) {
        throw std::out_of_range("WMO value exceeds the available data");
    }

    return std::to_integer<std::uint16_t>(bytes[offset]) |
        std::to_integer<std::uint16_t>(bytes[offset + 1]) << 8U;
}

std::uint32_t Wmo::read_uint32(const std::span<const std::byte> bytes, const std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint32_t)) {
        throw std::out_of_range("WMO value exceeds the available data");
    }

    return std::to_integer<std::uint32_t>(bytes[offset]) | std::to_integer<std::uint32_t>(bytes[offset + 1]) << 8U |
        std::to_integer<std::uint32_t>(bytes[offset + 2]) << 16U |
        std::to_integer<std::uint32_t>(bytes[offset + 3]) << 24U;
}

float Wmo::read_float(const std::span<const std::byte> bytes, const std::size_t offset) {
    return std::bit_cast<float>(read_uint32(bytes, offset));
}

Vector3 Wmo::read_vector(const std::span<const std::byte> bytes, const std::size_t offset) {
    return Vector3 {
        .x = read_float(bytes, offset),
        .y = read_float(bytes, offset + sizeof(float)),
        .z = read_float(bytes, offset + sizeof(float) * 2)
    };
}

}
