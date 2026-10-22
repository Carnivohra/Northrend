#include "Backend/SDL3/context.hpp"

#include "Backend/session.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace Northrend {

SDL3Context::SDL3Context(const std::string& title, int width, int height)
    : window(nullptr) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("Failed to initialize SDL3: ") + SDL_GetError());
    }

    this->window = SDL_CreateWindow(
        title.c_str(),
        width,
        height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!this->window) {
        const std::string error = std::string("Failed to create SDL3 window: ") + SDL_GetError();
        SDL_Quit();
        throw std::runtime_error(error);
    }
}

SDL3Context::~SDL3Context() {
    SDL_DestroyWindow(this->window);
    SDL_Quit();
}

bool SDL3Context::process_events(BackendSession& session) {
    bool running = true;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                running = false;
                break;
            case SDL_EVENT_WINDOW_MINIMIZED:
                session.suspend();
                break;
            case SDL_EVENT_WINDOW_RESTORED:
                session.resume();
                break;
            default:
                break;
        }
    }

    return running;
}

void SDL3Context::delay(std::uint32_t milliseconds) const noexcept {
    SDL_Delay(milliseconds);
}

NativeWindowHandle SDL3Context::native_handle() const {
#if defined(__ANDROID__) || defined(__linux__)
    const SDL_PropertiesID properties = SDL_GetWindowProperties(this->window);

    if (properties == 0) {
        throw std::runtime_error(std::string("Failed to get SDL3 window properties: ") + SDL_GetError());
    }
#endif

#if defined(__ANDROID__)
    void* nativeWindow = SDL_GetPointerProperty(
        properties,
        SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER,
        nullptr);

    if (nativeWindow) {
        return AndroidWindowHandle{ .window = nativeWindow };
    }

    throw std::runtime_error("SDL3 did not provide an Android native window");
#elif defined(__linux__)
    void* waylandDisplay = SDL_GetPointerProperty(
        properties,
        SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER,
        nullptr);
    void* waylandSurface = SDL_GetPointerProperty(
        properties,
        SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER,
        nullptr);

    if (waylandDisplay && waylandSurface) {
        return WaylandWindowHandle{
            .display = waylandDisplay,
            .surface = waylandSurface
        };
    }

    void* x11Display = SDL_GetPointerProperty(
        properties,
        SDL_PROP_WINDOW_X11_DISPLAY_POINTER,
        nullptr);
    const Sint64 x11Window = SDL_GetNumberProperty(
        properties,
        SDL_PROP_WINDOW_X11_WINDOW_NUMBER,
        0);

    if (x11Display && x11Window != 0) {
        return X11WindowHandle{
            .display = x11Display,
            .window = static_cast<std::uint64_t>(x11Window)
        };
    }

    throw std::runtime_error("SDL3 did not provide a Wayland or X11 window handle");
#else
    throw std::runtime_error("Native SDL3 window handles are not implemented for this platform");
#endif
}

WindowSize SDL3Context::size() const {
    int width = 0;
    int height = 0;

    if (!SDL_GetWindowSizeInPixels(this->window, &width, &height)) {
        throw std::runtime_error(std::string("Failed to get SDL3 window size: ") + SDL_GetError());
    }

    return WindowSize{
        .width = static_cast<std::uint32_t>(width),
        .height = static_cast<std::uint32_t>(height)
    };
}

}
