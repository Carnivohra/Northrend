#pragma once

#include "Render/frame.hpp"
#include "Render/Mesh/data.hpp"
#include "Render/Mesh/mesh.hpp"

namespace Northrend {

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual void store_mesh(MeshHandle handle, const MeshData& data) = 0;
    virtual void remove_mesh(MeshHandle handle) = 0;
    virtual void frame(const RenderFrame& frame) = 0;
    virtual void suspend() = 0;
};

}
