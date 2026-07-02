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

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS);

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer("SDL_libretro", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer);

    // Create the libretro environment.
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetCoreDirectory(lr, "cores");

    if (corePath && !SDL_Libretro_LoadCore(lr, corePath)) {
        SDL_Log("Failed to load core: %s", SDL_GetError());
    }

    if ((corePath || gamePath) && !SDL_Libretro_LoadGame(lr, gamePath)) {
        SDL_Log("Failed to load game: %s", SDL_GetError());
    }

    bool running = true;
    while (running && !SDL_Libretro_ShouldQuit(lr)) {
        // Check any events.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            // Fast Forward
            else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F && !event.key.repeat) {
                SDL_Libretro_SetSpeed(lr, 2.0f);
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
            }

            // Load State
            else if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_F4) {
                SDL_Libretro_LoadState(lr, "save.sav");
            }

            // Drag and drop a file to unload the current core and load the dropped game.
            else if (event.type == SDL_EVENT_DROP_FILE) {
                SDL_Libretro_UnloadCore(lr);
                SDL_Libretro_LoadGame(lr, event.drop.data);
            }

            // Pass all events to SDL_Libretro.
            SDL_Libretro_HandleEvent(lr, &event);
        }

        // Update the context
        SDL_Libretro_Update(lr);

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw the libretro context
        SDL_Libretro_Render(renderer, lr, NULL);

        // Tell them they can drop a file
        if (!SDL_Libretro_IsGameReady(lr)) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, 0, 0, "Drag & Drop a game to play");
        }

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
