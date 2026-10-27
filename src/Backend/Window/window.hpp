#pragma once

#include "Backend/Window/Handle/handle.hpp"

namespace Northrend {

class Window {
public:
    virtual ~Window() = default;

    virtual NativeWindowHandle native_handle() const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
};

}
