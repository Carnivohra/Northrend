#pragma once

namespace Northrend {

class Backend;
class BackendApplication;
struct AppPlan;

class App final {
public:
    App(Backend& backend, BackendApplication& application, const AppPlan& plan) noexcept;

    void run();

private:
    Backend& backend;
    BackendApplication& application;
    const AppPlan& plan;
};

}
