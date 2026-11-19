#include "Presentation/Map/Object/objects.hpp"

#include "Presentation/Map/Object/model.hpp"
#include "Presentation/Texture/cache.hpp"
#include "Render/camera.hpp"
#include "Render/frustum.hpp"
#include "Render/instance.hpp"
#include "Render/Material/allocator.hpp"
#include "Render/Mesh/allocator.hpp"
#include "Render/renderer.hpp"
#include "Render/scene.hpp"
#include "Render/visibility.hpp"
#include "World/Map/Object/asset.hpp"
#include "World/Map/Object/placement.hpp"
#include "World/Map/Tile/tile.hpp"

#ifdef NORTHREND_GPU_PROFILING
#include "Debug/interval.hpp"
#include "Debug/timing.hpp"

#include <chrono>
#include <iostream>
#endif

#include <algorithm>
#include <stdexcept>

namespace Northrend {

void PresentationMapObjects::load(const std::size_t x, const std::size_t y, const MapTile& tile) {
    if (x >= TileGrid::Size || y >= TileGrid::Size) {
        throw std::out_of_range("Presentation map object coordinate is outside the map");
    }

    PresentationMapObjectTile& destination = this->tiles[index(x, y)];
    destination.reset(tile.objects().size());
    this->visibilityDirty = true;
}

void PresentationMapObjects::unload(const std::size_t x, const std::size_t y, MeshHandleAllocator& meshAllocator,
    MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures, Renderer& renderer,
    RenderScene& scene) {
    if (x >= TileGrid::Size || y >= TileGrid::Size) {
        return;
    }

    PresentationMapObjectTile& tile = this->tiles[index(x, y)];

    for (const Entity entity : tile.entities) {
        scene.registry().destroy(entity);
    }

    for (const PresentationMapObjectInstance& instance : tile.instances) {
        this->release(instance.model(), meshAllocator, materialAllocator, textures, renderer);
    }

    tile.reset(0);
    this->visibilityDirty = true;
    this->collect();
}

void PresentationMapObjects::clear(MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator,
    PresentationTextureCache& textures, Renderer& renderer, RenderScene& scene) {
    for (std::size_t tile = 0; tile < this->tiles.size(); ++tile) {
        const std::size_t x = tile % TileGrid::Size;
        const std::size_t y = tile / TileGrid::Size;
        this->unload(x, y, meshAllocator, materialAllocator, textures, renderer, scene);
    }

    this->resources.clear();
    this->models.clear();
    this->cameraRevision = 0;
    this->visibleInstances = 0;
    this->visibleSections = 0;
    this->viewportWidth = 0;
    this->viewportHeight = 0;
    this->visibilityDirty = true;
}

bool PresentationMapObjects::prepare(const std::size_t x, const std::size_t y, const MapTile& tile, std::size_t& budget,
    MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures,
    Renderer& renderer, RenderScene& scene) {
    if (x >= TileGrid::Size || y >= TileGrid::Size) {
        return true;
    }

    PresentationMapObjectTile& destination = this->tiles[index(x, y)];
    const std::span<const MapObjectPlacement> placements = tile.objects();

    if (destination.completed.size() != placements.size()) {
        this->load(x, y, tile);
    }

    for (std::size_t index = 0; index < placements.size(); ++index) {
        if (destination.completed[index] || !placements[index].object.ready()) {
            continue;
        }

        try {
            const std::shared_ptr<const MapObjectAsset> object = placements[index].object.data();

            if (!object) {
                destination.completed[index] = true;
                --destination.remaining;
                continue;
            }

            const std::shared_ptr<const PresentationMapObjectModel> model = this->model(object, budget);

            if (!model) {
                continue;
            }

            const MapObjectPlacement& placement = placements[index];
            static_cast<void>(this->retain(model, textures, renderer));
            destination.instances.emplace_back(model, placement);
            destination.firstEntities.push_back(0);
            destination.entityCounts.push_back(0);
            destination.rendered.push_back(false);
        } catch (...) {
        }

        destination.completed[index] = true;
        --destination.remaining;
    }

    for (std::size_t instance = 0; instance < destination.instances.size(); ++instance) {
        if (!destination.rendered[instance]) {
            this->render(destination, instance, budget, meshAllocator, materialAllocator, textures, renderer, scene);
        }
    }

    const bool ready = destination.remaining == 0 &&
        std::ranges::all_of(destination.rendered, [](const bool rendered) { return rendered; });

    return ready;
}

void PresentationMapObjects::update(const Camera& camera, const int width, const int height, RenderScene& scene) {
    if (width <= 0 || height <= 0 ||
        (!this->visibilityDirty && camera.revision == this->cameraRevision && width == this->viewportWidth &&
            height == this->viewportHeight)) {
        return;
    }

#ifdef NORTHREND_GPU_PROFILING
    static Timing cullingTiming;
    const auto cullingStart = std::chrono::steady_clock::now();
#endif

    const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    const Frustum frustum(camera.projection(aspectRatio) * camera.view());
    std::size_t visibleInstances = 0;
    std::size_t visibleSections = 0;
    std::vector<bool> groupVisibility;

    for (PresentationMapObjectTile& tile : this->tiles) {
        for (std::size_t index = 0; index < tile.instances.size(); ++index) {
            const std::size_t entityCount = tile.entityCounts[index];

            if (entityCount == 0) {
                continue;
            }

            const PresentationMapObjectInstance& instance = tile.instances[index];
            const bool instanceVisible = frustum.intersects(instance.minimum(), instance.maximum());

            visibleInstances += instanceVisible;
            groupVisibility.assign(instance.group_count(), false);

            if (instanceVisible) {
                for (std::size_t group = 0; group < groupVisibility.size(); ++group) {
                    groupVisibility[group] =
                        frustum.intersects(instance.group_minimum(group), instance.group_maximum(group));
                }
            }

            const std::size_t firstEntity = tile.firstEntities[index];

            for (std::size_t entity = firstEntity; entity < firstEntity + entityCount; ++entity) {
                const std::uint32_t group = tile.entityGroups[entity];
                const bool visible = group < groupVisibility.size() && groupVisibility[group];
                visibleSections += visible;

                if (visible == tile.entityVisibility[entity]) {
                    continue;
                }

                const bool modified = scene.registry().modify<RenderVisibility>(
                    tile.entities[entity], [visible](RenderVisibility& visibility) { visibility.visible = visible; });

                if (!modified) {
                    throw std::runtime_error("WMO render instance is missing");
                }

                tile.entityVisibility[entity] = visible;
            }
        }
    }

    this->cameraRevision = camera.revision;
    this->visibleInstances = visibleInstances;
    this->visibleSections = visibleSections;
    this->viewportWidth = width;
    this->viewportHeight = height;
    this->visibilityDirty = false;

#ifdef NORTHREND_GPU_PROFILING
    const auto cullingEnd = std::chrono::steady_clock::now();
    const double milliseconds = std::chrono::duration<double, std::milli>(cullingEnd - cullingStart).count();

    if (const auto summary = cullingTiming.record(milliseconds)) {
        Timing::write(std::cout, "CPU WMO culling", *summary);
    }
#endif
}

std::shared_ptr<const PresentationMapObjectModel> PresentationMapObjects::model(
    const std::shared_ptr<const MapObjectAsset>& object, std::size_t& budget) {
    const auto position = this->models.find(object.get());

    if (position != this->models.end()) {
        if (std::shared_ptr<const PresentationMapObjectModel> existing = position->second.lock()) {
            return existing;
        }

        this->models.erase(position);
    }

    if (budget == 0) {
        return nullptr;
    }

    auto result = std::make_shared<PresentationMapObjectModel>(*object);
    this->models.emplace(object.get(), result);
    --budget;
    return result;
}

PresentationMapObjectResource& PresentationMapObjects::retain(
    const std::shared_ptr<const PresentationMapObjectModel>& model, PresentationTextureCache& textures,
    Renderer& renderer) {
    const auto existing = this->resources.find(model.get());

    if (existing != this->resources.end()) {
        existing->second->add_reference();
        return *existing->second;
    }

    const auto [position, inserted] = this->resources.try_emplace(model.get());

    if (!inserted) {
        throw std::runtime_error("Presentation map object resource already exists");
    }

    try {
        auto resource = std::make_unique<PresentationMapObjectResource>(model);
        resource->retain(textures, renderer);
        resource->add_reference();
        position->second = std::move(resource);
    } catch (...) {
        this->resources.erase(position);
        throw;
    }

    return *position->second;
}

void PresentationMapObjects::release(const std::shared_ptr<const PresentationMapObjectModel>& model,
    MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures,
    Renderer& renderer) {
    const auto position = this->resources.find(model.get());

    if (position == this->resources.end() || !position->second->remove_reference()) {
        return;
    }

    position->second->release(meshAllocator, materialAllocator, textures, renderer);
    this->resources.erase(position);
}

void PresentationMapObjects::render(PresentationMapObjectTile& tile, const std::size_t instanceIndex,
    std::size_t& budget,
    MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures,
    Renderer& renderer, RenderScene& scene) {
    const PresentationMapObjectInstance& instance = tile.instances[instanceIndex];
    const auto position = this->resources.find(instance.model().get());

    if (position == this->resources.end()) {
        return;
    }

    PresentationMapObjectResource& resource = *position->second;

    if (!resource.stored()) {
        if (budget == 0 || !resource.ready(textures)) {
            return;
        }

        resource.store(meshAllocator, materialAllocator, textures, renderer);
        --budget;
    }

    if (!resource.mesh()) {
        tile.rendered[instanceIndex] = true;
        return;
    }

    const std::span<const PresentationMapObjectSection> sections = instance.model()->geometry().sections();
    const std::size_t firstEntity = tile.entities.size();
    tile.entities.reserve(firstEntity + sections.size());
    tile.entityGroups.reserve(firstEntity + sections.size());
    tile.entityVisibility.reserve(firstEntity + sections.size());

    try {
        for (std::size_t section = 0; section < sections.size(); ++section) {
            const MaterialHandle material = resource.material(sections[section].material);

            if (!material) {
                continue;
            }

            const Entity entity = scene.registry().create();

            try {
                scene.registry().emplace<RenderInstance>(entity,
                    RenderInstance {
                        .mesh = resource.mesh(),
                        .material = material,
                        .transform = instance.transform(),
                        .section = static_cast<std::uint16_t>(section)
                    });
                scene.registry().emplace<RenderVisibility>(entity, RenderVisibility {.visible = false});
            } catch (...) {
                scene.registry().destroy(entity);
                throw;
            }

            tile.entities.push_back(entity);
            tile.entityGroups.push_back(sections[section].group);
            tile.entityVisibility.push_back(false);
        }
    } catch (...) {
        while (tile.entities.size() > firstEntity) {
            scene.registry().destroy(tile.entities.back());
            tile.entities.pop_back();
            tile.entityGroups.pop_back();
            tile.entityVisibility.pop_back();
        }

        throw;
    }

    tile.firstEntities[instanceIndex] = firstEntity;
    tile.entityCounts[instanceIndex] = tile.entities.size() - firstEntity;
    tile.rendered[instanceIndex] = true;
    this->visibilityDirty = true;
}

void PresentationMapObjects::collect() noexcept {
    for (auto position = this->models.begin(); position != this->models.end();) {
        if (position->second.expired()) {
            position = this->models.erase(position);
            continue;
        }

        ++position;
    }
}

#ifdef NORTHREND_GPU_PROFILING
void PresentationMapObjects::profile() const {
    static Interval interval;

    if (!interval.elapsed()) {
        return;
    }

    std::size_t instances = 0;
    std::size_t sections = 0;
    std::size_t pending = 0;

    for (const PresentationMapObjectTile& tile : this->tiles) {
        instances += tile.instances.size();
        sections += tile.entities.size();
        pending += tile.remaining;
    }

    std::cout << "WMO: instances=" << instances << " visible_instances=" << this->visibleInstances
              << " sections=" << sections << " visible_sections=" << this->visibleSections
              << " resources=" << this->resources.size() << " pending=" << pending << '\n';
}
#endif

}
