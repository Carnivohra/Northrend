#include "Render/Texture/allocator.hpp"

#include <limits>
#include <stdexcept>

namespace Northrend {

TextureHandle TextureHandleAllocator::allocate() {
    std::uint64_t value = this->nextValue.load(std::memory_order_relaxed);

    while (value != std::numeric_limits<std::uint64_t>::max()) {
        if (this->nextValue.compare_exchange_weak(
                value, value + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
            return TextureHandle(value);
        }
    }

    throw std::overflow_error("Texture handle space is exhausted");
}

}
