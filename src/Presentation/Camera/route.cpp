#include "Presentation/Camera/route.hpp"

#include "Render/camera.hpp"

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace Northrend {

CameraRoute::CameraRoute(const Vector3 center, const float radius, const float altitude, const float speed) :
    routeCenter(center),
    routeRadius(radius),
    routeAltitude(altitude),
    routeSpeed(speed),
    lapDuration(2.0 * std::numbers::pi * static_cast<double>(radius) / static_cast<double>(speed)) {
    if (!std::isfinite(this->routeCenter.x) || !std::isfinite(this->routeCenter.y) ||
        !std::isfinite(this->routeCenter.z) || !std::isfinite(this->routeRadius) ||
        !std::isfinite(this->routeAltitude) || !std::isfinite(this->routeSpeed) || this->routeRadius <= 0.0F ||
        this->routeSpeed <= 0.0F) {
        throw std::invalid_argument("Camera route parameters are invalid");
    }
}

void CameraRoute::apply(Camera& camera, const double elapsedSeconds) const {
    const double elapsed =
        std::isfinite(elapsedSeconds) && elapsedSeconds > 0.0 ? std::fmod(elapsedSeconds, this->lapDuration) : 0.0;
    const double angle = elapsed * static_cast<double>(this->routeSpeed) / static_cast<double>(this->routeRadius);
    const float cosine = static_cast<float>(std::cos(angle));
    const float sine = static_cast<float>(std::sin(angle));
    camera.position = Vector3 {
        .x = this->routeCenter.x + this->routeRadius * cosine,
        .y = this->routeCenter.y + this->routeRadius * sine,
        .z = this->routeCenter.z + this->routeAltitude
    };
    camera.look_at(this->routeCenter);
}

double CameraRoute::duration() const noexcept {
    return this->lapDuration;
}

}
