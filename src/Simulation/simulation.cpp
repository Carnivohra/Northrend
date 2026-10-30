#include "Simulation/simulation.hpp"

#include <optional>
#include <stdexcept>
#include <utility>

namespace Northrend {

Simulation::Simulation(PresentationExchange& presentationExchange, MapExchange& mapExchange) noexcept :
    presentationExchange(presentationExchange), state(mapExchange) {}

void Simulation::start() {}

void Simulation::tick(ThreadDuration delta) {
    this->state.tick(delta);

    for (std::size_t count = 0; count < TileChangesPerTick; ++count) {
        if (this->presentationExchange.tiles_full()) {
            break;
        }

        std::optional<TileChange> change = this->state.next_tile_change();

        if (!change) {
            break;
        }

        if (!this->presentationExchange.publish(std::move(*change))) {
            throw std::logic_error("Map presentation mailbox rejected an available tile change");
        }
    }

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
