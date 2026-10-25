#pragma once

#include "Render/frame.hpp"

namespace Northrend {

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual void frame(const RenderFrame& frame) = 0;
    virtual void suspend() = 0;
};

}
