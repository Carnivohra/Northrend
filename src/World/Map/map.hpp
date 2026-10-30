#pragma once

#include "World/Map/catalog.hpp"
#include "World/Map/terrain.hpp"

#include <optional>

namespace Northrend {

class Map final {
public:
    [[nodiscard]] MapCatalog& catalog() noexcept;
    [[nodiscard]] const MapCatalog& catalog() const noexcept;
    [[nodiscard]] Terrain& create_terrain();
    [[nodiscard]] Terrain* terrain() noexcept;
    [[nodiscard]] const Terrain* terrain() const noexcept;
    void clear_terrain() noexcept;

private:
    MapCatalog mapCatalog;
    std::optional<Terrain> mapTerrain;
};

}
