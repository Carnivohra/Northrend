#include "Backend/SDL3/sdl3.hpp"

#include "Backend/SDL3/window.hpp"
#include "Backend/plan.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace Northrend {

std::filesystem::path SDL3Backend::base_path() const {
    const char* path = SDL_GetBasePath();

    if (!path) {
        throw std::runtime_error("SDL3 could not resolve the application base path: " + std::string(SDL_GetError()));
    }

    return std::filesystem::path(path);
}

void SDL3Backend::run(BackendApplication& application, const BackendPlan& plan) {
    SDL3Window window(plan.title);
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
            static_cast<SDL3Window&>(window).resize(event.window.data1, event.window.data2);
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
