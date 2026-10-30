#include "World/Map/terrain.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace Northrend {

void Terrain::focus(const std::size_t x, const std::size_t y, const std::size_t radius) {
    if (x >= Size || y >= Size) {
        throw std::out_of_range("Terrain focus is outside the map");
    }

    for (TerrainTileState& state : this->states) {
        if (state == TerrainTileState::requested) {
            state = TerrainTileState::missing;
        }
    }

    this->requestPosition = 0;
    this->requestCount = 0;
    const std::size_t distanceLimit = std::min(radius, Size - 1);

    for (std::size_t distance = 0; distance <= distanceLimit; ++distance) {
        const std::size_t minimumX = x > distance ? x - distance : 0;
        const std::size_t maximumX = std::min(x + distance, Size - 1);
        const std::size_t minimumY = y > distance ? y - distance : 0;
        const std::size_t maximumY = std::min(y + distance, Size - 1);

        for (std::size_t tileX = minimumX; tileX <= maximumX; ++tileX) {
            this->enqueue(tileX, minimumY, distance);

            if (maximumY != minimumY) {
                this->enqueue(tileX, maximumY, distance);
            }
        }

        for (std::size_t tileY = minimumY + 1; tileY < maximumY; ++tileY) {
            this->enqueue(minimumX, tileY, distance);

            if (maximumX != minimumX) {
                this->enqueue(maximumX, tileY, distance);
            }
        }
    }
}

std::optional<TerrainRequest> Terrain::next_request() noexcept {
    while (this->requestPosition < this->requestCount) {
        const std::size_t position = this->requestPosition++;
        const std::uint16_t tile = this->requests[position];

        if (this->states[tile] != TerrainTileState::requested) {
            continue;
        }

        this->states[tile] = TerrainTileState::loading;
        return TerrainRequest(tile, this->requestDistances[position], this->currentGeneration);
    }

    return std::nullopt;
}

bool Terrain::complete(const TerrainRequest& request) noexcept {
    const std::uint16_t tile = index(request.x(), request.y());

    if (request.generation() != this->currentGeneration || this->states[tile] != TerrainTileState::loading) {
        return false;
    }

    this->states[tile] = TerrainTileState::ready;
    return true;
}

bool Terrain::fail(const TerrainRequest& request) noexcept {
    const std::uint16_t tile = index(request.x(), request.y());

    if (request.generation() != this->currentGeneration || this->states[tile] != TerrainTileState::loading) {
        return false;
    }

    this->states[tile] = TerrainTileState::failed;
    return true;
}

bool Terrain::activate(const std::size_t x, const std::size_t y) noexcept {
    if (x >= Size || y >= Size) {
        return false;
    }

    TerrainTileState& state = this->states[index(x, y)];

    if (state == TerrainTileState::active) {
        return true;
    }

    if (state != TerrainTileState::ready) {
        return false;
    }

    state = TerrainTileState::active;
    return true;
}

bool Terrain::deactivate(const std::size_t x, const std::size_t y) noexcept {
    if (x >= Size || y >= Size) {
        return false;
    }

    TerrainTileState& state = this->states[index(x, y)];

    if (state != TerrainTileState::active) {
        return false;
    }

    state = TerrainTileState::ready;
    return true;
}

void Terrain::release(const std::size_t x, const std::size_t y) noexcept {
    if (x < Size && y < Size) {
        this->states[index(x, y)] = TerrainTileState::missing;
    }
}

void Terrain::reset() noexcept {
    this->states.fill(TerrainTileState::missing);
    this->requestPosition = 0;
    this->requestCount = 0;

    if (this->currentGeneration == std::numeric_limits<std::uint32_t>::max()) {
        this->currentGeneration = 1;
    } else {
        ++this->currentGeneration;
    }
}

TerrainTileState Terrain::state(const std::size_t x, const std::size_t y) const noexcept {
    return x < Size && y < Size ? this->states[index(x, y)] : TerrainTileState::missing;
}

std::size_t Terrain::pending() const noexcept {
    std::size_t count = 0;

    for (std::size_t position = this->requestPosition; position < this->requestCount; ++position) {
        count += this->states[this->requests[position]] == TerrainTileState::requested;
    }

    return count;
}

void Terrain::enqueue(const std::size_t x, const std::size_t y, const std::size_t distance) noexcept {
    TerrainTileState& state = this->states[index(x, y)];

    if (state != TerrainTileState::missing) {
        return;
    }

    state = TerrainTileState::requested;
    this->requests[this->requestCount] = index(x, y);
    this->requestDistances[this->requestCount] = static_cast<std::uint8_t>(distance);
    ++this->requestCount;
}

}
