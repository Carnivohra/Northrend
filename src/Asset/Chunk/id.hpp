#pragma once

#include <cstddef>
#include <cstdint>

namespace Northrend {

class ChunkReader;

class ChunkId final {
public:
    constexpr ChunkId(char first, char second, char third, char fourth) noexcept :
        value(static_cast<std::uint32_t>(static_cast<unsigned char>(first)) |
            static_cast<std::uint32_t>(static_cast<unsigned char>(second)) << 8U |
            static_cast<std::uint32_t>(static_cast<unsigned char>(third)) << 16U |
            static_cast<std::uint32_t>(static_cast<unsigned char>(fourth)) << 24U) {}

    [[nodiscard]] constexpr std::uint32_t raw() const noexcept {
        return this->value;
    }

    constexpr bool operator==(const ChunkId&) const noexcept = default;

private:
    friend class ChunkReader;

    explicit constexpr ChunkId(std::uint32_t value) noexcept : value(value) {}

    [[nodiscard]] static constexpr ChunkId from_bytes(const std::byte* bytes) noexcept {
        return ChunkId(std::to_integer<std::uint32_t>(bytes[0]) | std::to_integer<std::uint32_t>(bytes[1]) << 8U |
            std::to_integer<std::uint32_t>(bytes[2]) << 16U | std::to_integer<std::uint32_t>(bytes[3]) << 24U);
    }

    std::uint32_t value;
};

}
