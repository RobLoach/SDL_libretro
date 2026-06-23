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

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS);

    SDL_Window* window = SDL_CreateWindow("SDL_libretro", 800, 600, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_SetRenderVSync(renderer, 1);

    // Create the libretro environment.
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRewindEnabled(lr, true, 0, 0);

    // Load the core.
    if (!SDL_Libretro_LoadCore(lr, corePath)) {
        SDL_Log("Failed to load core: %s", SDL_GetError());
        SDL_Libretro_Destroy(lr);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load the game.
    if (!SDL_Libretro_LoadGame(lr, gamePath, renderer)) {
        SDL_Log("Failed to load game: %s", SDL_GetError());
        SDL_Libretro_Destroy(lr);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    while (running && !SDL_Libretro_ShouldClose(lr)) {
        // Check any events.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            // Fast Forward
            else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F && !event.key.repeat) {
                SDL_Libretro_SetSpeed(lr, 5.0f);
            }
            else if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_F) {
                SDL_Libretro_SetSpeed(lr, 1.0f);
            }

            // Slow Motion
            else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_E && !event.key.repeat) {
                SDL_Libretro_SetSpeed(lr, 0.5f);
            }
            else if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_E) {
                SDL_Libretro_SetSpeed(lr, 1.0f);
            }

            // Rewind
            else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_R && !event.key.repeat) {
                SDL_Libretro_SetSpeed(lr, -1.0f);
            }
            else if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_R) {
                SDL_Libretro_SetSpeed(lr, 1.0f);
            }

            // Volume
            else if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_MINUS) {
                SDL_Libretro_SetVolume(lr, SDL_Libretro_GetVolume(lr) - 0.1f);
            }
            else if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_EQUALS) {
                SDL_Libretro_SetVolume(lr, SDL_Libretro_GetVolume(lr) + 0.1f);
            }

            // Screenshot
            else if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_F12) {
                SDL_Surface* screenshot = SDL_Libretro_CreateSurface(lr);
                SDL_SavePNG(screenshot, "screenshot.png");
                SDL_DestroySurface(screenshot);
            }

            // Save State
            else if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_F2) {
                SDL_Libretro_SaveState(lr, "save.sav");
                SDL_Libretro_SetMessage(lr, "State Saved", 3.0);
            }

            // Load State
            else if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_F4) {
                SDL_Libretro_LoadState(lr, "save.sav");
                SDL_Libretro_SetMessage(lr, "State Loaded", 3.0);
            }

            // Pass all events to SDL_Libretro.
            SDL_Libretro_HandleEvent(lr, &event);
        }

        // Update
        SDL_Libretro_RunFrame(lr);

        // Draw
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_Libretro_Render(lr, NULL);

        // Draw the current OSD message, if there is one.
        const char* message = SDL_Libretro_GetMessage(lr);
        if (message) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, 19.0f, 19.0f, message);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
