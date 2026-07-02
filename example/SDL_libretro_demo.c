/*
 * SDL_libretro demo
 *
 * Usage: SDL_libretro_demo [core.so] [game.rom]
 *
 * Both arguments are optional. With no game loaded the window still runs and
 * prompts you to drag & drop a game onto it.
 *
 * Uses SDL3's callback-based main model (SDL_MAIN_USE_CALLBACKS) so SDL drives
 * the loop. This is friendlier to platforms like Emscripten where the app
 * cannot own a blocking while() loop.
 */

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define SDL_LIBRETRO_IMPLEMENTATION
#include "SDL_libretro.h"

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Libretro* lr;
} AppContext;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    const char* corePath = argc > 1 ? argv[1] : NULL;
    const char* gamePath = argc > 2 ? argv[2] : NULL;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Window* window;
    SDL_Renderer* renderer;
    if (!SDL_CreateWindowAndRenderer("SDL_libretro_demo", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create the libretro environment.
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetCoreDirectory(lr, "cores");

    if (corePath && !SDL_Libretro_LoadCore(lr, corePath)) {
        SDL_Log("Failed to load core: %s", SDL_GetError());
    }

    if ((corePath || gamePath) && !SDL_Libretro_LoadGame(lr, gamePath)) {
        SDL_Log("Failed to load game: %s", SDL_GetError());
    }

    AppContext* app = SDL_calloc(1, sizeof(AppContext));
    if (!app) {
        return SDL_APP_FAILURE;
    }
    app->window = window;
    app->renderer = renderer;
    app->lr = lr;
    *appstate = app;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    AppContext* app = appstate;
    SDL_Libretro* lr = app->lr;

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    // Fast Forward
    else if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_F && !event->key.repeat) {
        SDL_Libretro_SetSpeed(lr, 2.0f);
    }
    else if (event->type == SDL_EVENT_KEY_UP && event->key.key == SDLK_F) {
        SDL_Libretro_SetSpeed(lr, 1.0f);
    }

    // Slow Motion
    else if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_E && !event->key.repeat) {
        SDL_Libretro_SetSpeed(lr, 0.5f);
    }
    else if (event->type == SDL_EVENT_KEY_UP && event->key.key == SDLK_E) {
        SDL_Libretro_SetSpeed(lr, 1.0f);
    }

    // Rewind
    else if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_R && !event->key.repeat) {
        SDL_Libretro_SetSpeed(lr, -1.0f);
    }
    else if (event->type == SDL_EVENT_KEY_UP && event->key.key == SDLK_R) {
        SDL_Libretro_SetSpeed(lr, 1.0f);
    }

    // Volume
    else if (event->type == SDL_EVENT_KEY_UP && event->key.key == SDLK_MINUS) {
        SDL_Libretro_SetVolume(lr, SDL_Libretro_GetVolume(lr) - 0.1f);
    }
    else if (event->type == SDL_EVENT_KEY_UP && event->key.key == SDLK_EQUALS) {
        SDL_Libretro_SetVolume(lr, SDL_Libretro_GetVolume(lr) + 0.1f);
    }

    // Screenshot
    else if (event->type == SDL_EVENT_KEY_UP && event->key.key == SDLK_F12) {
        SDL_Surface* screenshot = SDL_Libretro_CreateSurface(lr);
        SDL_SavePNG(screenshot, "screenshot.png");
        SDL_DestroySurface(screenshot);
    }

    // Save State
    else if (event->type == SDL_EVENT_KEY_UP && event->key.key == SDLK_F2) {
        SDL_Libretro_SaveState(lr, "save.sav");
    }

    // Load State
    else if (event->type == SDL_EVENT_KEY_UP && event->key.key == SDLK_F4) {
        SDL_Libretro_LoadState(lr, "save.sav");
    }

    // Drag and drop a file to unload the current core and load the dropped game.
    else if (event->type == SDL_EVENT_DROP_FILE) {
        SDL_Libretro_UnloadCore(lr);
        SDL_Libretro_LoadGame(lr, event->drop.data);
    }

    // Pass all events to SDL_Libretro.
    SDL_Libretro_HandleEvent(lr, event);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    AppContext* app = appstate;
    SDL_Libretro* lr = app->lr;
    SDL_Renderer* renderer = app->renderer;

    if (SDL_Libretro_ShouldQuit(lr)) {
        return SDL_APP_SUCCESS;
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

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    AppContext* app = appstate;
    if (app) {
        SDL_Libretro_Destroy(app->lr);
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);
        SDL_free(app);
    }
    SDL_Quit();
}
