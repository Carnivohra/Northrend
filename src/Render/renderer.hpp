#pragma once

#include "Render/frame.hpp"
#include "Render/Material/data.hpp"
#include "Render/Material/material.hpp"
#include "Render/Mesh/data.hpp"
#include "Render/Mesh/mesh.hpp"
#include "Render/Texture/data.hpp"
#include "Render/Texture/texture.hpp"

#include <span>

namespace Northrend {

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual void store_mesh(MeshHandle handle, const MeshData& data) = 0;
    virtual void remove_mesh(MeshHandle handle) = 0;
    virtual void store_material(MaterialHandle handle, const MaterialData& data) = 0;
    virtual void remove_material(MaterialHandle handle) = 0;
    virtual void store_texture(TextureHandle handle, const TextureData& data) = 0;
    virtual void remove_texture(TextureHandle handle) = 0;
    virtual void remove_textures(std::span<const TextureHandle> handles) = 0;
    virtual void frame(const RenderFrame& frame) = 0;
    virtual void suspend() = 0;
};

}
