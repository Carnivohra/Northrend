#pragma once

#include "Asset/Stream/request.hpp"
#include "Asset/id.hpp"
#include "World/Map/selection.hpp"
#include "World/Map/Tile/request.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Northrend {

class AssetSystem;
class MapExchange;
class ThreadPool;
class Work;
class TileJob;

class MapLoader final {
public:
    MapLoader(AssetSystem& assets, ThreadPool& threadPool, MapExchange& exchange) noexcept;
    ~MapLoader();

    MapLoader(const MapLoader&) = delete;
    MapLoader& operator=(const MapLoader&) = delete;
    MapLoader(MapLoader&&) = delete;
    MapLoader& operator=(MapLoader&&) = delete;

    void tick();
    void stop() noexcept;

private:
    [[nodiscard]] static AssetId map_asset(std::string_view map);
    [[nodiscard]] static AssetId tile_asset(std::string_view map, const TileRequest& request);
    static void stop_work(std::shared_ptr<Work>& work) noexcept;
    void finish_catalog();
    void start_catalog();
    void receive_selections();
    void finish_map();
    void start_map();
    void receive_tile_requests();
    void finish_parse();
    void start_parse();

    static constexpr std::size_t RequestsPerTick = 16;

    AssetSystem& assets;
    ThreadPool& threadPool;
    MapExchange& exchange;
    std::optional<std::pair<MapSelection, AssetRequest>> mapLoading;
    std::vector<std::pair<TileRequest, AssetRequest>> tileLoading;
    std::vector<std::unique_ptr<TileJob>> tileParsing;
    std::shared_ptr<Work> catalogWork;
    std::shared_ptr<Work> mapWork;
    std::string activeMap;
    std::size_t loadingPosition = 0;
    const std::size_t parseCapacity;
    bool catalogStarted = false;
    bool stopping = false;
};

}
