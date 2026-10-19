# SDL3
# https://github.com/libsdl-org/SDL
#
# Uses SDL3 if it's already a target (e.g. a parent project added it) or an
# installed SDL3 package is found; otherwise fetches and builds a static SDL3.

if(TARGET SDL3::SDL3)
    set(SDL3_FOUND TRUE)
    return()
endif()

# CONFIG mode only looks for SDL3Config.cmake, so this doesn't recurse into
# this find module.
find_package(SDL3 CONFIG QUIET)
if(SDL3_FOUND)
    return()
endif()

# A function scopes SDL's build options to the fetched SDL, so they don't
# leak into the including project.
function(_sdl_libretro_fetch_sdl3)
    set(SDL_STATIC ON)
    set(SDL_SHARED OFF)
    set(SDL_DISABLE_UNINSTALL ON)

    include(FetchContent)
    FetchContent_Declare(
        SDL3Source
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.18
    )
    FetchContent_MakeAvailable(SDL3Source)
endfunction()
_sdl_libretro_fetch_sdl3()

set(SDL3_FOUND TRUE)
