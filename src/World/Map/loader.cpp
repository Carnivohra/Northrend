#include "World/Map/loader.hpp"

#include "Asset/Dbc/table.hpp"
#include "Asset/Stream/priority.hpp"
#include "Asset/system.hpp"
#include "Thread/pool.hpp"
#include "Thread/work.hpp"
#include "World/Map/catalog.hpp"
#include "World/Map/exchange.hpp"
#include "World/Map/layout.hpp"
#include "World/Map/result.hpp"
#include "World/Map/Tile/job.hpp"
#include "World/Map/Tile/result.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

namespace Northrend {

MapLoader::MapLoader(AssetSystem& assets, ThreadPool& threadPool, MapExchange& exchange) noexcept :
    assets(assets), threadPool(threadPool), exchange(exchange), parseCapacity(threadPool.worker_count()) {
    this->tileLoading.reserve(MapExchange::TileCapacity);
    this->tileParsing.reserve(this->parseCapacity);
}

MapLoader::~MapLoader() {
    this->stop();
}

void MapLoader::tick() {
    if (this->stopping) {
        return;
    }

    this->finish_catalog();
    this->start_catalog();
    this->finish_map();
    this->receive_selections();
    this->start_map();
    this->finish_parse();
    this->receive_tile_requests();
    this->start_parse();
}

void MapLoader::stop() noexcept {
    if (this->stopping) {
        return;
    }

    this->stopping = true;
    stop_work(this->catalogWork);
    stop_work(this->mapWork);

    for (const std::unique_ptr<TileJob>& job : this->tileParsing) {
        job->stop();
    }

    this->tileParsing.clear();
    this->mapLoading.reset();
    this->tileLoading.clear();
    this->loadingPosition = 0;
    this->activeMap.clear();
    this->exchange.clear();
}

AssetId MapLoader::map_asset(const std::string_view map) {
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

AssetId MapLoader::tile_asset(const std::string_view map, const TileRequest& request) {
    if (map.empty()) {
        throw std::invalid_argument("Map tile name is empty");
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

void MapLoader::stop_work(std::shared_ptr<Work>& work) noexcept {
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

void MapLoader::finish_catalog() {
    if (!this->catalogWork || !this->catalogWork->ready()) {
        return;
    }

    std::shared_ptr<Work> work = std::move(this->catalogWork);
    work->wait();
}

void MapLoader::start_catalog() {
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

void MapLoader::receive_selections() {
    if (!this->assets.ready()) {
        return;
    }

    std::optional<std::pair<MapSelection, std::string>> selection;

    for (std::size_t count = 0; count < MapExchange::SelectionCapacity; ++count) {
        std::optional<std::pair<MapSelection, std::string>> current = this->exchange.next_selection();

        if (!current) {
            break;
        }

        selection = std::move(current);
    }

    if (!selection) {
        return;
    }

    this->mapLoading.reset();
    this->activeMap = std::move(selection->second);
    this->mapLoading.emplace(
        selection->first, this->assets.request(map_asset(this->activeMap), AssetPriority::critical));
    this->tileLoading.clear();
    this->loadingPosition = 0;

    while (this->exchange.next_tile_request()) {
    }
}

void MapLoader::finish_map() {
    if (!this->mapWork || !this->mapWork->ready()) {
        return;
    }

    std::shared_ptr<Work> work = std::move(this->mapWork);
    work->wait();
}

void MapLoader::start_map() {
    if (this->mapWork || !this->mapLoading || this->exchange.map_full() || !this->mapLoading->second.ready()) {
        return;
    }

    MapSelection selection = this->mapLoading->first;
    AssetRequest assetRequest = std::move(this->mapLoading->second);
    this->mapLoading.reset();
    this->mapWork =
        this->threadPool.submit(WorkPriority::critical, [this, selection, assetRequest = std::move(assetRequest)] {
            MapResult result = [&] {
                try {
                    return MapResult(selection, std::make_shared<MapLayout>(assetRequest.data()->bytes()));
                } catch (...) {
                    return MapResult(selection, std::current_exception());
                }
            }();

            if (!this->exchange.publish(std::move(result))) {
                throw std::runtime_error("Map result mailbox is full");
            }
        });
}

void MapLoader::receive_tile_requests() {
    if (!this->assets.ready()) {
        return;
    }

    for (std::size_t count = 0; count < RequestsPerTick; ++count) {
        std::optional<TileRequest> request = this->exchange.next_tile_request();

        if (!request) {
            return;
        }

        AssetPriority priority = AssetPriority::background;

        if (request->distance() == 0) {
            priority = AssetPriority::critical;
        } else if (request->distance() <= 2) {
            priority = AssetPriority::normal;
        }

        this->tileLoading.emplace_back(
            *request, this->assets.request(tile_asset(this->activeMap, *request), priority));
    }
}

void MapLoader::finish_parse() {
    std::size_t position = 0;
    std::size_t completed = 0;

    while (position < this->tileParsing.size() && completed < RequestsPerTick && !this->exchange.tiles_full()) {
        TileJob& job = *this->tileParsing[position];

        if (!job.ready()) {
            ++position;
            continue;
        }

        std::optional<TileResult> result = job.finish();

        if (position + 1 != this->tileParsing.size()) {
            this->tileParsing[position] = std::move(this->tileParsing.back());
        }

        this->tileParsing.pop_back();
        ++completed;

        if (result && !this->exchange.publish(std::move(*result))) {
            throw std::runtime_error("Map tile result mailbox is full");
        }
    }
}

void MapLoader::start_parse() {
    while (this->tileParsing.size() < this->parseCapacity && !this->tileLoading.empty()) {
        const std::size_t checks = std::min(RequestsPerTick, this->tileLoading.size());
        bool started = false;

        for (std::size_t count = 0; count < checks; ++count) {
            if (this->loadingPosition >= this->tileLoading.size()) {
                this->loadingPosition = 0;
            }

            const std::size_t position = this->loadingPosition;

            if (!this->tileLoading[position].second.ready()) {
                ++this->loadingPosition;
                continue;
            }

            TileRequest request = this->tileLoading[position].first;
            AssetRequest assetRequest = std::move(this->tileLoading[position].second);

            if (position + 1 != this->tileLoading.size()) {
                this->tileLoading[position] = std::move(this->tileLoading.back());
            }

            this->tileLoading.pop_back();

            if (this->loadingPosition >= this->tileLoading.size()) {
                this->loadingPosition = 0;
            }

            auto job = std::make_unique<TileJob>(request, std::move(assetRequest));
            job->start(this->threadPool);
            this->tileParsing.push_back(std::move(job));
            started = true;
            break;
        }

        if (!started) {
            return;
        }
    }
}

}
