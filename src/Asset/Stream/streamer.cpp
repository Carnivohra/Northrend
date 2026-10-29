#include "Asset/Stream/streamer.hpp"

#include "Asset/Stream/state.hpp"
#include "Asset/storage.hpp"
#include "Thread/pool.hpp"

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <utility>

namespace Northrend {

AssetStreamer::AssetStreamer(AssetStorage& storage, ThreadPool& pool) noexcept : storage(storage), pool(pool) {}

AssetStreamer::~AssetStreamer() {
    this->stop();
}

AssetRequest AssetStreamer::request(AssetId asset, AssetPriority priority) {
    const std::string key(asset.path());
    std::unique_lock lock(this->mutex);

    if (this->stopping) {
        throw std::runtime_error("Asset streamer is stopped");
    }

    this->prune();

    if (const auto position = this->active.find(key); position != this->active.end()) {
        if (std::shared_ptr<AssetState> state = position->second.lock()) {
            const auto workPosition = this->work.find(key);

            if (state->queued() && workPosition != this->work.end() &&
                static_cast<std::uint8_t>(priority) > static_cast<std::uint8_t>(state->priority()) &&
                this->pool.promote(workPosition->second, priority)) {
                state->set_priority(priority);
            }

            return AssetRequest(std::move(state));
        }

        this->active.erase(position);
        this->work.erase(key);
    }

    auto state = std::make_shared<AssetState>(std::move(asset), priority);
    this->active.emplace(key, state);

    try {
        this->work.emplace(key, nullptr);
    } catch (...) {
        this->active.erase(key);
        throw;
    }

    try {
        this->work.at(key) = this->pool.submit(priority, [this, state] { this->load(state); });
    } catch (...) {
        this->work.erase(key);
        this->active.erase(key);
        throw;
    }

    return AssetRequest(std::move(state));
}

void AssetStreamer::stop() noexcept {
    {
        const std::scoped_lock lock(this->mutex);

        if (this->stopping) {
            return;
        }

        this->stopping = true;

        for (const auto& entry : this->active) {
            if (const std::shared_ptr<AssetState> currentState = entry.second.lock()) {
                currentState->cancel();
            }
        }

        for (const auto& entry : this->work) {
            entry.second->cancel();
        }
    }

    while (true) {
        std::shared_ptr<Work> currentWork;

        {
            const std::scoped_lock lock(this->mutex);

            if (this->work.empty()) {
                this->active.clear();
                return;
            }

            currentWork = this->work.begin()->second;
        }

        try {
            currentWork->wait();
        } catch (...) {
        }

        const std::scoped_lock lock(this->mutex);

        for (auto position = this->work.begin(); position != this->work.end(); ++position) {
            if (position->second == currentWork) {
                this->active.erase(position->first);
                this->work.erase(position);
                break;
            }
        }
    }
}

void AssetStreamer::load(const std::shared_ptr<AssetState>& state) noexcept {
    if (!state->begin()) {
        return;
    }

    try {
        auto data = std::make_shared<AssetData>(this->storage.read(state->asset()));
        state->complete(std::move(data));
    } catch (...) {
        state->fail(std::current_exception());
    }
}

void AssetStreamer::prune() noexcept {
    for (auto position = this->work.begin(); position != this->work.end();) {
        const auto statePosition = this->active.find(position->first);

        if (position->second->ready() && (statePosition == this->active.end() || statePosition->second.expired())) {
            if (statePosition != this->active.end()) {
                this->active.erase(statePosition);
            }

            position = this->work.erase(position);
        } else {
            ++position;
        }
    }
}

}
