#include "App/app.hpp"

#include "Backend/backend.hpp"

namespace Northrend {

App::App(Backend& backend, BackendApplication& application) noexcept
    : backend(backend), application(application) {}

void App::run() {
    this->backend.run(this->application);
}

}
