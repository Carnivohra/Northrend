#include "Asset/Chunk/reader.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace Northrend {

ChunkReader::ChunkReader(std::span<const std::byte> bytes) noexcept : data(bytes) {}

std::optional<Chunk> ChunkReader::next() {
    constexpr std::size_t headerSize = 8;

    if (this->empty()) {
        return std::nullopt;
    }

    const std::size_t remaining = this->data.size() - this->offset;

    if (remaining < headerSize) {
        throw std::runtime_error("Truncated chunk header at offset " + std::to_string(this->offset));
    }

    const std::byte* header = this->data.data() + this->offset;
    const ChunkId identifier = ChunkId::from_bytes(header);
    const std::uint32_t size = std::to_integer<std::uint32_t>(header[4]) |
        std::to_integer<std::uint32_t>(header[5]) << 8U | std::to_integer<std::uint32_t>(header[6]) << 16U |
        std::to_integer<std::uint32_t>(header[7]) << 24U;

    if (size > remaining - headerSize) {
        throw std::runtime_error("Chunk payload exceeds the asset at offset " + std::to_string(this->offset));
    }

    const std::size_t payloadOffset = this->offset + headerSize;
    const std::span<const std::byte> payload = this->data.subspan(payloadOffset, size);
    this->offset = payloadOffset + size;

    return Chunk(identifier, payload);
}

bool ChunkReader::empty() const noexcept {
    return this->offset == this->data.size();
}

std::size_t ChunkReader::position() const noexcept {
    return this->offset;
}

}
