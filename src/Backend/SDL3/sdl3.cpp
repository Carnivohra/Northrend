#include "Backend/SDL3/sdl3.hpp"

#include "Backend/session.hpp"

#include <SDL3/SDL.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace Northrend {

namespace {
class SDLRuntime final {
public:
    SDLRuntime() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(std::string("Failed to initialize SDL3: ") + SDL_GetError());
        }
    }

    ~SDLRuntime() {
        SDL_Quit();
    }

    SDLRuntime(const SDLRuntime&) = delete;
    SDLRuntime& operator=(const SDLRuntime&) = delete;
};

struct SDLWindowDeleter final {
    void operator()(SDL_Window* window) const noexcept {
        SDL_DestroyWindow(window);
    }
};

using SDLWindow = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

SDLWindow create_window(const std::string& title, int width, int height) {
    SDLWindow window(SDL_CreateWindow(
        title.c_str(),
        width,
        height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY));

    if (!window) {
        throw std::runtime_error(std::string("Failed to create SDL3 window: ") + SDL_GetError());
    }

    return window;
}

bool process_events(BackendSession& session) {
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
}

SDL3Backend::SDL3Backend(std::string title, int width, int height)
    : title(std::move(title)), width(width), height(height) {
    if (this->title.empty()) {
        throw std::invalid_argument("SDL3 window title must not be empty");
    }

    if (this->width <= 0 || this->height <= 0) {
        throw std::invalid_argument("SDL3 window dimensions must be positive");
    }
}

void SDL3Backend::run(BackendApplication& application) {
    SDLRuntime runtime;
    SDLWindow window = create_window(this->title, this->width, this->height);
    BackendSession session(application);

    session.start();

    bool running = true;

    while (running) {
        running = process_events(session);

        if (running && session.active()) {
            application.update();
            SDL_Delay(1);
        } else if (running) {
            SDL_Delay(10);
        }
    }

    session.stop();
}

}
