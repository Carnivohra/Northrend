#include "World/Map/map.hpp"

namespace Northrend {

MapCatalog& Map::catalog() noexcept {
    return this->mapCatalog;
}

const MapCatalog& Map::catalog() const noexcept {
    return this->mapCatalog;
}

Terrain& Map::create_terrain() {
    return this->mapTerrain.emplace();
}

Terrain* Map::terrain() noexcept {
    return this->mapTerrain ? &*this->mapTerrain : nullptr;
}

const Terrain* Map::terrain() const noexcept {
    return this->mapTerrain ? &*this->mapTerrain : nullptr;
}

void Map::clear_terrain() noexcept {
    this->mapTerrain.reset();
}

}
