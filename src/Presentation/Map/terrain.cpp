#include "Presentation/Map/terrain.hpp"

#include "Render/instance.hpp"
#include "Render/Material/allocator.hpp"
#include "Render/Material/data.hpp"
#include "Render/frustum.hpp"
#include "Render/Mesh/allocator.hpp"
#include "Render/Mesh/data.hpp"
#include "Render/Mesh/level.hpp"
#include "Render/renderer.hpp"
#include "Render/Texture/data.hpp"
#include "Render/Texture/format.hpp"
#include "Render/camera.hpp"
#include "Render/scene.hpp"
#include "Presentation/Texture/cache.hpp"
#include "World/Map/Terrain/geometry.hpp"
#include "World/Map/Terrain/material.hpp"
#include "World/Map/Terrain/payload.hpp"
#include "World/Map/Tile/tile.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Northrend {

void PresentationTerrain::load(const std::size_t x, const std::size_t y, const MapTile& tile,
    const TerrainPayload& payload, MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
    PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene) {
    if (x >= TileGrid::Size || y >= TileGrid::Size) {
        throw std::out_of_range("Presentation terrain coordinate is outside the map");
    }

    const std::size_t tileIndex = index(x, y);
    this->release(tileIndex, meshAllocator, materialAllocator, textures, renderer, scene);

    const TerrainGeometry& geometry = tile.terrain();
    const TerrainMaterial& material = tile.material();
    std::vector<MeshVertex> vertices(geometry.vertices().begin(), geometry.vertices().end());
    std::vector<TextureHandle> materialTextures;
    materialTextures.reserve(material.texture_count());

    for (std::size_t texture = 0; texture < material.texture_count(); ++texture) {
        const TextureHandle handle = textures.find(material.texture(texture));

        if (!handle) {
            throw std::runtime_error("Terrain texture is missing from the presentation cache");
        }

        materialTextures.push_back(handle);
    }

    for (MeshVertex& vertex : vertices) {
        for (const std::uint8_t texture : vertex.textures) {
            if (texture >= materialTextures.size()) {
                throw std::runtime_error("Terrain vertex references an invalid texture");
            }
        }
    }

    static constexpr std::uint32_t AlphaLayerBits = 8;
    static constexpr std::uint32_t MaximumAlphaLayer = (std::uint32_t {1} << AlphaLayerBits) - 1;

    for (std::size_t chunkY = 0; chunkY < Adt::Size; ++chunkY) {
        for (std::size_t chunkX = 0; chunkX < Adt::Size; ++chunkX) {
            const std::size_t chunk = chunkY * Adt::Size + chunkX;
            const std::size_t layerCount = std::max(material.layer_count(chunkX, chunkY), std::size_t {1});
            const std::uint32_t alphaLayer = payload.alpha_layer(chunkX, chunkY);

            if (alphaLayer > MaximumAlphaLayer || layerCount > Adt::MaximumLayers) {
                throw std::overflow_error("Terrain material exceeds the packed vertex layout");
            }

            const std::uint32_t materialData =
                alphaLayer | static_cast<std::uint32_t>(layerCount - 1) << AlphaLayerBits;
            const std::size_t firstVertex = chunk * Adt::HeightsPerChunk;
            const std::size_t endVertex = firstVertex + Adt::HeightsPerChunk;

            for (std::size_t vertex = firstVertex; vertex < endVertex; ++vertex) {
                vertices[vertex].material = materialData;
            }
        }
    }

    Vector3 minimum {
        .x = std::numeric_limits<float>::max(),
        .y = std::numeric_limits<float>::max(),
        .z = std::numeric_limits<float>::max()
    };
    Vector3 maximum {
        .x = std::numeric_limits<float>::lowest(),
        .y = std::numeric_limits<float>::lowest(),
        .z = std::numeric_limits<float>::lowest()
    };
    for (const MeshVertex& vertex : vertices) {
        const Vector3& position = vertex.position;
        minimum.x = std::min(minimum.x, position.x);
        minimum.y = std::min(minimum.y, position.y);
        minimum.z = std::min(minimum.z, position.z);
        maximum.x = std::max(maximum.x, position.x);
        maximum.y = std::max(maximum.y, position.y);
        maximum.z = std::max(maximum.z, position.z);
    }

    std::array<MeshLevel, TerrainGeometry::LevelCount> levels;

    for (std::size_t level = 0; level < levels.size(); ++level) {
        const std::span<const std::uint16_t> indices = geometry.indices(level);
        levels[level] =
            MeshLevel {.indices = std::as_bytes(indices), .indexCount = static_cast<std::uint32_t>(indices.size())};
    }

    std::array<std::span<const std::byte>, TerrainPayload::MaximumMipCount> alphaMips;

    for (std::size_t mip = 0; mip < payload.alpha_mip_count(); ++mip) {
        alphaMips[mip] = payload.alpha_mip(mip);
    }

    const TextureData alphaData(TextureFormat::rgba8Unorm, Adt::AlphaSize, Adt::AlphaSize,
        static_cast<std::uint32_t>(payload.alpha_layer_count()),
        std::span<const std::span<const std::byte>>(alphaMips.data(), payload.alpha_mip_count()));
    const MeshData data(std::as_bytes(std::span<const MeshVertex>(vertices)), levels, MeshIndexType::unsigned16);
    MaterialHandle renderMaterial;
    MeshHandle mesh;
    TextureHandle alphaTexture;
    Entity entity;
    bool materialStored = false;
    bool meshStored = false;

    try {
        renderMaterial = materialAllocator.allocate();
        mesh = meshAllocator.allocate();
        alphaTexture = textures.store(alphaData, renderer);
        renderer.store_material(renderMaterial, MaterialData(materialTextures, alphaTexture));
        materialStored = true;
        renderer.store_mesh(mesh, data);
        meshStored = true;
        entity = scene.registry().create();
        scene.registry().emplace<RenderInstance>(
            entity, RenderInstance {.mesh = mesh, .material = renderMaterial, .transform = {}, .worldSpace = true});
    } catch (...) {
        if (entity) {
            scene.registry().destroy(entity);
        }

        if (meshStored) {
            renderer.remove_mesh(mesh);
        }

        if (materialStored) {
            renderer.remove_material(renderMaterial);
        }

        if (alphaTexture) {
            textures.remove(alphaTexture, renderer);
        }

        if (mesh) {
            meshAllocator.release(mesh);
        }

        if (renderMaterial) {
            materialAllocator.release(renderMaterial);
        }

        throw;
    }

    this->meshes[tileIndex] = mesh;
    this->materials[tileIndex] = renderMaterial;
    this->alphaTextures[tileIndex] = alphaTexture;
    this->entities[tileIndex] = entity;
    this->minimums[tileIndex] = minimum;
    this->maximums[tileIndex] = maximum;
    this->levels[tileIndex] = UnsetLevel;
    this->visibility[tileIndex] = true;
    this->levelsDirty = true;
}

void PresentationTerrain::unload(const std::size_t x, const std::size_t y, MeshHandleAllocator& meshAllocator,
    MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures, Renderer& renderer,
    RenderScene& scene) {
    if (x >= TileGrid::Size || y >= TileGrid::Size) {
        return;
    }

    this->release(index(x, y), meshAllocator, materialAllocator, textures, renderer, scene);
}

void PresentationTerrain::clear(MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
    PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene) {
    for (std::size_t tile = 0; tile < TileGrid::TileCount; ++tile) {
        this->release(tile, meshAllocator, materialAllocator, textures, renderer, scene);
    }

    this->levelsDirty = true;
}

void PresentationTerrain::update(const Camera& camera, const int width, const int height, RenderScene& scene) {
    if (width <= 0 || height <= 0 ||
        (!this->levelsDirty && camera.revision == this->cameraRevision && width == this->viewportWidth &&
            height == this->viewportHeight)) {
        return;
    }

    const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    const Frustum frustum(camera.projection(aspectRatio) * camera.view());

    for (std::size_t tile = 0; tile < TileGrid::TileCount; ++tile) {
        if (!this->entities[tile]) {
            continue;
        }

        const bool visible = frustum.intersects(this->minimums[tile], this->maximums[tile]);
        std::uint8_t level = this->levels[tile];

        if (visible) {
            const float size = projected_size(camera, height, this->minimums[tile], this->maximums[tile]);
            level = select_level(size, level);
        }

        if (visible == this->visibility[tile] && level == this->levels[tile]) {
            continue;
        }

        const bool modified =
            scene.registry().modify<RenderInstance>(this->entities[tile], [visible, level](RenderInstance& instance) {
                instance.visible = visible;

                if (visible) {
                    instance.level = level;
                }
            });

        if (!modified) {
            throw std::runtime_error("Terrain render instance is missing");
        }

        this->levels[tile] = level;
        this->visibility[tile] = visible;
    }

    this->cameraRevision = camera.revision;
    this->viewportWidth = width;
    this->viewportHeight = height;
    this->levelsDirty = false;
}

void PresentationTerrain::release(const std::size_t tile, MeshHandleAllocator& meshAllocator,
    MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures, Renderer& renderer,
    RenderScene& scene) {
    const Entity entity = this->entities[tile];
    const MeshHandle mesh = this->meshes[tile];
    const MaterialHandle material = this->materials[tile];
    const TextureHandle alphaTexture = this->alphaTextures[tile];

    if (entity) {
        scene.registry().destroy(entity);
        this->entities[tile] = {};
    }

    if (mesh) {
        renderer.remove_mesh(mesh);
        meshAllocator.release(mesh);
        this->meshes[tile] = {};
    }

    if (material) {
        renderer.remove_material(material);
        materialAllocator.release(material);
        this->materials[tile] = {};
    }

    if (alphaTexture) {
        textures.remove(alphaTexture, renderer);
        this->alphaTextures[tile] = {};
    }

    this->levels[tile] = UnsetLevel;
    this->visibility[tile] = false;
    this->levelsDirty = true;
}

float PresentationTerrain::projected_size(
    const Camera& camera, const int height, const Vector3& minimum, const Vector3& maximum) noexcept {
    const float halfX = (maximum.x - minimum.x) * 0.5F;
    const float halfY = (maximum.y - minimum.y) * 0.5F;
    const float halfZ = (maximum.z - minimum.z) * 0.5F;
    const float radius = std::sqrt(halfX * halfX + halfY * halfY + halfZ * halfZ);
    const float closestX = std::clamp(camera.position.x, minimum.x, maximum.x);
    const float closestY = std::clamp(camera.position.y, minimum.y, maximum.y);
    const float closestZ = std::clamp(camera.position.z, minimum.z, maximum.z);
    const float deltaX = camera.position.x - closestX;
    const float deltaY = camera.position.y - closestY;
    const float deltaZ = camera.position.z - closestZ;
    const float distance =
        std::max(std::sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ), std::max(camera.nearPlane, 0.001F));
    const float tangent = std::tan(camera.verticalFieldOfView * 0.5F);

    if (tangent <= 0.0F) {
        return std::numeric_limits<float>::max();
    }

    return radius * static_cast<float>(height) / (distance * tangent);
}

std::uint8_t PresentationTerrain::select_level(const float projectedSize, const std::uint8_t current) noexcept {
    static constexpr std::array<float, TerrainGeometry::LevelCount - 1> Thresholds {512.0F, 48.0F};
    static constexpr float CoarserHysteresis = 0.85F;
    static constexpr float FinerHysteresis = 1.15F;

    if (current >= TerrainGeometry::LevelCount) {
        std::uint8_t level = 0;

        while (level < Thresholds.size() && projectedSize < Thresholds[level]) {
            ++level;
        }

        return level;
    }

    std::uint8_t level = current;

    while (level < Thresholds.size() && projectedSize < Thresholds[level] * CoarserHysteresis) {
        ++level;
    }

    while (level > 0 && projectedSize > Thresholds[level - 1] * FinerHysteresis) {
        --level;
    }

    return level;
}

}
