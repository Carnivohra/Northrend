#pragma once

#include "World/Map/catalog.hpp"

namespace Northrend {

class Map final {
public:
    [[nodiscard]] MapCatalog& catalog() noexcept;
    [[nodiscard]] const MapCatalog& catalog() const noexcept;

private:
    MapCatalog mapCatalog;
};

}
