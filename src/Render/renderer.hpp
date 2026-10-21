#pragma once

namespace Northrend {

enum class RendererStatus {
    Uninitialized,
    Initializing,
    Ready
};

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual RendererStatus update() = 0;
    virtual void suspend() = 0;
};

}
