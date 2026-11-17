#pragma once

#include "Asset/Format/Wmo/wmo.hpp"
#include "Asset/Stream/priority.hpp"
#include "Asset/Stream/request.hpp"
#include "Asset/id.hpp"
#include "World/Map/Object/handle.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Northrend {

class AssetSystem;
class MapObjectAsset;
class MapObjectState;
class ThreadPool;
class Work;

class MapObjectLoader final {
public:
    MapObjectLoader(AssetSystem& assets, ThreadPool& threadPool) noexcept;
    ~MapObjectLoader();

    MapObjectLoader(const MapObjectLoader&) = delete;
    MapObjectLoader& operator=(const MapObjectLoader&) = delete;
    MapObjectLoader(MapObjectLoader&&) = delete;
    MapObjectLoader& operator=(MapObjectLoader&&) = delete;

    [[nodiscard]] MapObjectHandle reference(
        std::string_view asset, AssetPriority priority, std::uint32_t generation);
    void reset(std::uint32_t generation) noexcept;
    void tick();
    void stop() noexcept;

private:
    enum class Phase : std::uint8_t { rootLoading, rootParsing, groupsLoading, groupsParsing };

    struct Entry final {
        std::shared_ptr<MapObjectState> state;
        AssetRequest rootRequest;
        std::unique_ptr<Wmo> root;
        std::vector<AssetRequest> groupRequests;
        std::shared_ptr<MapObjectAsset> result;
        std::shared_ptr<Work> work;
        AssetPriority priority = AssetPriority::background;
        Phase phase = Phase::rootLoading;
    };

    [[nodiscard]] static std::string key(std::string_view asset);
    [[nodiscard]] static AssetId group_asset(const AssetId& root, std::size_t group);
    void receive_pending();
    void stop_entries() noexcept;

    static constexpr std::size_t TransitionsPerTick = 16;

    AssetSystem& assets;
    ThreadPool& threadPool;
    std::mutex referenceMutex;
    std::unordered_map<std::string, std::weak_ptr<MapObjectState>> active;
    std::vector<std::shared_ptr<MapObjectState>> pending;
    std::unordered_map<std::string, Entry> entries;
    std::uint32_t activeGeneration = 0;
    bool stopping = false;
};

}
