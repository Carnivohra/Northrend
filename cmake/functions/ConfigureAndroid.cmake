include_guard(GLOBAL)

# Generate the reusable Android package around a concrete game target.

function(northrend_configure_android_game)
  set(options)
  set(one_value_arguments
    APP_NAME
    COMPILE_SDK
    ICON
    MIN_SDK
    NDK_VERSION
    PACKAGE
    TARGET
    TARGET_SDK
    VERSION_CODE
    VERSION_NAME)

  cmake_parse_arguments(
    ANDROID_GAME
    "${options}"
    "${one_value_arguments}"
    ""
    ${ARGN})

  foreach(argument
      APP_NAME
      PACKAGE
      TARGET
      VERSION_CODE
      VERSION_NAME)
    if(NOT ANDROID_GAME_${argument})
      message(FATAL_ERROR
        "Android packaging for ${ANDROID_GAME_TARGET} requires ${argument}")
    endif()
  endforeach()

  if(NOT ANDROID_GAME_PACKAGE MATCHES
     "^[A-Za-z][A-Za-z0-9_]*(\.[A-Za-z][A-Za-z0-9_]*)+$")
    message(FATAL_ERROR
      "Invalid Android package name: ${ANDROID_GAME_PACKAGE}")
  endif()

  if(NOT ANDROID_GAME_COMPILE_SDK)
    set(ANDROID_GAME_COMPILE_SDK 35)
  endif()

  if(NOT ANDROID_GAME_MIN_SDK)
    set(ANDROID_GAME_MIN_SDK 24)
  endif()

  if(NOT ANDROID_GAME_NDK_VERSION)
    set(ANDROID_GAME_NDK_VERSION "28.2.13676358")
  endif()

  if(NOT ANDROID_GAME_TARGET_SDK)
    set(ANDROID_GAME_TARGET_SDK 35)
  endif()

  get_filename_component(engine_source
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.."
    ABSOLUTE)

  set(template_dir "${engine_source}/platform/android")
  set(output_dir "${CMAKE_BINARY_DIR}/android")

  file(MAKE_DIRECTORY "${output_dir}")
  file(COPY "${template_dir}/"
    DESTINATION "${output_dir}"
    PATTERN "*.in" EXCLUDE)

  set(NORTHREND_ANDROID_APP_NAME "${ANDROID_GAME_APP_NAME}")
  set(NORTHREND_ANDROID_CMAKE_VERSION "${CMAKE_VERSION}")
  set(NORTHREND_ANDROID_COMPILE_SDK "${ANDROID_GAME_COMPILE_SDK}")
  set(NORTHREND_ANDROID_ENGINE_SOURCE_DIR "${engine_source}")
  set(NORTHREND_ANDROID_GAME_SOURCE_DIR "${CMAKE_SOURCE_DIR}")
  set(NORTHREND_ANDROID_MIN_SDK "${ANDROID_GAME_MIN_SDK}")
  set(NORTHREND_ANDROID_NDK_VERSION "${ANDROID_GAME_NDK_VERSION}")
  set(NORTHREND_ANDROID_PACKAGE "${ANDROID_GAME_PACKAGE}")
  set(NORTHREND_ANDROID_TARGET_SDK "${ANDROID_GAME_TARGET_SDK}")
  set(NORTHREND_ANDROID_VERSION_CODE "${ANDROID_GAME_VERSION_CODE}")
  set(NORTHREND_ANDROID_VERSION_NAME "${ANDROID_GAME_VERSION_NAME}")

  if(ANDROID_GAME_ICON)
    get_filename_component(icon
      "${ANDROID_GAME_ICON}"
      ABSOLUTE
      BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

    if(NOT EXISTS "${icon}")
      message(FATAL_ERROR "Android icon does not exist: ${icon}")
    endif()

    file(MAKE_DIRECTORY "${output_dir}/app/src/main/res/drawable")
    file(COPY_FILE
      "${icon}"
      "${output_dir}/app/src/main/res/drawable/app_icon.png"
      ONLY_IF_DIFFERENT)
    set(NORTHREND_ANDROID_ICON_RESOURCE "@drawable/app_icon")
  else()
    set(NORTHREND_ANDROID_ICON_RESOURCE "@mipmap/ic_launcher")
  endif()

  configure_file(
    "${template_dir}/settings.gradle.in"
    "${output_dir}/settings.gradle"
    @ONLY)
  configure_file(
    "${template_dir}/app/build.gradle.in"
    "${output_dir}/app/build.gradle"
    @ONLY)
  configure_file(
    "${template_dir}/app/src/main/AndroidManifest.xml.in"
    "${output_dir}/app/src/main/AndroidManifest.xml"
    @ONLY)
  configure_file(
    "${template_dir}/app/src/main/res/values/strings.xml.in"
    "${output_dir}/app/src/main/res/values/strings.xml"
    @ONLY)

  message(STATUS
    "Generated ${ANDROID_GAME_TARGET} Android project: ${output_dir}")
endfunction()
