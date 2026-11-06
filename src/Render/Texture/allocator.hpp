#pragma once

#include "Render/Texture/texture.hpp"

#include <cstdint>
#include <mutex>
#include <vector>

namespace Northrend {

class TextureHandleAllocator final {
public:
    TextureHandleAllocator() = default;

    TextureHandleAllocator(const TextureHandleAllocator&) = delete;
    TextureHandleAllocator& operator=(const TextureHandleAllocator&) = delete;

    [[nodiscard]] TextureHandle allocate();
    void release(TextureHandle handle);

private:
    std::mutex access;
    std::vector<std::uint64_t> availableValues;
    std::uint64_t nextValue = 1;
};

}
