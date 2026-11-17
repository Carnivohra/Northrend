#pragma once

#include "Render/Texture/texture.hpp"

#include <cstdint>
#include <span>

namespace Northrend {

struct MaterialProperties final {
    std::uint32_t shader = 0;
    std::uint32_t blendMode = 0;
    std::uint32_t flags = 0;
    std::uint32_t textureCount = 0;
};

class MaterialData final {
public:
    MaterialData(std::span<const TextureHandle> textures, TextureHandle alpha, TextureHandle blended,
        MaterialProperties properties = {}) noexcept;

    [[nodiscard]] std::span<const TextureHandle> textures() const noexcept;
    [[nodiscard]] TextureHandle alpha() const noexcept;
    [[nodiscard]] TextureHandle blended() const noexcept;
    [[nodiscard]] MaterialProperties properties() const noexcept;

private:
    std::span<const TextureHandle> materialTextures;
    TextureHandle alphaTexture;
    TextureHandle blendedTexture;
    MaterialProperties materialProperties;
};

}
