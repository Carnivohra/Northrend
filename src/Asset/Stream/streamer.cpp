#include "Asset/Stream/streamer.hpp"

#include "Asset/Stream/state.hpp"
#include "Asset/storage.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>

namespace Northrend {

AssetStreamer::AssetStreamer(AssetStorage& storage, std::size_t workerCount) :
    storage(storage), workerCount(workerCount) {
    if (workerCount == 0) {
        throw std::invalid_argument("Asset streamer requires at least one worker");
    }

    this->workers.reserve(workerCount);

    try {
        for (std::size_t index = 0; index < workerCount; ++index) {
            this->workers.emplace_back([this] { this->work(); });
        }
    } catch (...) {
        {
            const std::scoped_lock lock(this->mutex);
            this->stopping = true;
        }

        this->wake.notify_all();

        for (std::thread& worker : this->workers) {
            worker.join();
        }

        throw;
    }
}

AssetStreamer::~AssetStreamer() {
    this->stop();
}

AssetRequest AssetStreamer::request(AssetId asset, AssetPriority priority) {
    const std::string key(asset.path());
    std::unique_lock lock(this->mutex);

    if (this->stopping) {
        throw std::runtime_error("Asset streamer is stopped");
    }

    if (const auto position = this->active.find(key); position != this->active.end()) {
        if (std::shared_ptr<AssetState> state = position->second.lock()) {
            if (state->queued() && static_cast<std::uint8_t>(priority) > static_cast<std::uint8_t>(state->priority())) {
                this->promote(state, priority);
            }

            return AssetRequest(std::move(state));
        }

        this->active.erase(position);
    }

    auto state = std::make_shared<AssetState>(std::move(asset), priority);
    this->active.emplace(key, state);
    this->queues[priority_index(priority)].push_back(state);
    lock.unlock();
    this->wake.notify_one();

    return AssetRequest(std::move(state));
}

std::size_t AssetStreamer::worker_count() const noexcept {
    return this->workerCount;
}

void AssetStreamer::stop() noexcept {
    {
        const std::scoped_lock lock(this->mutex);

        if (this->stopping) {
            return;
        }

        this->stopping = true;

        for (auto& queue : this->queues) {
            for (const std::shared_ptr<AssetState>& state : queue) {
                state->cancel();
            }

            queue.clear();
        }

        this->active.clear();
    }

    this->wake.notify_all();

    for (std::thread& worker : this->workers) {
        worker.join();
    }

    this->workers.clear();
}

std::size_t AssetStreamer::priority_index(AssetPriority priority) {
    const std::size_t index = static_cast<std::size_t>(priority);

    if (index >= PriorityCount) {
        throw std::invalid_argument("Invalid asset priority");
    }

    return index;
}

bool AssetStreamer::has_requests() const noexcept {
    return std::ranges::any_of(this->queues, [](const auto& queue) { return !queue.empty(); });
}

std::shared_ptr<AssetState> AssetStreamer::next_request() {
    for (std::size_t index = PriorityCount; index > 0; --index) {
        auto& queue = this->queues[index - 1];

        if (!queue.empty()) {
            std::shared_ptr<AssetState> state = std::move(queue.front());
            queue.pop_front();
            return state;
        }
    }

    return nullptr;
}

void AssetStreamer::promote(const std::shared_ptr<AssetState>& state, AssetPriority priority) {
    auto& queue = this->queues[priority_index(state->priority())];
    const auto position = std::ranges::find(queue, state);

    if (position != queue.end()) {
        queue.erase(position);
    }

    state->set_priority(priority);
    this->queues[priority_index(priority)].push_back(state);
}

void AssetStreamer::work() {
    while (true) {
        std::shared_ptr<AssetState> state;

        {
            std::unique_lock lock(this->mutex);
            this->wake.wait(lock, [this] { return this->stopping || this->has_requests(); });

            if (this->stopping) {
                return;
            }

            state = this->next_request();

            if (!state || !state->begin()) {
                continue;
            }
        }

        try {
            auto data = std::make_shared<AssetData>(this->storage.read(state->asset()));
            state->complete(std::move(data));
        } catch (...) {
            state->fail(std::current_exception());
        }

        this->remove(state);
    }
}

void AssetStreamer::remove(const std::shared_ptr<AssetState>& state) {
    const std::string key(state->asset().path());
    const std::scoped_lock lock(this->mutex);
    const auto position = this->active.find(key);

    if (position != this->active.end() && position->second.lock() == state) {
        this->active.erase(position);
    }
}

}
