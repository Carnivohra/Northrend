#pragma once

#include "Backend/application.hpp"
#include "Engine/fps.hpp"
#include "Render/exchange.hpp"
#include "Render/frame.hpp"
#include "Render/renderer.hpp"
#include "Thread/runner.hpp"
#include "World/camera.hpp"
#include "World/simulation.hpp"
#include "World/world.hpp"

#include <concepts>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>

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
        this->cameraEntity = this->presentationWorld.registry().create();

        const Camera& camera =
            this->presentationWorld.registry().template emplace<Camera>(
                this->cameraEntity);
        this->renderCamera = extract_camera(camera);
        this->cameraRevision = camera.revision;

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

        const Camera* camera =
            this->presentationWorld.registry().template get<Camera>(
                this->cameraEntity);

        if (!camera) {
            throw std::runtime_error("Presentation world has no active camera");
        }

        if (camera->revision != this->cameraRevision) {
            this->renderCamera = extract_camera(*camera);
            this->cameraRevision = camera->revision;
        }

        const RenderFrame frame{
            .snapshot = this->renderExchange.read_latest(),
            .camera = this->renderCamera,
            .cameraRevision = this->cameraRevision
        };
        this->renderer->frame(frame);

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

        if (this->cameraEntity) {
            this->presentationWorld.registry().destroy(this->cameraEntity);
            this->cameraEntity = Entity{};
        }

        this->cameraRevision = 0;
        this->renderer.reset();
    }

private:
    RenderExchange renderExchange;
    World presentationWorld;
    Entity cameraEntity;
    RenderCamera renderCamera;
    std::uint64_t cameraRevision = 0;
    std::optional<RendererType> renderer;
    std::optional<TaskRunner<Simulation>> simulation;
    Fps fps;
};

}
