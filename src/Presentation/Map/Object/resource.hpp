#pragma once

#include "Render/Material/material.hpp"
#include "Render/Mesh/mesh.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace Northrend {

class MaterialHandleAllocator;
class MeshHandleAllocator;
class AssetId;
class PresentationMapObjectModel;
class PresentationTextureCache;
class Renderer;

class PresentationMapObjectResource final {
public:
    explicit PresentationMapObjectResource(std::shared_ptr<const PresentationMapObjectModel> model) noexcept;

    PresentationMapObjectResource(const PresentationMapObjectResource&) = delete;
    PresentationMapObjectResource& operator=(const PresentationMapObjectResource&) = delete;

    void retain(PresentationTextureCache& textures, Renderer& renderer);
    void release(MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
        PresentationTextureCache& textures, Renderer& renderer);
    [[nodiscard]] bool ready(const PresentationTextureCache& textures) const;
    void store(MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
        PresentationTextureCache& textures, Renderer& renderer);

    [[nodiscard]] MeshHandle mesh() const noexcept;
    [[nodiscard]] MaterialHandle material(std::size_t index) const noexcept;
    [[nodiscard]] bool stored() const noexcept;
    void add_reference() noexcept;
    [[nodiscard]] bool remove_reference() noexcept;

private:
    std::shared_ptr<const PresentationMapObjectModel> objectModel;
    std::vector<const AssetId*> retainedTextures;
    std::vector<MaterialHandle> objectMaterials;
    MeshHandle objectMesh;
    std::size_t references = 0;
    bool texturesRetained = false;
    bool uploadComplete = false;
};

}
