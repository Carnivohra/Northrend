#pragma once

#include "Render/mesh.hpp"

#include <atomic>
#include <cstdint>

namespace Northrend {

class MeshRegistry final {
public:
    MeshRegistry() = default;

    MeshRegistry(const MeshRegistry&) = delete;
    MeshRegistry& operator=(const MeshRegistry&) = delete;

    [[nodiscard]] MeshHandle allocate();

private:
    std::atomic<std::uint64_t> nextValue = 1;
};

}
