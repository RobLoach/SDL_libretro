cmake_minimum_required(VERSION 3.16)
project(SDL_libretro
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "SDL3-based libretro frontend library"
)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(SDL3 REQUIRED)

# Core downloads for development/testing
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(SDL_LIBRETRO_DOWNLOAD_CORES "Download libretro cores for testing" ON)
else()
    option(SDL_LIBRETRO_DOWNLOAD_CORES "Download libretro cores for testing" OFF)
endif()

if(NOT DEFINED CORES_DIR)
    set(CORES_DIR "${CMAKE_BINARY_DIR}/cores")
endif()

if(SDL_LIBRETRO_DOWNLOAD_CORES)
    include(DownloadCores)
endif()

# Header-only (INTERFACE) library. Consumers define SDL_LIBRETRO_IMPLEMENTATION
# in exactly one translation unit before including <SDL_libretro.h>.
add_library(SDL_libretro INTERFACE)

target_include_directories(SDL_libretro INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/libretro-common/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(SDL_libretro INTERFACE SDL3::SDL3-static ${CMAKE_DL_LIBS})
target_compile_features(SDL_libretro INTERFACE c_std_99)

# Example
option(SDL_LIBRETRO_BUILD_EXAMPLES "Build example applications" ON)
if(SDL_LIBRETRO_BUILD_EXAMPLES)
    add_executable(SDL_libretro_basic example/SDL_libretro_basic.c)
    target_link_libraries(SDL_libretro_basic PRIVATE SDL_libretro)

    add_executable(SDL_libretro_demo example/SDL_libretro_demo.c)
    target_link_libraries(SDL_libretro_demo PRIVATE SDL_libretro)
    target_include_directories(SDL_libretro_demo PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nuklear_console/vendor/Nuklear
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nuklear_console/vendor/Nuklear/demo/sdl3_renderer
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nuklear_console/vendor/nuklear_gamepad
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nuklear_console/vendor/c-vector
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nuklear_console
    )
    target_compile_definitions(SDL_libretro_demo PRIVATE
        NK_GAMEPAD_SDL3
    )
endif()

# Tests
option(SDL_LIBRETRO_BUILD_TESTS "Build unit tests" ON)
if(SDL_LIBRETRO_BUILD_TESTS)
    enable_testing()

    add_library(test_core SHARED test/test_core.c)
    target_include_directories(test_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libretro-common/include)
    set_target_properties(test_core PROPERTIES PREFIX "" SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})

    add_executable(SDL_libretro_test test/SDL_libretro_test.c)
    target_link_libraries(SDL_libretro_test PRIVATE SDL_libretro SDL3::SDL3_test)
    target_compile_definitions(SDL_libretro_test PRIVATE
        TEST_CORE_PATH="$<TARGET_FILE:test_core>"
        TEST_CONTENT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test/test_content.txt")
    add_test(NAME SDL_libretro_test COMMAND SDL_libretro_test)
endif()

# Install (only when this is the top-level project)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    install(TARGETS SDL_libretro EXPORT SDL_libretroTargets)

    # Implementation fragments + the libretro API header are part of the
    # shipped header set (needed by the TU that defines the macro).
    install(
        DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "SDL_*.h"
    )
    install(FILES vendor/libretro-common/include/libretro.h DESTINATION include)
endif()

if(EMSCRIPTEN)
    if(NOT SDL_LIBRETRO_DOWNLOAD_CORES)
        message(FATAL_ERROR "Emscripten builds require SDL_LIBRETRO_DOWNLOAD_CORES=ON: "
            "cores must be downloaded so they can be preloaded into the virtual filesystem.")
    endif()
    include(EmscriptenConfig)
endif()
