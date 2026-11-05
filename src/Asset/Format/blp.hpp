#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace Northrend {

class AssetData;

class Blp final {
public:
    enum class Format : std::uint8_t { bc1, bc2, bc3 };

    static constexpr std::size_t MaximumMipCount = 16;

    explicit Blp(std::shared_ptr<const AssetData> data);

    [[nodiscard]] Format format() const noexcept;
    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;
    [[nodiscard]] std::size_t mip_count() const noexcept;
    [[nodiscard]] std::uint32_t mip_width(std::size_t mip) const;
    [[nodiscard]] std::uint32_t mip_height(std::size_t mip) const;
    [[nodiscard]] std::span<const std::byte> mip_data(std::size_t mip) const;

private:
    [[nodiscard]] std::size_t mip_size(std::uint32_t width, std::uint32_t height) const noexcept;
    [[nodiscard]] static std::uint32_t read_uint32(std::span<const std::byte> bytes, std::size_t offset);

    std::shared_ptr<const AssetData> data;
    std::array<std::span<const std::byte>, MaximumMipCount> mipData;
    Format pixelFormat = Format::bc1;
    std::uint32_t imageWidth = 0;
    std::uint32_t imageHeight = 0;
    std::uint8_t mipCount = 0;
};

}
