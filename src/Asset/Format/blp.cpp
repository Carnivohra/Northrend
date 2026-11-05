#include "Asset/Format/blp.hpp"

#include "Asset/data.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>
#include <string>
#include <utility>

namespace Northrend {

Blp::Blp(std::shared_ptr<const AssetData> data) : data(std::move(data)) {
    static constexpr std::uint32_t Magic = 0x32504C42;
    static constexpr std::uint32_t Version = 1;
    static constexpr std::uint8_t Compressed = 2;
    static constexpr std::uint8_t Dxt1 = 0;
    static constexpr std::uint8_t Dxt3 = 1;
    static constexpr std::uint8_t Dxt5 = 7;
    static constexpr std::size_t HeaderSize = 148;
    static constexpr std::size_t MipOffsetsOffset = 20;
    static constexpr std::size_t MipSizesOffset = 84;

    if (this->data == nullptr || this->data->size() < HeaderSize) {
        throw std::invalid_argument("BLP2 data is truncated");
    }

    const std::span<const std::byte> bytes = this->data->bytes();

    if (read_uint32(bytes, 0) != Magic || read_uint32(bytes, 4) != Version) {
        throw std::runtime_error("Unsupported BLP format");
    }

    if (std::to_integer<std::uint8_t>(bytes[8]) != Compressed) {
        throw std::runtime_error("Unsupported BLP2 color encoding");
    }

    switch (std::to_integer<std::uint8_t>(bytes[10])) {
    case Dxt1:
        this->pixelFormat = Format::bc1;
        break;
    case Dxt3:
        this->pixelFormat = Format::bc2;
        break;
    case Dxt5:
        this->pixelFormat = Format::bc3;
        break;
    default:
        throw std::runtime_error("Unsupported BLP2 block compression format");
    }

    this->imageWidth = read_uint32(bytes, 12);
    this->imageHeight = read_uint32(bytes, 16);

    if (this->imageWidth == 0 || this->imageHeight == 0) {
        throw std::runtime_error("BLP2 dimensions are invalid");
    }

    const bool hasMips = (std::to_integer<std::uint8_t>(bytes[11]) & 0x0FU) != 0;
    const std::size_t levels =
        hasMips ? std::bit_width(std::max(this->imageWidth, this->imageHeight)) : std::size_t {1};

    if (levels > MaximumMipCount) {
        throw std::runtime_error("BLP2 contains too many mip levels");
    }

    std::size_t previousEnd = HeaderSize;
    std::size_t validLevels = 0;

    for (std::size_t mip = 0; mip < levels; ++mip) {
        const std::size_t offset = read_uint32(bytes, MipOffsetsOffset + mip * sizeof(std::uint32_t));
        const std::size_t size = read_uint32(bytes, MipSizesOffset + mip * sizeof(std::uint32_t));

        if (mip != 0 && offset == 0 && size == 0) {
            break;
        }

        const std::uint32_t width = std::max(this->imageWidth >> mip, std::uint32_t {1});
        const std::uint32_t height = std::max(this->imageHeight >> mip, std::uint32_t {1});

        const std::size_t expectedSize = this->mip_size(width, height);

        if (offset < previousEnd || offset > bytes.size() || size > bytes.size() - offset) {
            throw std::runtime_error("BLP2 mip " + std::to_string(mip) + " is invalid (offset " +
                std::to_string(offset) + ", size " + std::to_string(size) + ", expected " +
                std::to_string(expectedSize) + ", previous end " + std::to_string(previousEnd) + ')');
        }

        if (size != expectedSize) {
            if (mip == 0) {
                throw std::runtime_error("BLP2 base mip has an invalid size");
            }

            break;
        }

        this->mipData[mip] = bytes.subspan(offset, size);
        previousEnd = offset + size;
        ++validLevels;
    }

    this->mipCount = static_cast<std::uint8_t>(validLevels);
}

Blp::Format Blp::format() const noexcept {
    return this->pixelFormat;
}

std::uint32_t Blp::width() const noexcept {
    return this->imageWidth;
}

std::uint32_t Blp::height() const noexcept {
    return this->imageHeight;
}

std::size_t Blp::mip_count() const noexcept {
    return this->mipCount;
}

std::uint32_t Blp::mip_width(const std::size_t mip) const {
    if (mip >= this->mipCount) {
        throw std::out_of_range("BLP2 mip level is outside the texture");
    }

    return std::max(this->imageWidth >> mip, std::uint32_t {1});
}

std::uint32_t Blp::mip_height(const std::size_t mip) const {
    if (mip >= this->mipCount) {
        throw std::out_of_range("BLP2 mip level is outside the texture");
    }

    return std::max(this->imageHeight >> mip, std::uint32_t {1});
}

std::span<const std::byte> Blp::mip_data(const std::size_t mip) const {
    if (mip >= this->mipCount) {
        throw std::out_of_range("BLP2 mip level is outside the texture");
    }

    return this->mipData[mip];
}

std::size_t Blp::mip_size(const std::uint32_t width, const std::uint32_t height) const noexcept {
    const std::size_t blocksX = (static_cast<std::size_t>(width) + 3) / 4;
    const std::size_t blocksY = (static_cast<std::size_t>(height) + 3) / 4;
    const std::size_t bytesPerBlock = this->pixelFormat == Format::bc1 ? 8 : 16;
    return blocksX * blocksY * bytesPerBlock;
}

std::uint32_t Blp::read_uint32(const std::span<const std::byte> bytes, const std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint32_t)) {
        throw std::out_of_range("BLP2 value exceeds the available data");
    }

    return std::to_integer<std::uint32_t>(bytes[offset]) | std::to_integer<std::uint32_t>(bytes[offset + 1]) << 8U |
        std::to_integer<std::uint32_t>(bytes[offset + 2]) << 16U |
        std::to_integer<std::uint32_t>(bytes[offset + 3]) << 24U;
}

}
