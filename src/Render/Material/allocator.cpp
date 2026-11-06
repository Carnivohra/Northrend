#include "Render/Material/allocator.hpp"

#include <limits>
#include <stdexcept>

namespace Northrend {

MaterialHandle MaterialHandleAllocator::allocate() {
    const std::scoped_lock lock(this->access);

    if (!this->availableValues.empty()) {
        const std::uint64_t value = this->availableValues.back();
        this->availableValues.pop_back();
        return MaterialHandle(value);
    }

    if (this->nextValue == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Material handle space is exhausted");
    }

    return MaterialHandle(this->nextValue++);
}

void MaterialHandleAllocator::release(const MaterialHandle handle) {
    if (!handle) {
        return;
    }

    const std::scoped_lock lock(this->access);
    this->availableValues.push_back(handle.id());
}

}
