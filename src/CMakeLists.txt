add_library(Northrend STATIC)
add_library(Northrend::Northrend ALIAS Northrend)

target_sources(Northrend
  PRIVATE
    App/app.cpp
    Backend/session.cpp
    Backend/session.hpp
    ECS/registry.cpp
    Render/camera.cpp
    World/camera.cpp
    World/simulation.cpp
    World/world.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
      App/app.hpp
      App/launch.hpp
      Backend/application.hpp
      Backend/backend.hpp
      Backend/Window/handle.hpp
      Backend/Window/size.hpp
      Backend/Window/window.hpp
      ECS/entity.hpp
      ECS/registry.hpp
      Engine/engine.hpp
      Engine/fps.hpp
      Math/matrix.hpp
      Math/quaternion.hpp
      Math/vector.hpp
      Render/camera.hpp
      Render/exchange.hpp
      Render/frame.hpp
      Render/renderer.hpp
      Render/snapshot.hpp
      Render/surface.hpp
      Thread/rate.hpp
      Thread/runner.hpp
      Thread/task.hpp
      World/camera.hpp
      World/simulation.hpp
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
    Northrend::CompileOptions)
