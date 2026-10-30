#pragma once

#include "ECS/entity.hpp"
#include "Math/vector.hpp"
#include "Render/Mesh/mesh.hpp"
#include "World/Map/Tile/grid.hpp"

#include <array>
#include <cstddef>

namespace Northrend {

class MapTile;
class MeshHandleAllocator;
class Renderer;
class RenderScene;

class PresentationTerrain final {
public:
    void load(std::size_t x, std::size_t y, const MapTile& tile, MeshHandleAllocator& allocator,
        Renderer& renderer, RenderScene& scene);
    void unload(std::size_t x, std::size_t y, Renderer& renderer, RenderScene& scene);
    void clear(Renderer& renderer, RenderScene& scene);

private:
    [[nodiscard]] static constexpr std::size_t index(const std::size_t x, const std::size_t y) noexcept {
        return y * TileGrid::Size + x;
    }

    void release(std::size_t tile, Renderer& renderer, RenderScene& scene);
    void frame(RenderScene& scene);

    std::array<MeshHandle, TileGrid::TileCount> meshes;
    std::array<Entity, TileGrid::TileCount> entities;
    std::array<Vector3, TileGrid::TileCount> minimums;
    std::array<Vector3, TileGrid::TileCount> maximums;
};

}
