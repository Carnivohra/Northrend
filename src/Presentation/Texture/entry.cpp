#include "Presentation/Texture/entry.hpp"

#include <utility>

namespace Northrend {

PresentationTextureEntry::PresentationTextureEntry(AssetRequest request, const TextureHandle texture) noexcept :
    request(std::move(request)), texture(texture) {}

}
