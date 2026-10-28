#pragma once

#include "Asset/storage.hpp"
#include "Backend/application.hpp"
#include "Engine/fps.hpp"
#include "Game/plan.hpp"
#include "Presentation/exchange.hpp"
#include "Simulation/simulation.hpp"
#include "Thread/runner.hpp"

#include <concepts>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <utility>

namespace Northrend {

class Window;

template <typename Type>
concept EnginePresentation =
    std::constructible_from<Type, PresentationExchange&> &&
    requires(Type presentation, const Window& window) {
        { presentation.start(window) } -> std::same_as<void>;
        { presentation.suspend() } -> std::same_as<void>;
        { presentation.tick() } -> std::same_as<void>;
        { presentation.stop() } -> std::same_as<void>;
    };

template <EnginePresentation PresentationType>
class Engine final : public BackendApplication {
public:
    explicit Engine(GamePlan plan)
        : plan(std::move(plan)),
          assets(this->plan.assets) {}

    void started(const Window& window) override {
        try {
            this->presentation.start(window);
            this->simulation.emplace(
                Simulation{ this->presentationExchange },
                128);
        } catch (...) {
            // Roll back before the backend releases the window.
            try {
                this->stopped();
            } catch (...) {
                // Preserve the original startup failure.
            }

            throw;
        }
    }

    void resumed() override {
        this->simulation->resume();
        this->fps.reset();
    }

    void suspended() override {
        if (this->simulation) {
            this->simulation->pause();
        }

        this->presentation.suspend();
    }

    void tick() override {
        this->simulation->rethrow_if_failed();
        this->presentation.tick();

        if (const auto currentFps = this->fps.record_frame()) {
            std::cout
                << "FPS: "
                << static_cast<std::uint32_t>(*currentFps + 0.5)
                << '\n';
        }
    }

    void stopped() override {
        std::exception_ptr failure;

        if (this->simulation) {
            try {
                this->simulation->stop();
            } catch (...) {
                failure = std::current_exception();
            }

            this->simulation.reset();
        }

        try {
            this->presentation.stop();
        } catch (...) {
            if (!failure) {
                failure = std::current_exception();
            }
        }

        if (failure) {
            std::rethrow_exception(failure);
        }
    }

private:
    GamePlan plan;
    AssetStorage assets;
    PresentationExchange presentationExchange;
    PresentationType presentation{ this->presentationExchange };
    std::optional<TaskRunner<Simulation>> simulation;
    Fps fps;
};

}
