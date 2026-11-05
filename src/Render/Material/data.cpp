#include "Render/Material/data.hpp"

namespace Northrend {

MaterialData::MaterialData(const std::span<const TextureHandle> textures) noexcept : materialTextures(textures) {}

std::span<const TextureHandle> MaterialData::textures() const noexcept {
    return this->materialTextures;
}

}
