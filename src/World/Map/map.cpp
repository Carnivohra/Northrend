#include "World/Map/map.hpp"

namespace Northrend {

MapCatalog& Map::catalog() noexcept {
    return this->mapCatalog;
}

const MapCatalog& Map::catalog() const noexcept {
    return this->mapCatalog;
}


}
