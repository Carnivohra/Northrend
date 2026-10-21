#pragma once

#include "Backend/Window/handle.hpp"
#include "Backend/Window/size.hpp"

namespace Northrend {

class Window {
public:
    virtual ~Window() = default;

    virtual NativeWindowHandle native_handle() const = 0;
    virtual WindowSize size() const = 0;
};

}
