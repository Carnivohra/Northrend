#include "World/Map/map.hpp"

#include "Asset/wdt.hpp"

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

    if (this->mapTerrain) {
        this->nextTerrainGeneration = next_generation(this->mapTerrain->generation());
    }

    const std::uint32_t generation = this->nextTerrainGeneration;
    this->nextTerrainGeneration = next_generation(generation);
    this->mapTerrain.reset();
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

bool Map::complete(MapDefinition definition) noexcept {
    const MapSelection& selection = definition.selection();

    if (this->mapState != MapState::loading || selection.id() != this->activeIdentifier ||
        selection.generation() != this->activeGeneration) {
        return false;
    }

    if (!definition.succeeded()) {
        this->mapState = MapState::failed;
        return true;
    }

    this->mapTerrain.emplace(this->activeGeneration, std::move(definition).data());
    this->mapState = MapState::ready;
    return true;
}

Terrain* Map::terrain() noexcept {
    return this->mapTerrain ? &*this->mapTerrain : nullptr;
}

const Terrain* Map::terrain() const noexcept {
    return this->mapTerrain ? &*this->mapTerrain : nullptr;
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
    if (this->mapTerrain) {
        this->nextTerrainGeneration = next_generation(this->mapTerrain->generation());
    }

    this->mapTerrain.reset();
    this->mapDirectory.clear();
    this->activeGeneration = 0;
    this->activeIdentifier = 0;
    this->mapState = MapState::empty;
}

std::uint32_t Map::next_generation(const std::uint32_t generation) noexcept {
    return generation == std::numeric_limits<std::uint32_t>::max() ? 1 : generation + 1;
}

}
