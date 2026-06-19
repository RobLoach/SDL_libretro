cmake_minimum_required(VERSION 3.16)
project(SDL_libretro
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "SDL3-based libretro frontend library"
)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(SDL3 REQUIRED)

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
endif()

# Tests
option(SDL_LIBRETRO_BUILD_TESTS "Build unit tests" ON)
if(SDL_LIBRETRO_BUILD_TESTS)
    enable_testing()
    add_executable(sdl_libretro_test test/sdl_libretro_test.c)
    target_link_libraries(sdl_libretro_test PRIVATE SDL_libretro SDL3::SDL3_test)
    add_test(NAME sdl_libretro_test COMMAND sdl_libretro_test)
endif()

# Install (only when this is the top-level project)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    install(TARGETS SDL_libretro EXPORT SDL_libretroTargets)
    install(FILES include/SDL_libretro.h DESTINATION include)
    # Implementation fragments + the libretro API header are part of the
    # shipped header set (needed by the TU that defines the macro).
    install(DIRECTORY include/ DESTINATION include
        FILES_MATCHING PATTERN "SDL_libretro_*.h")
    install(FILES vendor/libretro-common/include/libretro.h DESTINATION include)
endif()
