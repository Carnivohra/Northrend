#pragma once

#include "Presentation/exchange.hpp"
#include "Thread/task.hpp"
#include "World/world.hpp"

#include <cstddef>
#include <cstdint>

namespace Northrend {

class MapExchange;

class Simulation final {
public:
    Simulation(PresentationExchange& presentationExchange, MapExchange& mapExchange) noexcept;

    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;
    Simulation(Simulation&&) = default;
    Simulation& operator=(Simulation&&) = delete;

    void start();
    void tick(ThreadDuration delta);
    void stop();

    [[nodiscard]] World& world() noexcept;
    [[nodiscard]] const World& world() const noexcept;

private:
    static constexpr std::size_t TileChangesPerTick = 32;

    PresentationExchange& presentationExchange;
    World state;
    std::uint64_t sequence = 0;
};

static_assert(ThreadTask<Simulation>);

}
