#include "Presentation/Map/terrain.hpp"

#include "Render/instance.hpp"
#include "Render/frustum.hpp"
#include "Render/Mesh/allocator.hpp"
#include "Render/Mesh/data.hpp"
#include "Render/Mesh/level.hpp"
#include "Render/renderer.hpp"
#include "Render/camera.hpp"
#include "Render/scene.hpp"
#include "World/Map/Terrain/geometry.hpp"
#include "World/Map/Tile/tile.hpp"

#include <algorithm>
#include <array>
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

    const TerrainGeometry& geometry = tile.terrain();
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
    for (const MeshVertex& vertex : geometry.vertices()) {
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

    const MeshData data(std::as_bytes(geometry.vertices()), levels, MeshIndexType::unsigned16);
    const MeshHandle mesh = allocator.allocate();
    renderer.store_mesh(mesh, data);

    Entity entity;

    try {
        entity = scene.registry().create();
        scene.registry().emplace<RenderInstance>(
            entity, RenderInstance {.mesh = mesh, .transform = {}, .worldSpace = true});
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
    this->levels[tileIndex] = UnsetLevel;
    this->visibility[tileIndex] = true;
    this->levelsDirty = true;
}

void PresentationTerrain::unload(const std::size_t x, const std::size_t y, Renderer& renderer, RenderScene& scene) {
    if (x >= TileGrid::Size || y >= TileGrid::Size) {
        return;
    }

    this->release(index(x, y), renderer, scene);
}

void PresentationTerrain::clear(Renderer& renderer, RenderScene& scene) {
    for (std::size_t tile = 0; tile < TileGrid::TileCount; ++tile) {
        this->release(tile, renderer, scene);
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
    static constexpr std::array<float, TerrainGeometry::LevelCount - 1> Thresholds {512.0F, 192.0F, 64.0F};
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
