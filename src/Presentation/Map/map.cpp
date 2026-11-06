#include "Presentation/Map/map.hpp"

#include "Presentation/exchange.hpp"
#include "Presentation/Texture/cache.hpp"
#include "Render/renderer.hpp"
#include "World/Map/Terrain/material.hpp"
#include "World/Map/Tile/change.hpp"
#include "World/Map/Tile/tile.hpp"

#include <utility>

namespace Northrend {

void PresentationMap::apply(TileChange change, MeshHandleAllocator& meshAllocator,
    MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures, Renderer& renderer,
    RenderScene& scene) {
    if (change.kind() == TileChange::Kind::reset) {
        this->terrain.clear(meshAllocator, materialAllocator, textures, renderer, scene);
        this->tiles.fill(nullptr);

        for (std::unique_ptr<TerrainPayload>& payload : this->payloads) {
            payload.reset();
        }

        this->pending.fill(false);
        this->tileFocus.reset();
        textures.clear(renderer);
        this->currentGeneration = change.generation();
        return;
    }

    if (change.generation() != this->currentGeneration || change.x() >= TileGrid::Size ||
        change.y() >= TileGrid::Size) {
        return;
    }

    const std::size_t x = change.x();
    const std::size_t y = change.y();
    const std::size_t tile = index(x, y);

    if (change.kind() == TileChange::Kind::unloaded) {
        if (this->tiles[tile]) {
            this->terrain.unload(x, y, meshAllocator, materialAllocator, textures, renderer, scene);
            release_textures(*this->tiles[tile], textures, renderer);
        }

        this->tiles[tile].reset();
        this->payloads[tile].reset();
        this->pending[tile] = false;
        return;
    }

    std::shared_ptr<const MapTile> mapTile = std::move(change).tile();
    std::unique_ptr<TerrainPayload> payload = std::move(change).payload();

    if (!mapTile || !payload) {
        return;
    }

    if (this->tiles[tile]) {
        this->terrain.unload(x, y, meshAllocator, materialAllocator, textures, renderer, scene);
        release_textures(*this->tiles[tile], textures, renderer);
    }

    retain_textures(*mapTile, textures, renderer);
    this->tiles[tile] = std::move(mapTile);
    this->payloads[tile] = std::move(payload);
    this->pending[tile] = true;
}

void PresentationMap::focus(const Vector3& position, const std::size_t radius, PresentationExchange& exchange) {
    const TileFocus focus = TileFocus::from_world(position.x, position.y, radius);

    if (this->tileFocus && *this->tileFocus == focus) {
        return;
    }

    if (exchange.publish(focus)) {
        this->tileFocus = focus;
    }
}

void PresentationMap::prepare(MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
    PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene) {
    static constexpr std::size_t TilesPerFrame = 4;
    std::size_t prepared = 0;

    for (std::size_t tile = 0; tile < this->pending.size() && prepared < TilesPerFrame; ++tile) {
        if (!this->pending[tile] || !this->tiles[tile] || !this->payloads[tile]) {
            continue;
        }

        const TerrainMaterial& material = this->tiles[tile]->material();
        bool ready = true;

        for (std::size_t texture = 0; texture < material.texture_count(); ++texture) {
            if (!textures.ready(material.texture(texture))) {
                ready = false;
                break;
            }
        }

        if (!ready) {
            continue;
        }

        const std::size_t x = tile % TileGrid::Size;
        const std::size_t y = tile / TileGrid::Size;
        this->terrain.load(x, y, *this->tiles[tile], *this->payloads[tile], meshAllocator, materialAllocator, textures,
            renderer, scene);
        this->payloads[tile].reset();
        this->pending[tile] = false;
        ++prepared;
    }
}

void PresentationMap::clear(MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
    PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene) {
    this->terrain.clear(meshAllocator, materialAllocator, textures, renderer, scene);
    this->tiles.fill(nullptr);

    for (std::unique_ptr<TerrainPayload>& payload : this->payloads) {
        payload.reset();
    }

    this->pending.fill(false);
    this->tileFocus.reset();
    textures.clear(renderer);
    this->currentGeneration = 0;
}

void PresentationMap::update(const Camera& camera, const int width, const int height, RenderScene& scene) {
    this->terrain.update(camera, width, height, scene);
}

const MapTile* PresentationMap::tile(const std::size_t x, const std::size_t y) const noexcept {
    return x < TileGrid::Size && y < TileGrid::Size ? this->tiles[index(x, y)].get() : nullptr;
}

std::uint32_t PresentationMap::generation() const noexcept {
    return this->currentGeneration;
}

void PresentationMap::retain_textures(const MapTile& tile, PresentationTextureCache& textures, Renderer& renderer) {
    const TerrainMaterial& material = tile.material();
    std::size_t retained = 0;

    try {
        for (; retained < material.texture_count(); ++retained) {
            textures.retain(material.texture(retained));
        }
    } catch (...) {
        while (retained != 0) {
            textures.release(material.texture(--retained), renderer);
        }

        throw;
    }
}

void PresentationMap::release_textures(const MapTile& tile, PresentationTextureCache& textures, Renderer& renderer) {
    const TerrainMaterial& material = tile.material();

    for (std::size_t texture = 0; texture < material.texture_count(); ++texture) {
        textures.release(material.texture(texture), renderer);
    }
}

}
