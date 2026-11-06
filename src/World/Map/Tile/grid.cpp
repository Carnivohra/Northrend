#include "World/Map/Tile/grid.hpp"

#include "World/Map/layout.hpp"
#include "World/Map/Tile/focus.hpp"
#include "World/Map/Tile/result.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace Northrend {

TileGrid::TileGrid(const std::uint32_t generation, std::shared_ptr<const MapLayout> layout) noexcept :
    mapLayout(std::move(layout)), currentGeneration(generation == 0 ? 1 : generation) {}

std::vector<std::pair<std::size_t, std::size_t>> TileGrid::focus(const TileFocus& focus) {
    std::array<bool, TileCount> desired {};
    std::vector<std::pair<std::size_t, std::size_t>> released;
    this->requestPosition = 0;
    this->requestCount = 0;

    if (focus.active()) {
        const std::size_t distanceLimit = std::min(focus.radius(), Size - 1);
        const std::size_t squaredDistanceLimit = distanceLimit * distanceLimit;

        for (std::size_t y = 0; y < Size; ++y) {
            for (std::size_t x = 0; x < Size; ++x) {
                const std::size_t deltaX = x > focus.x() ? x - focus.x() : focus.x() - x;
                const std::size_t deltaY = y > focus.y() ? y - focus.y() : focus.y() - y;
                const std::size_t squaredDistance = deltaX * deltaX + deltaY * deltaY;
                desired[index(x, y)] =
                    squaredDistance <= squaredDistanceLimit && this->mapLayout->has_tile(x, y);
            }
        }
    }

    for (std::size_t y = 0; y < Size; ++y) {
        for (std::size_t x = 0; x < Size; ++x) {
            const std::uint16_t tile = index(x, y);
            TileState& state = this->states[tile];

            if (desired[tile]) {
                if (state == TileState::requested || state == TileState::failed) {
                    state = TileState::missing;
                }

                continue;
            }

            if (state == TileState::ready || state == TileState::active) {
                this->tiles[tile].reset();
                released.emplace_back(x, y);
            }

            state = TileState::missing;
        }
    }

    if (!focus.active()) {
        return released;
    }

    const std::size_t x = focus.x();
    const std::size_t y = focus.y();
    const std::size_t distanceLimit = std::min(focus.radius(), Size - 1);

    for (std::size_t distance = 0; distance <= distanceLimit; ++distance) {
        const std::size_t minimumX = x > distance ? x - distance : 0;
        const std::size_t maximumX = std::min(x + distance, Size - 1);
        const std::size_t minimumY = y > distance ? y - distance : 0;
        const std::size_t maximumY = std::min(y + distance, Size - 1);
        const std::size_t outerSquared = distance * distance;
        const std::size_t innerSquared = distance == 0 ? 0 : (distance - 1) * (distance - 1);

        for (std::size_t tileY = minimumY; tileY <= maximumY; ++tileY) {
            for (std::size_t tileX = minimumX; tileX <= maximumX; ++tileX) {
                const std::size_t deltaX = tileX > x ? tileX - x : x - tileX;
                const std::size_t deltaY = tileY > y ? tileY - y : y - tileY;
                const std::size_t squaredDistance = deltaX * deltaX + deltaY * deltaY;

                if (squaredDistance > outerSquared || (distance != 0 && squaredDistance <= innerSquared)) {
                    continue;
                }

                this->enqueue(tileX, tileY, distance);
            }
        }
    }

    return released;
}

std::optional<TileRequest> TileGrid::next_request() noexcept {
    while (this->requestPosition < this->requestCount) {
        const std::size_t position = this->requestPosition++;
        const std::uint16_t tile = this->requests[position];

        if (this->states[tile] != TileState::requested) {
            continue;
        }

        this->states[tile] = TileState::loading;
        return TileRequest(
            tile, this->requestDistances[position], this->currentGeneration, this->mapLayout->high_resolution_alpha());
    }

    return std::nullopt;
}

bool TileGrid::defer(const TileRequest& request) noexcept {
    if (request.generation() != this->currentGeneration || this->requestPosition == 0) {
        return false;
    }

    const std::uint16_t tile = index(request.x(), request.y());

    if (this->states[tile] != TileState::loading || this->requests[this->requestPosition - 1] != tile) {
        return false;
    }

    --this->requestPosition;
    this->states[tile] = TileState::requested;
    return true;
}

bool TileGrid::complete(TileResult result) noexcept {
    const TileRequest& request = result.request();
    const std::uint16_t tile = index(request.x(), request.y());

    if (request.generation() != this->currentGeneration || this->states[tile] != TileState::loading) {
        return false;
    }

    if (!result.succeeded()) {
        this->states[tile] = TileState::failed;
        return true;
    }

    this->tiles[tile] = std::move(result).data();
    this->states[tile] = TileState::ready;
    return true;
}

bool TileGrid::fail(const TileRequest& request) noexcept {
    const std::uint16_t tile = index(request.x(), request.y());

    if (request.generation() != this->currentGeneration || this->states[tile] != TileState::loading) {
        return false;
    }

    this->states[tile] = TileState::failed;
    return true;
}

bool TileGrid::activate(const std::size_t x, const std::size_t y) noexcept {
    if (x >= Size || y >= Size) {
        return false;
    }

    TileState& state = this->states[index(x, y)];

    if (state == TileState::active) {
        return true;
    }

    if (state != TileState::ready) {
        return false;
    }

    state = TileState::active;
    return true;
}

bool TileGrid::deactivate(const std::size_t x, const std::size_t y) noexcept {
    if (x >= Size || y >= Size) {
        return false;
    }

    TileState& state = this->states[index(x, y)];

    if (state != TileState::active) {
        return false;
    }

    state = TileState::ready;
    return true;
}

void TileGrid::release(const std::size_t x, const std::size_t y) noexcept {
    if (x < Size && y < Size) {
        const std::uint16_t tile = index(x, y);
        this->tiles[tile].reset();
        this->states[tile] = TileState::missing;
    }
}

void TileGrid::reset() noexcept {
    this->states.fill(TileState::missing);
    this->tiles.fill(nullptr);
    this->requestPosition = 0;
    this->requestCount = 0;

    if (this->currentGeneration == std::numeric_limits<std::uint32_t>::max()) {
        this->currentGeneration = 1;
    } else {
        ++this->currentGeneration;
    }
}

TileState TileGrid::state(const std::size_t x, const std::size_t y) const noexcept {
    return x < Size && y < Size ? this->states[index(x, y)] : TileState::missing;
}

const MapTile* TileGrid::tile(const std::size_t x, const std::size_t y) const noexcept {
    return x < Size && y < Size ? this->tiles[index(x, y)].get() : nullptr;
}

std::size_t TileGrid::pending() const noexcept {
    std::size_t count = 0;

    for (std::size_t position = this->requestPosition; position < this->requestCount; ++position) {
        count += this->states[this->requests[position]] == TileState::requested;
    }

    return count;
}

std::uint32_t TileGrid::generation() const noexcept {
    return this->currentGeneration;
}

void TileGrid::enqueue(const std::size_t x, const std::size_t y, const std::size_t distance) noexcept {
    if (this->mapLayout && !this->mapLayout->has_tile(x, y)) {
        return;
    }

    TileState& state = this->states[index(x, y)];

    if (state != TileState::missing) {
        return;
    }

    state = TileState::requested;
    this->requests[this->requestCount] = index(x, y);
    this->requestDistances[this->requestCount] = static_cast<std::uint8_t>(distance);
    ++this->requestCount;
}

}
