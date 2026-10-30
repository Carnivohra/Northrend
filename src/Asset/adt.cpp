#include "Asset/adt.hpp"

#include "Asset/Chunk/id.hpp"
#include "Asset/Chunk/reader.hpp"

#include <bit>
#include <bitset>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <utility>

namespace Northrend {

Adt::Adt(std::shared_ptr<const AssetData> data) : data(std::move(data)) {
    static constexpr ChunkId VersionId {'R', 'E', 'V', 'M'};
    static constexpr ChunkId HeaderId {'R', 'D', 'H', 'M'};
    static constexpr ChunkId IndexId {'N', 'I', 'C', 'M'};
    static constexpr ChunkId TerrainId {'K', 'N', 'C', 'M'};
    static constexpr ChunkId HeightsId {'T', 'V', 'C', 'M'};
    static constexpr std::size_t ChunkHeaderSize = 8;
    static constexpr std::size_t HeaderSize = 64;
    static constexpr std::size_t IndexEntrySize = 16;
    static constexpr std::size_t TerrainHeaderSize = 128;
    static constexpr std::size_t HeightDataSize = HeightsPerChunk * sizeof(float);

    if (this->data == nullptr || this->data->empty()) {
        throw std::invalid_argument("ADT data is empty");
    }

    const std::span<const std::byte> bytes = this->data->bytes();
    std::array<std::uint32_t, ChunkCount> indexedOffsets {};
    std::array<std::uint32_t, ChunkCount> indexedSizes {};
    std::bitset<ChunkCount> terrainChunks;
    ChunkReader reader(bytes);
    std::size_t indexChunkOffset = 0;
    std::size_t actualIndexChunkOffset = 0;
    bool versionFound = false;
    bool headerFound = false;
    bool indexFound = false;

    while (const std::optional<Chunk> chunk = reader.next()) {
        const std::size_t chunkOffset = reader.position() - chunk->size() - ChunkHeaderSize;

        if (chunk->id() == VersionId) {
            if (versionFound || chunk->size() != sizeof(std::uint32_t)) {
                throw std::runtime_error("ADT version chunk is invalid");
            }

            this->fileVersion = read_uint32(chunk->bytes(), 0);
            versionFound = true;
            continue;
        }

        if (chunk->id() == HeaderId) {
            if (headerFound || chunk->size() != HeaderSize) {
                throw std::runtime_error("ADT header chunk is invalid");
            }

            const std::size_t referencedIndexOffset = chunkOffset + ChunkHeaderSize + read_uint32(chunk->bytes(), 4);

            if (referencedIndexOffset > bytes.size() - ChunkHeaderSize) {
                throw std::runtime_error("ADT header references an invalid index chunk");
            }

            indexChunkOffset = referencedIndexOffset;
            headerFound = true;
            continue;
        }

        if (chunk->id() == IndexId) {
            if (indexFound || chunk->size() != ChunkCount * IndexEntrySize) {
                throw std::runtime_error("ADT index chunk is invalid");
            }

            if (headerFound && indexChunkOffset != chunkOffset) {
                throw std::runtime_error("ADT header does not reference the index chunk");
            }

            actualIndexChunkOffset = chunkOffset;

            for (std::size_t cell = 0; cell < ChunkCount; ++cell) {
                const std::size_t entryOffset = cell * IndexEntrySize;
                indexedOffsets[cell] = read_uint32(chunk->bytes(), entryOffset);
                indexedSizes[cell] = read_uint32(chunk->bytes(), entryOffset + sizeof(std::uint32_t));
            }

            indexFound = true;
            continue;
        }

        if (chunk->id() != TerrainId) {
            continue;
        }

        if (chunk->size() < TerrainHeaderSize) {
            throw std::runtime_error("ADT terrain chunk header is truncated");
        }

        const std::size_t x = read_uint32(chunk->bytes(), 4);
        const std::size_t y = read_uint32(chunk->bytes(), 8);
        const std::size_t cell = index(x, y);

        if (terrainChunks.test(cell)) {
            throw std::runtime_error("ADT contains a duplicate terrain chunk");
        }

        const std::uint32_t heightOffset = read_uint32(chunk->bytes(), 20);

        if (heightOffset < ChunkHeaderSize) {
            throw std::runtime_error("ADT terrain chunk contains an invalid height offset");
        }

        const std::size_t nestedOffset = static_cast<std::size_t>(heightOffset) - ChunkHeaderSize;

        if (nestedOffset > chunk->size() || chunk->size() - nestedOffset < ChunkHeaderSize) {
            throw std::runtime_error("ADT height chunk header is truncated");
        }

        const std::span<const std::byte> nested = chunk->bytes().subspan(nestedOffset);

        if (read_uint32(nested, 0) != HeightsId.raw() || read_uint32(nested, 4) != HeightDataSize ||
            nested.size() - ChunkHeaderSize < HeightDataSize) {
            throw std::runtime_error("ADT height chunk is invalid");
        }

        this->positionsY[cell] = read_float(chunk->bytes(), 104);
        this->positionsX[cell] = read_float(chunk->bytes(), 108);
        this->baseHeights[cell] = read_float(chunk->bytes(), 112);
        this->holeMasks[cell] = read_uint32(chunk->bytes(), 60);
        this->heightData[cell] = nested.subspan(ChunkHeaderSize, HeightDataSize);

        if (!std::isfinite(this->positionsX[cell]) || !std::isfinite(this->positionsY[cell]) ||
            !std::isfinite(this->baseHeights[cell])) {
            throw std::runtime_error("ADT terrain chunk contains an invalid position");
        }

        for (std::size_t vertex = 0; vertex < HeightsPerChunk; ++vertex) {
            if (!std::isfinite(read_float(this->heightData[cell], vertex * sizeof(float)))) {
                throw std::runtime_error("ADT terrain chunk contains an invalid height");
            }
        }

        terrainChunks.set(cell);
    }

    if (!versionFound || !headerFound || !indexFound) {
        throw std::runtime_error("ADT is missing a required chunk");
    }

    if (indexChunkOffset != actualIndexChunkOffset) {
        throw std::runtime_error("ADT header does not reference the index chunk");
    }

    if (this->fileVersion != 18) {
        throw std::runtime_error("Unsupported ADT version");
    }

    if (terrainChunks.count() != ChunkCount) {
        throw std::runtime_error("ADT does not contain all terrain chunks");
    }

    for (std::size_t cell = 0; cell < ChunkCount; ++cell) {
        const std::size_t offset = indexedOffsets[cell];
        const std::size_t size = indexedSizes[cell];

        if (offset > bytes.size() || size < ChunkHeaderSize || size > bytes.size() - offset ||
            offset + size > bytes.size() || read_uint32(bytes, offset) != TerrainId.raw() ||
            read_uint32(bytes, offset + 4) + ChunkHeaderSize != size) {
            throw std::runtime_error("ADT index contains an invalid terrain reference");
        }

        const std::span<const std::byte> terrain = bytes.subspan(offset + ChunkHeaderSize, size - ChunkHeaderSize);

        if (read_uint32(terrain, 4) != cell % Size || read_uint32(terrain, 8) != cell / Size) {
            throw std::runtime_error("ADT index references the wrong terrain cell");
        }
    }
}

std::uint32_t Adt::version() const noexcept {
    return this->fileVersion;
}

float Adt::position_x(const std::size_t x, const std::size_t y) const {
    return this->positionsX[index(x, y)];
}

float Adt::position_y(const std::size_t x, const std::size_t y) const {
    return this->positionsY[index(x, y)];
}

float Adt::base_height(const std::size_t x, const std::size_t y) const {
    return this->baseHeights[index(x, y)];
}

float Adt::height_offset(const std::size_t x, const std::size_t y, const std::size_t vertex) const {
    if (vertex >= HeightsPerChunk) {
        throw std::out_of_range("ADT height vertex is outside the terrain chunk");
    }

    return read_float(this->heightData[index(x, y)], vertex * sizeof(float));
}

float Adt::height(const std::size_t x, const std::size_t y, const std::size_t vertex) const {
    return this->base_height(x, y) + this->height_offset(x, y, vertex);
}

std::uint32_t Adt::holes(const std::size_t x, const std::size_t y) const {
    return this->holeMasks[index(x, y)];
}

std::size_t Adt::index(const std::size_t x, const std::size_t y) {
    if (x >= Size || y >= Size) {
        throw std::out_of_range("ADT terrain coordinate is outside the tile");
    }

    return y * Size + x;
}

std::uint32_t Adt::read_uint32(const std::span<const std::byte> bytes, const std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint32_t)) {
        throw std::out_of_range("ADT value exceeds the available data");
    }

    return std::to_integer<std::uint32_t>(bytes[offset]) | std::to_integer<std::uint32_t>(bytes[offset + 1]) << 8U |
        std::to_integer<std::uint32_t>(bytes[offset + 2]) << 16U |
        std::to_integer<std::uint32_t>(bytes[offset + 3]) << 24U;
}

float Adt::read_float(const std::span<const std::byte> bytes, const std::size_t offset) {
    return std::bit_cast<float>(read_uint32(bytes, offset));
}

}
