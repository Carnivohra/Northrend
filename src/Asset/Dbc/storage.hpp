#pragma once

#include "Asset/Dbc/table.hpp"
#include "Asset/Stream/request.hpp"
#include "Asset/id.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Northrend {

class AssetStorage;
class AssetStreamer;
class ThreadPool;
class Work;

class DbcStorage final {
public:
    DbcStorage(AssetStorage& assets, AssetStreamer& streamer, ThreadPool& threadPool) noexcept;
    ~DbcStorage();

    DbcStorage(const DbcStorage&) = delete;
    DbcStorage& operator=(const DbcStorage&) = delete;
    DbcStorage(DbcStorage&&) = delete;
    DbcStorage& operator=(DbcStorage&&) = delete;

    void start();
    void tick();

    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] const DbcTable* find(std::string_view path) const;
    [[nodiscard]] const DbcTable* find_file(std::string_view name) const;

    void request_stop() noexcept;
    void stop();

private:
    enum class State { idle, discovering, loading, ready, stopped };

    [[nodiscard]] static std::string key(std::string_view path);
    void finish_discovery();
    void finish_loading();

    static constexpr std::size_t TablesPerTick = 8;

    AssetStorage& assets;
    AssetStreamer& streamer;
    ThreadPool& threadPool;
    std::shared_ptr<Work> discovery;
    std::vector<AssetId> discovered;
    std::vector<std::pair<std::string, AssetRequest>> requests;
    std::vector<std::pair<std::string, DbcTable>> tables;
    State state = State::idle;
};

}
