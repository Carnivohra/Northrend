#pragma once

#include "Math/matrix.hpp"
#include "Math/vector.hpp"

#include <cstdint>
#include <memory>

namespace Northrend {

class MapObjectPlacement;
class PresentationMapObjectModel;

class PresentationMapObjectInstance final {
public:
    PresentationMapObjectInstance(
        std::shared_ptr<const PresentationMapObjectModel> model, const MapObjectPlacement& placement) noexcept;

    [[nodiscard]] const std::shared_ptr<const PresentationMapObjectModel>& model() const noexcept;
    [[nodiscard]] const Matrix4& transform() const noexcept;
    [[nodiscard]] const Vector3& minimum() const noexcept;
    [[nodiscard]] const Vector3& maximum() const noexcept;
    [[nodiscard]] std::uint32_t id() const noexcept;

private:
    [[nodiscard]] static Vector3 position(const Vector3& source) noexcept;
    [[nodiscard]] static Vector3 minimum(const MapObjectPlacement& placement) noexcept;
    [[nodiscard]] static Vector3 maximum(const MapObjectPlacement& placement) noexcept;
    [[nodiscard]] static Matrix4 transform(const MapObjectPlacement& placement) noexcept;

    std::shared_ptr<const PresentationMapObjectModel> objectModel;
    Matrix4 objectTransform;
    Vector3 minimumBounds;
    Vector3 maximumBounds;
    std::uint32_t placementId;
};

}
