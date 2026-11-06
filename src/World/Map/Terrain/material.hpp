#pragma once

#include "Asset/Format/adt.hpp"
#include "Asset/id.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Northrend {

class TerrainMaterial final {
public:
    explicit TerrainMaterial(const Adt& adt);

    [[nodiscard]] std::size_t texture_count() const noexcept;
    [[nodiscard]] const AssetId& texture(std::size_t texture) const;
    [[nodiscard]] std::size_t layer_count(std::size_t x, std::size_t y) const;
    [[nodiscard]] const AssetId& layer_texture(std::size_t x, std::size_t y, std::size_t layer) const;
    [[nodiscard]] std::uint32_t layer_flags(std::size_t x, std::size_t y, std::size_t layer) const;
    [[nodiscard]] std::uint32_t layer_effect(std::size_t x, std::size_t y, std::size_t layer) const;

private:
    [[nodiscard]] static std::size_t index(std::size_t x, std::size_t y);
    [[nodiscard]] static std::size_t layer_index(std::size_t x, std::size_t y, std::size_t layer);

    std::vector<AssetId> textures;
    std::array<std::uint32_t, Adt::ChunkCount * Adt::MaximumLayers> layerTextures {};
    std::array<std::uint32_t, Adt::ChunkCount * Adt::MaximumLayers> layerFlags {};
    std::array<std::uint32_t, Adt::ChunkCount * Adt::MaximumLayers> layerEffects {};
    std::array<std::uint8_t, Adt::ChunkCount> layerCounts {};
};

}
