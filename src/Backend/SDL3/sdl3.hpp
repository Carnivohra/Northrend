#pragma once

#include "Backend/backend.hpp"

#include <filesystem>
#include <string>

namespace Northrend {

class SDL3Backend final : public Backend {
public:
    SDL3Backend(std::string title, int width, int height);

    [[nodiscard]] std::filesystem::path base_path() const override;

    void run(BackendApplication& application) override;

private:
    bool process_events(Window& window) override;
    void delay(std::uint32_t milliseconds) const noexcept override;

    std::string title;
    int width;
    int height;
};

}
