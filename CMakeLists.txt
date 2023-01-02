cmake_minimum_required(VERSION 3.11)
project(SDL_libretro
    DESCRIPTION "SDL_libretro: libretro frontend using SDL."
    HOMEPAGE_URL "https://github.com/robloach/SDL_libretro"
    VERSION 0.0.1
    LANGUAGES C
)

set(CMAKE_EXE_EXPORTS_C_FLAG "-Wl,--export-dynamic") # Shared libraries with no builtin soname may not be linked safely by.

# Options
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    set(SDL_LIBRETRO_IS_MAIN TRUE)
else()
    set(SDL_LIBRETRO_IS_MAIN FALSE)
endif()
option(SDL_LIBRETRO_BUILD "Examples" ${SDL_LIBRETRO_IS_MAIN})

# SDL_libretro
add_library(SDL_libretro_h INTERFACE)
target_include_directories(SDL_libretro_h
    INTERFACE include
    INTERFACE vendor/libretro-common/include
)

# Examples
if (SDL_LIBRETRO_BUILD)
    add_subdirectory(bin)

    # Testing
    #include(CTest)
    #enable_testing()
    #if (BUILD_TESTING)
        # set(CTEST_CUSTOM_TESTS_IGNORE
        #     pkg-config--static
        # )
    #    add_subdirectory(test)
    #endif()
endif()