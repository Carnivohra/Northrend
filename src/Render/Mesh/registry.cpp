#include "Render/Mesh/registry.hpp"

#include <limits>
#include <stdexcept>

namespace Northrend {

MeshHandle MeshRegistry::allocate() {
    std::uint64_t value = this->nextValue.load(std::memory_order_relaxed);

    while (value != std::numeric_limits<std::uint64_t>::max()) {
        if (this->nextValue.compare_exchange_weak(
                value,
                value + 1,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
            return MeshHandle(value);
        }
    }

    throw std::overflow_error("Mesh handle space is exhausted");
}

}
