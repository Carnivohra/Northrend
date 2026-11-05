#pragma once

#include "Render/Texture/texture.hpp"

#include <span>

namespace Northrend {

class MaterialData final {
public:
    explicit MaterialData(std::span<const TextureHandle> textures) noexcept;

    [[nodiscard]] std::span<const TextureHandle> textures() const noexcept;

private:
    std::span<const TextureHandle> materialTextures;
};

}
