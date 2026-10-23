# Build only the dependencies selected by the active platform profile.

foreach(dependency
    "${NORTHREND_BACKEND}"
    "${NORTHREND_RENDERER}")
  if(NOT dependency)
    continue()
  endif()

  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dependency}/CMakeLists.txt")
    message(FATAL_ERROR "Missing dependency module for ${dependency}")
  endif()

  add_subdirectory("${dependency}")
endforeach()

add_subdirectory(Flecs)
add_subdirectory(GLM)
