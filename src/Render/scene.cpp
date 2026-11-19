#include "Render/scene.hpp"

#ifdef NORTHREND_GPU_PROFILING
#include "Debug/timing.hpp"

#include <chrono>
#include <iostream>
#endif

#include <cstddef>

namespace Northrend {

RenderScene::RenderScene() :
    instanceQuery(this->state.tracked_query<const RenderInstance>()),
    visibilityQuery(this->state.tracked_query<const RenderVisibility>()),
    sectionQuery(this->state.tracked_query<const RenderSections>()) {
    this->cameraEntity = this->state.create();

    const Camera& camera = this->state.emplace<Camera>(this->cameraEntity);
    this->cachedCamera = camera;
    this->cachedView = camera.view();
    this->cachedCameraRevision = camera.revision;

    this->lightEntity = this->state.create();
    this->state.emplace<DirectionalLight>(this->lightEntity);
}

Registry& RenderScene::registry() noexcept {
    return this->state;
}

const Registry& RenderScene::registry() const noexcept {
    return this->state;
}

const Camera& RenderScene::active_camera() const {
    const Camera* camera = this->state.get<Camera>(this->cameraEntity);

    if (!camera) {
        throw std::runtime_error("Render scene has no active camera");
    }

    return *camera;
}

const DirectionalLight& RenderScene::active_light() const {
    const DirectionalLight* light = this->state.get<DirectionalLight>(this->lightEntity);

    if (!light) {
        throw std::runtime_error("Render scene has no active directional light");
    }

    return *light;
}

RenderFrame RenderScene::frame() {
    this->refresh_instances();

    const Camera& camera = this->active_camera();
    const DirectionalLight& light = this->active_light();

    if (camera.revision != this->cachedCameraRevision) {
        this->cachedCamera = camera;
        this->cachedView = camera.view();
        this->cachedCameraRevision = camera.revision;
    }

    return {
        .instances = this->instances,
        .instanceVisibility = this->instanceVisibility,
        .sections = this->sections,
        .instancesRevision = this->instancesRevision,
        .visibilityRevision = this->visibilityRevision,
        .sectionsRevision = this->sectionsRevision,
        .camera = this->cachedCamera,
        .view = this->cachedView,
        .cameraRevision = this->cachedCameraRevision,
        .light = light
    };
}

void RenderScene::refresh_instances() {
    const bool instancesChanged = this->instanceQuery.changed();
    bool visibilityChanged = this->visibilityQuery.changed();
    const bool sectionsChanged = this->sectionQuery.changed();

    if (!instancesChanged && !visibilityChanged && !sectionsChanged) {
        return;
    }

#ifdef NORTHREND_GPU_PROFILING
    static Timing snapshotTiming;
    const auto snapshotStart = std::chrono::steady_clock::now();
#endif

    if (instancesChanged) {
        this->instances.clear();
        this->instanceIndices.clear();

        const std::size_t instanceCount = static_cast<std::size_t>(this->instanceQuery.count());

        if (this->instances.capacity() < instanceCount) {
            this->instances.reserve(instanceCount);
        }

        this->instanceQuery.run([this](flecs::iter& iterator) {
            while (iterator.next()) {
                const auto field = iterator.field<const RenderInstance>(0);
                const std::size_t count = iterator.count();

                if (count == 0) {
                    continue;
                }

                for (std::size_t index = 0; index < count; ++index) {
                    const std::size_t entity = static_cast<std::size_t>(iterator.entity(index).id() & ECS_ENTITY_MASK);
                    const std::size_t destination = this->instances.size();

                    if (entity >= this->instanceIndices.size()) {
                        this->instanceIndices.resize(entity + 1, 0);
                    }

                    this->instances.push_back(field[index]);
                    this->instanceIndices[entity] = destination + 1;
                }
            }
        });

        this->instanceVisibility.assign(this->instances.size(), true);
        visibilityChanged = true;
        ++this->instancesRevision;
    }

    if (visibilityChanged) {
        if (!instancesChanged) {
            this->instanceVisibility.assign(this->instances.size(), true);
        }

        this->visibilityQuery.run([this](flecs::iter& iterator) {
            while (iterator.next()) {
                const auto field = iterator.field<const RenderVisibility>(0);

                for (std::size_t index = 0; index < iterator.count(); ++index) {
                    const std::size_t entity = static_cast<std::size_t>(iterator.entity(index).id() & ECS_ENTITY_MASK);

                    if (entity < this->instanceIndices.size() && this->instanceIndices[entity] != 0) {
                        this->instanceVisibility[this->instanceIndices[entity] - 1] = field[index].visible;
                    }
                }
            }
        });

        ++this->visibilityRevision;
    }

    if (sectionsChanged) {
        this->sections.clear();

        const std::size_t sectionCount = static_cast<std::size_t>(this->sectionQuery.count());

        if (this->sections.capacity() < sectionCount) {
            this->sections.reserve(sectionCount);
        }

        this->sectionQuery.run([this](flecs::iter& iterator) {
            while (iterator.next()) {
                const auto field = iterator.field<const RenderSections>(0);
                const std::size_t count = iterator.count();

                for (std::size_t index = 0; index < count; ++index) {
                    if (field[index].visibility != 0) {
                        this->sections.push_back(field[index]);
                    }
                }
            }
        });

        ++this->sectionsRevision;
    }

#ifdef NORTHREND_GPU_PROFILING
    const auto snapshotEnd = std::chrono::steady_clock::now();
    const double milliseconds = std::chrono::duration<double, std::milli>(snapshotEnd - snapshotStart).count();

    if (const auto summary = snapshotTiming.record(milliseconds)) {
        Timing::write(std::cout, "CPU scene snapshot", *summary);
    }
#endif
}

}
