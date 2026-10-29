#include "Asset/Dbc/storage.hpp"

#include "Asset/Dbc/format.hpp"
#include "Asset/Stream/priority.hpp"
#include "Asset/Stream/streamer.hpp"
#include "Asset/storage.hpp"
#include "Thread/pool.hpp"
#include "Thread/work.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <stop_token>
#include <utility>

namespace Northrend {

DbcStorage::DbcStorage(AssetStorage& assets, AssetStreamer& streamer, ThreadPool& threadPool) noexcept :
    assets(assets), streamer(streamer), threadPool(threadPool) {}

DbcStorage::~DbcStorage() {
    try {
        this->stop();
    } catch (...) {
    }
}

void DbcStorage::start() {
    if (this->state != State::idle) {
        throw std::logic_error("DBC storage cannot be started more than once");
    }

    this->discovery = this->threadPool.submit(WorkPriority::background, [this](std::stop_token stopToken) {
        std::vector<AssetId> discovered = this->assets.database_assets(stopToken);

        if (!stopToken.stop_requested()) {
            this->discovered = std::move(discovered);
        }
    });
    this->state = State::discovering;
}

void DbcStorage::tick() {
    if (this->state == State::discovering) {
        this->finish_discovery();
    }

    if (this->state == State::loading) {
        this->finish_loading();
    }
}

bool DbcStorage::ready() const noexcept {
    return this->state == State::ready;
}

std::size_t DbcStorage::size() const noexcept {
    return this->tables.size();
}

const DbcTable* DbcStorage::find(const std::string_view path) const {
    if (!this->ready()) {
        return nullptr;
    }

    const std::string name = key(path);
    const auto position = std::ranges::lower_bound(this->tables, name, {}, [](const auto& entry) -> const std::string& {
        return entry.first;
    });

    if (position == this->tables.end() || position->first != name) {
        return nullptr;
    }

    return &position->second;
}

void DbcStorage::request_stop() noexcept {
    if (this->discovery) {
        this->discovery->cancel();
    }
}

void DbcStorage::stop() {
    if (this->state == State::stopped) {
        return;
    }

    this->request_stop();
    std::shared_ptr<Work> discovery = std::move(this->discovery);
    this->state = State::stopped;

    if (discovery) {
        discovery->wait();
    }

    this->discovered.clear();
    this->requests.clear();
    this->tables.clear();
}

std::string DbcStorage::key(const std::string_view path) {
    std::string result(path);
    std::ranges::transform(result, result.begin(), [](const unsigned char value) {
        return value == '\\' ? '/' : static_cast<char>(std::tolower(value));
    });
    return result;
}

void DbcStorage::finish_discovery() {
    if (!this->discovery || !this->discovery->ready()) {
        return;
    }

    std::shared_ptr<Work> discovery = std::move(this->discovery);
    discovery->wait();
    this->requests.reserve(this->discovered.size());
    this->tables.reserve(this->discovered.size());

    for (AssetId& asset : this->discovered) {
        std::string name = key(asset.path());
        this->requests.emplace_back(std::move(name), this->streamer.request(std::move(asset), AssetPriority::background));
    }

    this->discovered.clear();
    this->state = this->requests.empty() ? State::ready : State::loading;
}

void DbcStorage::finish_loading() {
    std::size_t completed = 0;

    for (std::size_t position = 0; position < this->requests.size() && completed < TablesPerTick;) {
        AssetRequest& request = this->requests[position].second;

        if (!request.ready()) {
            ++position;
            continue;
        }

        std::shared_ptr<const AssetData> data = request.data();

        if (!data->empty()) {
            if (!dbc_format(data->bytes())) {
                throw std::runtime_error("Unsupported database format: " + this->requests[position].first);
            }

            try {
                this->tables.emplace_back(this->requests[position].first, DbcTable(std::move(data)));
            } catch (const std::exception& error) {
                throw std::runtime_error("Failed to parse database " + this->requests[position].first + ": " +
                    error.what());
            }
        }

        if (position + 1 != this->requests.size()) {
            this->requests[position] = std::move(this->requests.back());
        }

        this->requests.pop_back();
        ++completed;
    }

    if (!this->requests.empty()) {
        return;
    }

    std::ranges::sort(this->tables, {}, [](const auto& entry) -> const std::string& { return entry.first; });
    this->state = State::ready;
}

}
