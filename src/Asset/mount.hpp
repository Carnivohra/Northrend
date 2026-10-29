#pragma once

#include <filesystem>

namespace Northrend {

struct AssetMount final {
    enum class Type { directory, mpq };

    Type type;
    std::filesystem::path path;
};

}
