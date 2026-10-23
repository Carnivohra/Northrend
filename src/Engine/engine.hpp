#pragma once

#include "Backend/application.hpp"
#include "Engine/fps.hpp"
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
            Simulation{ this->renderExchange },
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
        this->renderer->frame(this->renderExchange.read_latest());

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
    RenderExchange renderExchange;
    std::optional<RendererType> renderer;
    std::optional<TaskRunner<Simulation>> simulation;
    Fps fps;
};

}
