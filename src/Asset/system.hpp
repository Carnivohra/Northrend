#pragma once

#include "Asset/Dbc/store.hpp"
#include "Asset/Stream/streamer.hpp"
#include "Asset/plan.hpp"
#include "Asset/store.hpp"

#include <memory>

namespace Northrend {

class ThreadPool;
class Work;

class AssetSystem final {
public:
    AssetSystem(const AssetPlan& plan, ThreadPool& threadPool);
    ~AssetSystem();

    AssetSystem(const AssetSystem&) = delete;
    AssetSystem& operator=(const AssetSystem&) = delete;
    AssetSystem(AssetSystem&&) = delete;
    AssetSystem& operator=(AssetSystem&&) = delete;

    void start();
    void tick();

    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] AssetRequest request(AssetId asset, AssetPriority priority = AssetPriority::normal);
    [[nodiscard]] const DbcStore& databases() const noexcept;

    void request_stop() noexcept;
    void stop();

private:
    enum class State { idle, starting, ready, stopped };

    void finish_startup();

    AssetPlan plan;
    ThreadPool& threadPool;
    AssetStore store;
    AssetStreamer streamer;
    DbcStore dbcStore;
    std::shared_ptr<Work> startup;
    State state = State::idle;
};

}
