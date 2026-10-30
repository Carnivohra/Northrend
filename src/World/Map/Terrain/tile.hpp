#pragma once

#include "Asset/adt.hpp"
#include "World/Map/Terrain/geometry.hpp"

#include <memory>

namespace Northrend {

class AssetData;

class TerrainTile final {
public:
    explicit TerrainTile(std::shared_ptr<const AssetData> data);

    [[nodiscard]] const Adt& adt() const noexcept;
    [[nodiscard]] const TerrainGeometry& geometry() const noexcept;

private:
    Adt terrainData;
    TerrainGeometry terrainGeometry;
};

}
