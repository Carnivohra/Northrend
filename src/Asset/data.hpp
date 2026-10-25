#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace Northrend {

class AssetData final {
public:
    AssetData() = default;
    explicit AssetData(std::vector<std::byte> bytes) noexcept;

    [[nodiscard]] std::span<const std::byte> bytes() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    std::vector<std::byte> storage;
};

}
