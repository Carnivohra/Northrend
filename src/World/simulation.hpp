#pragma once

#include "Render/exchange.hpp"
#include "Thread/task.hpp"
#include "World/world.hpp"

#include <chrono>
#include <cstdint>

namespace Northrend {

class Simulation final {
public:
    explicit Simulation(RenderExchange& renderExchange) noexcept;

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
    RenderExchange& renderExchange;
    World state;
    std::chrono::nanoseconds elapsed{};
    std::uint64_t sequence = 0;
};

static_assert(ThreadTask<Simulation>);

}
