#include "Asset/Format/adt.hpp"

#include "Asset/Chunk/id.hpp"
#include "Asset/Chunk/reader.hpp"

#include <algorithm>
#include <bit>
#include <bitset>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace Northrend {

Adt::Adt(std::shared_ptr<const AssetData> data) : data(std::move(data)) {
    static constexpr ChunkId VersionId {'R', 'E', 'V', 'M'};
    static constexpr ChunkId HeaderId {'R', 'D', 'H', 'M'};
    static constexpr ChunkId IndexId {'N', 'I', 'C', 'M'};
    static constexpr ChunkId TexturesId {'X', 'E', 'T', 'M'};
    static constexpr ChunkId TerrainId {'K', 'N', 'C', 'M'};
    static constexpr ChunkId HeightsId {'T', 'V', 'C', 'M'};
    static constexpr ChunkId NormalsId {'R', 'N', 'C', 'M'};
    static constexpr ChunkId LayersId {'Y', 'L', 'C', 'M'};
    static constexpr ChunkId AlphaId {'L', 'A', 'C', 'M'};
    static constexpr std::size_t ChunkHeaderSize = 8;
    static constexpr std::size_t HeaderSize = 64;
    static constexpr std::size_t IndexEntrySize = 16;
    static constexpr std::size_t TerrainHeaderSize = 128;
    static constexpr std::size_t HeightDataSize = HeightsPerChunk * sizeof(float);
    static constexpr std::size_t NormalDataSize = HeightsPerChunk * 3;

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
    std::size_t textureChunkOffset = 0;
    std::size_t actualTextureChunkOffset = 0;
    bool versionFound = false;
    bool headerFound = false;
    bool indexFound = false;
    bool texturesFound = false;

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
            const std::size_t referencedTextureOffset = chunkOffset + ChunkHeaderSize + read_uint32(chunk->bytes(), 8);

            if (referencedIndexOffset > bytes.size() - ChunkHeaderSize ||
                referencedTextureOffset > bytes.size() - ChunkHeaderSize) {
                throw std::runtime_error("ADT header references an invalid chunk");
            }

            indexChunkOffset = referencedIndexOffset;
            textureChunkOffset = referencedTextureOffset;
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

        if (chunk->id() == TexturesId) {
            if (texturesFound) {
                throw std::runtime_error("ADT contains duplicate texture names");
            }

            if (headerFound && textureChunkOffset != chunkOffset) {
                throw std::runtime_error("ADT header does not reference the texture chunk");
            }

            const std::span<const std::byte> textureBytes = chunk->bytes();
            std::size_t offset = 0;

            while (offset < textureBytes.size()) {
                std::size_t end = offset;

                while (end < textureBytes.size() && textureBytes[end] != std::byte {0}) {
                    ++end;
                }

                if (end == textureBytes.size()) {
                    throw std::runtime_error("ADT texture name is not terminated");
                }

                if (end == offset) {
                    throw std::runtime_error("ADT contains an empty texture name");
                }

                this->textureNames.emplace_back(
                    reinterpret_cast<const char*>(textureBytes.data() + offset), end - offset);
                offset = end + 1;
            }

            actualTextureChunkOffset = chunkOffset;
            texturesFound = true;
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
        const std::uint32_t normalOffset = read_uint32(chunk->bytes(), 24);
        const std::size_t layers = read_uint32(chunk->bytes(), 12);
        const std::uint32_t layerOffset = read_uint32(chunk->bytes(), 28);
        const std::uint32_t alphaOffset = read_uint32(chunk->bytes(), 36);
        const std::uint32_t alphaSize = read_uint32(chunk->bytes(), 40);

        if (layers > MaximumLayers) {
            throw std::runtime_error("ADT terrain chunk contains too many texture layers");
        }

        if (heightOffset < ChunkHeaderSize || normalOffset < ChunkHeaderSize ||
            (layers != 0 && layerOffset < ChunkHeaderSize)) {
            throw std::runtime_error("ADT terrain chunk contains an invalid data offset");
        }

        const std::size_t heightChunkOffset = static_cast<std::size_t>(heightOffset) - ChunkHeaderSize;
        const std::size_t normalChunkOffset = static_cast<std::size_t>(normalOffset) - ChunkHeaderSize;
        const std::size_t layerChunkOffset = layers == 0 ? 0 : static_cast<std::size_t>(layerOffset) - ChunkHeaderSize;

        if (heightChunkOffset > chunk->size() || chunk->size() - heightChunkOffset < ChunkHeaderSize ||
            normalChunkOffset > chunk->size() || chunk->size() - normalChunkOffset < ChunkHeaderSize ||
            (layers != 0 && (layerChunkOffset > chunk->size() || chunk->size() - layerChunkOffset < ChunkHeaderSize))) {
            throw std::runtime_error("ADT terrain data chunk header is truncated");
        }

        const std::span<const std::byte> heights = chunk->bytes().subspan(heightChunkOffset);
        const std::span<const std::byte> normals = chunk->bytes().subspan(normalChunkOffset);

        if (read_uint32(heights, 0) != HeightsId.raw() || read_uint32(heights, 4) != HeightDataSize ||
            heights.size() - ChunkHeaderSize < HeightDataSize) {
            throw std::runtime_error("ADT height chunk is invalid");
        }

        const std::size_t normalChunkSize = read_uint32(normals, 4);

        if (read_uint32(normals, 0) != NormalsId.raw() || normalChunkSize < NormalDataSize ||
            normals.size() - ChunkHeaderSize < normalChunkSize) {
            throw std::runtime_error("ADT normal chunk is invalid");
        }

        if (layers != 0) {
            const std::span<const std::byte> layerChunk = chunk->bytes().subspan(layerChunkOffset);
            const std::size_t layerBytes = layers * LayerSize;

            if (read_uint32(layerChunk, 0) != LayersId.raw() || read_uint32(layerChunk, 4) != layerBytes ||
                layerChunk.size() - ChunkHeaderSize < layerBytes) {
                throw std::runtime_error("ADT texture layer chunk is invalid");
            }

            this->layerData[cell] = layerChunk.subspan(ChunkHeaderSize, layerBytes);
        }

        if (alphaSize != 0) {
            if (alphaOffset < ChunkHeaderSize || alphaSize < ChunkHeaderSize) {
                throw std::runtime_error("ADT terrain chunk contains invalid alpha map metadata");
            }

            const std::size_t alphaChunkOffset = static_cast<std::size_t>(alphaOffset) - ChunkHeaderSize;

            if (alphaChunkOffset > chunk->size() || alphaSize > chunk->size() - alphaChunkOffset) {
                throw std::runtime_error("ADT alpha map chunk is truncated");
            }

            const std::span<const std::byte> alphaChunk = chunk->bytes().subspan(alphaChunkOffset, alphaSize);
            const std::size_t alphaBytes = alphaSize - ChunkHeaderSize;

            if (read_uint32(alphaChunk, 0) != AlphaId.raw()) {
                throw std::runtime_error("ADT alpha map chunk is invalid");
            }

            this->alphaData[cell] = alphaChunk.subspan(ChunkHeaderSize, alphaBytes);
        }

        for (std::size_t layer = 0; layer < layers; ++layer) {
            const std::size_t entry = layer * LayerSize;
            const std::uint32_t flags = read_uint32(this->layerData[cell], entry + 4);
            const std::size_t offset = read_uint32(this->layerData[cell], entry + 8);

            if ((flags & LayerHasAlpha) != 0 && offset >= this->alphaData[cell].size()) {
                throw std::runtime_error("ADT texture layer references invalid alpha map data");
            }
        }

        this->positionsX[cell] = read_float(chunk->bytes(), 104);
        this->positionsY[cell] = read_float(chunk->bytes(), 108);
        this->baseHeights[cell] = read_float(chunk->bytes(), 112);
        this->holeMasks[cell] = read_uint32(chunk->bytes(), 60);
        this->terrainFlags[cell] = read_uint32(chunk->bytes(), 0);
        this->layerCounts[cell] = static_cast<std::uint8_t>(layers);
        this->heightData[cell] = heights.subspan(ChunkHeaderSize, HeightDataSize);
        this->normalData[cell] = normals.subspan(ChunkHeaderSize, NormalDataSize);

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

    if (!versionFound || !headerFound || !indexFound || !texturesFound) {
        throw std::runtime_error("ADT is missing a required chunk");
    }

    if (indexChunkOffset != actualIndexChunkOffset || textureChunkOffset != actualTextureChunkOffset) {
        throw std::runtime_error("ADT header does not reference the expected chunks");
    }

    if (this->fileVersion != 18) {
        throw std::runtime_error("Unsupported ADT version");
    }

    if (terrainChunks.count() != ChunkCount) {
        throw std::runtime_error("ADT does not contain all terrain chunks");
    }

    for (const std::span<const std::byte> layers : this->layerData) {
        for (std::size_t offset = 0; offset < layers.size(); offset += LayerSize) {
            if (read_uint32(layers, offset) >= this->textureNames.size()) {
                throw std::runtime_error("ADT texture layer references an invalid texture");
            }
        }
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

std::array<std::int8_t, 3> Adt::normal(const std::size_t x, const std::size_t y, const std::size_t vertex) const {
    if (vertex >= HeightsPerChunk) {
        throw std::out_of_range("ADT normal vertex is outside the terrain chunk");
    }

    const std::span<const std::byte> normals = this->normalData[index(x, y)];
    const std::size_t offset = vertex * 3;
    return {
        std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(normals[offset])),
        std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(normals[offset + 1])),
        std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(normals[offset + 2]))
    };
}

std::uint32_t Adt::holes(const std::size_t x, const std::size_t y) const {
    return this->holeMasks[index(x, y)];
}

std::size_t Adt::texture_count() const noexcept {
    return this->textureNames.size();
}

std::string_view Adt::texture_name(const std::size_t texture) const {
    return this->textureNames.at(texture);
}

std::size_t Adt::layer_count(const std::size_t x, const std::size_t y) const {
    return this->layerCounts[index(x, y)];
}

std::uint32_t Adt::layer_texture(const std::size_t x, const std::size_t y, const std::size_t layer) const {
    return read_uint32(this->layerData[index(x, y)], this->layer_offset(x, y, layer));
}

std::uint32_t Adt::layer_flags(const std::size_t x, const std::size_t y, const std::size_t layer) const {
    return read_uint32(this->layerData[index(x, y)], this->layer_offset(x, y, layer) + 4);
}

std::uint32_t Adt::layer_effect(const std::size_t x, const std::size_t y, const std::size_t layer) const {
    return read_uint32(this->layerData[index(x, y)], this->layer_offset(x, y, layer) + 12);
}

std::span<const std::byte> Adt::alpha_data(const std::size_t x, const std::size_t y, const std::size_t layer) const {
    const std::size_t cell = index(x, y);
    const std::size_t entry = this->layer_offset(x, y, layer);
    const std::uint32_t flags = read_uint32(this->layerData[cell], entry + 4);

    if ((flags & LayerHasAlpha) == 0) {
        return {};
    }

    const std::size_t begin = read_uint32(this->layerData[cell], entry + 8);
    std::size_t end = this->alphaData[cell].size();

    for (std::size_t candidate = 0; candidate < this->layerCounts[cell]; ++candidate) {
        const std::size_t candidateEntry = candidate * LayerSize;
        const std::uint32_t candidateFlags = read_uint32(this->layerData[cell], candidateEntry + 4);

        if ((candidateFlags & LayerHasAlpha) == 0) {
            continue;
        }

        const std::size_t candidateOffset = read_uint32(this->layerData[cell], candidateEntry + 8);

        if (candidateOffset > begin && candidateOffset < end) {
            end = candidateOffset;
        }
    }

    return this->alphaData[cell].subspan(begin, end - begin);
}

void Adt::decode_alpha(const std::size_t x, const std::size_t y, const std::size_t layer, const bool highResolution,
    const std::span<std::uint8_t, AlphaTexels> output) const {
    const std::size_t cell = index(x, y);
    const std::uint32_t flags = this->layer_flags(x, y, layer);
    const std::span<const std::byte> input = this->alpha_data(x, y, layer);

    if (input.empty()) {
        throw std::invalid_argument("ADT texture layer has no alpha map");
    }

    if ((flags & LayerAlphaCompressed) != 0) {
        decode_alpha_rle(input, output);
    } else if (highResolution) {
        if (input.size() < AlphaTexels) {
            throw std::runtime_error("ADT 8-bit alpha map is truncated");
        }

        for (std::size_t texel = 0; texel < AlphaTexels; ++texel) {
            output[texel] = std::to_integer<std::uint8_t>(input[texel]);
        }
    } else {
        decode_alpha_packed(input, output);
    }

    if ((this->terrainFlags[cell] & DoNotFixAlpha) == 0) {
        correct_alpha_edges(output);
    }
}

void Adt::decode_alpha_rle(const std::span<const std::byte> input, const std::span<std::uint8_t, AlphaTexels> output) {
    std::size_t source = 0;
    std::size_t destination = 0;

    while (destination < output.size()) {
        if (source >= input.size()) {
            throw std::runtime_error("ADT compressed alpha map is truncated");
        }

        const std::uint8_t header = std::to_integer<std::uint8_t>(input[source++]);
        const std::size_t count = header & 0x7FU;

        if (count == 0 || count > output.size() - destination) {
            throw std::runtime_error("ADT compressed alpha map contains an invalid run");
        }

        if ((header & 0x80U) != 0) {
            if (source >= input.size()) {
                throw std::runtime_error("ADT compressed alpha map run is truncated");
            }

            const std::uint8_t value = std::to_integer<std::uint8_t>(input[source++]);
            std::fill_n(output.begin() + static_cast<std::ptrdiff_t>(destination), count, value);
        } else {
            if (count > input.size() - source) {
                throw std::runtime_error("ADT compressed alpha map literals are truncated");
            }

            for (std::size_t value = 0; value < count; ++value) {
                output[destination + value] = std::to_integer<std::uint8_t>(input[source + value]);
            }

            source += count;
        }

        destination += count;
    }
}

void Adt::decode_alpha_packed(
    const std::span<const std::byte> input, const std::span<std::uint8_t, AlphaTexels> output) {
    static constexpr std::size_t PackedSize = AlphaTexels / 2;

    if (input.size() < PackedSize) {
        throw std::runtime_error("ADT 4-bit alpha map is truncated");
    }

    for (std::size_t texel = 0; texel < AlphaTexels; ++texel) {
        const std::uint8_t packed = std::to_integer<std::uint8_t>(input[texel / 2]);
        const std::uint8_t value = texel % 2 == 0 ? packed & 0x0FU : packed >> 4U;
        output[texel] = static_cast<std::uint8_t>(value * 17U);
    }
}

void Adt::correct_alpha_edges(const std::span<std::uint8_t, AlphaTexels> alpha) noexcept {
    for (std::size_t row = 0; row < AlphaSize - 1; ++row) {
        const std::size_t rowOffset = row * AlphaSize;
        alpha[rowOffset + AlphaSize - 1] = alpha[rowOffset + AlphaSize - 2];
    }

    const std::size_t sourceRow = (AlphaSize - 2) * AlphaSize;
    const std::size_t destinationRow = (AlphaSize - 1) * AlphaSize;
    std::copy_n(alpha.begin() + static_cast<std::ptrdiff_t>(sourceRow), AlphaSize,
        alpha.begin() + static_cast<std::ptrdiff_t>(destinationRow));
}

std::size_t Adt::index(const std::size_t x, const std::size_t y) {
    if (x >= Size || y >= Size) {
        throw std::out_of_range("ADT terrain coordinate is outside the tile");
    }

    return y * Size + x;
}

std::size_t Adt::layer_offset(const std::size_t x, const std::size_t y, const std::size_t layer) const {
    const std::size_t cell = index(x, y);

    if (layer >= this->layerCounts[cell]) {
        throw std::out_of_range("ADT texture layer is outside the terrain chunk");
    }

    return layer * LayerSize;
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
