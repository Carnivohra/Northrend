#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Northrend {

class Wdt final {
public:
    static constexpr std::size_t Size = 64;

    explicit Wdt(std::span<const std::byte> bytes);

    [[nodiscard]] std::uint32_t version() const noexcept;
    [[nodiscard]] std::uint32_t flags() const noexcept;
    [[nodiscard]] bool has_tile(std::size_t x, std::size_t y) const noexcept;
    [[nodiscard]] std::size_t tile_count() const noexcept;

private:
    [[nodiscard]] static std::uint32_t read_uint32(std::span<const std::byte> bytes, std::size_t offset);

    std::bitset<Size * Size> tiles;
    std::uint32_t fileVersion = 0;
    std::uint32_t headerFlags = 0;
};

}
