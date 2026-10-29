#pragma once

namespace Northrend {

class Backend;
class BackendApplication;
struct GamePlan;

class App final {
public:
    App(Backend& backend, BackendApplication& application, const GamePlan& plan) noexcept;

    void run();

private:
    Backend& backend;
    BackendApplication& application;
    const GamePlan& plan;
};

}
