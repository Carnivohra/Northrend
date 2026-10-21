#include "Backend/SDL3/context.hpp"

#include "Backend/session.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace Northrend {

SDL3Context::SDL3Context(const std::string& title, int width, int height)
    : window(nullptr) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("Failed to initialize SDL3: ") + SDL_GetError());
    }

    this->window = SDL_CreateWindow(
        title.c_str(),
        width,
        height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!this->window) {
        const std::string error = std::string("Failed to create SDL3 window: ") + SDL_GetError();
        SDL_Quit();
        throw std::runtime_error(error);
    }
}

SDL3Context::~SDL3Context() {
    SDL_DestroyWindow(this->window);
    SDL_Quit();
}

bool SDL3Context::process_events(BackendSession& session) {
    bool running = true;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                running = false;
                break;
            case SDL_EVENT_WINDOW_MINIMIZED:
                session.suspend();
                break;
            case SDL_EVENT_WINDOW_RESTORED:
                session.resume();
                break;
            default:
                break;
        }
    }

    return running;
}

void SDL3Context::delay(std::uint32_t milliseconds) const noexcept {
    SDL_Delay(milliseconds);
}

}
