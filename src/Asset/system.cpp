#include "Asset/system.hpp"

#include "Thread/pool.hpp"
#include "Thread/work.hpp"

#include <stdexcept>
#include <stop_token>
#include <utility>

namespace Northrend {

AssetSystem::AssetSystem(const AssetPlan& plan, ThreadPool& threadPool) :
    plan(plan), threadPool(threadPool), streamer(this->storage, threadPool) {}

AssetSystem::~AssetSystem() {
    try {
        this->stop();
    } catch (...) {
    }
}

void AssetSystem::start() {
    if (this->state != State::idle) {
        throw std::logic_error("Asset system cannot be started more than once");
    }

    this->startup = this->threadPool.submit(WorkPriority::critical, [this](std::stop_token stopToken) {
        AssetStorage storage(this->plan, stopToken);

        if (!stopToken.stop_requested()) {
            this->storage = std::move(storage);
        }
    });
    this->state = State::starting;
}

bool AssetSystem::ready() const noexcept {
    return this->state == State::ready;
}

AssetRequest AssetSystem::request(AssetId asset, const AssetPriority priority) {
    this->rethrow_if_failed();

    if (!this->ready()) {
        throw std::logic_error("Assets cannot be requested before the asset system is ready");
    }

    return this->streamer.request(std::move(asset), priority);
}

void AssetSystem::request_stop() noexcept {
    if (this->startup) {
        this->startup->cancel();
    }
}

void AssetSystem::rethrow_if_failed() {
    if (!this->startup || !this->startup->ready()) {
        return;
    }

    std::shared_ptr<Work> startup = std::move(this->startup);
    startup->wait();
    this->state = State::ready;
}

void AssetSystem::stop() {
    if (this->state == State::stopped) {
        return;
    }

    this->request_stop();
    this->streamer.stop();
    std::shared_ptr<Work> startup = std::move(this->startup);
    this->state = State::stopped;

    if (startup) {
        startup->wait();
    }
}

}
