#include "World/Map/Object/loader.hpp"

#include "Asset/Format/Wmo/group.hpp"
#include "Asset/system.hpp"
#include "Thread/pool.hpp"
#include "Thread/work.hpp"
#include "World/Map/Object/asset.hpp"
#include "World/Map/Object/state.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <stdexcept>
#include <utility>

namespace Northrend {

MapObjectLoader::MapObjectLoader(AssetSystem& assets, ThreadPool& threadPool) noexcept :
    assets(assets), threadPool(threadPool) {}

MapObjectLoader::~MapObjectLoader() {
    this->stop();
}

MapObjectHandle MapObjectLoader::reference(
    const std::string_view asset, const AssetPriority priority, const std::uint32_t generation) {
    AssetId identifier(asset);
    const std::string identifierKey = key(identifier.path());
    const std::scoped_lock lock(this->referenceMutex);

    if (this->stopping || generation == 0 || generation != this->activeGeneration) {
        return {};
    }

    if (const auto position = this->active.find(identifierKey); position != this->active.end()) {
        if (std::shared_ptr<MapObjectState> state = position->second.lock()) {
            state->promote(priority);
            return MapObjectHandle(std::move(state));
        }

        this->active.erase(position);
    }

    auto state = std::make_shared<MapObjectState>(std::move(identifier), priority, generation);
    this->active.emplace(identifierKey, state);
    this->pending.push_back(state);
    return MapObjectHandle(std::move(state));
}

void MapObjectLoader::reset(const std::uint32_t generation) noexcept {
    {
        const std::scoped_lock lock(this->referenceMutex);
        this->activeGeneration = generation;

        for (const std::shared_ptr<MapObjectState>& state : this->pending) {
            state->cancel();
        }

        this->pending.clear();
        this->active.clear();
    }

    this->stop_entries();
}

void MapObjectLoader::tick() {
    if (this->stopping) {
        return;
    }

    this->receive_pending();
    std::size_t transitions = 0;

    for (auto position = this->entries.begin();
         position != this->entries.end() && transitions < TransitionsPerTick;) {
        Entry& entry = position->second;

        if (entry.state.use_count() == 1) {
            if (entry.work) {
                entry.work->cancel();

                try {
                    entry.work->wait();
                } catch (...) {
                }
            }

            entry.state->cancel();
            position = this->entries.erase(position);
            ++transitions;
            continue;
        }

        try {
            const AssetPriority requestedPriority = entry.state->priority();

            if (static_cast<std::uint8_t>(requestedPriority) > static_cast<std::uint8_t>(entry.priority)) {
                if (entry.phase == Phase::rootLoading) {
                    entry.rootRequest = this->assets.request(entry.state->asset(), requestedPriority);
                } else if (entry.phase == Phase::groupsLoading) {
                    for (std::size_t group = 0; group < entry.groupRequests.size(); ++group) {
                        entry.groupRequests[group] =
                            this->assets.request(group_asset(entry.state->asset(), group), requestedPriority);
                    }
                } else if (entry.work) {
                    static_cast<void>(this->threadPool.promote(entry.work, requestedPriority));
                }

                entry.priority = requestedPriority;
            }

            switch (entry.phase) {
            case Phase::rootLoading:
                if (!entry.rootRequest.ready()) {
                    ++position;
                    continue;
                }

                entry.work = this->threadPool.submit(entry.priority, [&entry](const std::stop_token stopToken) {
                    if (!stopToken.stop_requested()) {
                        entry.root = std::make_unique<Wmo>(entry.rootRequest.data());
                    }
                });
                entry.phase = Phase::rootParsing;
                break;
            case Phase::rootParsing:
                if (!entry.work->ready()) {
                    ++position;
                    continue;
                }

                entry.work->wait();
                entry.work.reset();
                entry.groupRequests.reserve(entry.root->header().groupCount);

                for (std::size_t group = 0; group < entry.root->header().groupCount; ++group) {
                    entry.groupRequests.push_back(
                        this->assets.request(group_asset(entry.state->asset(), group), entry.priority));
                }

                entry.phase = Phase::groupsLoading;
                break;
            case Phase::groupsLoading:
                if (!std::ranges::all_of(entry.groupRequests, &AssetRequest::ready)) {
                    ++position;
                    continue;
                }

                entry.work = this->threadPool.submit(entry.priority, [&entry](const std::stop_token stopToken) {
                    if (stopToken.stop_requested()) {
                        return;
                    }

                    std::vector<WmoGroup> groups;
                    groups.reserve(entry.groupRequests.size());

                    for (const AssetRequest& request : entry.groupRequests) {
                        if (stopToken.stop_requested()) {
                            return;
                        }

                        groups.emplace_back(request.data());
                    }

                    entry.result =
                        std::make_shared<MapObjectAsset>(std::move(entry.root), std::move(groups));
                });
                entry.phase = Phase::groupsParsing;
                break;
            case Phase::groupsParsing:
                if (!entry.work->ready()) {
                    ++position;
                    continue;
                }

                entry.work->wait();
                entry.state->complete(std::move(entry.result));
                position = this->entries.erase(position);
                ++transitions;
                continue;
            }
        } catch (...) {
            entry.state->fail(std::current_exception());
            position = this->entries.erase(position);
            ++transitions;
            continue;
        }

        ++position;
        ++transitions;
    }
}

void MapObjectLoader::stop() noexcept {
    {
        const std::scoped_lock lock(this->referenceMutex);

        if (this->stopping) {
            return;
        }

        this->stopping = true;
        this->activeGeneration = 0;

        for (const std::shared_ptr<MapObjectState>& state : this->pending) {
            state->cancel();
        }

        this->pending.clear();
        this->active.clear();
    }

    this->stop_entries();
}

std::string MapObjectLoader::key(const std::string_view asset) {
    std::string result(asset);
    std::ranges::transform(result, result.begin(), [](const unsigned char value) {
        return static_cast<char>(std::tolower(value));
    });
    return result;
}

AssetId MapObjectLoader::group_asset(const AssetId& root, const std::size_t group) {
    std::string path(root.path());

    if (path.size() < 4 || path[path.size() - 4] != '.' ||
        std::tolower(static_cast<unsigned char>(path[path.size() - 3])) != 'w' ||
        std::tolower(static_cast<unsigned char>(path[path.size() - 2])) != 'm' ||
        std::tolower(static_cast<unsigned char>(path[path.size() - 1])) != 'o') {
        throw std::invalid_argument("Map object root does not use the WMO extension");
    }

    path.resize(path.size() - 4);
    path.push_back('_');
    const std::string number = std::to_string(group);

    if (number.size() < 3) {
        path.append(3 - number.size(), '0');
    }

    path.append(number);
    path.append(".wmo");
    return AssetId(path);
}

void MapObjectLoader::receive_pending() {
    std::vector<std::shared_ptr<MapObjectState>> received;

    {
        const std::scoped_lock lock(this->referenceMutex);
        received.swap(this->pending);

        for (auto position = this->active.begin(); position != this->active.end();) {
            if (position->second.expired()) {
                position = this->active.erase(position);
                continue;
            }

            ++position;
        }
    }

    for (std::shared_ptr<MapObjectState>& state : received) {
        if (state->generation() != this->activeGeneration || state->ready()) {
            state->cancel();
            continue;
        }

        const std::string identifierKey = key(state->asset().path());
        const AssetPriority priority = state->priority();
        AssetRequest rootRequest = this->assets.request(state->asset(), priority);
        this->entries.emplace(identifierKey, Entry {
            .state = std::move(state),
            .rootRequest = std::move(rootRequest),
            .root = {},
            .groupRequests = {},
            .result = {},
            .work = {},
            .priority = priority,
            .phase = Phase::rootLoading
        });
    }
}

void MapObjectLoader::stop_entries() noexcept {
    for (auto& [identifier, entry] : this->entries) {
        if (entry.work) {
            entry.work->cancel();

            try {
                entry.work->wait();
            } catch (...) {
            }
        }

        entry.state->cancel();
    }

    this->entries.clear();
}

}
