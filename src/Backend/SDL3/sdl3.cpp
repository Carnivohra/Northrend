#include "Backend/SDL3/sdl3.hpp"

#include "Backend/SDL3/input.hpp"
#include "Backend/SDL3/window.hpp"
#include "Backend/plan.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace Northrend {

SDL3Backend::SDL3Backend() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        throw std::runtime_error(std::string("Failed to initialize SDL3: ") + SDL_GetError());
    }
}

SDL3Backend::~SDL3Backend() {
    this->eventInput.close();
    SDL_Quit();
}

std::filesystem::path SDL3Backend::base_path() const {
    const char* path = SDL_GetBasePath();

    if (!path) {
        throw std::runtime_error("SDL3 could not resolve the application base path: " + std::string(SDL_GetError()));
    }

    return std::filesystem::path(path);
}

void SDL3Backend::run(BackendApplication& application, const BackendPlan& plan) {
    SDL3Window window(plan.title);

    try {
        this->execute(application, window);
    } catch (...) {
        this->eventInput.close();
        throw;
    }

    this->eventInput.close();
}

bool SDL3Backend::process_events(Window& window, Input& input) {
    bool running = true;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        this->eventInput.process(event, input);

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
