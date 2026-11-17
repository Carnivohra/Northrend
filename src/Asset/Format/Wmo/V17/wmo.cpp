#include "Asset/Format/Wmo/wmo.hpp"

#include "Asset/Chunk/id.hpp"
#include "Asset/Chunk/reader.hpp"
#include "Asset/data.hpp"

#include <cmath>
#include <optional>
#include <span>
#include <stdexcept>

namespace Northrend {

void Wmo::parse_version17() {
    static constexpr ChunkId HeaderId {'D', 'H', 'O', 'M'};
    static constexpr ChunkId TextureId {'X', 'T', 'O', 'M'};
    static constexpr ChunkId MaterialId {'T', 'M', 'O', 'M'};
    static constexpr ChunkId GroupNameId {'N', 'G', 'O', 'M'};
    static constexpr ChunkId GroupInfoId {'I', 'G', 'O', 'M'};
    static constexpr std::size_t HeaderSize = 64;
    static constexpr std::size_t MaterialSize = 64;
    static constexpr std::size_t GroupSize = 32;

    std::span<const std::byte> textureData;
    std::span<const std::byte> materialData;
    std::span<const std::byte> groupNameData;
    std::span<const std::byte> groupData;
    ChunkReader reader(this->data->bytes());
    bool headerFound = false;
    bool texturesFound = false;
    bool materialsFound = false;
    bool groupNamesFound = false;
    bool groupsFound = false;

    while (const std::optional<Chunk> chunk = reader.next()) {
        if (chunk->id() == HeaderId) {
            if (headerFound || chunk->size() != HeaderSize) {
                throw std::runtime_error("WMO 17 header chunk is invalid");
            }

            this->rootHeader = Header {
                .materialCount = read_uint32(chunk->bytes(), 0),
                .groupCount = read_uint32(chunk->bytes(), 4),
                .portalCount = read_uint32(chunk->bytes(), 8),
                .lightCount = read_uint32(chunk->bytes(), 12),
                .doodadNameCount = read_uint32(chunk->bytes(), 16),
                .doodadCount = read_uint32(chunk->bytes(), 20),
                .doodadSetCount = read_uint32(chunk->bytes(), 24),
                .ambientColor = read_uint32(chunk->bytes(), 28),
                .id = read_uint32(chunk->bytes(), 32),
                .minimum = read_vector(chunk->bytes(), 36),
                .maximum = read_vector(chunk->bytes(), 48),
                .flags = read_uint16(chunk->bytes(), 60),
                .lodCount = read_uint16(chunk->bytes(), 62)
            };
            headerFound = true;
            continue;
        }

        const auto store = [](const Chunk& chunk, std::span<const std::byte>& destination, bool& found) {
            if (found) {
                throw std::runtime_error("WMO 17 contains a duplicate root chunk");
            }

            destination = chunk.bytes();
            found = true;
        };

        if (chunk->id() == TextureId) {
            store(*chunk, textureData, texturesFound);
        } else if (chunk->id() == MaterialId) {
            store(*chunk, materialData, materialsFound);
        } else if (chunk->id() == GroupNameId) {
            store(*chunk, groupNameData, groupNamesFound);
        } else if (chunk->id() == GroupInfoId) {
            store(*chunk, groupData, groupsFound);
        }
    }

    if (!headerFound || !texturesFound || !materialsFound || !groupNamesFound || !groupsFound) {
        throw std::runtime_error("WMO 17 is missing a required root chunk");
    }

    if (materialData.size() % MaterialSize != 0 || groupData.size() % GroupSize != 0) {
        throw std::runtime_error("WMO 17 root table has an invalid size");
    }

    if (materialData.size() / MaterialSize != this->rootHeader.materialCount ||
        groupData.size() / GroupSize != this->rootHeader.groupCount) {
        throw std::runtime_error("WMO 17 root table count does not match the header");
    }

    const auto finite = [](const Vector3& value) {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };

    if (!finite(this->rootHeader.minimum) || !finite(this->rootHeader.maximum)) {
        throw std::runtime_error("WMO 17 header contains invalid bounds");
    }

    this->materials.reserve(this->rootHeader.materialCount);

    for (std::size_t offset = 0; offset < materialData.size(); offset += MaterialSize) {
        this->materials.push_back(Material {
            .texture1 = read_string(textureData, read_uint32(materialData, offset + 12), true),
            .texture2 = read_string(textureData, read_uint32(materialData, offset + 24), true),
            .flags = read_uint32(materialData, offset),
            .shader = read_uint32(materialData, offset + 4),
            .blendMode = read_uint32(materialData, offset + 8),
            .sidnColor = read_uint32(materialData, offset + 16),
            .frameSidnColor = read_uint32(materialData, offset + 20),
            .diffuseColor = read_uint32(materialData, offset + 28),
            .groundType = read_uint32(materialData, offset + 32),
            .color2 = read_uint32(materialData, offset + 40),
            .flags2 = read_uint32(materialData, offset + 44)
        });
    }

    this->groups.reserve(this->rootHeader.groupCount);

    for (std::size_t offset = 0; offset < groupData.size(); offset += GroupSize) {
        const Vector3 minimum = read_vector(groupData, offset + 4);
        const Vector3 maximum = read_vector(groupData, offset + 16);
        const std::uint32_t nameOffset = read_uint32(groupData, offset + 28);

        if (!finite(minimum) || !finite(maximum)) {
            throw std::runtime_error("WMO 17 group contains invalid bounds");
        }

        this->groups.push_back(Group {
            .name = nameOffset == ~std::uint32_t {0} ? std::string_view {} : read_string(groupNameData, nameOffset, false),
            .minimum = minimum,
            .maximum = maximum,
            .flags = read_uint32(groupData, offset)
        });
    }
}

}
