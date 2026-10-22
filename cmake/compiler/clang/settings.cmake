# Share strict warnings with the engine and generated game targets.

target_compile_options(northrend-compile-options
  INTERFACE
    -Wall
    -Wextra
    -Wpedantic)
