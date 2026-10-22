include_guard(GLOBAL)

function(northrend_add_game)
  set(options)
  set(one_value_arguments
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

  set(NORTHREND_GAME_APPLICATION "${GAME_APPLICATION}")
  set(NORTHREND_GAME_HEADER "${GAME_HEADER}")
  set(game_launcher
    "${CMAKE_CURRENT_BINARY_DIR}/${GAME_TARGET}-launcher.cpp")

  configure_file(
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../templates/Game.cpp.in"
    "${game_launcher}"
    @ONLY)

  add_executable(${GAME_TARGET})

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

  target_link_libraries(${GAME_TARGET}
    PRIVATE
      Northrend::Northrend)

  if(IPO_SUPPORTED)
    set_target_properties(${GAME_TARGET} PROPERTIES
      INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  endif()
endfunction()
