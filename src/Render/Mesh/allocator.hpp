#pragma once

#include "Render/Mesh/mesh.hpp"

#include <cstdint>
#include <mutex>
#include <vector>

namespace Northrend {

class MeshHandleAllocator final {
public:
    MeshHandleAllocator() = default;

    MeshHandleAllocator(const MeshHandleAllocator&) = delete;
    MeshHandleAllocator& operator=(const MeshHandleAllocator&) = delete;

    [[nodiscard]] MeshHandle allocate();
    void release(MeshHandle handle);

private:
    std::mutex access;
    std::vector<std::uint64_t> availableValues;
    std::uint64_t nextValue = 1;
};

}
