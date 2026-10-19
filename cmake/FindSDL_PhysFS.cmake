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
    GIT_TAG f7e99df2cc5f4aede3613e22f6561fa1de4f5fc7
)
FetchContent_MakeAvailable(SDL_PhysFS)

set(SDL_PhysFS_FOUND TRUE)
