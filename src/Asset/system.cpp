#include "Asset/system.hpp"

#include "Thread/pool.hpp"
#include "Thread/work.hpp"

#include <exception>
#include <stdexcept>
#include <stop_token>
#include <utility>

namespace Northrend {

AssetSystem::AssetSystem(const AssetPlan& plan, ThreadPool& threadPool) :
    plan(plan), threadPool(threadPool), streamer(this->storage, threadPool),
    dbcStorage(this->storage, this->streamer, threadPool) {}

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

void AssetSystem::tick() {
    this->finish_startup();

    if (this->ready()) {
        this->dbcStorage.tick();
        this->streamer.tick();
    }
}

bool AssetSystem::ready() const noexcept {
    return this->state == State::ready;
}

AssetRequest AssetSystem::request(AssetId asset, const AssetPriority priority) {
    this->finish_startup();

    if (!this->ready()) {
        throw std::logic_error("Assets cannot be requested before the asset system is ready");
    }

    return this->streamer.request(std::move(asset), priority);
}

const DbcStorage& AssetSystem::databases() const noexcept {
    return this->dbcStorage;
}

void AssetSystem::request_stop() noexcept {
    if (this->startup) {
        this->startup->cancel();
    }

    this->dbcStorage.request_stop();
}

void AssetSystem::stop() {
    if (this->state == State::stopped) {
        return;
    }

    this->request_stop();
    this->streamer.stop();
    std::shared_ptr<Work> startup = std::move(this->startup);
    std::exception_ptr failure;
    this->state = State::stopped;

    try {
        this->dbcStorage.stop();
    } catch (...) {
        failure = std::current_exception();
    }

    if (startup) {
        try {
            startup->wait();
        } catch (...) {
            if (!failure) {
                failure = std::current_exception();
            }
        }
    }

    if (failure) {
        std::rethrow_exception(failure);
    }
}

void AssetSystem::finish_startup() {
    if (!this->startup || !this->startup->ready()) {
        return;
    }

    std::shared_ptr<Work> startup = std::move(this->startup);
    startup->wait();
    this->state = State::ready;
    this->dbcStorage.start();
}

}
