add_library(Northrend STATIC)
add_library(Northrend::Northrend ALIAS Northrend)

target_sources(Northrend
  PRIVATE
    App/app.cpp
    Asset/Chunk/reader.cpp
    Asset/Dbc/format.cpp
    Asset/Dbc/storage.cpp
    Asset/Dbc/table.cpp
    Asset/Stream/request.cpp
    Asset/Stream/state.cpp
    Asset/Stream/streamer.cpp
    Asset/Stream/state.hpp
    Asset/adt.cpp
    Asset/data.cpp
    Asset/directory.cpp
    Asset/id.cpp
    Asset/mpq.cpp
    Asset/storage.cpp
    Asset/system.cpp
    Asset/wdt.cpp
    Backend/backend.cpp
    Config/config.cpp
    Config/wtf.cpp
    ECS/registry.cpp
    Math/Glm/matrix.cpp
    Math/Glm/matrix.hpp
    Math/matrix.cpp
    Presentation/map.cpp
    Render/camera.cpp
    Render/Mesh/allocator.cpp
    Render/scene.cpp
    Simulation/runtime.cpp
    Simulation/simulation.cpp
    Thread/pool.cpp
    Thread/worker.cpp
    Thread/work.cpp
    World/Map/catalog.cpp
    World/Map/entry.cpp
    World/Map/exchange.cpp
    World/Map/layout.cpp
    World/Map/loader.cpp
    World/Map/map.cpp
    World/Map/result.cpp
    World/Map/selection.cpp
    World/Map/Terrain/geometry.cpp
    World/Map/Terrain/tile.cpp
    World/Map/Tile/change.cpp
    World/Map/Tile/grid.cpp
    World/Map/Tile/job.cpp
    World/Map/Tile/request.cpp
    World/Map/Tile/result.cpp
    World/Map/Tile/tile.cpp
    World/world.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
      App/app.hpp
      App/launcher.hpp
      Asset/Chunk/chunk.hpp
      Asset/Chunk/id.hpp
      Asset/Chunk/reader.hpp
      Asset/Dbc/format.hpp
      Asset/Dbc/storage.hpp
      Asset/Dbc/table.hpp
      Asset/Stream/priority.hpp
      Asset/Stream/request.hpp
      Asset/Stream/streamer.hpp
      Asset/adt.hpp
      Asset/data.hpp
      Asset/directory.hpp
      Asset/id.hpp
      Asset/mount.hpp
      Asset/mpq.hpp
      Asset/plan.hpp
      Asset/source.hpp
      Asset/storage.hpp
      Asset/system.hpp
      Asset/wdt.hpp
      Backend/application.hpp
      Backend/backend.hpp
      Backend/plan.hpp
      Backend/Window/Handle/handle.hpp
      Backend/Window/window.hpp
      Config/config.hpp
      Config/plan.hpp
      Config/wtf.hpp
      ECS/entity.hpp
      ECS/registry.hpp
      Engine/engine.hpp
      Engine/fps.hpp
      Engine/plan.hpp
      App/plan.hpp
      Math/matrix.hpp
      Math/quaternion.hpp
      Math/vector.hpp
      Presentation/exchange.hpp
      Presentation/presentation.hpp
      Presentation/runtime.hpp
      Presentation/map.hpp
      Render/camera.hpp
      Render/frame.hpp
      Render/instance.hpp
      Render/Mesh/allocator.hpp
      Render/Mesh/mesh.hpp
      Render/renderer.hpp
      Render/scene.hpp
      Simulation/runtime.hpp
      Simulation/simulation.hpp
      Thread/pool.hpp
      Thread/mailbox.hpp
      Thread/runner.hpp
      Thread/task.hpp
      Thread/worker.hpp
      Thread/work.hpp
      World/Map/catalog.hpp
      World/Map/entry.hpp
      World/Map/exchange.hpp
      World/Map/layout.hpp
      World/Map/loader.hpp
      World/Map/map.hpp
      World/Map/result.hpp
      World/Map/selection.hpp
      World/Map/Terrain/geometry.hpp
      World/Map/Terrain/tile.hpp
      World/Map/Tile/change.hpp
      World/Map/Tile/grid.hpp
      World/Map/Tile/job.hpp
      World/Map/Tile/request.hpp
      World/Map/Tile/result.hpp
      World/Map/Tile/tile.hpp
      World/registry.hpp
      World/snapshot.hpp
      World/world.hpp
      ${NORTHREND_PLATFORM_HEADERS})

# Component modules own their sources, dependencies, and compile definitions.

if(NORTHREND_BACKEND)
  string(TOLOWER "${NORTHREND_BACKEND}" backend_name)
  include("${CMAKE_CURRENT_LIST_DIR}/../cmake/backend/${backend_name}.cmake")
endif()

if(NORTHREND_RENDERER)
  string(TOLOWER "${NORTHREND_RENDERER}" renderer_name)
  include("${CMAKE_CURRENT_LIST_DIR}/../cmake/renderer/${renderer_name}.cmake")
endif()

target_compile_features(Northrend
  PUBLIC
    cxx_std_20)

if(NORTHREND_IPO_SUPPORTED)
  set_target_properties(Northrend PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()

target_link_libraries(Northrend
  PUBLIC
    flecs::flecs_static
  PRIVATE
    glm::glm
    StormLib::storm
    Threads::Threads
    Northrend::CompileOptions)
