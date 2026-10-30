#include "World/Map/map.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace Northrend {

MapCatalog& Map::catalog() noexcept {
    return this->mapCatalog;
}

const MapCatalog& Map::catalog() const noexcept {
    return this->mapCatalog;
}

void Map::select(const std::uint32_t map) {
    const MapEntry* entry = this->mapCatalog.find(map);

    if (!entry) {
        throw std::out_of_range("Map is not present in the map catalog");
    }

    if (this->mapTiles) {
        this->nextTileGeneration = next_generation(this->mapTiles->generation());
    }

    const std::uint32_t generation = this->nextTileGeneration;
    this->nextTileGeneration = next_generation(generation);
    this->mapTiles.reset();
    this->mapDirectory = entry->directory();
    this->activeGeneration = generation;
    this->activeIdentifier = map;
    this->mapState = MapState::requested;
}

std::optional<MapSelection> Map::next_selection() noexcept {
    if (this->mapState != MapState::requested) {
        return std::nullopt;
    }

    this->mapState = MapState::loading;
    return MapSelection(this->activeIdentifier, this->activeGeneration);
}

bool Map::defer(const MapSelection& selection) noexcept {
    if (this->mapState != MapState::loading || selection.id() != this->activeIdentifier ||
        selection.generation() != this->activeGeneration) {
        return false;
    }

    this->mapState = MapState::requested;
    return true;
}

bool Map::complete(MapResult result) noexcept {
    const MapSelection& selection = result.selection();

    if (this->mapState != MapState::loading || selection.id() != this->activeIdentifier ||
        selection.generation() != this->activeGeneration) {
        return false;
    }

    if (!result.succeeded()) {
        this->mapState = MapState::failed;
        return true;
    }

    this->mapTiles.emplace(this->activeGeneration, std::move(result).data());
    this->mapState = MapState::ready;
    return true;
}

TileGrid* Map::tiles() noexcept {
    return this->mapTiles ? &*this->mapTiles : nullptr;
}

const TileGrid* Map::tiles() const noexcept {
    return this->mapTiles ? &*this->mapTiles : nullptr;
}

std::string_view Map::directory() const noexcept {
    return this->mapState != MapState::empty ? std::string_view(this->mapDirectory) : std::string_view {};
}

MapState Map::state() const noexcept {
    return this->mapState;
}

std::uint32_t Map::id() const noexcept {
    return this->activeIdentifier;
}

void Map::clear() noexcept {
    if (this->mapTiles) {
        this->nextTileGeneration = next_generation(this->mapTiles->generation());
    }

    this->mapTiles.reset();
    this->mapDirectory.clear();
    this->activeGeneration = 0;
    this->activeIdentifier = 0;
    this->mapState = MapState::empty;
}

std::uint32_t Map::next_generation(const std::uint32_t generation) noexcept {
    return generation == std::numeric_limits<std::uint32_t>::max() ? 1 : generation + 1;
}

}
