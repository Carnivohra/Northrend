#include "Render/Texture/blend.hpp"

#include <limits>
#include <stdexcept>

namespace Northrend {

TextureBlendData::TextureBlendData(const std::span<const TextureHandle> textures, const TextureHandle alpha,
    const std::span<const Cell> cells, const std::uint32_t columns, const std::uint32_t rows,
    const std::uint32_t cellSize, const float textureScale) :
    sourceTextures(textures),
    alphaTexture(alpha),
    blendCells(cells),
    columnCount(columns),
    rowCount(rows),
    cellExtent(cellSize),
    sourceScale(textureScale) {
    if (this->sourceTextures.empty() || !this->alphaTexture || this->blendCells.empty() || this->columnCount == 0 ||
        this->rowCount == 0 || this->cellExtent == 0 || this->sourceScale <= 0.0F) {
        throw std::invalid_argument("Texture blend data is incomplete");
    }

    if (this->columnCount > std::numeric_limits<std::uint32_t>::max() / this->rowCount ||
        this->blendCells.size() != static_cast<std::size_t>(this->columnCount) * this->rowCount ||
        this->columnCount > std::numeric_limits<std::uint32_t>::max() / this->cellExtent ||
        this->rowCount > std::numeric_limits<std::uint32_t>::max() / this->cellExtent) {
        throw std::invalid_argument("Texture blend grid dimensions are invalid");
    }

    for (const Cell& cell : this->blendCells) {
        if (cell.layerCount == 0 || cell.layerCount > cell.textures.size()) {
            throw std::invalid_argument("Texture blend cell has an invalid layer count");
        }

        for (std::size_t layer = 0; layer < cell.layerCount; ++layer) {
            if (cell.textures[layer] >= this->sourceTextures.size()) {
                throw std::invalid_argument("Texture blend cell references an invalid source texture");
            }
        }
    }
}

std::span<const TextureHandle> TextureBlendData::textures() const noexcept {
    return this->sourceTextures;
}

TextureHandle TextureBlendData::alpha() const noexcept {
    return this->alphaTexture;
}

std::span<const TextureBlendData::Cell> TextureBlendData::cells() const noexcept {
    return this->blendCells;
}

std::uint32_t TextureBlendData::columns() const noexcept {
    return this->columnCount;
}

std::uint32_t TextureBlendData::rows() const noexcept {
    return this->rowCount;
}

std::uint32_t TextureBlendData::cell_size() const noexcept {
    return this->cellExtent;
}

std::uint32_t TextureBlendData::width() const noexcept {
    return this->columnCount * this->cellExtent;
}

std::uint32_t TextureBlendData::height() const noexcept {
    return this->rowCount * this->cellExtent;
}

float TextureBlendData::texture_scale() const noexcept {
    return this->sourceScale;
}

}
