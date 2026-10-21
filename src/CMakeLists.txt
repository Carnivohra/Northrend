add_library(Northrend STATIC)
add_library(Northrend::Northrend ALIAS Northrend)

target_sources(Northrend
  PRIVATE
    App/app.cpp
    Backend/SDL3/sdl3.cpp
    Engine/engine.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
      App/app.hpp
      Backend/application.hpp
      Backend/backend.hpp
      Backend/SDL3/sdl3.hpp
      Engine/engine.hpp)

target_compile_features(Northrend
  PUBLIC
    cxx_std_20)

if(IPO_SUPPORTED)
  set_target_properties(Northrend PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()

target_link_libraries(Northrend
  PRIVATE
    Northrend::CompileOptions
    SDL3::SDL3)
