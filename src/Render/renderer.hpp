#pragma once

#include "Render/frame.hpp"

namespace Northrend {

enum class RendererStatus {
    Uninitialized,
    Initializing,
    Ready
};

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual RendererStatus frame(const RenderFrame& frame) = 0;
    virtual void suspend() = 0;
};

}
