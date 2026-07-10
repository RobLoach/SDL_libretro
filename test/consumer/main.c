#define SDL_LIBRETRO_IMPLEMENTATION
#include <SDL_libretro.h>

int main(void) {
    SDL_Libretro* lr = SDL_Libretro_Create();
    if (!lr) return 1;
    SDL_Libretro_Destroy(lr);
    return 0;
}
