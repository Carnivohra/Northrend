#include "World/Map/exchange.hpp"

#include <utility>

namespace Northrend {

bool MapExchange::submit(const std::string_view map, const MapSelection& selection) {
    if (this->selections.full()) {
        return false;
    }

    return this->selections.try_emplace(selection, map);
}

bool MapExchange::submit(const TileRequest& request) {
    if (this->tileRequests.full()) {
        return false;
    }

    return this->tileRequests.try_push(request);
}

std::optional<MapCatalog> MapExchange::catalog() {
    return this->catalogs.try_pop();
}

std::optional<MapResult> MapExchange::map() {
    return this->maps.try_pop();
}

std::optional<TileResult> MapExchange::tile() {
    return this->tiles.try_pop();
}

std::optional<std::pair<MapSelection, std::string>> MapExchange::next_selection() {
    return this->selections.try_pop();
}

std::optional<TileRequest> MapExchange::next_tile_request() {
    return this->tileRequests.try_pop();
}

bool MapExchange::publish(MapCatalog catalog) {
    return this->catalogs.try_push(std::move(catalog));
}

bool MapExchange::publish(MapResult result) {
    return this->maps.try_push(std::move(result));
}

bool MapExchange::publish(TileResult result) {
    return this->tiles.try_push(std::move(result));
}

bool MapExchange::map_full() const noexcept {
    return this->maps.full();
}

bool MapExchange::tiles_full() const noexcept {
    return this->tiles.full();
}

void MapExchange::clear() {
    this->selections.clear();
    this->tileRequests.clear();
    this->catalogs.clear();
    this->maps.clear();
    this->tiles.clear();
}

}
