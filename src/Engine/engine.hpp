#pragma once

#include "Asset/system.hpp"
#include "Backend/application.hpp"
#include "Engine/fps.hpp"
#include "Engine/plan.hpp"
#include "Presentation/runtime.hpp"
#include "Simulation/runtime.hpp"
#include "Thread/pool.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <thread>

namespace Northrend {

template <PresentationModule PresentationType> class Engine final : public BackendApplication {
public:
    explicit Engine(const EnginePlan& plan) :
        threadPool(worker_count()),
        assets(plan.assets, this->threadPool),
        simulation(this->presentation.exchange(), this->assets, this->threadPool) {}

    void started(const Window& window) override {
        try {
            this->presentation.start(window);
            this->assets.start();
            this->simulation.start();
        } catch (...) {
            // Roll back before the backend releases the window.
            try {
                this->stopped();
            } catch (...) {
                // Preserve the original startup failure.
            }

            throw;
        }
    }

    void resumed() override {
        this->simulation.resume();
        this->fps.reset();
    }

    void suspended() override {
        this->simulation.pause();
        this->presentation.suspend();
    }

    void tick() override {
        this->assets.tick();
        this->simulation.tick();
        this->presentation.tick();

        if (const auto currentFps = this->fps.record_frame()) {
            std::cout << "FPS: " << static_cast<std::uint32_t>(*currentFps + 0.5) << '\n';
        }
    }

    void stopped() override {
        std::exception_ptr failure;

        this->assets.request_stop();

        try {
            this->simulation.stop();
        } catch (...) {
            failure = std::current_exception();
        }

        try {
            this->presentation.stop();
        } catch (...) {
            if (!failure) {
                failure = std::current_exception();
            }
        }

        try {
            this->assets.stop();
        } catch (...) {
            if (!failure) {
                failure = std::current_exception();
            }
        }

        this->threadPool.stop();

        if (failure) {
            std::rethrow_exception(failure);
        }
    }

private:
    [[nodiscard]] static std::size_t worker_count() noexcept {
        const std::size_t concurrency = std::thread::hardware_concurrency();
        return std::clamp(concurrency / 4, std::size_t {1}, std::size_t {4});
    }

    PresentationRuntime<PresentationType> presentation;
    Fps fps;
    ThreadPool threadPool;
    AssetSystem assets;
    SimulationRuntime simulation;
};

}
