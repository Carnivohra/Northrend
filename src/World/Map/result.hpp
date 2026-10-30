#pragma once

#include "World/Map/request.hpp"

#include <exception>
#include <memory>

namespace Northrend {

class Adt;

class TerrainResult final {
public:
    TerrainResult(TerrainRequest request, std::shared_ptr<const Adt> adt) noexcept;
    TerrainResult(TerrainRequest request, std::exception_ptr failure) noexcept;

    [[nodiscard]] const TerrainRequest& request() const noexcept;
    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] std::shared_ptr<const Adt> data() && noexcept;
    void rethrow_if_failed() const;

private:
    TerrainRequest terrainRequest;
    std::shared_ptr<const Adt> terrainData;
    std::exception_ptr terrainFailure;
};

}
