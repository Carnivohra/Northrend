#include "Simulation/simulation.hpp"

#include <chrono>

namespace Northrend {

Simulation::Simulation(PresentationExchange& presentationExchange) noexcept
    : presentationExchange(presentationExchange) {}

void Simulation::start() {}

void Simulation::tick(ThreadDuration delta) {
    this->state.registry().progress(std::chrono::duration<float>(delta));

    this->presentationExchange.write() = WorldSnapshot{
        .sequence = ++this->sequence
    };
    this->presentationExchange.publish();
}

void Simulation::stop() {}

World& Simulation::world() noexcept {
    return this->state;
}

const World& Simulation::world() const noexcept {
    return this->state;
}

}
