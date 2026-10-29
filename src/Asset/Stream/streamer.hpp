#pragma once

#include "Asset/Stream/priority.hpp"
#include "Asset/Stream/request.hpp"
#include "Asset/id.hpp"

#include <array>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Northrend {

class AssetState;
class AssetStorage;

class AssetStreamer final {
public:
    AssetStreamer(AssetStorage& storage, std::size_t workerCount);
    ~AssetStreamer();

    AssetStreamer(const AssetStreamer&) = delete;
    AssetStreamer& operator=(const AssetStreamer&) = delete;
    AssetStreamer(AssetStreamer&&) = delete;
    AssetStreamer& operator=(AssetStreamer&&) = delete;

    [[nodiscard]] AssetRequest request(AssetId asset, AssetPriority priority = AssetPriority::normal);
    [[nodiscard]] std::size_t worker_count() const noexcept;

    void stop() noexcept;

private:
    static constexpr std::size_t PriorityCount = 3;

    [[nodiscard]] static std::size_t priority_index(AssetPriority priority);
    [[nodiscard]] bool has_requests() const noexcept;
    [[nodiscard]] std::shared_ptr<AssetState> next_request();
    void promote(const std::shared_ptr<AssetState>& state, AssetPriority priority);
    void work();
    void remove(const std::shared_ptr<AssetState>& state);

    AssetStorage& storage;
    mutable std::mutex mutex;
    std::condition_variable wake;
    std::array<std::deque<std::shared_ptr<AssetState>>, PriorityCount> queues;
    std::unordered_map<std::string, std::weak_ptr<AssetState>> active;
    std::vector<std::thread> workers;
    const std::size_t workerCount;
    bool stopping = false;
};

}
