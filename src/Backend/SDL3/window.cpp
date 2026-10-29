#include "Backend/SDL3/window.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace Northrend {

SDL3Window::SDL3Window(const std::string& title) : window(nullptr) {
    if (title.empty()) {
        throw std::invalid_argument("SDL3 window title must not be empty");
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("Failed to initialize SDL3: ") + SDL_GetError());
    }

    const SDL_DisplayID display = SDL_GetPrimaryDisplay();
    SDL_Rect bounds;

    if (display == 0 || !SDL_GetDisplayUsableBounds(display, &bounds)) {
        const std::string error = std::string("Failed to get the primary display size: ") + SDL_GetError();
        SDL_Quit();
        throw std::runtime_error(error);
    }

    this->window =
        SDL_CreateWindow(title.c_str(), bounds.w, bounds.h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!this->window) {
        const std::string error = std::string("Failed to create SDL3 window: ") + SDL_GetError();
        SDL_Quit();
        throw std::runtime_error(error);
    }

    if (!SDL_GetWindowSizeInPixels(this->window, &this->pixelWidth, &this->pixelHeight)) {
        const std::string error = std::string("Failed to get SDL3 window size: ") + SDL_GetError();
        SDL_DestroyWindow(this->window);
        SDL_Quit();
        throw std::runtime_error(error);
    }
}

SDL3Window::~SDL3Window() {
    SDL_DestroyWindow(this->window);
    SDL_Quit();
}

void SDL3Window::resize(int width, int height) noexcept {
    this->pixelWidth = width;
    this->pixelHeight = height;
}

NativeWindowHandle SDL3Window::native_handle() const {
#if defined(__ANDROID__) || defined(__linux__)
    const SDL_PropertiesID properties = SDL_GetWindowProperties(this->window);

    if (properties == 0) {
        throw std::runtime_error(std::string("Failed to get SDL3 window properties: ") + SDL_GetError());
    }
#endif

#if defined(__ANDROID__)
    void* nativeWindow = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);

    if (nativeWindow) {
        return AndroidWindowHandle {.window = nativeWindow};
    }

    throw std::runtime_error("SDL3 did not provide an Android native window");
#elif defined(__linux__)
    void* waylandDisplay = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
    void* waylandSurface = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);

    if (waylandDisplay && waylandSurface) {
        return WaylandWindowHandle {.display = waylandDisplay, .surface = waylandSurface};
    }

    void* x11Display = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    const Sint64 x11Window = SDL_GetNumberProperty(properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);

    if (x11Display && x11Window != 0) {
        return X11WindowHandle {.display = x11Display, .window = static_cast<std::uint64_t>(x11Window)};
    }

    throw std::runtime_error("SDL3 did not provide a Wayland or X11 window handle");
#else
    throw std::runtime_error("Native SDL3 window handles are not implemented for this platform");
#endif
}

int SDL3Window::width() const noexcept {
    return this->pixelWidth;
}

int SDL3Window::height() const noexcept {
    return this->pixelHeight;
}

}
