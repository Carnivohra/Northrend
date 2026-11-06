#include "World/Map/Terrain/material.hpp"

#include <stdexcept>

namespace Northrend {

TerrainMaterial::TerrainMaterial(const Adt& adt) {
    this->textures.reserve(adt.texture_count());

    for (std::size_t texture = 0; texture < adt.texture_count(); ++texture) {
        this->textures.emplace_back(adt.texture_name(texture));
    }

    for (std::size_t y = 0; y < Adt::Size; ++y) {
        for (std::size_t x = 0; x < Adt::Size; ++x) {
            const std::size_t layerCount = adt.layer_count(x, y);
            this->layerCounts[index(x, y)] = static_cast<std::uint8_t>(layerCount);

            for (std::size_t layer = 0; layer < layerCount; ++layer) {
                const std::size_t position = layer_index(x, y, layer);
                this->layerTextures[position] = adt.layer_texture(x, y, layer);
                this->layerFlags[position] = adt.layer_flags(x, y, layer);
                this->layerEffects[position] = adt.layer_effect(x, y, layer);
            }
        }
    }
}

std::size_t TerrainMaterial::texture_count() const noexcept {
    return this->textures.size();
}

const AssetId& TerrainMaterial::texture(const std::size_t texture) const {
    return this->textures.at(texture);
}

std::size_t TerrainMaterial::layer_count(const std::size_t x, const std::size_t y) const {
    return this->layerCounts.at(index(x, y));
}

const AssetId& TerrainMaterial::layer_texture(const std::size_t x, const std::size_t y, const std::size_t layer) const {
    if (layer >= this->layer_count(x, y)) {
        throw std::out_of_range("Terrain material layer is outside the chunk");
    }

    return this->texture(this->layerTextures[layer_index(x, y, layer)]);
}

std::uint32_t TerrainMaterial::layer_flags(const std::size_t x, const std::size_t y, const std::size_t layer) const {
    if (layer >= this->layer_count(x, y)) {
        throw std::out_of_range("Terrain material layer is outside the chunk");
    }

    return this->layerFlags[layer_index(x, y, layer)];
}

std::uint32_t TerrainMaterial::layer_effect(const std::size_t x, const std::size_t y, const std::size_t layer) const {
    if (layer >= this->layer_count(x, y)) {
        throw std::out_of_range("Terrain material layer is outside the chunk");
    }

    return this->layerEffects[layer_index(x, y, layer)];
}

std::size_t TerrainMaterial::index(const std::size_t x, const std::size_t y) {
    if (x >= Adt::Size || y >= Adt::Size) {
        throw std::out_of_range("Terrain material coordinate is outside the tile");
    }

    return y * Adt::Size + x;
}

std::size_t TerrainMaterial::layer_index(const std::size_t x, const std::size_t y, const std::size_t layer) {
    return index(x, y) * Adt::MaximumLayers + layer;
}

}
