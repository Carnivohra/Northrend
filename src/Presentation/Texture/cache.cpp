#include "Presentation/Texture/cache.hpp"

#include "Asset/Stream/priority.hpp"
#include "Asset/system.hpp"
#include "Render/renderer.hpp"
#include "Render/Texture/data.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Northrend {

PresentationTextureCache::PresentationTextureCache(AssetSystem& assets) noexcept : assets(assets) {}

void PresentationTextureCache::retain(AssetId asset) {
    const std::string key(asset.path());
    const auto existing = this->entries.find(key);

    if (existing != this->entries.end()) {
        ++existing->second->references;
        return;
    }

    AssetRequest request = this->assets.request(std::move(asset), AssetPriority::background);
    const TextureHandle texture = this->handles.allocate();

    try {
        auto entry = std::make_unique<PresentationTextureEntry>(std::move(request), texture);
        const auto [stored, inserted] = this->entries.emplace(key, std::move(entry));

        if (!inserted) {
            throw std::runtime_error("Texture cache entry already exists");
        }

        try {
            this->pending.push_back(stored->second.get());
        } catch (...) {
            this->entries.erase(stored);
            throw;
        }
    } catch (...) {
        this->handles.release(texture);
        throw;
    }
}

void PresentationTextureCache::release(const AssetId& asset, Renderer& renderer) {
    const auto existing = this->entries.find(std::string(asset.path()));

    if (existing == this->entries.end()) {
        return;
    }

    PresentationTextureEntry& entry = *existing->second;

    if (--entry.references != 0) {
        return;
    }

    if (entry.uploaded) {
        renderer.remove_texture(entry.texture);
    } else {
        this->remove_pending(entry);
    }

    this->handles.release(entry.texture);
    this->entries.erase(existing);
}

void PresentationTextureCache::tick(Renderer& renderer) {
    std::size_t uploaded = 0;

    std::size_t position = 0;

    while (position < this->pending.size() && uploaded < UploadsPerFrame) {
        PresentationTextureEntry& entry = *this->pending[position];

        if (!entry.request.ready()) {
            ++position;
            continue;
        }

        Blp blp = [&] {
            try {
                return Blp(entry.request.data());
            } catch (const std::exception& exception) {
                const auto stored = std::ranges::find_if(
                    this->entries, [&entry](const auto& pair) { return pair.second.get() == &entry; });
                const std::string path = stored != this->entries.end() ? stored->first : "<unknown>";
                throw std::runtime_error("Failed to parse texture " + path + ": " + exception.what());
            }
        }();
        std::array<std::span<const std::byte>, Blp::MaximumMipCount> mips;

        for (std::size_t mip = 0; mip < blp.mip_count(); ++mip) {
            mips[mip] = blp.mip_data(mip);
        }

        const TextureData data(texture_format(blp.format()), blp.width(), blp.height(),
            std::span<const std::span<const std::byte>>(mips.data(), blp.mip_count()));
        renderer.store_texture(entry.texture, data);
        entry.uploaded = true;
        entry.request = {};
        this->pending[position] = this->pending.back();
        this->pending.pop_back();
        ++uploaded;
    }
}

void PresentationTextureCache::clear(Renderer& renderer) {
    std::vector<TextureHandle> uploaded;
    uploaded.reserve(this->entries.size());

    for (const auto& entryPair : this->entries) {
        if (entryPair.second->uploaded) {
            uploaded.push_back(entryPair.second->texture);
        }
    }

    renderer.remove_textures(uploaded);

    for (const auto& entryPair : this->entries) {
        this->handles.release(entryPair.second->texture);
    }

    this->pending.clear();
    this->entries.clear();
}

TextureHandle PresentationTextureCache::store(const TextureData& data, Renderer& renderer) {
    const TextureHandle texture = this->handles.allocate();

    try {
        renderer.store_texture(texture, data);
    } catch (...) {
        this->handles.release(texture);
        throw;
    }

    return texture;
}

void PresentationTextureCache::remove(const TextureHandle texture, Renderer& renderer) {
    renderer.remove_texture(texture);
    this->handles.release(texture);
}

TextureHandle PresentationTextureCache::find(const AssetId& asset) const {
    const auto existing = this->entries.find(std::string(asset.path()));
    return existing != this->entries.end() ? existing->second->texture : TextureHandle {};
}

bool PresentationTextureCache::ready(const AssetId& asset) const {
    const auto existing = this->entries.find(std::string(asset.path()));
    return existing != this->entries.end() && existing->second->uploaded;
}

TextureFormat PresentationTextureCache::texture_format(const Blp::Format format) noexcept {
    switch (format) {
    case Blp::Format::bc1:
        return TextureFormat::bc1Srgb;
    case Blp::Format::bc2:
        return TextureFormat::bc2Srgb;
    case Blp::Format::bc3:
        return TextureFormat::bc3Srgb;
    }

    return TextureFormat::bc1Srgb;
}

void PresentationTextureCache::remove_pending(const PresentationTextureEntry& entry) noexcept {
    const auto existing = std::ranges::find(this->pending, &entry);

    if (existing == this->pending.end()) {
        return;
    }

    *existing = this->pending.back();
    this->pending.pop_back();
}

}
