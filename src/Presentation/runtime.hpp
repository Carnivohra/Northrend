#pragma once

#include "Presentation/exchange.hpp"

#include <concepts>
#include <cstddef>

namespace Northrend {

class Input;
class AssetSystem;
class Window;

template <typename Type>
concept PresentationModule = std::constructible_from<Type, PresentationExchange&, AssetSystem&, std::size_t> &&
    requires(Type presentation, const Window& window, const Input& input) {
        { presentation.start(window) } -> std::same_as<void>;
        { presentation.suspend() } -> std::same_as<void>;
        { presentation.tick(input) } -> std::same_as<void>;
        { presentation.stop() } -> std::same_as<void>;
    };

template <PresentationModule PresentationType> class PresentationRuntime final {
public:
    PresentationRuntime(AssetSystem& assets, const std::size_t renderDistance) :
        presentation(this->presentationExchange, assets, renderDistance) {}

    PresentationRuntime(const PresentationRuntime&) = delete;
    PresentationRuntime& operator=(const PresentationRuntime&) = delete;
    PresentationRuntime(PresentationRuntime&&) = delete;
    PresentationRuntime& operator=(PresentationRuntime&&) = delete;

    void start(const Window& window) {
        this->presentation.start(window);
    }

    void suspend() {
        this->presentation.suspend();
    }

    void tick(const Input& input) {
        this->presentation.tick(input);
    }

    void stop() {
        this->presentation.stop();
    }

    [[nodiscard]] PresentationExchange& exchange() noexcept {
        return this->presentationExchange;
    }

private:
    PresentationExchange presentationExchange;
    PresentationType presentation;
};

}
