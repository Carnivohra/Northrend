#pragma once

#include "Render/Texture/texture.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Northrend {

class TextureBlendData final {
public:
    struct Cell final {
        std::array<std::uint32_t, 4> textures {};
        std::uint32_t alphaLayer = 0;
        std::uint32_t layerCount = 1;
        std::array<std::uint32_t, 2> padding {};
    };

    TextureBlendData(std::span<const TextureHandle> textures, TextureHandle alpha, std::span<const Cell> cells,
        std::uint32_t columns, std::uint32_t rows, std::uint32_t cellSize, float textureScale);

    [[nodiscard]] std::span<const TextureHandle> textures() const noexcept;
    [[nodiscard]] TextureHandle alpha() const noexcept;
    [[nodiscard]] std::span<const Cell> cells() const noexcept;
    [[nodiscard]] std::uint32_t columns() const noexcept;
    [[nodiscard]] std::uint32_t rows() const noexcept;
    [[nodiscard]] std::uint32_t cell_size() const noexcept;
    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;
    [[nodiscard]] float texture_scale() const noexcept;

private:
    std::span<const TextureHandle> sourceTextures;
    TextureHandle alphaTexture;
    std::span<const Cell> blendCells;
    std::uint32_t columnCount;
    std::uint32_t rowCount;
    std::uint32_t cellExtent;
    float sourceScale;
};

static_assert(sizeof(TextureBlendData::Cell) == sizeof(std::uint32_t) * 8);

}
