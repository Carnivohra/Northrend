#pragma once

#include "Render/Mesh/mesh.hpp"

#include <atomic>
#include <cstdint>

namespace Northrend {

class MeshHandleAllocator final {
public:
    MeshHandleAllocator() = default;

    MeshHandleAllocator(const MeshHandleAllocator&) = delete;
    MeshHandleAllocator& operator=(const MeshHandleAllocator&) = delete;

    [[nodiscard]] MeshHandle allocate();

private:
    std::atomic<std::uint64_t> nextValue = 1;
};

}
