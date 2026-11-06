#include "Render/Texture/allocator.hpp"

#include <limits>
#include <stdexcept>

namespace Northrend {

TextureHandle TextureHandleAllocator::allocate() {
    const std::scoped_lock lock(this->access);

    if (!this->availableValues.empty()) {
        const std::uint64_t value = this->availableValues.back();
        this->availableValues.pop_back();
        return TextureHandle(value);
    }

    if (this->nextValue == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Texture handle space is exhausted");
    }

    return TextureHandle(this->nextValue++);
}

void TextureHandleAllocator::release(const TextureHandle handle) {
    if (!handle) {
        return;
    }

    const std::scoped_lock lock(this->access);
    this->availableValues.push_back(handle.id());
}

}
