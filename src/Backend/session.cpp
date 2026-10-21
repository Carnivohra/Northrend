#include "Backend/session.hpp"

#include "Backend/application.hpp"

namespace Northrend {

BackendSession::BackendSession(BackendApplication& application) noexcept
    : application(application) {}

BackendSession::~BackendSession() {
    this->stop_after_failure();
}

void BackendSession::start() {
    this->application.started();
    this->started = true;

    this->application.resumed();
    this->resumed = true;
}

void BackendSession::resume() {
    if (this->resumed) {
        return;
    }

    this->application.resumed();
    this->resumed = true;
}

void BackendSession::suspend() {
    if (!this->resumed) {
        return;
    }

    this->resumed = false;
    this->application.suspended();
}

void BackendSession::stop() {
    this->suspend();

    if (!this->started) {
        return;
    }

    this->started = false;
    this->application.stopped();
}

bool BackendSession::active() const noexcept {
    return this->resumed;
}

void BackendSession::stop_after_failure() noexcept {
    if (this->resumed) {
        this->resumed = false;

        try {
            this->application.suspended();
        } catch (...) {
        }
    }

    if (this->started) {
        this->started = false;

        try {
            this->application.stopped();
        } catch (...) {
        }
    }
}

}
