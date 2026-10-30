#pragma once

#include "Presentation/Map/terrain.hpp"
#include "World/Map/Tile/grid.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace Northrend {

class MapTile;
class MeshHandleAllocator;
class Renderer;
class RenderScene;
class TileChange;

class PresentationMap final {
public:
    void apply(TileChange change, MeshHandleAllocator& allocator, Renderer& renderer, RenderScene& scene);
    void clear(Renderer& renderer, RenderScene& scene);

    [[nodiscard]] const MapTile* tile(std::size_t x, std::size_t y) const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;

private:
    [[nodiscard]] static constexpr std::size_t index(const std::size_t x, const std::size_t y) noexcept {
        return y * TileGrid::Size + x;
    }

    std::array<std::shared_ptr<const MapTile>, TileGrid::TileCount> tiles;
    PresentationTerrain terrain;
    std::uint32_t currentGeneration = 0;
};

}
