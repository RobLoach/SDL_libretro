# Provides the SDL_PhysFS header-only interface target.

if(TARGET SDL_PhysFS)
    set(SDL_PhysFS_FOUND TRUE)
    return()
endif()

include(FetchContent)
FetchContent_Declare(
    SDL_PhysFS
    GIT_REPOSITORY https://github.com/RobLoach/SDL_PhysFS.git
    GIT_TAG d2b650adace303d8156365f811b5420a7e9c21e1
)
FetchContent_MakeAvailable(SDL_PhysFS)

set(SDL_PhysFS_FOUND TRUE)
