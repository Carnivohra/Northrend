#pragma once

#include "Asset/data.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace Northrend {

class Adt final {
public:
    static constexpr std::size_t Size = 16;
    static constexpr std::size_t ChunkCount = Size * Size;
    static constexpr std::size_t HeightsPerChunk = 145;
    static constexpr std::size_t MaximumLayers = 4;
    static constexpr std::size_t AlphaSize = 64;
    static constexpr std::size_t AlphaTexels = AlphaSize * AlphaSize;

    explicit Adt(std::shared_ptr<const AssetData> data);

    [[nodiscard]] std::uint32_t version() const noexcept;
    [[nodiscard]] float position_x(std::size_t x, std::size_t y) const;
    [[nodiscard]] float position_y(std::size_t x, std::size_t y) const;
    [[nodiscard]] float base_height(std::size_t x, std::size_t y) const;
    [[nodiscard]] float height_offset(std::size_t x, std::size_t y, std::size_t vertex) const;
    [[nodiscard]] float height(std::size_t x, std::size_t y, std::size_t vertex) const;
    [[nodiscard]] std::array<std::int8_t, 3> normal(std::size_t x, std::size_t y, std::size_t vertex) const;
    [[nodiscard]] std::uint32_t holes(std::size_t x, std::size_t y) const;
    [[nodiscard]] std::size_t texture_count() const noexcept;
    [[nodiscard]] std::string_view texture_name(std::size_t texture) const;
    [[nodiscard]] std::size_t layer_count(std::size_t x, std::size_t y) const;
    [[nodiscard]] std::uint32_t layer_texture(std::size_t x, std::size_t y, std::size_t layer) const;
    [[nodiscard]] std::uint32_t layer_flags(std::size_t x, std::size_t y, std::size_t layer) const;
    [[nodiscard]] std::uint32_t layer_effect(std::size_t x, std::size_t y, std::size_t layer) const;
    [[nodiscard]] std::span<const std::byte> alpha_data(std::size_t x, std::size_t y, std::size_t layer) const;
    void decode_alpha(std::size_t x, std::size_t y, std::size_t layer, bool highResolution,
        std::span<std::uint8_t, AlphaTexels> output) const;

private:
    static constexpr std::size_t LayerSize = 16;
    static constexpr std::uint32_t LayerHasAlpha = 0x100;
    static constexpr std::uint32_t LayerAlphaCompressed = 0x200;
    static constexpr std::uint32_t DoNotFixAlpha = 0x8000;

    [[nodiscard]] static std::size_t index(std::size_t x, std::size_t y);
    [[nodiscard]] std::size_t layer_offset(std::size_t x, std::size_t y, std::size_t layer) const;
    static void decode_alpha_rle(std::span<const std::byte> input, std::span<std::uint8_t, AlphaTexels> output);
    static void decode_alpha_packed(std::span<const std::byte> input, std::span<std::uint8_t, AlphaTexels> output);
    static void correct_alpha_edges(std::span<std::uint8_t, AlphaTexels> alpha) noexcept;
    [[nodiscard]] static std::uint32_t read_uint32(std::span<const std::byte> bytes, std::size_t offset);
    [[nodiscard]] static float read_float(std::span<const std::byte> bytes, std::size_t offset);

    std::shared_ptr<const AssetData> data;
    std::array<std::span<const std::byte>, ChunkCount> heightData;
    std::array<std::span<const std::byte>, ChunkCount> normalData;
    std::array<std::span<const std::byte>, ChunkCount> layerData;
    std::array<std::span<const std::byte>, ChunkCount> alphaData;
    std::array<float, ChunkCount> positionsX {};
    std::array<float, ChunkCount> positionsY {};
    std::array<float, ChunkCount> baseHeights {};
    std::array<std::uint32_t, ChunkCount> holeMasks {};
    std::array<std::uint32_t, ChunkCount> terrainFlags {};
    std::array<std::uint8_t, ChunkCount> layerCounts {};
    std::vector<std::string_view> textureNames;
    std::uint32_t fileVersion = 0;
};

}
