#pragma once

#include <compare>
#include <cstdint>
#include <type_traits>

namespace Northrend {

class TextureHandleAllocator;

class TextureHandle final {
public:
    constexpr TextureHandle() noexcept = default;

    [[nodiscard]] constexpr std::uint64_t id() const noexcept {
        return this->value;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return this->value != 0;
    }

    auto operator<=>(const TextureHandle&) const = default;

private:
    friend class TextureHandleAllocator;

    explicit constexpr TextureHandle(const std::uint64_t value) noexcept : value(value) {}

    std::uint64_t value = 0;
};

static_assert(std::is_trivially_copyable_v<TextureHandle>);
static_assert(std::is_standard_layout_v<TextureHandle>);

}
