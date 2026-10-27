#pragma once

#include "Backend/backend.hpp"

#include <string>

namespace Northrend {

class SDL3Backend final : public Backend {
public:
    SDL3Backend(std::string title, int width, int height);

    void run(BackendApplication& application) override;

private:
    bool process_events(Window& window) override;
    void delay(std::uint32_t milliseconds) const noexcept override;

    std::string title;
    int width;
    int height;
};

}
