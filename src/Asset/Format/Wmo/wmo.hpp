#pragma once

#include "Math/vector.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace Northrend {

class AssetData;

class Wmo final {
public:
    struct Header final {
        std::uint32_t materialCount;
        std::uint32_t groupCount;
        std::uint32_t portalCount;
        std::uint32_t lightCount;
        std::uint32_t doodadNameCount;
        std::uint32_t doodadCount;
        std::uint32_t doodadSetCount;
        std::uint32_t ambientColor;
        std::uint32_t id;
        Vector3 minimum;
        Vector3 maximum;
        std::uint16_t flags;
        std::uint16_t lodCount;
    };

    struct Material final {
        std::string_view texture1;
        std::string_view texture2;
        std::uint32_t flags;
        std::uint32_t shader;
        std::uint32_t blendMode;
        std::uint32_t sidnColor;
        std::uint32_t frameSidnColor;
        std::uint32_t diffuseColor;
        std::uint32_t groundType;
        std::uint32_t color2;
        std::uint32_t flags2;
    };

    struct Group final {
        std::string_view name;
        Vector3 minimum;
        Vector3 maximum;
        std::uint32_t flags;
    };

    explicit Wmo(std::shared_ptr<const AssetData> data);

    [[nodiscard]] std::uint32_t version() const noexcept;
    [[nodiscard]] const Header& header() const noexcept;
    [[nodiscard]] const Material& material(std::size_t index) const;
    [[nodiscard]] const Group& group(std::size_t index) const;

private:
    void parse_version17();
    [[nodiscard]] static std::string_view read_string(
        std::span<const std::byte> bytes, std::uint32_t offset, bool emptyAllowed);
    [[nodiscard]] static std::uint16_t read_uint16(std::span<const std::byte> bytes, std::size_t offset);
    [[nodiscard]] static std::uint32_t read_uint32(std::span<const std::byte> bytes, std::size_t offset);
    [[nodiscard]] static float read_float(std::span<const std::byte> bytes, std::size_t offset);
    [[nodiscard]] static Vector3 read_vector(std::span<const std::byte> bytes, std::size_t offset);

    std::shared_ptr<const AssetData> data;
    Header rootHeader {};
    std::vector<Material> materials;
    std::vector<Group> groups;
    std::uint32_t fileVersion = 0;
};

}
