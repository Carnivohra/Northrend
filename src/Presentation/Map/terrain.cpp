#include "Presentation/Map/terrain.hpp"

#include "Render/instance.hpp"
#include "Render/Mesh/allocator.hpp"
#include "Render/Mesh/data.hpp"
#include "Render/renderer.hpp"
#include "Render/scene.hpp"
#include "World/Map/Terrain/geometry.hpp"
#include "World/Map/Terrain/tile.hpp"
#include "World/Map/Tile/tile.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>

namespace Northrend {

void PresentationTerrain::load(const std::size_t x, const std::size_t y, const MapTile& tile,
    MeshHandleAllocator& allocator, Renderer& renderer, RenderScene& scene) {
    if (x >= TileGrid::Size || y >= TileGrid::Size) {
        throw std::out_of_range("Presentation terrain coordinate is outside the map");
    }

    const std::size_t tileIndex = index(x, y);
    this->release(tileIndex, renderer, scene);

    const TerrainGeometry& geometry = tile.terrain().geometry();
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
    for (const Vector3& position : geometry.vertices()) {
        minimum.x = std::min(minimum.x, position.x);
        minimum.y = std::min(minimum.y, position.y);
        minimum.z = std::min(minimum.z, position.z);
        maximum.x = std::max(maximum.x, position.x);
        maximum.y = std::max(maximum.y, position.y);
        maximum.z = std::max(maximum.z, position.z);
    }

    const MeshData data(
        std::as_bytes(geometry.vertices()),
        std::as_bytes(geometry.indices()),
        static_cast<std::uint32_t>(geometry.indices().size()),
        MeshIndexType::unsigned16);
    const MeshHandle mesh = allocator.allocate();
    renderer.store_mesh(mesh, data);

    Entity entity;

    try {
        entity = scene.registry().create();
        scene.registry().emplace<RenderInstance>(entity, RenderInstance {.mesh = mesh, .transform = {}});
    } catch (...) {
        if (entity) {
            scene.registry().destroy(entity);
        }

        renderer.remove_mesh(mesh);
        throw;
    }

    this->meshes[tileIndex] = mesh;
    this->entities[tileIndex] = entity;
    this->minimums[tileIndex] = minimum;
    this->maximums[tileIndex] = maximum;
    this->frame(scene);
}

void PresentationTerrain::unload(
    const std::size_t x, const std::size_t y, Renderer& renderer, RenderScene& scene) {
    if (x >= TileGrid::Size || y >= TileGrid::Size) {
        return;
    }

    this->release(index(x, y), renderer, scene);
    this->frame(scene);
}

void PresentationTerrain::clear(Renderer& renderer, RenderScene& scene) {
    for (std::size_t tile = 0; tile < TileGrid::TileCount; ++tile) {
        this->release(tile, renderer, scene);
    }
}

void PresentationTerrain::release(const std::size_t tile, Renderer& renderer, RenderScene& scene) {
    const Entity entity = this->entities[tile];
    const MeshHandle mesh = this->meshes[tile];

    if (entity) {
        scene.registry().destroy(entity);
        this->entities[tile] = {};
    }

    if (mesh) {
        renderer.remove_mesh(mesh);
        this->meshes[tile] = {};
    }
}

void PresentationTerrain::frame(RenderScene& scene) {
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
    bool available = false;

    for (std::size_t tile = 0; tile < TileGrid::TileCount; ++tile) {
        if (!this->meshes[tile]) {
            continue;
        }

        minimum.x = std::min(minimum.x, this->minimums[tile].x);
        minimum.y = std::min(minimum.y, this->minimums[tile].y);
        minimum.z = std::min(minimum.z, this->minimums[tile].z);
        maximum.x = std::max(maximum.x, this->maximums[tile].x);
        maximum.y = std::max(maximum.y, this->maximums[tile].y);
        maximum.z = std::max(maximum.z, this->maximums[tile].z);
        available = true;
    }

    if (!available) {
        return;
    }

    const Vector3 center {
        .x = (minimum.x + maximum.x) * 0.5F,
        .y = (minimum.y + maximum.y) * 0.5F,
        .z = (minimum.z + maximum.z) * 0.5F
    };
    const float width = maximum.x - minimum.x;
    const float depth = maximum.y - minimum.y;
    const float height = maximum.z - minimum.z;
    const float radius = std::max(1.0F, std::sqrt(width * width + depth * depth + height * height) * 0.5F);

    scene.modify_active_camera([center, radius](Camera& camera) {
        constexpr Vector3 Direction {.x = 0.45F, .y = 0.45F, .z = 0.771362F};
        const float distance = radius / std::sin(camera.verticalFieldOfView * 0.5F) * 1.1F;

        camera.position = Vector3 {
            .x = center.x + Direction.x * distance,
            .y = center.y + Direction.y * distance,
            .z = center.z + Direction.z * distance
        };
        camera.nearPlane = std::max(1.0F, distance - radius * 1.25F);
        camera.farPlane = distance + radius * 1.25F;
        camera.look_at(center);
    });
}

}
