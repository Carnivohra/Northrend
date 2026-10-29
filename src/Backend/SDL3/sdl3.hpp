#pragma once

#include "Backend/backend.hpp"

#include <filesystem>

namespace Northrend {

class SDL3Backend final : public Backend {
public:
    [[nodiscard]] std::filesystem::path base_path() const override;

    void run(BackendApplication& application, const GamePlan& plan) override;

private:
    bool process_events(Window& window) override;
    void delay(std::uint32_t milliseconds) const noexcept override;
};

}
