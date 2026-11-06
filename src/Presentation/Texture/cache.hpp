#pragma once

#include "Asset/Format/blp.hpp"
#include "Asset/id.hpp"
#include "Presentation/Texture/entry.hpp"
#include "Render/Texture/allocator.hpp"
#include "Render/Texture/format.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Northrend {

class AssetSystem;
class Renderer;
class TextureData;

class PresentationTextureCache final {
public:
    explicit PresentationTextureCache(AssetSystem& assets) noexcept;

    PresentationTextureCache(const PresentationTextureCache&) = delete;
    PresentationTextureCache& operator=(const PresentationTextureCache&) = delete;

    void retain(AssetId asset);
    void release(const AssetId& asset, Renderer& renderer);
    void tick(Renderer& renderer);
    void clear(Renderer& renderer);
    [[nodiscard]] TextureHandle store(const TextureData& data, Renderer& renderer);
    void remove(TextureHandle texture, Renderer& renderer);
    [[nodiscard]] TextureHandle find(const AssetId& asset) const;
    [[nodiscard]] bool ready(const AssetId& asset) const;

private:
    [[nodiscard]] static TextureFormat texture_format(Blp::Format format) noexcept;
    void remove_pending(const PresentationTextureEntry& entry) noexcept;

    static constexpr std::size_t UploadsPerFrame = 4;

    AssetSystem& assets;
    TextureHandleAllocator handles;
    std::unordered_map<std::string, std::unique_ptr<PresentationTextureEntry>> entries;
    std::vector<PresentationTextureEntry*> pending;
};

}
