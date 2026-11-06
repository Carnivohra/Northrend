#include "World/Map/Terrain/payload.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace Northrend {

TerrainPayload::TerrainPayload(const Adt& adt, const bool highResolutionAlpha) {
    static constexpr std::size_t Channels = 4;
    static constexpr std::size_t LayerBytes = Adt::AlphaTexels * Channels;
    std::array<std::uint8_t, Adt::AlphaTexels> decoded;

    for (std::size_t y = 0; y < Adt::Size; ++y) {
        for (std::size_t x = 0; x < Adt::Size; ++x) {
            const std::size_t layerCount = adt.layer_count(x, y);

            if (layerCount <= 1) {
                continue;
            }

            const std::size_t cell = y * Adt::Size + x;
            this->alphaLayers[cell] = this->alphaLayerCount++;
            const std::size_t offset = this->alphaMaps.size();
            this->alphaMaps.resize(offset + LayerBytes, std::byte {0});

            for (std::size_t texel = 0; texel < Adt::AlphaTexels; ++texel) {
                this->alphaMaps[offset + texel * Channels + 3] = std::byte {255};
            }

            for (std::size_t layer = 1; layer < layerCount; ++layer) {
                adt.decode_alpha(x, y, layer, highResolutionAlpha, decoded);

                for (std::size_t texel = 0; texel < decoded.size(); ++texel) {
                    this->alphaMaps[offset + texel * Channels + layer - 1] = std::byte {decoded[texel]};
                }
            }
        }
    }

    if (this->alphaMaps.empty()) {
        this->alphaMaps.resize(LayerBytes, std::byte {0});
        this->alphaLayerCount = 1;
    }

    this->alphaMipOffsets[0] = 0;
    this->alphaMipSizes[0] = this->alphaMaps.size();
    this->alphaMipCount = 1;
    std::size_t sourceSize = Adt::AlphaSize;

    while (sourceSize > 1) {
        const std::size_t destinationSize = sourceSize / 2;
        const std::size_t sourceOffset = this->alphaMipOffsets[this->alphaMipCount - 1];
        const std::size_t destinationOffset = this->alphaMaps.size();
        const std::size_t destinationBytes =
            static_cast<std::size_t>(this->alphaLayerCount) * destinationSize * destinationSize * Channels;
        this->alphaMaps.resize(destinationOffset + destinationBytes);

        for (std::size_t layer = 0; layer < this->alphaLayerCount; ++layer) {
            const std::size_t sourceLayer = sourceOffset + layer * sourceSize * sourceSize * Channels;
            const std::size_t destinationLayer =
                destinationOffset + layer * destinationSize * destinationSize * Channels;

            for (std::size_t y = 0; y < destinationSize; ++y) {
                for (std::size_t x = 0; x < destinationSize; ++x) {
                    for (std::size_t channel = 0; channel < Channels; ++channel) {
                        std::uint32_t value = 0;

                        for (std::size_t offsetY = 0; offsetY < 2; ++offsetY) {
                            for (std::size_t offsetX = 0; offsetX < 2; ++offsetX) {
                                const std::size_t source = sourceLayer +
                                    ((y * 2 + offsetY) * sourceSize + x * 2 + offsetX) * Channels + channel;
                                value += std::to_integer<std::uint8_t>(this->alphaMaps[source]);
                            }
                        }

                        const std::size_t destination =
                            destinationLayer + (y * destinationSize + x) * Channels + channel;
                        this->alphaMaps[destination] = static_cast<std::byte>((value + 2) / 4);
                    }
                }
            }
        }

        this->alphaMipOffsets[this->alphaMipCount] = destinationOffset;
        this->alphaMipSizes[this->alphaMipCount] = destinationBytes;
        ++this->alphaMipCount;
        sourceSize = destinationSize;
    }
}

std::uint16_t TerrainPayload::alpha_layer(const std::size_t x, const std::size_t y) const {
    if (x >= Adt::Size || y >= Adt::Size) {
        throw std::out_of_range("Terrain payload coordinate is outside the tile");
    }

    return this->alphaLayers[y * Adt::Size + x];
}

std::size_t TerrainPayload::alpha_layer_count() const noexcept {
    return this->alphaLayerCount;
}

std::size_t TerrainPayload::alpha_mip_count() const noexcept {
    return this->alphaMipCount;
}

std::span<const std::byte> TerrainPayload::alpha_mip(const std::size_t level) const {
    if (level >= this->alphaMipCount) {
        throw std::out_of_range("Terrain alpha mip level is outside the payload");
    }

    return std::span<const std::byte>(this->alphaMaps).subspan(this->alphaMipOffsets[level], this->alphaMipSizes[level]);
}

}
