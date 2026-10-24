#include "Presentation/presentation.hpp"

#include <cstddef>
#include <stdexcept>

namespace Northrend {

Presentation::Presentation()
    : instanceQuery(this->state.tracked_query<const RenderInstance>()) {
    this->cameraEntity = this->state.create();

    const Camera& camera =
        this->state.emplace<Camera>(this->cameraEntity);
    this->cachedCamera = extract_camera(camera);
    this->cachedCameraRevision = camera.revision;
}

void Presentation::synchronize(const RenderSnapshot& snapshot) noexcept {
    if (snapshot.sequence != this->latestSnapshot.sequence) {
        this->latestSnapshot = snapshot;
    }
}

Registry& Presentation::registry() noexcept {
    return this->state;
}

const Registry& Presentation::registry() const noexcept {
    return this->state;
}

Camera& Presentation::active_camera() {
    Camera* camera = this->state.get<Camera>(this->cameraEntity);

    if (!camera) {
        throw std::runtime_error("Presentation has no active camera");
    }

    return *camera;
}

RenderFrame Presentation::frame() {
    this->refresh_instances();

    const Camera& camera = this->active_camera();

    if (camera.revision != this->cachedCameraRevision) {
        this->cachedCamera = extract_camera(camera);
        this->cachedCameraRevision = camera.revision;
    }

    return {
        .snapshot = this->latestSnapshot,
        .instances = this->instances,
        .camera = this->cachedCamera,
        .cameraRevision = this->cachedCameraRevision
    };
}

void Presentation::refresh_instances() {
    if (!this->instanceQuery.changed()) {
        return;
    }

    this->instances.clear();

    const std::size_t instanceCount =
        static_cast<std::size_t>(this->instanceQuery.count());

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

            const RenderInstance* begin = &field[0];
            this->instances.insert(
                this->instances.end(),
                begin,
                begin + count);
        }
    });
}

}
