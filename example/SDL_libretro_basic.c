/*
 * SDL_libretro basic example
 *
 * Usage: SDL_libretro_basic <core.so> [game.rom]
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define SDL_LIBRETRO_IMPLEMENTATION
#include "SDL_libretro.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        SDL_Log("Usage: %s <core> [game]", argv[0]);
        return 1;
    }

    const char* corePath = argv[1];
    const char* gamePath = argc > 2 ? argv[2] : NULL;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);

    SDL_Window* window = SDL_CreateWindow("SDL_libretro", 800, 600,
        SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_SetRenderVSync(renderer, 1);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetSystemDirectory(lr, "system");
    SDL_Libretro_SetSaveDirectory(lr, "saves");

    if (!SDL_Libretro_LoadCore(lr, corePath)) {
        SDL_Log("Failed to load core: %s", SDL_GetError());
        SDL_Libretro_Destroy(lr);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!SDL_Libretro_LoadGame(lr, gamePath, renderer)) {
        SDL_Log("Failed to load game: %s", SDL_GetError());
        SDL_Libretro_Destroy(lr);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Libretro_InitAudio(lr);

    bool running = true;
    while (running && !SDL_Libretro_ShouldClose(lr)) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            SDL_Libretro_HandleEvent(lr, &event);
        }

        SDL_Libretro_RunFrame(lr);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_Libretro_Render(lr, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
