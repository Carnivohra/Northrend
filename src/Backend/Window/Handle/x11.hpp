#pragma once

#include <cstdint>

namespace Northrend {

struct X11WindowHandle {
    void* display;
    std::uint64_t window;
};

}
