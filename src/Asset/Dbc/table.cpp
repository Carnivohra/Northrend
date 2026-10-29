#include "Asset/Dbc/table.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace Northrend {

DbcTable::DbcTable(std::shared_ptr<const AssetData> data) : data(std::move(data)) {
    static constexpr std::size_t headerSize = 20;
    if (!this->data) {
        throw std::invalid_argument("DBC data cannot be empty");
    }

    const std::span<const std::byte> bytes = this->data->bytes();

    if (bytes.size() < headerSize) {
        throw std::runtime_error("DBC header is incomplete");
    }

    if (bytes[0] != std::byte {'W'} || bytes[1] != std::byte {'D'} || bytes[2] != std::byte {'B'} ||
        bytes[3] != std::byte {'C'}) {
        throw std::runtime_error("DBC signature is invalid");
    }

    this->recordCount = read_uint32(bytes, 4);
    this->fieldCount = read_uint32(bytes, 8);
    this->recordSize = read_uint32(bytes, 12);
    const std::size_t stringSize = read_uint32(bytes, 16);

    if (this->fieldCount == 0 || this->recordSize == 0) {
        throw std::runtime_error("DBC record layout is invalid");
    }

    if (this->recordSize != 0 && this->recordCount > std::numeric_limits<std::size_t>::max() / this->recordSize) {
        throw std::runtime_error("DBC record data exceeds the platform address space");
    }

    const std::size_t recordsSize = this->recordCount * this->recordSize;

    if (recordsSize > bytes.size() - headerSize || stringSize != bytes.size() - headerSize - recordsSize) {
        throw std::runtime_error("DBC data size does not match its header");
    }

    this->records = bytes.subspan(headerSize, recordsSize);
    this->strings = bytes.subspan(headerSize + recordsSize, stringSize);
}

std::size_t DbcTable::record_count() const noexcept {
    return this->recordCount;
}

std::size_t DbcTable::field_count() const noexcept {
    return this->fieldCount;
}

std::size_t DbcTable::record_size() const noexcept {
    return this->recordSize;
}

std::span<const std::byte> DbcTable::record(const std::size_t index) const {
    if (index >= this->recordCount) {
        throw std::out_of_range("DBC record index is out of range");
    }

    return this->records.subspan(index * this->recordSize, this->recordSize);
}

std::uint32_t DbcTable::field(const std::size_t record, const std::size_t column) const {
    if (record >= this->recordCount) {
        throw std::out_of_range("DBC record index is out of range");
    }

    if (column >= this->fieldCount) {
        throw std::out_of_range("DBC field index is out of range");
    }

    if (this->fieldCount > std::numeric_limits<std::size_t>::max() / sizeof(std::uint32_t) ||
        this->fieldCount * sizeof(std::uint32_t) != this->recordSize) {
        throw std::logic_error("DBC fields are not stored as contiguous 32-bit values");
    }

    return read_uint32(this->records, record * this->recordSize + column * sizeof(std::uint32_t));
}

std::string_view DbcTable::string(const std::uint32_t offset) const {
    if (offset >= this->strings.size()) {
        throw std::out_of_range("DBC string offset is out of range");
    }

    const auto begin = this->strings.begin() + offset;
    const auto end = std::find(begin, this->strings.end(), std::byte {0});

    if (end == this->strings.end()) {
        throw std::runtime_error("DBC string is not terminated");
    }

    return std::string_view {
        reinterpret_cast<const char*>(this->strings.data() + offset),
        static_cast<std::size_t>(end - begin)
    };
}

std::uint32_t DbcTable::read_uint32(const std::span<const std::byte> bytes, const std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint32_t)) {
        throw std::out_of_range("DBC value exceeds the available data");
    }

    return std::to_integer<std::uint32_t>(bytes[offset]) |
        std::to_integer<std::uint32_t>(bytes[offset + 1]) << 8U |
        std::to_integer<std::uint32_t>(bytes[offset + 2]) << 16U |
        std::to_integer<std::uint32_t>(bytes[offset + 3]) << 24U;
}

}
