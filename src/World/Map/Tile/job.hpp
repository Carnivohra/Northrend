#pragma once

#include "Asset/Stream/request.hpp"
#include "Asset/Stream/priority.hpp"
#include "World/Map/Tile/request.hpp"
#include "World/Map/Tile/result.hpp"

#include <memory>
#include <optional>
#include <stop_token>

namespace Northrend {

class ThreadPool;
class Work;
class MapObjectLoader;

class TileJob final {
public:
    TileJob(TileRequest request, AssetRequest assetRequest, MapObjectLoader& objects, AssetPriority priority) noexcept;
    ~TileJob();

    TileJob(const TileJob&) = delete;
    TileJob& operator=(const TileJob&) = delete;
    TileJob(TileJob&&) = delete;
    TileJob& operator=(TileJob&&) = delete;

    void start(ThreadPool& threadPool);
    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] std::optional<TileResult> finish();
    void stop() noexcept;

private:
    void execute(std::stop_token stopToken);

    TileRequest tileRequest;
    AssetRequest assetRequest;
    MapObjectLoader& objects;
    AssetPriority objectPriority;
    std::optional<TileResult> tileResult;
    std::shared_ptr<Work> work;
};

}
