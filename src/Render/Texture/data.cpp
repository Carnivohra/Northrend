#include "Render/Texture/data.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>

namespace Northrend {

TextureData::TextureData(const TextureFormat format, const std::uint32_t width, const std::uint32_t height,
    const std::span<const std::span<const std::byte>> mips) :
    mipData(mips.begin(), mips.end()), pixelFormat(format), imageWidth(width), imageHeight(height) {
    if (this->imageWidth == 0 || this->imageHeight == 0 || this->mipData.empty()) {
        throw std::invalid_argument("Texture data is incomplete");
    }

    const std::size_t maximumMipCount = std::bit_width(std::max(this->imageWidth, this->imageHeight));

    if (this->mipData.size() > maximumMipCount) {
        throw std::invalid_argument("Texture contains too many mip levels");
    }

    for (std::size_t mip = 0; mip < this->mipData.size(); ++mip) {
        if (this->mipData[mip].size_bytes() != this->mip_size(this->mip_width(mip), this->mip_height(mip))) {
            throw std::invalid_argument("Texture mip level has an invalid size");
        }
    }
}

TextureFormat TextureData::format() const noexcept {
    return this->pixelFormat;
}

std::uint32_t TextureData::width() const noexcept {
    return this->imageWidth;
}

std::uint32_t TextureData::height() const noexcept {
    return this->imageHeight;
}

std::size_t TextureData::mip_count() const noexcept {
    return this->mipData.size();
}

std::uint32_t TextureData::mip_width(const std::size_t mip) const {
    if (mip >= this->mipData.size()) {
        throw std::out_of_range("Texture mip level is outside the texture");
    }

    return std::max(this->imageWidth >> mip, std::uint32_t {1});
}

std::uint32_t TextureData::mip_height(const std::size_t mip) const {
    if (mip >= this->mipData.size()) {
        throw std::out_of_range("Texture mip level is outside the texture");
    }

    return std::max(this->imageHeight >> mip, std::uint32_t {1});
}

std::span<const std::byte> TextureData::mip(const std::size_t mip) const {
    return this->mipData.at(mip);
}

std::size_t TextureData::mip_size(const std::uint32_t width, const std::uint32_t height) const noexcept {
    if (this->pixelFormat == TextureFormat::r8Unorm) {
        return static_cast<std::size_t>(width) * height;
    }

    const std::size_t blocksX = (static_cast<std::size_t>(width) + 3) / 4;
    const std::size_t blocksY = (static_cast<std::size_t>(height) + 3) / 4;
    const std::size_t bytesPerBlock = this->pixelFormat == TextureFormat::bc1Srgb ? 8 : 16;
    return blocksX * blocksY * bytesPerBlock;
}

}
