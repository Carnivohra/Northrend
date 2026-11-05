#include "World/Map/Terrain/material.hpp"

#include <utility>

namespace Northrend {

TerrainMaterial::TerrainMaterial(Adt adt, const bool highResolutionAlpha) :
    terrainData(std::move(adt)), highResolutionAlpha(highResolutionAlpha) {
    this->textures.reserve(this->terrainData.texture_count());

    for (std::size_t texture = 0; texture < this->terrainData.texture_count(); ++texture) {
        this->textures.emplace_back(this->terrainData.texture_name(texture));
    }
}

std::size_t TerrainMaterial::texture_count() const noexcept {
    return this->textures.size();
}

const AssetId& TerrainMaterial::texture(const std::size_t texture) const {
    return this->textures.at(texture);
}

std::size_t TerrainMaterial::layer_count(const std::size_t x, const std::size_t y) const {
    return this->terrainData.layer_count(x, y);
}

const AssetId& TerrainMaterial::layer_texture(const std::size_t x, const std::size_t y, const std::size_t layer) const {
    return this->texture(this->terrainData.layer_texture(x, y, layer));
}

std::uint32_t TerrainMaterial::layer_flags(const std::size_t x, const std::size_t y, const std::size_t layer) const {
    return this->terrainData.layer_flags(x, y, layer);
}

std::uint32_t TerrainMaterial::layer_effect(const std::size_t x, const std::size_t y, const std::size_t layer) const {
    return this->terrainData.layer_effect(x, y, layer);
}

void TerrainMaterial::decode_alpha(const std::size_t x, const std::size_t y, const std::size_t layer,
    const std::span<std::uint8_t, Adt::AlphaTexels> output) const {
    this->terrainData.decode_alpha(x, y, layer, this->highResolutionAlpha, output);
}

}
