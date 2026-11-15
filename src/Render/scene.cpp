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
        .sections = this->sections,
        .instancesRevision = this->instancesRevision,
        .camera = this->cachedCamera,
        .view = this->cachedView,
        .cameraRevision = this->cachedCameraRevision,
        .light = light
    };
}

void RenderScene::refresh_instances() {
    const bool instancesChanged = this->instanceQuery.changed();
    const bool sectionsChanged = this->sectionQuery.changed();

    if (!instancesChanged && !sectionsChanged) {
        return;
    }

#ifdef NORTHREND_GPU_PROFILING
    static Timing snapshotTiming;
    const auto snapshotStart = std::chrono::steady_clock::now();
#endif

    if (instancesChanged) {
        this->instances.clear();

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
                    if (field[index].visible) {
                        this->instances.push_back(field[index]);
                    }
                }
            }
        });
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
    }

    ++this->instancesRevision;

#ifdef NORTHREND_GPU_PROFILING
    const auto snapshotEnd = std::chrono::steady_clock::now();
    const double milliseconds = std::chrono::duration<double, std::milli>(snapshotEnd - snapshotStart).count();

    if (const auto summary = snapshotTiming.record(milliseconds)) {
        Timing::write(std::cout, "CPU scene snapshot", *summary);
    }
#endif
}

}
