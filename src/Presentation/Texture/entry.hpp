#pragma once

#include "Asset/Stream/request.hpp"
#include "Render/Texture/texture.hpp"

#include <cstddef>

namespace Northrend {

class PresentationTextureCache;

class PresentationTextureEntry final {
public:
    PresentationTextureEntry(AssetRequest request, TextureHandle texture) noexcept;

private:
    friend class PresentationTextureCache;

    AssetRequest request;
    TextureHandle texture;
    std::size_t references = 1;
    bool uploaded = false;
};

}
