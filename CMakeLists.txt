cmake_minimum_required(VERSION 3.16)
project(SDL_libretro
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "SDL3-based libretro frontend library"
)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(SDL3 REQUIRED)

add_library(SDL_libretro STATIC
    src/SDL_libretro.c
    src/SDL_libretro_video.c
    src/SDL_libretro_audio.c
    src/SDL_libretro_input.c
    src/SDL_libretro_env.c
    src/SDL_libretro_options.c
    src/SDL_libretro_serialize.c
)

target_include_directories(SDL_libretro PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(SDL_libretro PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libretro-common/include
)

target_link_libraries(SDL_libretro PUBLIC SDL3::SDL3-static)
target_link_libraries(SDL_libretro PRIVATE ${CMAKE_DL_LIBS})
target_compile_features(SDL_libretro PRIVATE c_std_99)

# Example
option(SDL_LIBRETRO_BUILD_EXAMPLES "Build example applications" ON)
if(SDL_LIBRETRO_BUILD_EXAMPLES)
    add_executable(SDL_libretro_basic example/SDL_libretro_basic.c)
    target_link_libraries(SDL_libretro_basic PRIVATE SDL_libretro)
endif()

# Install (only when this is the top-level project)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    install(TARGETS SDL_libretro)
    install(FILES include/SDL_libretro.h DESTINATION include)
endif()
