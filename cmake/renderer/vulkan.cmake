# Combine the Vulkan core with the surface implementation from the platform profile.

if(NOT NORTHREND_VULKAN_PLATFORM_SOURCES)
  message(FATAL_ERROR "Vulkan is not implemented for ${CMAKE_SYSTEM_NAME}")
endif()

function(northrend_embed_spirv source symbol output_name)
  set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
  set(output_directory "${CMAKE_CURRENT_BINARY_DIR}/generated/Render/Vulkan/Shader")
  set(output_path "${output_directory}/${output_name}.hpp")

  file(READ "${source_path}" shader_hex HEX)
  string(LENGTH "${shader_hex}" shader_hex_length)
  math(EXPR shader_size "${shader_hex_length} / 2")
  string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " shader_bytes "${shader_hex}")

  set(SHADER_SIZE "${shader_size}")
  set(SHADER_SYMBOL "${symbol}")
  set(SHADER_BYTES "${shader_bytes}")

  file(MAKE_DIRECTORY "${output_directory}")
  configure_file(
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/shader.hpp.in"
    "${output_path}"
    @ONLY)

  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source_path}")
  target_sources(Northrend PRIVATE "${source_path}" "${output_path}")
endfunction()

target_sources(Northrend
  PRIVATE
    Render/Vulkan/buffer.cpp
    Render/Vulkan/buffer.hpp
    Render/Vulkan/depth.cpp
    Render/Vulkan/depth.hpp
    Render/Vulkan/device.cpp
    Render/Vulkan/device.hpp
    Render/Vulkan/frame.cpp
    Render/Vulkan/frame.hpp
    Render/Vulkan/mesh.cpp
    Render/Vulkan/mesh.hpp
    Render/Vulkan/pipeline.cpp
    Render/Vulkan/pipeline.hpp
    Render/Vulkan/Platform/surface.hpp
    Render/Vulkan/Shader/triangle.frag
    Render/Vulkan/Shader/triangle.vert
    Render/Vulkan/swapchain.cpp
    Render/Vulkan/swapchain.hpp
    Render/Vulkan/triangle.cpp
    Render/Vulkan/triangle.hpp
    Render/Vulkan/vulkan.cpp
    ${NORTHREND_VULKAN_PLATFORM_SOURCES}
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
      Render/Vulkan/vulkan.hpp)

northrend_embed_spirv(
  Render/Vulkan/Shader/triangle.vert.spv
  TriangleVertex
  triangle_vertex)
northrend_embed_spirv(
  Render/Vulkan/Shader/triangle.frag.spv
  TriangleFragment
  triangle_fragment)

target_include_directories(Northrend
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated")

target_compile_definitions(Northrend
  PRIVATE
    ${NORTHREND_VULKAN_PLATFORM_DEFINITIONS}
    $<$<CONFIG:Debug>:NORTHREND_VULKAN_VALIDATION>)

target_compile_definitions(volk
  PRIVATE
    ${NORTHREND_VULKAN_PLATFORM_DEFINITIONS})

target_link_libraries(Northrend
  PRIVATE
    volk::volk)
