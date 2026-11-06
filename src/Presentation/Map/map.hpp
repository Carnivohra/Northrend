#pragma once

#include "Presentation/Map/terrain.hpp"
#include "World/Map/Terrain/payload.hpp"
#include "World/Map/Tile/focus.hpp"
#include "World/Map/Tile/grid.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

namespace Northrend {

struct Camera;
class MapTile;
class MaterialHandleAllocator;
class MeshHandleAllocator;
class PresentationTextureCache;
class PresentationExchange;
class Renderer;
class RenderScene;
class TileChange;

class PresentationMap final {
public:
    void apply(TileChange change, MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
        PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene);
    void focus(const Vector3& position, std::size_t radius, PresentationExchange& exchange);
    void prepare(MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
        PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene);
    void clear(MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
        PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene);
    void update(const Camera& camera, int width, int height, RenderScene& scene);

    [[nodiscard]] const MapTile* tile(std::size_t x, std::size_t y) const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;

private:
    [[nodiscard]] static constexpr std::size_t index(const std::size_t x, const std::size_t y) noexcept {
        return y * TileGrid::Size + x;
    }

    static void retain_textures(const MapTile& tile, PresentationTextureCache& textures, Renderer& renderer);
    static void release_textures(const MapTile& tile, PresentationTextureCache& textures, Renderer& renderer);

    std::array<std::shared_ptr<const MapTile>, TileGrid::TileCount> tiles;
    std::array<std::unique_ptr<TerrainPayload>, TileGrid::TileCount> payloads;
    std::array<bool, TileGrid::TileCount> pending;
    PresentationTerrain terrain;
    std::optional<TileFocus> tileFocus;
    std::uint32_t currentGeneration = 0;
};

}
