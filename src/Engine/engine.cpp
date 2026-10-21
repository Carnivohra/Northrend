#include "Engine/engine.hpp"

namespace Northrend {

Engine::Engine() {}

void Engine::started() {}

void Engine::resumed() {}

void Engine::suspended() {}

void Engine::update() {
    this->tick();
}

void Engine::stopped() {}

void Engine::tick() {}

}
