#pragma once

namespace Northrend {

class Backend;
class BackendApplication;

class App final {
public:
    App(Backend& backend, BackendApplication& application) noexcept;
    void run();

private:
    Backend& backend;
    BackendApplication& application;
};

}
