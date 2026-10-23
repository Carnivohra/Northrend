#include "World/simulation.hpp"

#include <chrono>

namespace Northrend {

Simulation::Simulation(RenderExchange& renderExchange) noexcept
    : renderExchange(renderExchange) {}

void Simulation::start() {
    this->cameraEntity = this->state.registry().create();
    this->state.registry().emplace<Camera>(this->cameraEntity);
}

void Simulation::tick(ThreadDuration delta) {
    this->state.registry().progress(std::chrono::duration<float>(delta));
    this->elapsed += delta;

    const Camera* camera =
        this->state.registry().get<Camera>(this->cameraEntity);

    this->renderExchange.write() = RenderSnapshot{
        .sequence = ++this->sequence,
        .simulationTime = this->elapsed,
        .camera = camera ? extract_camera(*camera) : RenderCamera{}
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
