# SDL_PhysFS
# https://github.com/RobLoach/SDL_PhysFS

if(TARGET SDL_PhysFS)
    set(SDL_PhysFS_FOUND TRUE)
    return()
endif()

include(FetchContent)
FetchContent_Declare(
    SDL_PhysFS
    GIT_REPOSITORY https://github.com/RobLoach/SDL_PhysFS.git
    GIT_TAG 7ad3eb1
)
FetchContent_MakeAvailable(SDL_PhysFS)

set(SDL_PhysFS_FOUND TRUE)
