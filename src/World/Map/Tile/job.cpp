#include "World/Map/Tile/job.hpp"

#include "Thread/pool.hpp"
#include "Thread/work.hpp"
#include "World/Map/Tile/tile.hpp"

#include <exception>
#include <stdexcept>
#include <utility>

namespace Northrend {

TileJob::TileJob(TileRequest request, AssetRequest assetRequest) noexcept :
    tileRequest(request), assetRequest(std::move(assetRequest)) {}

TileJob::~TileJob() {
    this->stop();
}

void TileJob::start(ThreadPool& threadPool) {
    if (this->work) {
        throw std::logic_error("Map tile job is already running");
    }

    this->work = threadPool.submit(WorkPriority::normal, [this](const std::stop_token stopToken) {
        this->execute(stopToken);
    });
}

bool TileJob::ready() const noexcept {
    return this->work && this->work->ready();
}

std::optional<TileResult> TileJob::finish() {
    if (!this->ready()) {
        return std::nullopt;
    }

    std::shared_ptr<Work> completed = std::move(this->work);
    completed->wait();
    return std::move(this->tileResult);
}

void TileJob::stop() noexcept {
    if (!this->work) {
        return;
    }

    this->work->cancel();

    try {
        this->work->wait();
    } catch (...) {
    }

    this->work.reset();
    this->tileResult.reset();
}

void TileJob::execute(const std::stop_token stopToken) {
    if (stopToken.stop_requested()) {
        return;
    }

    TileResult result = [&] {
        try {
            return TileResult(this->tileRequest, std::make_shared<MapTile>(this->assetRequest.data()));
        } catch (...) {
            return TileResult(this->tileRequest, std::current_exception());
        }
    }();

    if (!stopToken.stop_requested()) {
        this->tileResult.emplace(std::move(result));
    }
}

}
