#include "Render/Material/data.hpp"

namespace Northrend {

MaterialData::MaterialData(
    const std::span<const TextureHandle> textures, const TextureHandle alpha, const TextureHandle blended,
    const MaterialProperties properties) noexcept :
    materialTextures(textures), alphaTexture(alpha), blendedTexture(blended), materialProperties(properties) {}

std::span<const TextureHandle> MaterialData::textures() const noexcept {
    return this->materialTextures;
}

TextureHandle MaterialData::alpha() const noexcept {
    return this->alphaTexture;
}

TextureHandle MaterialData::blended() const noexcept {
    return this->blendedTexture;
}

MaterialProperties MaterialData::properties() const noexcept {
    return this->materialProperties;
}

}
