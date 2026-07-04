/*
 * SDL_libretro_demo: A demonstration of what SDL_libretro can do.
 *
 * Usage: SDL_libretro_demo [core.so] [game.rom]
 *        SDL_libretro_demo core.so --subsystem <ident> "path1" ["path2" ...]
 */

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define SDL_LIBRETRO_IMPLEMENTATION
#include "SDL_libretro.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Libretro* lr;
} AppContext;

/**
 * Called when dragging and dropping a game onto the window.
 */
static void SDL_Libretro_DemoLoadDroppedGame(AppContext* app, const char* path) {
    SDL_Libretro_UnloadCore(app->lr);
    SDL_Libretro_LoadGame(app->lr, path);
}

#ifdef __EMSCRIPTEN__
/**
 * Called from SDL_libretro_demo_web.js to pass over to DemoLoadDroppedGame()
 */
EMSCRIPTEN_KEEPALIVE
void SDL_Libretro_DemoDropFile(AppContext* app, const char* path) {
    if (app && path) {
        SDL_Libretro_DemoLoadDroppedGame(app, path);
    }
}
#endif

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    // Subsystem detection with "--subsystem <ident> path1 [path2 ...]"
    const char* subsystemIdent = NULL;
    const char** subsystemPaths = NULL;
    unsigned subsystemPathCount = 0;
    int subsystemArg = 0;
    for (int i = 1; i < argc; i++) {
        if (SDL_strcmp(argv[i], "--subsystem") == 0) {
            subsystemArg = i;
            subsystemIdent = (i + 1 < argc) ? argv[i + 1] : NULL;
            if (i + 2 < argc) {
                subsystemPaths = (const char**)&argv[i + 2];
                subsystemPathCount = (unsigned)(argc - (i + 2));
            }
            break;
        }
    }

    // The core is the first positional argument. The game path is the second,
    // unless "--subsystem" consumes the remaining arguments.
    const char* corePath = NULL;
    const char* gamePath = NULL;
    if (subsystemArg != 0) {
        // e.g. "core.so --subsystem sgb a.gb b.gb"; a core must precede the flag.
        corePath = subsystemArg > 1 ? argv[1] : NULL;
    } else {
        corePath = argc > 2 ? argv[1] : NULL;
        gamePath = argc > 2 ? argv[2] : (argc > 1 ? argv[1] : NULL);
    }

    // Initialize
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create the Window
    SDL_Window* window;
    SDL_Renderer* renderer;
    if (!SDL_CreateWindowAndRenderer("SDL_libretro_demo", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create the libretro environment.
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetCoreDirectory(lr, "cores");
    SDL_Libretro_SetSystemDirectory(lr, "system");
    SDL_Libretro_InitConfigFile(lr, "SDL_libretro_demo.cfg");

    if (corePath && !SDL_Libretro_LoadCore(lr, corePath)) {
        SDL_Log("Failed to load core: %s", SDL_GetError());
    }

    if (subsystemArg != 0) {
        if (!subsystemIdent || subsystemPathCount == 0) {
            SDL_Log("Usage: <core> --subsystem <ident> \"path1\" [\"path2\" ...]");
        } else if (!SDL_Libretro_LoadGameSpecial(lr, subsystemIdent, subsystemPaths, subsystemPathCount)) {
            SDL_Log("Failed to load subsystem '%s': %s", subsystemIdent, SDL_GetError());
        }
    } else if ((corePath || gamePath) && !SDL_Libretro_LoadGame(lr, gamePath)) {
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

#ifdef __EMSCRIPTEN__
    // Hand the app pointer to the drag & drop bridge; it passes it back on drop.
    EM_ASM({ Module.installDemoDrop($0); }, app);
#else
    // Don't have rewind on Web
    SDL_Libretro_SetRewindEnabled(lr, true, 0, 0);
#endif

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
        char savePath[4096];
        SDL_Libretro_GetSavePath(lr, ".sav", savePath, sizeof(savePath));
        SDL_Libretro_SaveState(lr, savePath);
    }

    // Load State
    else if (event->type == SDL_EVENT_KEY_UP && event->key.key == SDLK_F4) {
        char savePath[4096];
        SDL_Libretro_GetSavePath(lr, ".sav", savePath, sizeof(savePath));
        SDL_Libretro_LoadState(lr, savePath);
    }

    // Drag and drop a file to unload the current core and load the dropped game.
    else if (event->type == SDL_EVENT_DROP_FILE) {
        SDL_Libretro_DemoLoadDroppedGame(app, event->drop.data);
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
        SDL_RenderDebugText(renderer, 19.0f, 19.0f, "Drag & Drop a game to play");
    }

    // Draw the current OSD message, if there is one.
    const char* message = SDL_Libretro_GetMessage(lr);
    if (message) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(renderer, 19.0f, 27.0f, message);
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
