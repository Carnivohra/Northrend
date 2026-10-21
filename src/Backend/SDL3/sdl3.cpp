#include "Backend/SDL3/sdl3.hpp"

#include "Backend/SDL3/context.hpp"
#include "Backend/session.hpp"

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

void SDL3Backend::run(BackendApplication& application) {
    SDL3Context context(this->title, this->width, this->height);
    BackendSession session(application);

    session.start();

    bool running = true;

    while (running) {
        running = context.process_events(session);

        if (running && session.active()) {
            application.update();
            context.delay(1);
        } else if (running) {
            context.delay(10);
        }
    }

    session.stop();
}

}
