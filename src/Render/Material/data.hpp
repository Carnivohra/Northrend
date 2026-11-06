#pragma once

#include "Render/Texture/texture.hpp"

#include <span>

namespace Northrend {

class MaterialData final {
public:
    MaterialData(std::span<const TextureHandle> textures, TextureHandle alpha) noexcept;

    [[nodiscard]] std::span<const TextureHandle> textures() const noexcept;
    [[nodiscard]] TextureHandle alpha() const noexcept;

private:
    std::span<const TextureHandle> materialTextures;
    TextureHandle alphaTexture;
};

}
