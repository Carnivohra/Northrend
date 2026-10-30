#pragma once

#include <cstddef>
#include <cstdint>

namespace Northrend {

class TileGrid;

class TileRequest final {
public:
    [[nodiscard]] std::size_t x() const noexcept;
    [[nodiscard]] std::size_t y() const noexcept;
    [[nodiscard]] std::size_t distance() const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;

private:
    friend class TileGrid;

    static constexpr std::size_t Size = 64;

    TileRequest(std::uint16_t tile, std::uint8_t distance, std::uint32_t generation) noexcept;

    std::uint16_t tile;
    std::uint8_t tileDistance;
    std::uint32_t requestGeneration;
};

}
