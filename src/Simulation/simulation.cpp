#include "Simulation/simulation.hpp"

namespace Northrend {

Simulation::Simulation(PresentationExchange& presentationExchange, WorldExchange& worldExchange) noexcept :
    presentationExchange(presentationExchange), state(worldExchange) {}

void Simulation::start() {}

void Simulation::tick(ThreadDuration delta) {
    this->state.tick(delta);

    this->presentationExchange.write() = WorldSnapshot {.sequence = ++this->sequence};
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
