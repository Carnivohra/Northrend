#pragma once

#include "Presentation/exchange.hpp"
#include "Presentation/map.hpp"
#include "Render/renderer.hpp"
#include "Render/scene.hpp"
#include "World/snapshot.hpp"

#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>

namespace Northrend {

class Window;

template <typename Type>
concept PresentationRenderer = std::derived_from<Type, Renderer> && std::constructible_from<Type, const Window&>;

template <PresentationRenderer RendererType> class Presentation final {
public:
    explicit Presentation(PresentationExchange& presentationExchange) : presentationExchange(presentationExchange) {}

    void start(const Window& window) {
        this->renderer.emplace(window);
    }

    void suspend() {
        if (this->renderer) {
            this->renderer->suspend();
        }
    }

    void tick() {
        for (std::size_t count = 0; count < TileChangesPerFrame; ++count) {
            std::optional<TileChange> change = this->presentationExchange.next_tile();

            if (!change) {
                break;
            }

            this->map.apply(std::move(*change));
        }

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
    static constexpr std::size_t TileChangesPerFrame = 32;

    PresentationExchange& presentationExchange;
    WorldSnapshot latestSnapshot;
    PresentationMap map;
    RenderScene scene;
    std::optional<RendererType> renderer;
};

}
