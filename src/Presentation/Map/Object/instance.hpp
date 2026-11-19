#pragma once

#include "Math/matrix.hpp"
#include "Math/vector.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace Northrend {

class MapObjectPlacement;
class PresentationMapObjectModel;

class PresentationMapObjectInstance final {
public:
    PresentationMapObjectInstance(
        std::shared_ptr<const PresentationMapObjectModel> model, const MapObjectPlacement& placement);

    [[nodiscard]] const std::shared_ptr<const PresentationMapObjectModel>& model() const noexcept;
    [[nodiscard]] const Matrix4& transform() const noexcept;
    [[nodiscard]] const Vector3& minimum() const noexcept;
    [[nodiscard]] const Vector3& maximum() const noexcept;
    [[nodiscard]] std::size_t group_count() const noexcept;
    [[nodiscard]] const Vector3& group_minimum(std::size_t group) const noexcept;
    [[nodiscard]] const Vector3& group_maximum(std::size_t group) const noexcept;
private:
    [[nodiscard]] static Vector3 position(const Vector3& source) noexcept;
    [[nodiscard]] static Vector3 minimum(const MapObjectPlacement& placement) noexcept;
    [[nodiscard]] static Vector3 maximum(const MapObjectPlacement& placement) noexcept;
    [[nodiscard]] static Matrix4 transform(const MapObjectPlacement& placement) noexcept;
    [[nodiscard]] static std::pair<Vector3, Vector3> transform_bounds(
        const Matrix4& transform, const Vector3& minimum, const Vector3& maximum) noexcept;

    std::shared_ptr<const PresentationMapObjectModel> objectModel;
    Matrix4 objectTransform;
    Vector3 minimumBounds;
    Vector3 maximumBounds;
    std::vector<Vector3> groupMinimums;
    std::vector<Vector3> groupMaximums;
};

}
