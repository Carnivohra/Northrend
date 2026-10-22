# Register SDL3 only when selected by the active platform profile.

target_sources(Northrend
  PRIVATE
    Backend/SDL3/context.cpp
    Backend/SDL3/context.hpp
    Backend/SDL3/sdl3.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
      Backend/SDL3/sdl3.hpp)

target_link_libraries(Northrend
  PRIVATE
    SDL3::SDL3)
