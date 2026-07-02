/*
 * SDL_libretro basic example
 *
 * Usage: SDL_libretro_basic [core.so] [game.rom]
 *
 * Both arguments are optional. With no game loaded the window still runs and
 * prompts you to drag & drop a game onto it.
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define SDL_LIBRETRO_IMPLEMENTATION
#include "SDL_libretro.h"

int main(int argc, char* argv[]) {
    const char* corePath = argc > 1 ? argv[1] : NULL;
    const char* gamePath = argc > 2 ? argv[2] : NULL;
    if (!gamePath) {
        SDL_Log("Usage: SDL_libertro_basic <core> <game>");
        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS);

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer("SDL_libretro_basic", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer);

    // Create the libretro environment.
    SDL_Libretro* lr = SDL_Libretro_Create();

    // Load the core.
    if (!SDL_Libretro_LoadCore(lr, corePath)) {
        SDL_Log("Failed to load core: %s", SDL_GetError());
        return 1;
    }

    // Load the game.
    if (!SDL_Libretro_LoadGame(lr, gamePath)) {
        SDL_Log("Failed to load game: %s", SDL_GetError());
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
