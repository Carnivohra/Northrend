#include "World/world.hpp"

#include "World/Map/exchange.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace Northrend {

World::World(MapExchange& exchange) noexcept : exchange(exchange) {
    this->tileChanges.reserve(TileChangesPerTick);
}

void World::tick(const ThreadDuration delta) {
    this->receive_catalog();
    this->receive_map();
    this->receive_tiles();
    this->worldRegistry.progress(std::chrono::duration<float>(delta));
    this->request_selection();
    this->request_tiles();
}

std::optional<TileChange> World::next_tile_change() {
    if (this->tileChangePosition >= this->tileChanges.size()) {
        this->tileChanges.clear();
        this->tileChangePosition = 0;
        return std::nullopt;
    }

    TileChange change = std::move(this->tileChanges[this->tileChangePosition++]);

    if (this->tileChangePosition == this->tileChanges.size()) {
        this->tileChanges.clear();
        this->tileChangePosition = 0;
    }

    return change;
}

WorldRegistry& World::registry() noexcept {
    return this->worldRegistry;
}

const WorldRegistry& World::registry() const noexcept {
    return this->worldRegistry;
}

Map& World::map() noexcept {
    return this->worldMap;
}

const Map& World::map() const noexcept {
    return this->worldMap;
}

void World::receive_catalog() {
    std::optional<MapCatalog> catalog = this->exchange.catalog();

    if (catalog) {
        this->worldMap.catalog() = std::move(*catalog);
    }
}

void World::receive_map() {
    std::optional<MapResult> result = this->exchange.map();

    if (!result || !this->worldMap.complete(std::move(*result))) {
        return;
    }

    TileGrid* tiles = this->worldMap.tiles();

    if (tiles) {
        tiles->load_all();
    }
}

void World::receive_tiles() {
    TileGrid* tiles = this->worldMap.tiles();

    for (std::size_t count = 0; count < TileChangesPerTick; ++count) {
        std::optional<TileResult> result = this->exchange.tile();

        if (!result) {
            return;
        }

        if (tiles) {
            const TileRequest request = result->request();
            std::shared_ptr<const MapTile> tile = result->data();

            if (tiles->complete(std::move(*result)) && tile) {
                this->tileChanges.emplace_back(request, std::move(tile));
            }
        }
    }
}

void World::request_selection() {
    const std::optional<MapSelection> selection = this->worldMap.next_selection();

    if (!selection) {
        return;
    }

    if (this->exchange.submit(this->worldMap.directory(), *selection)) {
        this->tileChanges.emplace_back(selection->generation());
        return;
    }

    if (!this->worldMap.defer(*selection)) {
        throw std::logic_error("Map selection could not be deferred");
    }
}

void World::request_tiles() {
    TileGrid* tiles = this->worldMap.tiles();

    if (!tiles) {
        return;
    }

    for (std::size_t count = 0; count < TileChangesPerTick; ++count) {
        const std::optional<TileRequest> request = tiles->next_request();

        if (!request) {
            return;
        }

        if (this->exchange.submit(*request)) {
            continue;
        }

        if (!tiles->defer(*request)) {
            throw std::logic_error("Map tile request could not be deferred");
        }

        return;
    }
}

}
