#pragma once

#include "Backend/application.hpp"

#include <cstdint>
#include <filesystem>

namespace Northrend {

struct GamePlan;

class Backend {
public:
    virtual ~Backend() = default;

    [[nodiscard]] virtual std::filesystem::path base_path() const = 0;
    virtual void run(BackendApplication& application, const GamePlan& plan) = 0;

protected:
    void execute(BackendApplication& application, Window& window);
    void resume();
    void suspend();

    virtual bool process_events(Window& window) = 0;
    virtual void delay(std::uint32_t milliseconds) const noexcept = 0;

private:
    void stop();
    void stop_after_failure() noexcept;

    BackendApplication* application = nullptr;
    bool started = false;
    bool resumed = false;
};

}
