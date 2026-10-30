#include "Simulation/runtime.hpp"

#include "Asset/system.hpp"
#include "Presentation/exchange.hpp"
#include "Thread/pool.hpp"

#include <stdexcept>

namespace Northrend {

SimulationRuntime::SimulationRuntime(
    PresentationExchange& presentationExchange, AssetSystem& assets, ThreadPool& threadPool) noexcept :
    presentationExchange(presentationExchange), loader(assets, threadPool, this->exchange) {}

SimulationRuntime::~SimulationRuntime() {
    this->stop_without_rethrow();
}

void SimulationRuntime::start() {
    this->runner.emplace(Simulation {this->presentationExchange, this->exchange}, TickRate);
}

void SimulationRuntime::resume() noexcept {
    this->runner->resume();
}

void SimulationRuntime::pause() noexcept {
    if (this->runner) {
        this->runner->pause();
    }
}

void SimulationRuntime::request_map(const std::uint32_t map) {
    if (!this->exchange.request_map(map)) {
        throw std::overflow_error("Map request mailbox is full");
    }
}

void SimulationRuntime::tick() {
    this->loader.tick();

    if (this->runner) {
        this->runner->rethrow_if_failed();
    }
}

void SimulationRuntime::stop() {
    if (!this->runner) {
        this->loader.stop();
        return;
    }

    try {
        this->runner->stop();
    } catch (...) {
        this->runner.reset();
        this->loader.stop();
        throw;
    }

    this->runner.reset();
    this->loader.stop();
}

void SimulationRuntime::stop_without_rethrow() noexcept {
    if (this->runner) {
        try {
            this->runner->stop();
        } catch (...) {
        }

        this->runner.reset();
    }

    this->loader.stop();
}

}
