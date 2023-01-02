#ifndef SDL_LIBRETRO_H__
#define SDL_LIBRETRO_H__

// SDL.h
#ifndef SDL_LIBRETRO_SDL_H
#define SDL_LIBRETRO_SDL_H <SDL2/SDL.h>
#endif
#include SDL_LIBRETRO_SDL_H

// bool
#include <stdbool.h>

// libretro.h
#ifndef SDL_LIBRETRO_LIBRETRO_H
#define SDL_LIBRETRO_LIBRETRO_H "libretro.h"
#endif
#include SDL_LIBRETRO_LIBRETRO_H

typedef struct SDL_libretro {
    void* handle;
    bool initialized;

	void (*retro_init)(void);
	void (*retro_deinit)(void);
	unsigned (*retro_api_version)(void);
	void (*retro_get_system_info)(struct retro_system_info *info);
	void (*retro_get_system_av_info)(struct retro_system_av_info *info);
	void (*retro_set_controller_port_device)(unsigned port, unsigned device);
	void (*retro_reset)(void);
	void (*retro_run)(void);
//	size_t retro_serialize_size(void);
//	bool retro_serialize(void *data, size_t size);
//	bool retro_unserialize(const void *data, size_t size);
//	void retro_cheat_reset(void);
//	void retro_cheat_set(unsigned index, bool enabled, const char *code);
	bool (*retro_load_game)(const struct retro_game_info *game);
//	bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info);
	void (*retro_unload_game)(void);
//	unsigned retro_get_region(void);
//	void *retro_get_memory_data(unsigned id);
//	size_t retro_get_memory_size(unsigned id);
} SDL_libretro;

SDL_libretro* SDL_libretro_LoadCore(const char* coreFile);
void SDL_libretro_UnloadCore(SDL_libretro* core);

#endif  // SDL_LIBRETRO_H__

// TODO: Remove this.
#define SDL_LIBRETRO_IMPLEMENTATION

#ifdef SDL_LIBRETRO_IMPLEMENTATION
#ifndef SDL_LIBRETRO_IMPLEMENTATION_ONCE
#define SDL_LIBRETRO_IMPLEMENTATION_ONCE

#define SDL_libretro_LoadFunction(H, V, S) do { \
    if (!((*(void**)&V) = SDL_LoadFunction(H, #S))) { \
        SDL_Log("Failed to find object: %s", #S); \
        SDL_UnloadObject(H); \
        return NULL; \
    } \
} while (0)

/**
 * Loads the given core file.
 * 
 * @return A pointer to the SDL_libretro instance, or NULL on error. Use SDL_GetError() for information on failure.
 */
SDL_libretro* SDL_libretro_LoadCore(const char* coreFile) {
    void* handle = SDL_LoadObject(coreFile);
    if (handle == NULL) {
        SDL_Log("Failed to load object");
        return NULL;
    }

    SDL_libretro* output = (SDL_libretro*)SDL_malloc(sizeof(SDL_libretro));
    if (output == NULL) {
        SDL_Log("Failed to allocate memory");
        SDL_UnloadObject(handle);
        return NULL;
    }

    SDL_libretro_LoadFunction(handle, output->retro_init, "retro_init");
    SDL_libretro_LoadFunction(handle, output->retro_deinit, "retro_deinit");
    SDL_libretro_LoadFunction(handle, output->retro_api_version, "retro_api_version");
    SDL_libretro_LoadFunction(handle, output->retro_get_system_info, "retro_get_system_info");
    SDL_libretro_LoadFunction(handle, output->retro_get_system_av_info, "retro_get_system_av_info");
    SDL_libretro_LoadFunction(handle, output->retro_set_controller_port_device, "retro_set_controller_port_device");
    SDL_libretro_LoadFunction(handle, output->retro_reset, "retro_reset");
    SDL_libretro_LoadFunction(handle, output->retro_run, "retro_run");
    SDL_libretro_LoadFunction(handle, output->retro_load_game, "retro_load_game");
    SDL_libretro_LoadFunction(handle, output->retro_unload_game, "retro_unload_game");

    output->handle = handle;
    return output;
}

/**
 * Unloads the given SDL_libretro instance.
 */
void SDL_libretro_UnloadCore(SDL_libretro* core) {
    if (core == NULL) {
        return;
    }

    if (core->handle != NULL) {
        SDL_UnloadObject(core->handle);
        core->handle = NULL;
    }
}

#endif  // SDL_LIBRETRO_IMPLEMENTATION_ONCE
#endif  // SDL_LIBRETRO_IMPLEMENTATION