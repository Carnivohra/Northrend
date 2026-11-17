#pragma once

#include "ECS/entity.hpp"
#include "Math/vector.hpp"
#include "Render/Mesh/mesh.hpp"
#include "Render/Material/material.hpp"
#include "Render/Texture/texture.hpp"
#include "World/Map/Tile/grid.hpp"
#include "World/Map/Terrain/geometry.hpp"

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
    void update(const Camera& camera, int width, int height, Renderer& renderer, RenderScene& scene);

private:
    [[nodiscard]] static constexpr std::size_t index(const std::size_t x, const std::size_t y) noexcept {
        return y * TileGrid::Size + x;
    }

    void release(std::size_t tile, MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
        PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene);
    void activate(std::size_t tile) noexcept;
    void deactivate(std::size_t tile) noexcept;
    [[nodiscard]] static float projected_size(
        const Camera& camera, int height, const Vector3& minimum, const Vector3& maximum) noexcept;
    [[nodiscard]] static std::uint8_t select_level(float projectedSize, std::uint8_t current) noexcept;

    static constexpr std::uint8_t UnsetLevel = 0xFF;

    std::array<MeshHandle, TileGrid::TileCount> meshes;
    std::array<MaterialHandle, TileGrid::TileCount> materials;
    std::array<TextureHandle, TileGrid::TileCount> alphaTextures;
    std::array<TextureHandle, TileGrid::TileCount> blendedTextures;
    std::array<bool, TileGrid::TileCount> blendedReady {};
    std::array<Entity, TileGrid::TileCount> entities;
    std::array<Vector3, TileGrid::TileCount> minimums;
    std::array<Vector3, TileGrid::TileCount> maximums;
    std::array<std::array<Vector3, TerrainGeometry::ClusterCount>, TileGrid::TileCount> clusterMinimums;
    std::array<std::array<Vector3, TerrainGeometry::ClusterCount>, TileGrid::TileCount> clusterMaximums;
    std::array<std::array<std::uint8_t, TerrainGeometry::ClusterCount>, TileGrid::TileCount> levels;
    std::array<std::uint8_t, TileGrid::TileCount> selectedLevels;
    std::array<std::array<bool, TerrainGeometry::ClusterCount>, TileGrid::TileCount> visibility;
    std::array<std::uint16_t, TileGrid::TileCount> activeTiles;
    std::array<std::uint16_t, TileGrid::TileCount> activePositions {};
    std::size_t activeCount = 0;
    std::uint64_t cameraRevision = 0;
    int viewportWidth = 0;
    int viewportHeight = 0;
    bool levelsDirty = true;
};

}
