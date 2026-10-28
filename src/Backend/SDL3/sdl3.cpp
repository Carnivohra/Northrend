#include "Backend/SDL3/sdl3.hpp"

#include "Backend/SDL3/window.hpp"
#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace Northrend {

SDL3Backend::SDL3Backend(std::string title, int width, int height)
    : title(std::move(title)), width(width), height(height) {
    if (this->title.empty()) {
        throw std::invalid_argument("SDL3 window title must not be empty");
    }

    if (this->width <= 0 || this->height <= 0) {
        throw std::invalid_argument("SDL3 window dimensions must be positive");
    }
}

std::filesystem::path SDL3Backend::base_path() const {
    const char* path = SDL_GetBasePath();

    if (!path) {
        throw std::runtime_error(
            "SDL3 could not resolve the application base path: " +
            std::string(SDL_GetError()));
    }

    return std::filesystem::path(path);
}

void SDL3Backend::run(BackendApplication& application) {
    SDL3Window window(this->title, this->width, this->height);
    this->execute(application, window);
}

bool SDL3Backend::process_events(Window& window) {
    bool running = true;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                running = false;
                break;
            case SDL_EVENT_WINDOW_MINIMIZED:
                this->suspend();
                break;
            case SDL_EVENT_WINDOW_RESTORED:
                this->resume();
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                static_cast<SDL3Window&>(window).resize(
                    event.window.data1,
                    event.window.data2);
                break;
            default:
                break;
        }
    }

    return running;
}

void SDL3Backend::delay(std::uint32_t milliseconds) const noexcept {
    SDL_Delay(milliseconds);
}

}
