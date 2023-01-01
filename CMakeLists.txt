cmake_minimum_required(VERSION 3.11)
project(SDL_libretro
    DESCRIPTION "SDL_libretro: libretro frontend using SDL."
    HOMEPAGE_URL "https://github.com/robloach/SDL_libretro"
    VERSION 0.0.1
    LANGUAGES C
)

# Options
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    set(SDL_LIBRETRO_IS_MAIN TRUE)
else()
    set(SDL_LIBRETRO_IS_MAIN FALSE)
endif()
option(SDL_LIBRETRO_BUILD_EXAMPLES "Examples" ${SDL_LIBRETRO_IS_MAIN})

# SDL_libretro
add_subdirectory(include)

# Examples
if (SDL_LIBRETRO_BUILD_EXAMPLES)
    add_subdirectory(examples)

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