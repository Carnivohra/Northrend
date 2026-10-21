if(NORTHREND_BACKEND STREQUAL "SDL3")
  add_subdirectory(SDL3)
endif()

if(NORTHREND_RENDERER STREQUAL "Vulkan")
  add_subdirectory(Vulkan)
endif()
