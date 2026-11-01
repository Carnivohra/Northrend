#include "Backend/backend.hpp"

#include <stdexcept>

namespace Northrend {

void Backend::execute(BackendApplication& application, Window& window) {
    if (this->application) {
        throw std::logic_error("Backend is already running");
    }

    this->application = &application;
    this->input.reset();
    this->input.begin_frame();

    try {
        application.started(window);
        this->started = true;
        this->resume();

        while (true) {
            this->input.begin_frame();

            if (!this->process_events(window, this->input)) {
                break;
            }

            if (this->resumed) {
                application.tick(this->input);
            } else {
                this->delay(10);
            }
        }

        this->stop();
    } catch (...) {
        this->stop_after_failure();
        this->application = nullptr;
        throw;
    }

    this->application = nullptr;
}

void Backend::resume() {
    if (this->resumed) {
        return;
    }

    this->application->resumed();
    this->resumed = true;
}

void Backend::suspend() {
    if (!this->resumed) {
        return;
    }

    this->resumed = false;
    this->application->suspended();
}

void Backend::stop() {
    this->suspend();

    if (!this->started) {
        return;
    }

    this->started = false;
    this->application->stopped();
}

void Backend::stop_after_failure() noexcept {
    if (this->resumed) {
        this->resumed = false;

        try {
            this->application->suspended();
        } catch (...) {
        }
    }

    if (this->started) {
        this->started = false;

        try {
            this->application->stopped();
        } catch (...) {
        }
    }
}

}
