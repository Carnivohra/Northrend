#pragma once

#include "Asset/data.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace Northrend {

class DbcTable final {
public:
    explicit DbcTable(std::shared_ptr<const AssetData> data);

    [[nodiscard]] std::size_t record_count() const noexcept;
    [[nodiscard]] std::size_t field_count() const noexcept;
    [[nodiscard]] std::size_t record_size() const noexcept;
    [[nodiscard]] std::span<const std::byte> record(std::size_t index) const;

    [[nodiscard]] std::uint32_t field(std::size_t record, std::size_t column) const;
    [[nodiscard]] std::string_view string(std::uint32_t offset) const;

private:
    [[nodiscard]] static std::uint32_t read_uint32(std::span<const std::byte> bytes, std::size_t offset);

    std::shared_ptr<const AssetData> data;
    std::span<const std::byte> records;
    std::span<const std::byte> strings;
    std::size_t recordCount = 0;
    std::size_t fieldCount = 0;
    std::size_t recordSize = 0;
};

}
