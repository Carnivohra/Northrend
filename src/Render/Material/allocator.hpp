#pragma once

#include "Render/Material/material.hpp"

#include <atomic>
#include <cstdint>

namespace Northrend {

class MaterialHandleAllocator final {
public:
    MaterialHandleAllocator() = default;

    MaterialHandleAllocator(const MaterialHandleAllocator&) = delete;
    MaterialHandleAllocator& operator=(const MaterialHandleAllocator&) = delete;

    [[nodiscard]] MaterialHandle allocate();

private:
    std::atomic<std::uint64_t> nextValue = 1;
};

}
