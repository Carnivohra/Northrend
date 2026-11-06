#pragma once

#include "Asset/Format/adt.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Northrend {

class TerrainPayload final {
public:
    static constexpr std::size_t MaximumMipCount = 7;

    TerrainPayload(const Adt& adt, bool highResolutionAlpha);

    [[nodiscard]] std::uint16_t alpha_layer(std::size_t x, std::size_t y) const;
    [[nodiscard]] std::size_t alpha_layer_count() const noexcept;
    [[nodiscard]] std::size_t alpha_mip_count() const noexcept;
    [[nodiscard]] std::span<const std::byte> alpha_mip(std::size_t level) const;

private:
    std::array<std::uint16_t, Adt::ChunkCount> alphaLayers {};
    std::array<std::size_t, MaximumMipCount> alphaMipOffsets {};
    std::array<std::size_t, MaximumMipCount> alphaMipSizes {};
    std::vector<std::byte> alphaMaps;
    std::uint16_t alphaLayerCount = 0;
    std::uint8_t alphaMipCount = 0;
};

}
