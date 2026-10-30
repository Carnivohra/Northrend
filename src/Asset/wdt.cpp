#include "Asset/wdt.hpp"

#include "Asset/Chunk/id.hpp"
#include "Asset/Chunk/reader.hpp"

#include <optional>
#include <stdexcept>

namespace Northrend {

Wdt::Wdt(const std::span<const std::byte> bytes) {
    static constexpr ChunkId VersionId {'R', 'E', 'V', 'M'};
    static constexpr ChunkId HeaderId {'D', 'H', 'P', 'M'};
    static constexpr ChunkId TilesId {'N', 'I', 'A', 'M'};
    static constexpr std::size_t HeaderSize = 32;
    static constexpr std::size_t TileSize = 8;
    static constexpr std::uint32_t ExistingTile = 0x1;

    ChunkReader reader(bytes);
    bool versionFound = false;
    bool headerFound = false;
    bool tilesFound = false;

    while (const std::optional<Chunk> chunk = reader.next()) {
        if (chunk->id() == VersionId) {
            if (versionFound || chunk->size() != sizeof(std::uint32_t)) {
                throw std::runtime_error("WDT version chunk is invalid");
            }

            this->fileVersion = read_uint32(chunk->bytes(), 0);
            versionFound = true;
            continue;
        }

        if (chunk->id() == HeaderId) {
            if (headerFound || chunk->size() != HeaderSize) {
                throw std::runtime_error("WDT header chunk is invalid");
            }

            this->headerFlags = read_uint32(chunk->bytes(), 0);
            headerFound = true;
            continue;
        }

        if (chunk->id() == TilesId) {
            if (tilesFound || chunk->size() != Size * Size * TileSize) {
                throw std::runtime_error("WDT tile chunk is invalid");
            }

            for (std::size_t index = 0; index < this->tiles.size(); ++index) {
                this->tiles[index] = (read_uint32(chunk->bytes(), index * TileSize) & ExistingTile) != 0;
            }

            tilesFound = true;
        }
    }

    if (!versionFound || !headerFound || !tilesFound) {
        throw std::runtime_error("WDT is missing a required chunk");
    }

    if (this->fileVersion != 18) {
        throw std::runtime_error("Unsupported WDT version");
    }
}

std::uint32_t Wdt::version() const noexcept {
    return this->fileVersion;
}

std::uint32_t Wdt::flags() const noexcept {
    return this->headerFlags;
}

bool Wdt::has_tile(const std::size_t x, const std::size_t y) const noexcept {
    return x < Size && y < Size && this->tiles[y * Size + x];
}

std::size_t Wdt::tile_count() const noexcept {
    return this->tiles.count();
}

std::uint32_t Wdt::read_uint32(const std::span<const std::byte> bytes, const std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint32_t)) {
        throw std::out_of_range("WDT value exceeds the available data");
    }

    return std::to_integer<std::uint32_t>(bytes[offset]) |
        std::to_integer<std::uint32_t>(bytes[offset + 1]) << 8U |
        std::to_integer<std::uint32_t>(bytes[offset + 2]) << 16U |
        std::to_integer<std::uint32_t>(bytes[offset + 3]) << 24U;
}

}
