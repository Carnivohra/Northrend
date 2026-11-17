#include "Presentation/Map/Object/resource.hpp"

#include "Presentation/Map/Object/geometry.hpp"
#include "Presentation/Map/Object/material.hpp"
#include "Presentation/Map/Object/model.hpp"
#include "Presentation/Texture/cache.hpp"
#include "Render/Material/allocator.hpp"
#include "Render/Material/data.hpp"
#include "Render/Mesh/allocator.hpp"
#include "Render/Mesh/data.hpp"
#include "Render/renderer.hpp"

#include <span>
#include <utility>

namespace Northrend {

PresentationMapObjectResource::PresentationMapObjectResource(
    std::shared_ptr<const PresentationMapObjectModel> model) noexcept :
    objectModel(std::move(model)) {}

void PresentationMapObjectResource::retain(PresentationTextureCache& textures, Renderer& renderer) {
    if (this->texturesRetained) {
        return;
    }

    std::size_t textureCount = 0;

    for (const PresentationMapObjectMaterial& material : this->objectModel->materials()) {
        textureCount += static_cast<std::size_t>(material.texture1.has_value());
        textureCount += static_cast<std::size_t>(material.texture2.has_value());
    }

    this->retainedTextures.reserve(textureCount);

    try {
        for (const PresentationMapObjectMaterial& material : this->objectModel->materials()) {
            if (material.texture1) {
                textures.retain(*material.texture1);
                this->retainedTextures.push_back(&*material.texture1);
            }

            if (material.texture2) {
                textures.retain(*material.texture2);
                this->retainedTextures.push_back(&*material.texture2);
            }
        }
    } catch (...) {
        for (const AssetId* texture : this->retainedTextures) {
            textures.release(*texture, renderer);
        }

        this->retainedTextures.clear();
        throw;
    }

    this->texturesRetained = true;
}

void PresentationMapObjectResource::release(MeshHandleAllocator& meshAllocator,
    MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures, Renderer& renderer) {
    if (this->objectMesh) {
        renderer.remove_mesh(this->objectMesh);
        meshAllocator.release(this->objectMesh);
        this->objectMesh = {};
    }

    for (const MaterialHandle material : this->objectMaterials) {
        if (material) {
            renderer.remove_material(material);
            materialAllocator.release(material);
        }
    }

    this->objectMaterials.clear();

    for (const AssetId* texture : this->retainedTextures) {
        textures.release(*texture, renderer);
    }

    this->retainedTextures.clear();
    this->texturesRetained = false;
    this->uploadComplete = false;
}

bool PresentationMapObjectResource::ready(const PresentationTextureCache& textures) const {
    for (const AssetId* texture : this->retainedTextures) {
        if (!textures.ready(*texture)) {
            return false;
        }
    }

    return true;
}

void PresentationMapObjectResource::store(MeshHandleAllocator& meshAllocator,
    MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures, Renderer& renderer) {
    if (this->uploadComplete) {
        return;
    }

    const PresentationMapObjectGeometry& geometry = this->objectModel->geometry();

    if (geometry.vertices().empty() || geometry.sections().empty()) {
        this->uploadComplete = true;
        return;
    }

    std::vector<MeshLevel> sections;
    sections.reserve(geometry.sections().size());

    for (const PresentationMapObjectSection& section : geometry.sections()) {
        const std::span<const std::uint32_t> indices =
            geometry.indices().subspan(section.firstIndex, section.indexCount);
        sections.push_back(
            MeshLevel {.indices = std::as_bytes(indices), .indexCount = static_cast<std::uint32_t>(indices.size())});
    }

    const MeshData meshData(std::as_bytes(geometry.vertices()), sections, 1, MeshIndexType::unsigned32);
    MeshHandle mesh;
    std::vector<MaterialHandle> materials(this->objectModel->materials().size());
    bool meshStored = false;

    try {
        mesh = meshAllocator.allocate();
        renderer.store_mesh(mesh, meshData);
        meshStored = true;

        for (std::size_t index = 0; index < this->objectModel->materials().size(); ++index) {
            const PresentationMapObjectMaterial& material = this->objectModel->materials()[index];

            if (!material.texture1) {
                continue;
            }

            std::vector<TextureHandle> materialTextures;
            materialTextures.push_back(textures.find(*material.texture1));

            if (material.texture2) {
                materialTextures.push_back(textures.find(*material.texture2));
            }

            materials[index] = materialAllocator.allocate();
            const MaterialProperties properties {
                .shader = material.shader,
                .blendMode = material.blendMode,
                .flags = material.flags
            };
            renderer.store_material(materials[index], MaterialData(materialTextures, {}, {}, properties));
        }
    } catch (...) {
        for (const MaterialHandle material : materials) {
            if (material) {
                renderer.remove_material(material);
                materialAllocator.release(material);
            }
        }

        if (meshStored) {
            renderer.remove_mesh(mesh);
        }

        if (mesh) {
            meshAllocator.release(mesh);
        }

        throw;
    }

    this->objectMesh = mesh;
    this->objectMaterials = std::move(materials);
    this->uploadComplete = true;
}

MeshHandle PresentationMapObjectResource::mesh() const noexcept {
    return this->objectMesh;
}

MaterialHandle PresentationMapObjectResource::material(const std::size_t index) const noexcept {
    return index < this->objectMaterials.size() ? this->objectMaterials[index] : MaterialHandle {};
}

bool PresentationMapObjectResource::stored() const noexcept {
    return this->uploadComplete;
}

void PresentationMapObjectResource::add_reference() noexcept {
    ++this->references;
}

bool PresentationMapObjectResource::remove_reference() noexcept {
    return this->references != 0 && --this->references == 0;
}

}
