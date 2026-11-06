#pragma once

#include "World/Map/Terrain/geometry.hpp"
#include "World/Map/Terrain/material.hpp"

namespace Northrend {

class MapTile final {
public:
    explicit MapTile(const Adt& adt);

    [[nodiscard]] const TerrainGeometry& terrain() const noexcept;
    [[nodiscard]] const TerrainMaterial& material() const noexcept;

private:
    TerrainGeometry terrainGeometry;
    TerrainMaterial terrainMaterial;
};

}
