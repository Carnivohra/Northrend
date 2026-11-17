#include "Presentation/Map/objects.hpp"

#include "Presentation/Map/Object/model.hpp"
#include "Presentation/Texture/cache.hpp"
#include "Render/instance.hpp"
#include "Render/Material/allocator.hpp"
#include "Render/Mesh/allocator.hpp"
#include "Render/renderer.hpp"
#include "Render/scene.hpp"
#include "World/Map/Object/asset.hpp"
#include "World/Map/Object/placement.hpp"
#include "World/Map/Tile/tile.hpp"

#include <algorithm>
#include <stdexcept>

namespace Northrend {

void PresentationMapObjects::load(const std::size_t x, const std::size_t y, const MapTile& tile) {
    if (x >= TileGrid::Size || y >= TileGrid::Size) {
        throw std::out_of_range("Presentation map object coordinate is outside the map");
    }

    Tile& destination = this->tiles[index(x, y)];
    destination.instances.clear();
    destination.entities.clear();
    destination.rendered.clear();
    destination.rendered.reserve(tile.objects().size());
    destination.completed.assign(tile.objects().size(), false);
    destination.remaining = tile.objects().size();
    destination.instances.reserve(tile.objects().size());
}

void PresentationMapObjects::unload(const std::size_t x, const std::size_t y, MeshHandleAllocator& meshAllocator,
    MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures, Renderer& renderer,
    RenderScene& scene) {
    if (x >= TileGrid::Size || y >= TileGrid::Size) {
        return;
    }

    Tile& tile = this->tiles[index(x, y)];

    for (const Entity entity : tile.entities) {
        scene.registry().destroy(entity);
    }

    for (const PresentationMapObjectInstance& instance : tile.instances) {
        this->release(instance.model(), meshAllocator, materialAllocator, textures, renderer);
    }

    tile.instances.clear();
    tile.entities.clear();
    tile.rendered.clear();
    tile.completed.clear();
    tile.remaining = 0;
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
}

bool PresentationMapObjects::prepare(const std::size_t x, const std::size_t y, const MapTile& tile, std::size_t& budget,
    MeshHandleAllocator& meshAllocator, MaterialHandleAllocator& materialAllocator, PresentationTextureCache& textures,
    Renderer& renderer, RenderScene& scene) {
    if (x >= TileGrid::Size || y >= TileGrid::Size) {
        return true;
    }

    Tile& destination = this->tiles[index(x, y)];
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

    return destination.remaining == 0 &&
        std::ranges::all_of(destination.rendered, [](const bool rendered) { return rendered; });
}

std::span<const PresentationMapObjectInstance> PresentationMapObjects::instances(
    const std::size_t x, const std::size_t y) const noexcept {
    if (x >= TileGrid::Size || y >= TileGrid::Size) {
        return {};
    }

    return this->tiles[index(x, y)].instances;
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

void PresentationMapObjects::render(Tile& tile, const std::size_t instanceIndex, std::size_t& budget,
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

    try {
        for (std::size_t section = 0; section < sections.size(); ++section) {
            const MaterialHandle material = resource.material(sections[section].material);

            if (!material) {
                continue;
            }

            const Entity entity = scene.registry().create();
            scene.registry().emplace<RenderInstance>(entity,
                RenderInstance {
                    .mesh = resource.mesh(),
                    .material = material,
                    .transform = instance.transform(),
                    .section = static_cast<std::uint16_t>(section)
                });
            tile.entities.push_back(entity);
        }
    } catch (...) {
        while (tile.entities.size() > firstEntity) {
            scene.registry().destroy(tile.entities.back());
            tile.entities.pop_back();
        }

        throw;
    }

    tile.rendered[instanceIndex] = true;
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

}
