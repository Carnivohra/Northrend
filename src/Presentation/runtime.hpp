#pragma once

#include "Presentation/exchange.hpp"

#include <concepts>

namespace Northrend {

class Window;

template <typename Type>
concept PresentationModule =
    std::constructible_from<Type, PresentationExchange&> && requires(Type presentation, const Window& window) {
        { presentation.start(window) } -> std::same_as<void>;
        { presentation.suspend() } -> std::same_as<void>;
        { presentation.tick() } -> std::same_as<void>;
        { presentation.stop() } -> std::same_as<void>;
    };

template <PresentationModule PresentationType> class PresentationRuntime final {
public:
    PresentationRuntime() = default;

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

    void tick() {
        this->presentation.tick();
    }

    void stop() {
        this->presentation.stop();
    }

    [[nodiscard]] PresentationExchange& exchange() noexcept {
        return this->presentationExchange;
    }

private:
    PresentationExchange presentationExchange;
    PresentationType presentation {this->presentationExchange};
};

}
