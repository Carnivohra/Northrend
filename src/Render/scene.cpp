#include "Render/scene.hpp"

#include <cstddef>

namespace Northrend {

RenderScene::RenderScene() : instanceQuery(this->state.tracked_query<const RenderInstance>()) {
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
        .instancesRevision = this->instancesRevision,
        .camera = this->cachedCamera,
        .view = this->cachedView,
        .cameraRevision = this->cachedCameraRevision,
        .light = light
    };
}

void RenderScene::refresh_instances() {
    if (!this->instanceQuery.changed()) {
        return;
    }

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

    ++this->instancesRevision;
}

}
