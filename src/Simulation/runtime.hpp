#pragma once

#include "Simulation/simulation.hpp"
#include "Thread/runner.hpp"
#include "World/Map/exchange.hpp"
#include "World/Map/loader.hpp"

#include <cstdint>
#include <optional>

namespace Northrend {

class AssetSystem;
class PresentationExchange;
class ThreadPool;

class SimulationRuntime final {
public:
    SimulationRuntime(PresentationExchange& presentationExchange, AssetSystem& assets, ThreadPool& threadPool) noexcept;
    ~SimulationRuntime();

    SimulationRuntime(const SimulationRuntime&) = delete;
    SimulationRuntime& operator=(const SimulationRuntime&) = delete;
    SimulationRuntime(SimulationRuntime&&) = delete;
    SimulationRuntime& operator=(SimulationRuntime&&) = delete;

    void start();
    void resume() noexcept;
    void pause() noexcept;
    void request_map(std::uint32_t map);
    void tick();
    void stop();

private:
    static constexpr int TickRate = 128;

    void stop_without_rethrow() noexcept;

    PresentationExchange& presentationExchange;
    MapExchange exchange;
    MapLoader loader;
    std::optional<TaskRunner<Simulation>> runner;
};

}
