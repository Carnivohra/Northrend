#pragma once

#include "Render/Material/material.hpp"

#include <cstdint>
#include <mutex>
#include <vector>

namespace Northrend {

class MaterialHandleAllocator final {
public:
    MaterialHandleAllocator() = default;

    MaterialHandleAllocator(const MaterialHandleAllocator&) = delete;
    MaterialHandleAllocator& operator=(const MaterialHandleAllocator&) = delete;

    [[nodiscard]] MaterialHandle allocate();
    void release(MaterialHandle handle);

private:
    std::mutex access;
    std::vector<std::uint64_t> availableValues;
    std::uint64_t nextValue = 1;
};

}
