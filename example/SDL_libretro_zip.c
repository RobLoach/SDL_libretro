/*
 * SDL_libretro_zip: Load libretro content straight from a .zip archive.
 *
 * Usage: SDL_libretro_zip <core.so> <game.zip> [entry]
 *
 * Requires configuring with -DSDL_LIBRETRO_ENABLE_MINIZIP=ON so minizip-ng is
 * linked. The archive stays mounted for the life of the game, so the core reads
 * the ROM and any companion files directly from the .zip via the libretro VFS.
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define SDL_LIBRETRO_IMPLEMENTATION
#include "SDL_libretro.h"
#include "SDL_libretro_minizip.h"

int main(int argc, char* argv[]) {
    const char* corePath = argc > 1 ? argv[1] : NULL;
    const char* zipPath  = argc > 2 ? argv[2] : NULL;
    const char* entry    = argc > 3 ? argv[3] : NULL; // optional: name the inner file
    if (!corePath || !zipPath) {
        //SDL_Log("Usage: SDL_libretro_zip <core> <game.zip> [entry]");
//        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS);

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer("SDL_libretro_zip", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer);

    SDL_Libretro* lr = SDL_Libretro_Create();

    // Load the core.
    // if (!SDL_Libretro_LoadCore(lr, corePath)) {
    //     SDL_Log("Failed to load core: %s", SDL_GetError());
    //     return 1;
    // }

    // Load the game from the .zip. When `entry` is NULL the content file is
    // auto-detected; otherwise the named archive entry is loaded.
    if (!SDL_Libretro_LoadGame_Zip(lr, zipPath)) {
    //if (!SDL_Libretro_LoadGame_ZipEntry(lr, zipPath, entry)) {
        SDL_Log("Failed to load game from zip: %s", SDL_GetError());
        return 1;
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            // Pass all events to SDL_Libretro.
            SDL_Libretro_HandleEvent(lr, &event);
        }

        // Update the context
        SDL_Libretro_Update(lr);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw the libretro context
        SDL_Libretro_Render(renderer, lr, NULL);

        SDL_RenderPresent(renderer);
    }

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
