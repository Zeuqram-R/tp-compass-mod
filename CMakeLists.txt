cmake_minimum_required(VERSION 3.25)
project(tp_compass CXX)
set(CMAKE_CXX_STANDARD 20)

# Two ways to build this mod:
#  1. Standalone, against the Dusklight mod SDK (produces build/mods/tp_compass.dusk):
#       cmake -B build -DDUSK_DIR=/path/to/dusklight && cmake --build build
#  2. In-tree, copied into dusklight/mods/tp_compass and added with add_subdirectory().
#     This is how the iOS build bundles the mod inside the app (see .github/workflows).
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(DUSK_DIR "" CACHE PATH "Path to the Dusklight source tree")
    if (NOT DUSK_DIR)
        message(FATAL_ERROR "Set DUSK_DIR to a Dusklight checkout (tag v2.0.3)")
    endif ()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_subdirectory("${DUSK_DIR}/sdk" dusk-sdk EXCLUDE_FROM_ALL)
endif ()

option(TP_COMPASS_BUNDLE "Bundle the mod inside the Dusklight app (required on iOS)" OFF)
set(_tp_compass_bundle)
if (TP_COMPASS_BUNDLE)
    set(_tp_compass_bundle BUNDLE)
endif ()

add_mod(tp_compass
    FEATURES game fmt
    SOURCES
        src/mod.cpp
        src/settings.cpp
        src/game_state.cpp
        src/pins.cpp
        src/compass_draw.cpp
        src/ui_panel.cpp
        src/input.cpp
        src/chests.cpp
        src/location.cpp
    MOD_JSON mod.json
    ${_tp_compass_bundle}
)
