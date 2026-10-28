#pragma once

#include <filesystem>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace Northrend {

class Wtf final {
public:
    using Command = std::vector<std::string>;
    using Commands = std::vector<Command>;

    Wtf() = default;
    explicit Wtf(const std::filesystem::path& path);

    void load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path) const;

    [[nodiscard]] Commands& commands() noexcept;
    [[nodiscard]] const Commands& commands() const noexcept;

private:
    [[nodiscard]] static Command parse(std::string_view line);
    static void write(std::ostream& stream, const Command& command);

    Commands content;
};

}
