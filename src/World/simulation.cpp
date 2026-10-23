#include "World/simulation.hpp"

#include <chrono>

namespace Northrend {

Simulation::Simulation(RenderExchange& renderExchange) noexcept
    : renderExchange(renderExchange) {}

void Simulation::start() {}

void Simulation::tick(ThreadDuration delta) {
    this->state.registry().progress(std::chrono::duration<float>(delta));
    this->elapsed += delta;

    this->renderExchange.write() = RenderSnapshot{
        .sequence = ++this->sequence,
        .simulationTime = this->elapsed
    };
    this->renderExchange.publish();
}

void Simulation::stop() {}

World& Simulation::world() noexcept {
    return this->state;
}

const World& Simulation::world() const noexcept {
    return this->state;
}

}
