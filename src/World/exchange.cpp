#include "World/exchange.hpp"

#include <utility>

namespace Northrend {

bool WorldExchange::submit(const std::string_view map, const MapSelection& selection) {
    if (this->selections.full()) {
        return false;
    }

    return this->selections.try_emplace(selection, map);
}

bool WorldExchange::submit(const TerrainRequest& request) {
    if (this->requests.full()) {
        return false;
    }

    return this->requests.try_push(request);
}

std::optional<MapCatalog> WorldExchange::catalog() {
    return this->catalogs.try_pop();
}

std::optional<MapDefinition> WorldExchange::definition() {
    return this->definitions.try_pop();
}

std::optional<TerrainResult> WorldExchange::result() {
    return this->results.try_pop();
}

std::optional<std::pair<MapSelection, std::string>> WorldExchange::next_selection() {
    return this->selections.try_pop();
}

std::optional<TerrainRequest> WorldExchange::next_request() {
    return this->requests.try_pop();
}

bool WorldExchange::publish(MapCatalog catalog) {
    return this->catalogs.try_push(std::move(catalog));
}

bool WorldExchange::publish(MapDefinition definition) {
    return this->definitions.try_push(std::move(definition));
}

bool WorldExchange::publish(TerrainResult result) {
    return this->results.try_push(std::move(result));
}

bool WorldExchange::definition_full() const noexcept {
    return this->definitions.full();
}

bool WorldExchange::result_full() const noexcept {
    return this->results.full();
}

void WorldExchange::clear() {
    this->selections.clear();
    this->requests.clear();
    this->catalogs.clear();
    this->definitions.clear();
    this->results.clear();
}

}
