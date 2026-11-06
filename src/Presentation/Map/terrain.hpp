#pragma once

#include "ECS/entity.hpp"
#include "Math/vector.hpp"
#include "Render/Mesh/mesh.hpp"
#include "Render/Material/material.hpp"
#include "Render/Texture/texture.hpp"
#include "World/Map/Tile/grid.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Northrend {

struct Camera;
class MapTile;
class TerrainPayload;
class MaterialHandleAllocator;
class MeshHandleAllocator;
class PresentationTextureCache;
class Renderer;
class RenderScene;

class PresentationTerrain final {
public:
    void load(std::size_t x, std::size_t y, const MapTile& tile, const TerrainPayload& payload,
        MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
        PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene);
    void unload(std::size_t x, std::size_t y, MeshHandleAllocator& meshAllocator,
        MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures, Renderer& renderer,
        RenderScene& scene);
    void clear(MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
        PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene);
    void update(const Camera& camera, int width, int height, RenderScene& scene);

private:
    [[nodiscard]] static constexpr std::size_t index(const std::size_t x, const std::size_t y) noexcept {
        return y * TileGrid::Size + x;
    }

    void release(std::size_t tile, MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
        PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene);
    [[nodiscard]] static float projected_size(
        const Camera& camera, int height, const Vector3& minimum, const Vector3& maximum) noexcept;
    [[nodiscard]] static std::uint8_t select_level(float projectedSize, std::uint8_t current) noexcept;

    static constexpr std::uint8_t UnsetLevel = 0xFF;

    std::array<MeshHandle, TileGrid::TileCount> meshes;
    std::array<MaterialHandle, TileGrid::TileCount> materials;
    std::array<TextureHandle, TileGrid::TileCount> alphaTextures;
    std::array<Entity, TileGrid::TileCount> entities;
    std::array<Vector3, TileGrid::TileCount> minimums;
    std::array<Vector3, TileGrid::TileCount> maximums;
    std::array<std::uint8_t, TileGrid::TileCount> levels;
    std::array<bool, TileGrid::TileCount> visibility;
    std::uint64_t cameraRevision = 0;
    int viewportWidth = 0;
    int viewportHeight = 0;
    bool levelsDirty = true;
};

}
