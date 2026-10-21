#pragma once

namespace Northrend {

class BackendApplication;
class Window;

class BackendSession final {
public:
    explicit BackendSession(BackendApplication& application) noexcept;
    ~BackendSession();

    BackendSession(const BackendSession&) = delete;
    BackendSession& operator=(const BackendSession&) = delete;

    void start(const Window& window);
    void resume();
    void suspend();
    void stop();

    bool active() const noexcept;

private:
    void stop_after_failure() noexcept;

    BackendApplication& application;
    bool started = false;
    bool resumed = false;
};

}
