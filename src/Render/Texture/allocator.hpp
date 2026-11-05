#pragma once

#include "Render/Texture/texture.hpp"

#include <atomic>
#include <cstdint>

namespace Northrend {

class TextureHandleAllocator final {
public:
    TextureHandleAllocator() = default;

    TextureHandleAllocator(const TextureHandleAllocator&) = delete;
    TextureHandleAllocator& operator=(const TextureHandleAllocator&) = delete;

    [[nodiscard]] TextureHandle allocate();

private:
    std::atomic<std::uint64_t> nextValue = 1;
};

}
