#pragma once

#include "Math/vector.hpp"

namespace Northrend {

struct Camera;

class CameraRoute final {
public:
    CameraRoute(Vector3 center, float radius, float altitude, float speed);

    void apply(Camera& camera, double elapsedSeconds) const;
    [[nodiscard]] double duration() const noexcept;

private:
    Vector3 routeCenter;
    float routeRadius;
    float routeAltitude;
    float routeSpeed;
    double lapDuration;
};

}
