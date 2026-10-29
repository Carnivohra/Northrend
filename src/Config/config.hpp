#pragma once

#include "Config/wtf.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace Northrend {

class Config final {
public:
    Config() = default;
    explicit Config(const std::filesystem::path& path);

    void load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path) const;

    [[nodiscard]] bool contains(std::string_view name) const noexcept;
    [[nodiscard]] std::string value(std::string_view name, std::string fallback = {}) const;

    void set(std::string name, std::string value);

private:
    [[nodiscard]] Wtf::Command* find(std::string_view name) noexcept;
    [[nodiscard]] const Wtf::Command* find(std::string_view name) const noexcept;

    [[nodiscard]] static bool equal_ascii(std::string_view left, std::string_view right) noexcept;

    Wtf document;
};

}
