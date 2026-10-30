#include "World/world.hpp"

#include "World/exchange.hpp"

#include <chrono>
#include <optional>
#include <stdexcept>
#include <utility>

namespace Northrend {

World::World(WorldExchange& exchange) noexcept : exchange(exchange) {}

void World::tick(const ThreadDuration delta) {
    this->receive_catalog();
    this->receive_definition();
    this->receive_terrain();
    this->worldRegistry.progress(std::chrono::duration<float>(delta));
    this->request_selection();
    this->request_terrain();
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

void World::receive_definition() {
    std::optional<MapDefinition> definition = this->exchange.definition();

    if (definition) {
        static_cast<void>(this->worldMap.complete(std::move(*definition)));
    }
}

void World::receive_terrain() {
    Terrain* terrain = this->worldMap.terrain();

    for (std::size_t count = 0; count < TerrainTransfersPerTick; ++count) {
        std::optional<TerrainResult> result = this->exchange.result();

        if (!result) {
            return;
        }

        if (terrain) {
            static_cast<void>(terrain->complete(std::move(*result)));
        }
    }
}

void World::request_selection() {
    const std::optional<MapSelection> selection = this->worldMap.next_selection();

    if (!selection) {
        return;
    }

    if (this->exchange.submit(this->worldMap.directory(), *selection)) {
        return;
    }

    if (!this->worldMap.defer(*selection)) {
        throw std::logic_error("Map selection could not be deferred");
    }
}

void World::request_terrain() {
    Terrain* terrain = this->worldMap.terrain();

    if (!terrain) {
        return;
    }

    for (std::size_t count = 0; count < TerrainTransfersPerTick; ++count) {
        const std::optional<TerrainRequest> request = terrain->next_request();

        if (!request) {
            return;
        }

        if (this->exchange.submit(*request)) {
            continue;
        }

        if (!terrain->defer(*request)) {
            throw std::logic_error("Terrain request could not be deferred");
        }

        return;
    }
}

}
