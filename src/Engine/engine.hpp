#pragma once

#include "Backend/application.hpp"
#include "Engine/fps.hpp"
#include "Presentation/presentation.hpp"
#include "Render/exchange.hpp"
#include "Render/renderer.hpp"
#include "Thread/runner.hpp"
#include "World/simulation.hpp"

#include <concepts>
#include <cstdint>
#include <iostream>
#include <optional>

namespace Northrend {

class Window;

template <typename Type>
concept EngineRenderer =
    std::derived_from<Type, Renderer> &&
    std::constructible_from<Type, const Window&>;

template <EngineRenderer RendererType>
class Engine final : public BackendApplication {
public:
    Engine() = default;

    void started(const Window& window) override {
        this->renderer.emplace(window);
        this->simulation.emplace(
            Simulation{ this->simulationOutput },
            TickRate::fixed(128));
    }

    void resumed() override {
        this->simulation->resume();
        this->fps.reset();
    }

    void suspended() override {
        if (this->simulation) {
            this->simulation->pause();
        }

        if (this->renderer) {
            this->renderer->suspend();
        }
    }

    void tick() override {
        this->simulation->rethrow_if_failed();

        this->presentation.synchronize(
            this->simulationOutput.read_latest());
        this->renderer->frame(this->presentation.frame());

        if (const auto currentFps = this->fps.record_frame()) {
            std::cout
                << "FPS: "
                << static_cast<std::uint32_t>(*currentFps + 0.5)
                << '\n';
        }
    }

    void stopped() override {
        if (this->simulation) {
            this->simulation->stop();
            this->simulation.reset();
        }

        this->renderer.reset();
    }

private:
    RenderExchange simulationOutput;
    Presentation presentation;
    std::optional<RendererType> renderer;
    std::optional<TaskRunner<Simulation>> simulation;
    Fps fps;
};

}
