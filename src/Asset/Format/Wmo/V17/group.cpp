#include "Asset/Format/Wmo/group.hpp"

#include "Asset/Chunk/id.hpp"
#include "Asset/Chunk/reader.hpp"
#include "Asset/data.hpp"

#include <cmath>
#include <optional>
#include <span>
#include <stdexcept>

namespace Northrend {

void WmoGroup::parse_version17() {
    static constexpr ChunkId GroupId {'P', 'G', 'O', 'M'};
    static constexpr ChunkId PolygonId {'Y', 'P', 'O', 'M'};
    static constexpr ChunkId IndexId {'I', 'V', 'O', 'M'};
    static constexpr ChunkId VertexId {'T', 'V', 'O', 'M'};
    static constexpr ChunkId NormalId {'R', 'N', 'O', 'M'};
    static constexpr ChunkId TextureCoordinateId {'V', 'T', 'O', 'M'};
    static constexpr ChunkId BatchId {'A', 'B', 'O', 'M'};
    static constexpr std::size_t HeaderSize = 68;
    static constexpr std::size_t PolygonSize = 2;
    static constexpr std::size_t VectorSize = sizeof(float) * 3;
    static constexpr std::size_t TextureCoordinateSize = sizeof(float) * 2;
    static constexpr std::size_t BatchSize = 24;

    std::span<const std::byte> polygonBytes;
    std::span<const std::byte> indexBytes;
    std::span<const std::byte> vertexBytes;
    std::span<const std::byte> normalBytes;
    std::vector<std::span<const std::byte>> textureCoordinateSets;
    std::span<const std::byte> batchBytes;
    std::span<const std::byte> nestedBytes;
    ChunkReader reader(this->data->bytes());
    bool groupFound = false;
    bool polygonsFound = false;
    bool indicesFound = false;
    bool verticesFound = false;
    bool normalsFound = false;
    bool batchesFound = false;

    while (const std::optional<Chunk> chunk = reader.next()) {
        if (chunk->id() != GroupId) {
            continue;
        }

        if (groupFound || chunk->size() < HeaderSize) {
            throw std::runtime_error("WMO 17 group header chunk is invalid");
        }

        this->groupHeader = Header {
            .nameOffset = read_uint32(chunk->bytes(), 0),
            .descriptiveNameOffset = read_uint32(chunk->bytes(), 4),
            .flags = read_uint32(chunk->bytes(), 8),
            .minimum = read_vector(chunk->bytes(), 12),
            .maximum = read_vector(chunk->bytes(), 24),
            .portalStart = read_uint16(chunk->bytes(), 36),
            .portalCount = read_uint16(chunk->bytes(), 38),
            .transparentBatchCount = read_uint16(chunk->bytes(), 40),
            .interiorBatchCount = read_uint16(chunk->bytes(), 42),
            .exteriorBatchCount = read_uint16(chunk->bytes(), 44),
            .additionalBatchCount = read_uint16(chunk->bytes(), 46),
            .fogIds = {
                std::to_integer<std::uint8_t>(chunk->bytes()[48]),
                std::to_integer<std::uint8_t>(chunk->bytes()[49]),
                std::to_integer<std::uint8_t>(chunk->bytes()[50]),
                std::to_integer<std::uint8_t>(chunk->bytes()[51])
            },
            .liquidType = read_uint32(chunk->bytes(), 52),
            .id = read_uint32(chunk->bytes(), 56)
        };
        nestedBytes = chunk->bytes().subspan(HeaderSize);
        groupFound = true;
    }

    if (!groupFound) {
        throw std::runtime_error("WMO 17 group is missing its MOGP chunk");
    }

    const auto store = [](const Chunk& chunk, std::span<const std::byte>& destination, bool& found) {
        if (found) {
            throw std::runtime_error("WMO 17 group contains a duplicate geometry chunk");
        }

        destination = chunk.bytes();
        found = true;
    };
    ChunkReader nested(nestedBytes);

    while (const std::optional<Chunk> chunk = nested.next()) {
        if (chunk->id() == PolygonId) {
            store(*chunk, polygonBytes, polygonsFound);
        } else if (chunk->id() == IndexId) {
            store(*chunk, indexBytes, indicesFound);
        } else if (chunk->id() == VertexId) {
            store(*chunk, vertexBytes, verticesFound);
        } else if (chunk->id() == NormalId) {
            store(*chunk, normalBytes, normalsFound);
        } else if (chunk->id() == TextureCoordinateId) {
            textureCoordinateSets.push_back(chunk->bytes());
        } else if (chunk->id() == BatchId) {
            store(*chunk, batchBytes, batchesFound);
        }
    }

    if (!polygonsFound || !indicesFound || !verticesFound || !normalsFound || textureCoordinateSets.empty() ||
        !batchesFound) {
        throw std::runtime_error("WMO 17 group is missing required geometry");
    }

    if (polygonBytes.size() % PolygonSize != 0 || indexBytes.size() % sizeof(std::uint16_t) != 0 ||
        vertexBytes.size() % VectorSize != 0 || normalBytes.size() % VectorSize != 0 ||
        batchBytes.size() % BatchSize != 0) {
        throw std::runtime_error("WMO 17 group geometry has an invalid size");
    }

    const std::size_t polygonCount = polygonBytes.size() / PolygonSize;
    const std::size_t indexCount = indexBytes.size() / sizeof(std::uint16_t);
    const std::size_t vertexCount = vertexBytes.size() / VectorSize;
    const std::size_t batchCount = batchBytes.size() / BatchSize;
    const std::size_t headerBatchCount = this->groupHeader.transparentBatchCount +
        this->groupHeader.interiorBatchCount + this->groupHeader.exteriorBatchCount +
        this->groupHeader.additionalBatchCount;

    if (indexCount != polygonCount * 3 || normalBytes.size() / VectorSize != vertexCount) {
        throw std::runtime_error("WMO 17 group geometry tables do not match");
    }

    if (batchCount != headerBatchCount) {
        throw std::runtime_error("WMO 17 group batch count does not match the header");
    }

    for (const std::span<const std::byte> textureCoordinates : textureCoordinateSets) {
        if (textureCoordinates.size() % TextureCoordinateSize != 0 ||
            textureCoordinates.size() / TextureCoordinateSize != vertexCount) {
            throw std::runtime_error("WMO 17 texture coordinate table does not match the vertices");
        }
    }

    const auto finite = [](const Vector3& value) {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };

    if (!finite(this->groupHeader.minimum) || !finite(this->groupHeader.maximum)) {
        throw std::runtime_error("WMO 17 group contains invalid bounds");
    }

    this->polygonData.reserve(polygonCount);

    for (std::size_t offset = 0; offset < polygonBytes.size(); offset += PolygonSize) {
        this->polygonData.push_back(Polygon {
            .flags = std::to_integer<std::uint8_t>(polygonBytes[offset]),
            .material = std::to_integer<std::uint8_t>(polygonBytes[offset + 1])
        });
    }

    this->indexData.reserve(indexCount);

    for (std::size_t offset = 0; offset < indexBytes.size(); offset += sizeof(std::uint16_t)) {
        const std::uint16_t index = read_uint16(indexBytes, offset);

        if (index >= vertexCount) {
            throw std::runtime_error("WMO 17 group index references an invalid vertex");
        }

        this->indexData.push_back(index);
    }

    this->vertexData.reserve(vertexCount);
    this->normalData.reserve(vertexCount);

    for (std::size_t vertex = 0; vertex < vertexCount; ++vertex) {
        const Vector3 position = read_vector(vertexBytes, vertex * VectorSize);
        const Vector3 normal = read_vector(normalBytes, vertex * VectorSize);

        if (!finite(position)) {
            throw std::runtime_error("WMO 17 group contains an invalid vertex position");
        }

        if (!finite(normal)) {
            throw std::runtime_error("WMO 17 group contains an invalid vertex normal");
        }

        this->vertexData.push_back(position);
        this->normalData.push_back(normal);
    }

    this->textureCoordinateData.reserve(textureCoordinateSets.size());

    for (const std::span<const std::byte> textureCoordinates : textureCoordinateSets) {
        std::vector<TextureCoordinate>& output = this->textureCoordinateData.emplace_back();
        output.reserve(vertexCount);

        for (std::size_t vertex = 0; vertex < vertexCount; ++vertex) {
            const float rawU = read_float(textureCoordinates, vertex * TextureCoordinateSize);
            const float rawV = read_float(textureCoordinates, vertex * TextureCoordinateSize + sizeof(float));
            const float u = std::isfinite(rawU) ? rawU : 0.0F;
            const float v = std::isfinite(rawV) ? rawV : 0.0F;
            output.push_back(TextureCoordinate {.u = u, .v = v});
        }
    }

    this->batchData.reserve(batchCount);

    for (std::size_t offset = 0; offset < batchBytes.size(); offset += BatchSize) {
        const std::uint32_t firstIndex = read_uint32(batchBytes, offset + 12);
        const std::uint16_t batchIndexCount = read_uint16(batchBytes, offset + 16);
        const std::uint16_t firstVertex = read_uint16(batchBytes, offset + 18);
        const std::uint16_t lastVertex = read_uint16(batchBytes, offset + 20);

        if (firstIndex > indexCount || batchIndexCount > indexCount - firstIndex || firstVertex > lastVertex ||
            lastVertex >= vertexCount) {
            throw std::runtime_error("WMO 17 group batch references invalid geometry");
        }

        this->batchData.push_back(Batch {
            .minimum = {
                read_int16(batchBytes, offset),
                read_int16(batchBytes, offset + 2),
                read_int16(batchBytes, offset + 4)
            },
            .maximum = {
                read_int16(batchBytes, offset + 6),
                read_int16(batchBytes, offset + 8),
                read_int16(batchBytes, offset + 10)
            },
            .firstIndex = firstIndex,
            .indexCount = batchIndexCount,
            .firstVertex = firstVertex,
            .lastVertex = lastVertex,
            .flags = std::to_integer<std::uint8_t>(batchBytes[offset + 22]),
            .material = std::to_integer<std::uint8_t>(batchBytes[offset + 23])
        });
    }
}

}
