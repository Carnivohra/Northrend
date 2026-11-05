#pragma once

#include <compare>
#include <cstdint>
#include <type_traits>

namespace Northrend {

class MaterialHandleAllocator;

class MaterialHandle final {
public:
    constexpr MaterialHandle() noexcept = default;

    [[nodiscard]] constexpr std::uint64_t id() const noexcept {
        return this->value;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return this->value != 0;
    }

    auto operator<=>(const MaterialHandle&) const = default;

private:
    friend class MaterialHandleAllocator;

    explicit constexpr MaterialHandle(const std::uint64_t value) noexcept : value(value) {}

    std::uint64_t value = 0;
};

static_assert(std::is_trivially_copyable_v<MaterialHandle>);
static_assert(std::is_standard_layout_v<MaterialHandle>);

}
