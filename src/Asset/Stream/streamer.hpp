#pragma once

#include "Asset/Stream/priority.hpp"
#include "Asset/Stream/request.hpp"
#include "Asset/id.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Northrend {

class AssetState;
class AssetStorage;
class ThreadPool;
class Work;

class AssetStreamer final {
public:
    AssetStreamer(AssetStorage& storage, ThreadPool& pool) noexcept;
    ~AssetStreamer();

    AssetStreamer(const AssetStreamer&) = delete;
    AssetStreamer& operator=(const AssetStreamer&) = delete;
    AssetStreamer(AssetStreamer&&) = delete;
    AssetStreamer& operator=(AssetStreamer&&) = delete;

    [[nodiscard]] AssetRequest request(AssetId asset, AssetPriority priority = AssetPriority::normal);

    void stop() noexcept;

private:
    void load(const std::shared_ptr<AssetState>& state) noexcept;
    void prune() noexcept;

    AssetStorage& storage;
    ThreadPool& pool;
    mutable std::mutex mutex;
    std::unordered_map<std::string, std::weak_ptr<AssetState>> active;
    std::unordered_map<std::string, std::shared_ptr<Work>> work;
    bool stopping = false;
};

}
