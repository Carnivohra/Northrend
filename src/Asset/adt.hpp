#pragma once

#include "Asset/data.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace Northrend {

class Adt final {
public:
    static constexpr std::size_t Size = 16;
    static constexpr std::size_t ChunkCount = Size * Size;
    static constexpr std::size_t HeightsPerChunk = 145;

    explicit Adt(std::shared_ptr<const AssetData> data);

    [[nodiscard]] std::uint32_t version() const noexcept;
    [[nodiscard]] float position_x(std::size_t x, std::size_t y) const;
    [[nodiscard]] float position_y(std::size_t x, std::size_t y) const;
    [[nodiscard]] float base_height(std::size_t x, std::size_t y) const;
    [[nodiscard]] float height_offset(std::size_t x, std::size_t y, std::size_t vertex) const;
    [[nodiscard]] float height(std::size_t x, std::size_t y, std::size_t vertex) const;
    [[nodiscard]] std::uint32_t holes(std::size_t x, std::size_t y) const;

private:
    [[nodiscard]] static std::size_t index(std::size_t x, std::size_t y);
    [[nodiscard]] static std::uint32_t read_uint32(std::span<const std::byte> bytes, std::size_t offset);
    [[nodiscard]] static float read_float(std::span<const std::byte> bytes, std::size_t offset);

    std::shared_ptr<const AssetData> data;
    std::array<std::span<const std::byte>, ChunkCount> heightData;
    std::array<float, ChunkCount> positionsX {};
    std::array<float, ChunkCount> positionsY {};
    std::array<float, ChunkCount> baseHeights {};
    std::array<std::uint32_t, ChunkCount> holeMasks {};
    std::uint32_t fileVersion = 0;
};

}
