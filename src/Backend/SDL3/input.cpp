#include "Backend/SDL3/input.hpp"

#include "Input/input.hpp"

namespace Northrend {

void SDL3Input::process(const SDL_Event& event, Input& input) noexcept {
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
        input.keyboard().press(key(event.key.scancode));
        break;
    case SDL_EVENT_KEY_UP:
        input.keyboard().release(key(event.key.scancode));
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (const std::optional<MouseButton> button = mouse_button(event.button.button)) {
            input.mouse().press(*button);
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (const std::optional<MouseButton> button = mouse_button(event.button.button)) {
            input.mouse().release(*button);
        }
        break;
    case SDL_EVENT_MOUSE_MOTION:
        input.mouse().move(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
        break;
    case SDL_EVENT_MOUSE_WHEEL: {
        const float direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0F : 1.0F;
        input.mouse().scroll(event.wheel.x * direction, event.wheel.y * direction);
        break;
    }
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        input.reset();
        break;
    default:
        break;
    }
}

Key SDL3Input::key(const SDL_Scancode scancode) noexcept {
    const auto value = static_cast<std::uint16_t>(scancode);

    if (scancode == SDL_SCANCODE_UNKNOWN || value >= static_cast<std::uint16_t>(Key::count)) {
        return Key::unknown;
    }

    return static_cast<Key>(value);
}

std::optional<MouseButton> SDL3Input::mouse_button(const Uint8 button) noexcept {
    switch (button) {
    case SDL_BUTTON_LEFT:
        return MouseButton::left;
    case SDL_BUTTON_MIDDLE:
        return MouseButton::middle;
    case SDL_BUTTON_RIGHT:
        return MouseButton::right;
    case SDL_BUTTON_X1:
        return MouseButton::back;
    case SDL_BUTTON_X2:
        return MouseButton::forward;
    default:
        return std::nullopt;
    }
}

}
