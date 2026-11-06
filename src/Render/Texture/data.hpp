#pragma once

#include "Render/Texture/format.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Northrend {

class TextureData final {
public:
    TextureData(TextureFormat format, std::uint32_t width, std::uint32_t height,
        std::span<const std::span<const std::byte>> mips);
    TextureData(TextureFormat format, std::uint32_t width, std::uint32_t height, std::uint32_t layers,
        std::span<const std::span<const std::byte>> mips);

    [[nodiscard]] TextureFormat format() const noexcept;
    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;
    [[nodiscard]] std::uint32_t layers() const noexcept;
    [[nodiscard]] bool array() const noexcept;
    [[nodiscard]] std::size_t mip_count() const noexcept;
    [[nodiscard]] std::uint32_t mip_width(std::size_t mip) const;
    [[nodiscard]] std::uint32_t mip_height(std::size_t mip) const;
    [[nodiscard]] std::span<const std::byte> mip(std::size_t mip) const;

private:
    TextureData(TextureFormat format, std::uint32_t width, std::uint32_t height, std::uint32_t layers,
        std::span<const std::span<const std::byte>> mips, bool array);
    [[nodiscard]] std::size_t mip_size(std::uint32_t width, std::uint32_t height) const noexcept;

    std::vector<std::span<const std::byte>> mipData;
    TextureFormat pixelFormat;
    std::uint32_t imageWidth;
    std::uint32_t imageHeight;
    std::uint32_t imageLayers;
    bool imageArray;
};

}
