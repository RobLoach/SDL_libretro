#include <SDL2/SDL.h>

#define SDL_APP_IMPLEMENTATION
#include "../vendor/SDL_app/SDL_app.h"

#define SDL_LIBRETRO_IMPLEMENTATION
#include "../include/SDL_libretro.h"

#define SOKOL_ARGS_IMPL
#include "../vendor/sokol/sokol_args.h"

typedef struct AppData {
    SDL_Window* window;
    SDL_Renderer* renderer;
    const char* coreToLoad;
    const char* contentToLoad;
} AppData;

bool LoadCore(AppData* appData, const char* core, const char* content) {
    SDL_Log("Loading core: %s", core);
    if (!SDL_libretro_LoadCore(core)) {
        return false;
    }
    return SDL_libretro_LoadGame(content);
}

void Init(SDL_App* app) {
    AppData* appData = (AppData*)app->userData;

    // Input Validation
    if (appData->coreToLoad == NULL) {
        SDL_Log("Use ./sdl_libretro core=fceumm_libretro.so content=kirby.nes");
        app->shouldClose = true;
        app->exitStatus = 1;
        return;
    }

    // Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        SDL_Log("SDL_Init(): %s", SDL_GetError());
        SDL_AppClose(app);
        app->exitStatus = 1;
        app->shouldClose = true;
        return;
    }

    // Window
	appData->window = SDL_CreateWindow("SDL_libretro", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 450, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (appData->window == NULL) {
        SDL_Log("SDL_CreateWindow(): %s", SDL_GetError());
        SDL_AppClose(app);
        app->exitStatus = 1;
        app->shouldClose = true;
        return;
    }

    // Renderer
	appData->renderer = SDL_CreateRenderer(appData->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (appData->renderer == NULL) {
        SDL_Log("SDL_CreateRenderer(): %s", SDL_GetError());
        SDL_AppClose(app);
        app->exitStatus = 1;
        app->shouldClose = true;
        return;
    }

    // Core
    LoadCore(appData, appData->coreToLoad, appData->contentToLoad);
}

void Update(SDL_App* app) {
    AppData* appData = (AppData*)app->userData;
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
            case SDL_QUIT:
                SDL_AppClose(app);
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        SDL_AppClose(app);
                    break;
                }
                break;
        }
    }

    SDL_libretro_Update();

    if (appData->renderer != NULL) {
        // Clear the background
        SDL_SetRenderDrawColor(appData->renderer, 0, 0, 0, 255);
        SDL_RenderClear(appData->renderer);

        // Render the game on the screen
        SDL_libretro_Render(appData->renderer);

        SDL_RenderPresent(appData->renderer);
    }
}

void Close(SDL_App* app) {
    AppData* appData = (AppData*)app->userData;
    SDL_libretro_UnloadCore();

    SDL_Log("SDL_DestroyRenderer");
    if (appData->renderer != NULL) {
	    //SDL_DestroyRenderer(appData->renderer);
    }
    appData->renderer = NULL;

    SDL_Log("SDL_DestroyWindow");
	SDL_DestroyWindow(appData->window);
    appData->window = NULL;
    SDL_Log("SDL_Quit");
	SDL_Quit();
    SDL_Log("sargs_shutdown");
    sargs_shutdown();
    SDL_Log("FSDAAFSD");
}

void* SDL_libretro_MemAlloc(size_t size, void* user_data) {
    return SDL_malloc(size);
}

void SDL_libretro_MemFree(void* ptr, void* user_data) {
    if (ptr) {
        SDL_free(ptr);
    }
}

SDL_App Main(int argc, char* argv[]) {
    AppData* appData = SDL_malloc(sizeof(AppData));
    int exitStatus = 0;
    bool shouldClose = false;
    sargs_setup(&(sargs_desc){
        .argc = argc,
        .argv = argv,
        .allocator = {
            .alloc = SDL_libretro_MemAlloc,
            .free = SDL_libretro_MemFree,
        },
    });

    appData->coreToLoad = sargs_exists("core") ? sargs_value("core") : NULL;
    appData->contentToLoad = sargs_exists("content") ? sargs_value("content") : NULL;

    return (SDL_App) {
        .init = Init,
        .update = Update,
        .close = Close,
        .userData = appData,
        .exitStatus = exitStatus,
        .shouldClose = shouldClose,
    };
}
