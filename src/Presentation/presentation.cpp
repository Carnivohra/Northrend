#include "Presentation/presentation.hpp"

#include <stdexcept>

namespace Northrend {

Presentation::Presentation() {
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
    const Camera& camera = this->active_camera();

    if (camera.revision != this->cachedCameraRevision) {
        this->cachedCamera = extract_camera(camera);
        this->cachedCameraRevision = camera.revision;
    }

    return {
        .snapshot = this->latestSnapshot,
        .camera = this->cachedCamera,
        .cameraRevision = this->cachedCameraRevision
    };
}

}
