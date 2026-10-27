#pragma once

#include "Presentation/exchange.hpp"
#include "Render/renderer.hpp"
#include "Render/scene.hpp"
#include "World/snapshot.hpp"

#include <concepts>
#include <optional>

namespace Northrend {

class Window;

template <typename Type>
concept PresentationRenderer =
    std::derived_from<Type, Renderer> &&
    std::constructible_from<Type, const Window&>;

template <PresentationRenderer RendererType>
class Presentation final {
public:
    explicit Presentation(PresentationExchange& presentationExchange)
        : presentationExchange(presentationExchange) {}

    void start(const Window& window) {
        this->renderer.emplace(window);
    }

    void suspend() {
        if (this->renderer) {
            this->renderer->suspend();
        }
    }

    void tick() {
        const WorldSnapshot& snapshot = this->presentationExchange.read_latest();

        if (snapshot.sequence != this->latestSnapshot.sequence) {
            this->latestSnapshot = snapshot;
        }

        this->renderer->frame(this->scene.frame());
    }

    void stop() {
        this->renderer.reset();
    }

private:
    PresentationExchange& presentationExchange;
    WorldSnapshot latestSnapshot;
    RenderScene scene;
    std::optional<RendererType> renderer;
};

}
