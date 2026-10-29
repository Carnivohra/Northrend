#pragma once

#include "Backend/Window/window.hpp"

#include <string>

struct SDL_Window;

namespace Northrend {

class SDL3Window final : public Window {
public:
    explicit SDL3Window(const std::string& title);
    ~SDL3Window();

    SDL3Window(const SDL3Window&) = delete;
    SDL3Window& operator=(const SDL3Window&) = delete;

    void resize(int width, int height) noexcept;

    NativeWindowHandle native_handle() const override;
    int width() const noexcept override;
    int height() const noexcept override;

private:
    SDL_Window* window;
    int pixelWidth = 0;
    int pixelHeight = 0;
};

}
