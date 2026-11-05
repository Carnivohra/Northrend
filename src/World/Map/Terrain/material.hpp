#pragma once

#include "Asset/Format/adt.hpp"
#include "Asset/id.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Northrend {

class TerrainMaterial final {
public:
    TerrainMaterial(Adt adt, bool highResolutionAlpha);

    [[nodiscard]] std::size_t texture_count() const noexcept;
    [[nodiscard]] const AssetId& texture(std::size_t texture) const;
    [[nodiscard]] std::size_t layer_count(std::size_t x, std::size_t y) const;
    [[nodiscard]] const AssetId& layer_texture(std::size_t x, std::size_t y, std::size_t layer) const;
    [[nodiscard]] std::uint32_t layer_flags(std::size_t x, std::size_t y, std::size_t layer) const;
    [[nodiscard]] std::uint32_t layer_effect(std::size_t x, std::size_t y, std::size_t layer) const;
    void decode_alpha(
        std::size_t x, std::size_t y, std::size_t layer, std::span<std::uint8_t, Adt::AlphaTexels> output) const;

private:
    Adt terrainData;
    std::vector<AssetId> textures;
    bool highResolutionAlpha;
};

}
