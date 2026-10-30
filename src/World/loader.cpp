#include "World/loader.hpp"

#include "Asset/Dbc/table.hpp"
#include "Asset/Stream/priority.hpp"
#include "Asset/adt.hpp"
#include "Asset/system.hpp"
#include "Asset/wdt.hpp"
#include "Thread/pool.hpp"
#include "Thread/work.hpp"
#include "World/Map/catalog.hpp"
#include "World/Map/definition.hpp"
#include "World/Map/request.hpp"
#include "World/Map/result.hpp"
#include "World/exchange.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

namespace Northrend {

WorldLoader::WorldLoader(AssetSystem& assets, ThreadPool& threadPool, WorldExchange& exchange) noexcept :
    assets(assets), threadPool(threadPool), exchange(exchange) {
    this->loading.reserve(WorldExchange::TerrainCapacity);
}

WorldLoader::~WorldLoader() {
    this->stop();
}

void WorldLoader::tick() {
    if (this->stopping) {
        return;
    }

    this->finish_catalog();
    this->start_catalog();
    this->finish_definition();
    this->receive_selections();
    this->start_definition();
    this->finish_parse();
    this->receive_requests();
    this->start_parse();
}

void WorldLoader::stop() noexcept {
    if (this->stopping) {
        return;
    }

    this->stopping = true;
    stop_work(this->catalogWork);
    stop_work(this->definitionWork);
    stop_work(this->parsing);
    this->definitionLoading.reset();
    this->loading.clear();
    this->loadingPosition = 0;
    this->activeMap.clear();
    this->exchange.clear();
}

AssetId WorldLoader::definition_asset(const std::string_view map) {
    if (map.empty()) {
        throw std::invalid_argument("Map name is empty");
    }

    std::string path;
    path.reserve(16 + map.size() * 2);
    path.append("World/Maps/");
    path.append(map);
    path.push_back('/');
    path.append(map);
    path.append(".wdt");
    return AssetId(path);
}

AssetId WorldLoader::terrain_asset(const std::string_view map, const TerrainRequest& request) {
    if (map.empty()) {
        throw std::invalid_argument("Terrain map name is empty");
    }

    std::string path;
    path.reserve(22 + map.size() * 2);
    path.append("World/Maps/");
    path.append(map);
    path.push_back('/');
    path.append(map);
    path.push_back('_');
    path.append(std::to_string(request.x()));
    path.push_back('_');
    path.append(std::to_string(request.y()));
    path.append(".adt");
    return AssetId(path);
}

void WorldLoader::stop_work(std::shared_ptr<Work>& work) noexcept {
    if (!work) {
        return;
    }

    work->cancel();

    try {
        work->wait();
    } catch (...) {
    }

    work.reset();
}

void WorldLoader::finish_catalog() {
    if (!this->catalogWork || !this->catalogWork->ready()) {
        return;
    }

    std::shared_ptr<Work> work = std::move(this->catalogWork);
    work->wait();
}

void WorldLoader::start_catalog() {
    if (this->catalogStarted || !this->assets.databases().ready()) {
        return;
    }

    const DbcTable* table = this->assets.databases().find_file("Map.dbc");

    if (!table) {
        throw std::runtime_error("Map.dbc was not found in the mounted assets");
    }

    this->catalogStarted = true;

    try {
        this->catalogWork = this->threadPool.submit(WorkPriority::background, [this, table] {
            MapCatalog catalog(*table);

            if (!this->exchange.publish(std::move(catalog))) {
                throw std::runtime_error("Map catalog mailbox is full");
            }
        });
    } catch (...) {
        this->catalogStarted = false;
        throw;
    }
}

void WorldLoader::receive_selections() {
    if (!this->assets.ready()) {
        return;
    }

    std::optional<std::pair<MapSelection, std::string>> selection;

    for (std::size_t count = 0; count < WorldExchange::SelectionCapacity; ++count) {
        std::optional<std::pair<MapSelection, std::string>> current = this->exchange.next_selection();

        if (!current) {
            break;
        }

        selection = std::move(current);
    }

    if (!selection) {
        return;
    }

    this->definitionLoading.reset();
    this->activeMap = std::move(selection->second);
    this->definitionLoading.emplace(
        selection->first, this->assets.request(definition_asset(this->activeMap), AssetPriority::critical));
    this->loading.clear();
    this->loadingPosition = 0;

    while (this->exchange.next_request()) {
    }
}

void WorldLoader::finish_definition() {
    if (!this->definitionWork || !this->definitionWork->ready()) {
        return;
    }

    std::shared_ptr<Work> work = std::move(this->definitionWork);
    work->wait();
}

void WorldLoader::start_definition() {
    if (this->definitionWork || !this->definitionLoading || this->exchange.definition_full() ||
        !this->definitionLoading->second.ready()) {
        return;
    }

    MapSelection selection = this->definitionLoading->first;
    AssetRequest assetRequest = std::move(this->definitionLoading->second);
    this->definitionLoading.reset();
    this->definitionWork =
        this->threadPool.submit(WorkPriority::critical, [this, selection, assetRequest = std::move(assetRequest)] {
            MapDefinition definition = [&] {
                try {
                    return MapDefinition(selection, std::make_shared<Wdt>(assetRequest.data()->bytes()));
                } catch (...) {
                    return MapDefinition(selection, std::current_exception());
                }
            }();

            if (!this->exchange.publish(std::move(definition))) {
                throw std::runtime_error("Map definition mailbox is full");
            }
        });
}

void WorldLoader::receive_requests() {
    if (!this->assets.ready()) {
        return;
    }

    for (std::size_t count = 0; count < RequestsPerTick; ++count) {
        std::optional<TerrainRequest> request = this->exchange.next_request();

        if (!request) {
            return;
        }

        AssetPriority priority = AssetPriority::background;

        if (request->distance() == 0) {
            priority = AssetPriority::critical;
        } else if (request->distance() <= 2) {
            priority = AssetPriority::normal;
        }

        this->loading.emplace_back(*request, this->assets.request(terrain_asset(this->activeMap, *request), priority));
    }
}

void WorldLoader::finish_parse() {
    if (!this->parsing || !this->parsing->ready()) {
        return;
    }

    std::shared_ptr<Work> work = std::move(this->parsing);
    work->wait();
}

void WorldLoader::start_parse() {
    if (this->parsing || this->loading.empty() || this->exchange.result_full()) {
        return;
    }

    const std::size_t checks = std::min(RequestsPerTick, this->loading.size());

    for (std::size_t count = 0; count < checks; ++count) {
        if (this->loadingPosition >= this->loading.size()) {
            this->loadingPosition = 0;
        }

        const std::size_t position = this->loadingPosition;

        if (!this->loading[position].second.ready()) {
            ++this->loadingPosition;
            continue;
        }

        TerrainRequest request = this->loading[position].first;
        AssetRequest assetRequest = std::move(this->loading[position].second);

        if (position + 1 != this->loading.size()) {
            this->loading[position] = std::move(this->loading.back());
        }

        this->loading.pop_back();

        if (this->loadingPosition >= this->loading.size()) {
            this->loadingPosition = 0;
        }

        this->parsing =
            this->threadPool.submit(WorkPriority::normal, [this, request, assetRequest = std::move(assetRequest)] {
                TerrainResult result = [&] {
                    try {
                        return TerrainResult(request, std::make_shared<Adt>(assetRequest.data()));
                    } catch (...) {
                        return TerrainResult(request, std::current_exception());
                    }
                }();

                if (!this->exchange.publish(std::move(result))) {
                    throw std::runtime_error("Terrain result mailbox is full");
                }
            });
        return;
    }
}

}
