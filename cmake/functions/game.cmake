include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/android.cmake")

# Create a game target whose concrete application type is known at compile time.

function(northrend_add_game)
  set(options)
  set(one_value_arguments
    ANDROID_APP_NAME
    ANDROID_BUILD_TOOLS
    ANDROID_COMPILE_SDK
    ANDROID_ICON
    ANDROID_MIN_SDK
    ANDROID_NDK_VERSION
    ANDROID_PACKAGE
    ANDROID_TARGET_SDK
    ANDROID_VERSION_CODE
    ANDROID_VERSION_NAME
    APPLICATION
    HEADER
    TARGET)
  set(multi_value_arguments
    SOURCES)

  cmake_parse_arguments(
    GAME
    "${options}"
    "${one_value_arguments}"
    "${multi_value_arguments}"
    ${ARGN})

  if(GAME_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unknown arguments for northrend_add_game: ${GAME_UNPARSED_ARGUMENTS}")
  endif()

  foreach(argument
      APPLICATION
      HEADER
      TARGET)
    if(NOT GAME_${argument})
      message(FATAL_ERROR
        "northrend_add_game requires ${argument}")
    endif()
  endforeach()

  if(NOT GAME_SOURCES)
    message(FATAL_ERROR "northrend_add_game requires SOURCES")
  endif()

  if(NORTHREND_ANDROID_BOOTSTRAP)
    if(NOT GAME_ANDROID_PACKAGE)
      message(FATAL_ERROR
        "Android builds require ANDROID_PACKAGE for ${GAME_TARGET}")
    endif()

    northrend_configure_android_game(
      TARGET "${GAME_TARGET}"
      APP_NAME "${GAME_ANDROID_APP_NAME}"
      PACKAGE "${GAME_ANDROID_PACKAGE}"
      VERSION_CODE "${GAME_ANDROID_VERSION_CODE}"
      VERSION_NAME "${GAME_ANDROID_VERSION_NAME}"
      ICON "${GAME_ANDROID_ICON}"
      BUILD_TOOLS "${GAME_ANDROID_BUILD_TOOLS}"
      COMPILE_SDK "${GAME_ANDROID_COMPILE_SDK}"
      MIN_SDK "${GAME_ANDROID_MIN_SDK}"
      NDK_VERSION "${GAME_ANDROID_NDK_VERSION}"
      TARGET_SDK "${GAME_ANDROID_TARGET_SDK}")

    return()
  endif()

  # Generate the minimal native entry point for the selected application type.

  set(NORTHREND_GAME_APPLICATION "${GAME_APPLICATION}")
  set(NORTHREND_GAME_HEADER "${GAME_HEADER}")
  set(game_launcher
    "${CMAKE_CURRENT_BINARY_DIR}/${GAME_TARGET}-launcher.cpp")

  configure_file(
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../templates/game.cpp.in"
    "${game_launcher}"
    @ONLY)

  if(ANDROID)
    add_library(${GAME_TARGET} SHARED)
    set_target_properties(${GAME_TARGET} PROPERTIES
      OUTPUT_NAME main)
  else()
    add_executable(${GAME_TARGET})
    set_target_properties(${GAME_TARGET} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  endif()

  target_sources(${GAME_TARGET}
    PRIVATE
      ${game_launcher}
      ${GAME_SOURCES})

  target_include_directories(${GAME_TARGET}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR})

  target_compile_features(${GAME_TARGET}
    PRIVATE
      cxx_std_20)

  # Apply Northrend's compiler policy to every generated game target.

  target_link_libraries(${GAME_TARGET}
    PRIVATE
      Northrend::CompileOptions
      Northrend::Northrend)

  if(NORTHREND_IPO_SUPPORTED)
    set_target_properties(${GAME_TARGET} PROPERTIES
      INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  endif()
endfunction()
