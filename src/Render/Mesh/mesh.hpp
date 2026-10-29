#pragma once

#include <compare>
#include <cstdint>
#include <type_traits>

namespace Northrend {

class MeshHandleAllocator;

class MeshHandle final {
public:
    constexpr MeshHandle() noexcept = default;

    [[nodiscard]] constexpr std::uint64_t id() const noexcept {
        return this->value;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return this->value != 0;
    }

    auto operator<=>(const MeshHandle&) const = default;

private:
    friend class MeshHandleAllocator;

    explicit constexpr MeshHandle(std::uint64_t value) noexcept : value(value) {}

    std::uint64_t value = 0;
};

static_assert(std::is_trivially_copyable_v<MeshHandle>);
static_assert(std::is_standard_layout_v<MeshHandle>);

}
