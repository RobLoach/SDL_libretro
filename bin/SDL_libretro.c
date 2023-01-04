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
    return SDL_libretro_LoadCore(core);
}

void UnloadCore(AppData* appData) {
    SDL_libretro_UnloadCore();
}

void Init(SDL_App* app) {
    AppData* appData = (AppData*)app->userData;

    // Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        SDL_Log("SDL_Init(): %s", SDL_GetError());
        SDL_AppClose(app);
        app->exitStatus = 1;
        return;
    }

    // Window
	appData->window = SDL_CreateWindow("SDL_libretro", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 450, SDL_WINDOW_SHOWN);
    if (appData->window == NULL) {
        SDL_Log("SDL_CreateWindow(): %s", SDL_GetError());
        SDL_AppClose(app);
        app->exitStatus = 1;
        return;
    }

    // Renderer
	appData->renderer = SDL_CreateRenderer(appData->window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (appData->renderer == NULL) {
        SDL_Log("SDL_CreateRenderer(): %s", SDL_GetError());
        SDL_AppClose(app);
        app->exitStatus = 1;
        return;
    }

    // Core
    if (appData->coreToLoad != NULL) {
        LoadCore(appData, appData->coreToLoad, appData->contentToLoad);
    }
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

    // Clear the background
    SDL_SetRenderDrawColor(appData->renderer, 102, 191, 255, 255);
    SDL_RenderClear(appData->renderer);

    // Draw a red square
    SDL_SetRenderDrawColor(appData->renderer, 230, 41, 55, 255);
    SDL_Rect rect = {100, 100, 100, 100};
    SDL_RenderFillRect(appData->renderer, &rect);
    
    SDL_RenderPresent(appData->renderer);
}

void Close(SDL_App* app) {
    AppData* appData = (AppData*)app->userData;
    UnloadCore(appData);
	SDL_DestroyRenderer(appData->renderer);
	SDL_DestroyWindow(appData->window);
	SDL_Quit();
    sargs_shutdown();
}

void* SDL_libretro_MemAlloc(size_t size, void* user_data) {
    return SDL_malloc(size);
}

void SDL_libretro_MemFree(void* ptr, void* user_data) {
    SDL_free(ptr);
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
        }
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