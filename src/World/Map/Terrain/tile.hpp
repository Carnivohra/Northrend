#pragma once

#include "World/Map/Terrain/geometry.hpp"

namespace Northrend {

class Adt;

class TerrainTile final {
public:
    explicit TerrainTile(const Adt& adt);

    [[nodiscard]] const TerrainGeometry& geometry() const noexcept;

private:
    TerrainGeometry terrainGeometry;
};

}
