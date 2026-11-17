#pragma once

#include "Presentation/Map/Object/geometry.hpp"
#include "Presentation/Map/Object/material.hpp"

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace Northrend {

class MapObjectAsset;

class PresentationMapObjectModel final {
public:
    explicit PresentationMapObjectModel(const MapObjectAsset& object);

    [[nodiscard]] const PresentationMapObjectGeometry& geometry() const noexcept;
    [[nodiscard]] std::span<const PresentationMapObjectMaterial> materials() const noexcept;

private:
    [[nodiscard]] static std::optional<AssetId> texture(std::string_view path);

    PresentationMapObjectGeometry modelGeometry;
    std::vector<PresentationMapObjectMaterial> modelMaterials;
};

}
