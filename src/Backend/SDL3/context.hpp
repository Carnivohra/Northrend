#pragma once

#include <cstdint>
#include <string>

struct SDL_Window;

namespace Northrend {

class BackendSession;

class SDL3Context final {
public:
    SDL3Context(const std::string& title, int width, int height);
    ~SDL3Context();

    SDL3Context(const SDL3Context&) = delete;
    SDL3Context& operator=(const SDL3Context&) = delete;

    bool process_events(BackendSession& session);
    void delay(std::uint32_t milliseconds) const noexcept;

private:
    SDL_Window* window;
};

}
