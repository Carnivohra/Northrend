#include "Render/Mesh/allocator.hpp"

#include <limits>
#include <stdexcept>

namespace Northrend {

MeshHandle MeshHandleAllocator::allocate() {
    const std::scoped_lock lock(this->access);

    if (!this->availableValues.empty()) {
        const std::uint64_t value = this->availableValues.back();
        this->availableValues.pop_back();
        return MeshHandle(value);
    }

    if (this->nextValue == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Mesh handle space is exhausted");
    }

    return MeshHandle(this->nextValue++);
}

void MeshHandleAllocator::release(const MeshHandle handle) {
    if (!handle) {
        return;
    }

    const std::scoped_lock lock(this->access);
    this->availableValues.push_back(handle.id());
}

}
