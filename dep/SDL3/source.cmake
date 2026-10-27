include_guard(GLOBAL)

include(FetchContent)

# Share the pinned archive between Android packaging and native compilation.

function(northrend_declare_sdl3)
  FetchContent_Declare(SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
    URL_HASH SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    ${ARGN})
endfunction()
