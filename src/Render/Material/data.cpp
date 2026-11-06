#include "Render/Material/data.hpp"

namespace Northrend {

MaterialData::MaterialData(const std::span<const TextureHandle> textures, const TextureHandle alpha) noexcept :
    materialTextures(textures), alphaTexture(alpha) {}

std::span<const TextureHandle> MaterialData::textures() const noexcept {
    return this->materialTextures;
}

TextureHandle MaterialData::alpha() const noexcept {
    return this->alphaTexture;
}

}
