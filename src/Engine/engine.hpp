#pragma once

#include "Asset/system.hpp"
#include "Backend/application.hpp"
#include "Engine/plan.hpp"
#include "Input/input.hpp"
#include "Presentation/runtime.hpp"
#include "Simulation/runtime.hpp"
#include "Thread/pool.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <thread>

#ifdef NORTHREND_GPU_PROFILING
#include "Debug/timing.hpp"

#include <chrono>
#endif

namespace Northrend {

template <PresentationModule PresentationType> class Engine final : public BackendApplication {
public:
    explicit Engine(const EnginePlan& plan) :
        threadPool(worker_count()),
        assets(plan.assets, this->threadPool),
        presentation(this->assets, plan.renderDistance),
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

#ifdef NORTHREND_GPU_PROFILING
        this->cpuTiming.reset();
#endif
    }

    void suspended() override {
        this->simulation.pause();
        this->presentation.suspend();
    }

    void request_map(const std::uint32_t map) {
        this->simulation.request_map(map);
    }

    void tick(const Input& input) override {
        this->assets.tick();
        this->simulation.tick();

#ifdef NORTHREND_GPU_PROFILING
        const auto renderStart = std::chrono::steady_clock::now();
#endif

        this->presentation.tick(input);

#ifdef NORTHREND_GPU_PROFILING
        const auto renderEnd = std::chrono::steady_clock::now();
        const double milliseconds = std::chrono::duration<double, std::milli>(renderEnd - renderStart).count();

        if (const auto summary = this->cpuTiming.record(milliseconds)) {
            Timing::write(std::cout, "CPU render", *summary);
        }
#endif
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

#ifdef NORTHREND_GPU_PROFILING
    Timing cpuTiming;
#endif

    ThreadPool threadPool;
    AssetSystem assets;
    PresentationRuntime<PresentationType> presentation;
    SimulationRuntime simulation;
};

}
