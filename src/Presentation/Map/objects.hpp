#pragma once

#include "ECS/entity.hpp"
#include "Presentation/Map/Object/instance.hpp"
#include "Presentation/Map/Object/resource.hpp"
#include "World/Map/Tile/grid.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace Northrend {

class MapObjectAsset;
class MapTile;
class MaterialHandleAllocator;
class MeshHandleAllocator;
class PresentationMapObjectModel;
class PresentationTextureCache;
class Renderer;
class RenderScene;

class PresentationMapObjects final {
public:
    void load(std::size_t x, std::size_t y, const MapTile& tile);
    void unload(std::size_t x, std::size_t y, MeshHandleAllocator& meshAllocator,
        MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures, Renderer& renderer,
        RenderScene& scene);
    void clear(MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
        PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene);
    [[nodiscard]] bool prepare(std::size_t x, std::size_t y, const MapTile& tile, std::size_t& budget,
        MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
        PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene);
    [[nodiscard]] std::span<const PresentationMapObjectInstance> instances(std::size_t x, std::size_t y) const noexcept;

private:
    struct Tile final {
        std::vector<PresentationMapObjectInstance> instances;
        std::vector<Entity> entities;
        std::vector<bool> rendered;
        std::vector<bool> completed;
        std::size_t remaining = 0;
    };

    [[nodiscard]] static constexpr std::size_t index(const std::size_t x, const std::size_t y) noexcept {
        return y * TileGrid::Size + x;
    }

    [[nodiscard]] std::shared_ptr<const PresentationMapObjectModel> model(
        const std::shared_ptr<const MapObjectAsset>& object, std::size_t& budget);
    [[nodiscard]] PresentationMapObjectResource& retain(const std::shared_ptr<const PresentationMapObjectModel>& model,
        PresentationTextureCache& textures, Renderer& renderer);
    void release(const std::shared_ptr<const PresentationMapObjectModel>& model, MeshHandleAllocator& meshAllocator,
        MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures, Renderer& renderer);
    void render(Tile& tile, std::size_t instance, std::size_t& budget, MeshHandleAllocator& meshAllocator,
        MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures, Renderer& renderer,
        RenderScene& scene);
    void collect() noexcept;

    std::array<Tile, TileGrid::TileCount> tiles;
    std::unordered_map<const MapObjectAsset*, std::weak_ptr<const PresentationMapObjectModel>> models;
    std::unordered_map<const PresentationMapObjectModel*, std::unique_ptr<PresentationMapObjectResource>> resources;
};

}
