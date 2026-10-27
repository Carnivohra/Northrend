#pragma once

#include "Presentation/exchange.hpp"
#include "Thread/task.hpp"
#include "World/world.hpp"

#include <cstdint>

namespace Northrend {

class Simulation final {
public:
    explicit Simulation(PresentationExchange& presentationExchange) noexcept;

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
    PresentationExchange& presentationExchange;
    World state;
    std::uint64_t sequence = 0;
};

static_assert(ThreadTask<Simulation>);

}
