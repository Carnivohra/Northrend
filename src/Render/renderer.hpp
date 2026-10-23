#pragma once

#include "Render/snapshot.hpp"

namespace Northrend {

enum class RendererStatus {
    Uninitialized,
    Initializing,
    Ready
};

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual RendererStatus frame(const RenderSnapshot& snapshot) = 0;
    virtual void suspend() = 0;
};

}
