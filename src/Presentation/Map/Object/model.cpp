#include "Presentation/Map/Object/model.hpp"

#include "Asset/Format/Wmo/wmo.hpp"
#include "World/Map/Object/asset.hpp"

#include <optional>

namespace Northrend {

std::optional<AssetId> PresentationMapObjectModel::texture(const std::string_view path) {
    return path.empty() ? std::nullopt : std::optional<AssetId>(std::in_place, path);
}

PresentationMapObjectModel::PresentationMapObjectModel(const MapObjectAsset& object) : modelGeometry(object) {
    const Wmo& root = object.root();
    this->modelMaterials.reserve(root.header().materialCount);

    for (std::size_t index = 0; index < root.header().materialCount; ++index) {
        const Wmo::Material& material = root.material(index);
        this->modelMaterials.push_back(PresentationMapObjectMaterial {
            .texture1 = texture(material.texture1),
            .texture2 = texture(material.texture2),
            .flags = material.flags,
            .shader = material.shader,
            .blendMode = material.blendMode,
            .sidnColor = material.sidnColor,
            .frameSidnColor = material.frameSidnColor,
            .diffuseColor = material.diffuseColor,
            .groundType = material.groundType,
            .color2 = material.color2,
            .flags2 = material.flags2
        });
    }
}

const PresentationMapObjectGeometry& PresentationMapObjectModel::geometry() const noexcept {
    return this->modelGeometry;
}

std::span<const PresentationMapObjectMaterial> PresentationMapObjectModel::materials() const noexcept {
    return this->modelMaterials;
}

}
