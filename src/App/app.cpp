#include "App/app.hpp"

#include "Backend/backend.hpp"

namespace Northrend {

App::App(Backend& backend, BackendApplication& application, const GamePlan& plan) noexcept :
    backend(backend), application(application), plan(plan) {}

void App::run() {
    this->backend.run(this->application, this->plan);
}

}
