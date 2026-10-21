#pragma once

#include "Backend/application.hpp"
#include "Render/renderer.hpp"

#include <concepts>
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

    void resumed() override {}

    void suspended() override {
        if (this->renderer) {
            this->renderer->suspend();
        }
    }

    void tick() override {
        this->renderer->frame();
    }

    void stopped() override {
        this->renderer.reset();
    }

private:
    std::optional<RendererType> renderer;
};

}
