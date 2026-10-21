#include "Backend/SDL3/sdl3.hpp"

#include <SDL3/SDL.h>

#include <exception>
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

void stop_after_failure(BackendApplication& application, bool started, bool resumed) noexcept {
    if (resumed) {
        try {
            application.suspended();
        } catch (...) {
        }
    }

    if (started) {
        try {
            application.stopped();
        } catch (...) {
        }
    }
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
    SDLWindow window(SDL_CreateWindow(
        this->title.c_str(),
        this->width,
        this->height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY));

    if (!window) {
        throw std::runtime_error(std::string("Failed to create SDL3 window: ") + SDL_GetError());
    }

    bool running = true;
    bool started = false;
    bool resumed = false;

    try {
        application.started();
        started = true;

        application.resumed();
        resumed = true;

        while (running) {
            SDL_Event event;

            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_EVENT_QUIT:
                    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                        running = false;
                        break;
                    case SDL_EVENT_WINDOW_MINIMIZED:
                        if (resumed) {
                            resumed = false;
                            application.suspended();
                        }
                        break;
                    case SDL_EVENT_WINDOW_RESTORED:
                        if (!resumed) {
                            application.resumed();
                            resumed = true;
                        }
                        break;
                    default:
                        break;
                }
            }

            if (running && resumed) {
                application.update();
                SDL_Delay(1);
            } else if (running) {
                SDL_Delay(10);
            }
        }

        if (resumed) {
            resumed = false;
            application.suspended();
        }

        started = false;
        application.stopped();
    } catch (...) {
        stop_after_failure(application, started, resumed);
        throw;
    }
}

}
