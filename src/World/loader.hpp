#pragma once

#include "Asset/Stream/request.hpp"
#include "Asset/id.hpp"
#include "World/Map/request.hpp"
#include "World/Map/selection.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Northrend {

class AssetSystem;
class ThreadPool;
class Work;
class WorldExchange;

class WorldLoader final {
public:
    WorldLoader(AssetSystem& assets, ThreadPool& threadPool, WorldExchange& exchange) noexcept;
    ~WorldLoader();

    WorldLoader(const WorldLoader&) = delete;
    WorldLoader& operator=(const WorldLoader&) = delete;
    WorldLoader(WorldLoader&&) = delete;
    WorldLoader& operator=(WorldLoader&&) = delete;

    void tick();
    void stop() noexcept;

private:
    [[nodiscard]] static AssetId definition_asset(std::string_view map);
    [[nodiscard]] static AssetId terrain_asset(std::string_view map, const TerrainRequest& request);
    static void stop_work(std::shared_ptr<Work>& work) noexcept;
    void finish_catalog();
    void start_catalog();
    void receive_selections();
    void finish_definition();
    void start_definition();
    void receive_requests();
    void finish_parse();
    void start_parse();

    static constexpr std::size_t RequestsPerTick = 16;

    AssetSystem& assets;
    ThreadPool& threadPool;
    WorldExchange& exchange;
    std::optional<std::pair<MapSelection, AssetRequest>> definitionLoading;
    std::vector<std::pair<TerrainRequest, AssetRequest>> loading;
    std::shared_ptr<Work> catalogWork;
    std::shared_ptr<Work> definitionWork;
    std::shared_ptr<Work> parsing;
    std::string activeMap;
    std::size_t loadingPosition = 0;
    bool catalogStarted = false;
    bool stopping = false;
};

}
