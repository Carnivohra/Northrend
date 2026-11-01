#pragma once

#include "Input/keyboard.hpp"
#include "Input/mouse.hpp"

#include <SDL3/SDL_events.h>

#include <optional>

namespace Northrend {

class Input;

class SDL3Input final {
public:
    static void process(const SDL_Event& event, Input& input) noexcept;

private:
    [[nodiscard]] static Key key(SDL_Scancode scancode) noexcept;
    [[nodiscard]] static std::optional<MouseButton> mouse_button(Uint8 button) noexcept;
};

}
