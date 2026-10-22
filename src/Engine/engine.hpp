#pragma once

#include "Backend/application.hpp"
#include "Engine/fps.hpp"
#include "Render/renderer.hpp"

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
    }

    void resumed() override {
        this->fps.reset();
    }

    void suspended() override {
        if (this->renderer) {
            this->renderer->suspend();
        }
    }

    void tick() override {
        this->renderer->frame();

        if (const auto currentFps = this->fps.record_frame()) {
            std::cout
                << "FPS: "
                << static_cast<std::uint32_t>(*currentFps + 0.5)
                << '\n';
        }
    }

    void stopped() override {
        this->renderer.reset();
    }

private:
    std::optional<RendererType> renderer;
    Fps fps;
};

}
