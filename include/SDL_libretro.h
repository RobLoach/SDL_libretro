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

SDL_bool SDL_libretro_LoadCore(const char* coreFile);
void SDL_libretro_UnloadCore();
SDL_bool SDL_libretro_CoreIsLoaded();

#endif  // SDL_LIBRETRO_H__

// TODO: Remove this.
#define SDL_LIBRETRO_IMPLEMENTATION

#ifdef SDL_LIBRETRO_IMPLEMENTATION
#ifndef SDL_LIBRETRO_IMPLEMENTATION_ONCE
#define SDL_LIBRETRO_IMPLEMENTATION_ONCE

typedef struct SDL_libretro {
    void* handle;
    enum retro_pixel_format pixel_format;
    struct retro_audio_callback audio_callback;
    struct retro_game_geometry game_geometry;
    bool supports_no_game;

	void (*retro_init)(void);
	void (*retro_deinit)(void);
	unsigned (*retro_api_version)(void);
	void (*retro_get_system_info)(struct retro_system_info *info);
	void (*retro_get_system_av_info)(struct retro_system_av_info *info);
	void (*retro_set_controller_port_device)(unsigned port, unsigned device);
	void (*retro_reset)(void);
	void (*retro_run)(void);
	size_t (*retro_serialize_size)(void);
	bool (*retro_serialize)(void *data, size_t size);
	bool (*retro_unserialize)(const void *data, size_t size);
	void (*retro_cheat_reset)(void);
	void (*retro_cheat_set)(unsigned index, bool enabled, const char *code);
	bool (*retro_load_game)(const struct retro_game_info *game);
	bool (*retro_load_game_special)(unsigned game_type, const struct retro_game_info *info, size_t num_info);
	void (*retro_unload_game)(void);
	unsigned (*retro_get_region)(void);
	void* (*retro_get_memory_data)(unsigned id);
	size_t (*retro_get_memory_size)(unsigned id);
} SDL_libretro;

/**
 * The global instance of SDL_libretro.
 */
SDL_libretro* SDL_libretro_instance = NULL;

/**
 * Attempts to load the given function, and exits on failure.
 */
#define SDL_libretro_LoadFunction(destination, S) do { \
    if (!((*(void**)&destination) = SDL_LoadFunction(SDL_libretro_instance->handle, #S))) { \
        SDL_Log("[libretro] Failed to find object: %s", #S); \
        SDL_libretro_UnloadCore(); \
        return false; \
    } \
} while (0)
#define SDL_libretro_LoadCoreFunction(S) SDL_libretro_LoadFunction(SDL_libretro_instance->S, S)

static void SDL_libretro_Log(enum retro_log_level level, const char *fmt, ...) {
	char buffer[4096] = {0};
	static const char * levelstr[] = { "dbg", "inf", "wrn", "err" };
	va_list va;

	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

    switch (level) {
        case RETRO_LOG_DEBUG:
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[libretro] %s", buffer);
            break;
        case RETRO_LOG_INFO:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[libretro] %s", buffer);
            break;
        case RETRO_LOG_WARN:
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[libretro] %s", buffer);
            break;
        case RETRO_LOG_ERROR:
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[libretro] %s", buffer);
            break;
        default:
            break;
    }
}


bool SDL_libretro_Environment(unsigned cmd, void *data) {
	switch (cmd) {
    case RETRO_ENVIRONMENT_SET_VARIABLES: {
        // const struct retro_variable *vars = (const struct retro_variable *)data;
        // size_t num_vars = 0;

        // for (const struct retro_variable *v = vars; v->key; ++v) {
        //     num_vars++;
        // }

        // g_vars = (struct retro_variable*)calloc(num_vars + 1, sizeof(*g_vars));
        // for (unsigned i = 0; i < num_vars; ++i) {
        //     const struct retro_variable *invar = &vars[i];
        //     struct retro_variable *outvar = &g_vars[i];

        //     const char *semicolon = strchr(invar->value, ';');
        //     const char *first_pipe = strchr(invar->value, '|');

        //     SDL_assert(semicolon && *semicolon);
        //     semicolon++;
        //     while (isspace(*semicolon))
        //         semicolon++;

        //     if (first_pipe) {
        //         outvar->value = malloc((first_pipe - semicolon) + 1);
        //         memcpy((char*)outvar->value, semicolon, first_pipe - semicolon);
        //         ((char*)outvar->value)[first_pipe - semicolon] = '\0';
        //     } else {
        //         outvar->value = strdup(semicolon);
        //     }

        //     outvar->key = strdup(invar->key);
        //     SDL_assert(outvar->key && outvar->value);
        // }

        return true;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        // struct retro_variable *var = (struct retro_variable *)data;

        // if (!g_vars)
        //     return false;

        // for (const struct retro_variable *v = g_vars; v->key; ++v) {
        //     if (strcmp(var->key, v->key) == 0) {
        //         var->value = v->value;
        //         break;
        //     }
        // }

        return true;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
        bool *bval = (bool*)data;
		*bval = false;
        return true;
    }
	case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
		struct retro_log_callback *cb = (struct retro_log_callback *)data;
		cb->log = SDL_libretro_Log;
        return true;
	}
    case RETRO_ENVIRONMENT_GET_PERF_INTERFACE: {
        // struct retro_perf_callback *perf = (struct retro_perf_callback *)data;
        // perf->get_time_usec = cpu_features_get_time_usec;
        // perf->get_cpu_features = core_get_cpu_features;
        // perf->get_perf_counter = core_get_perf_counter;
        // perf->perf_register = core_perf_register;
        // perf->perf_start = core_perf_start;
        // perf->perf_stop = core_perf_stop;
        // perf->perf_log = core_perf_log;
        return false;
    }
	case RETRO_ENVIRONMENT_GET_CAN_DUPE: {
		bool *bval = (bool*)data;
		*bval = true;
        return true;
    }
	case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
		const enum retro_pixel_format *fmt = (enum retro_pixel_format *)data;

		if (*fmt > RETRO_PIXEL_FORMAT_RGB565) {
			return false;
        }

        SDL_libretro_instance->pixel_format = *fmt;
		return true;
	}
    case RETRO_ENVIRONMENT_SET_HW_RENDER: {
        // struct retro_hw_render_callback *hw = (struct retro_hw_render_callback*)data;
        // hw->get_current_framebuffer = core_get_current_framebuffer;
        // hw->get_proc_address = (retro_hw_get_proc_address_t)SDL_GL_GetProcAddress;
        // g_video.hw = *hw;
        return false;
    }
    case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK: {
        const struct retro_frame_time_callback *frame_time =
            (const struct retro_frame_time_callback*)data;
        //runloop_frame_time = *frame_time;
        return false;
    }
    case RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK: {
        struct retro_audio_callback *audio_cb = (struct retro_audio_callback*)data;
        SDL_libretro_instance->audio_callback = *audio_cb;
        return true;
    }
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
        const char **dir = (const char**)data;
        *dir = ".";
        return true;
    }
    case RETRO_ENVIRONMENT_SET_GEOMETRY: {
        const struct retro_game_geometry *geom = (const struct retro_game_geometry *)data;
        SDL_libretro_instance->game_geometry = *geom;
        return true;
    }
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: {
        SDL_libretro_instance->supports_no_game = *(bool*)data;
        return true;
    }
    case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE: {
        int *value = (int*)data;
        *value = 1 << 0 | 1 << 1;
        return true;
    }
	default:
		SDL_libretro_Log(RETRO_LOG_DEBUG, "Unhandled env #%u", cmd);
		return false;
	}

    return false;
}

/**
 * Loads the given core file.
 */
SDL_bool SDL_libretro_LoadCore(const char* coreFile) {
    void (*set_environment)(retro_environment_t) = NULL;
	void (*set_video_refresh)(retro_video_refresh_t) = NULL;
	void (*set_input_poll)(retro_input_poll_t) = NULL;
	void (*set_input_state)(retro_input_state_t) = NULL;
	void (*set_audio_sample)(retro_audio_sample_t) = NULL;
	void (*set_audio_sample_batch)(retro_audio_sample_batch_t) = NULL;
	

    SDL_libretro_UnloadCore();
    SDL_libretro_instance = (SDL_libretro*)SDL_malloc(sizeof(SDL_libretro));
    if (SDL_libretro_instance == NULL) {
        SDL_Log("[libretro] Failed to allocate memory");
        return false;
    }

    // Initialize some variables.
    SDL_libretro_instance->supports_no_game = false;

    // Load the core
    SDL_libretro_instance->handle = SDL_LoadObject(coreFile);
    if (SDL_libretro_instance->handle == NULL) {
        SDL_Log("[libretro] Failed to load object: %s", SDL_GetError());
        SDL_libretro_UnloadCore();
        return false;
    }

    SDL_libretro_LoadCoreFunction(retro_init);
    SDL_libretro_LoadCoreFunction(retro_deinit);
    SDL_libretro_LoadCoreFunction(retro_api_version);
    SDL_libretro_LoadCoreFunction(retro_get_system_info);
    SDL_libretro_LoadCoreFunction(retro_get_system_av_info);
    SDL_libretro_LoadCoreFunction(retro_set_controller_port_device);
    SDL_libretro_LoadCoreFunction(retro_reset);
    SDL_libretro_LoadCoreFunction(retro_run);
    SDL_libretro_LoadCoreFunction(retro_load_game);
    SDL_libretro_LoadCoreFunction(retro_unload_game);

	SDL_libretro_LoadFunction(set_environment, retro_set_environment);
	SDL_libretro_LoadFunction(set_video_refresh, retro_set_video_refresh);
	SDL_libretro_LoadFunction(set_input_poll, retro_set_input_poll);
	SDL_libretro_LoadFunction(set_input_state, retro_set_input_state);
	SDL_libretro_LoadFunction(set_audio_sample, retro_set_audio_sample);
	SDL_libretro_LoadFunction(set_audio_sample_batch, retro_set_audio_sample_batch);

    if (SDL_libretro_instance->retro_api_version() > 1) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[libretro] Only libretro API 1 is supported");
        SDL_libretro_UnloadCore();
        return false;
    }

    set_environment(SDL_libretro_Environment);
	// set_video_refresh(SDL_libretro_VideoRefresh);
	// set_input_poll(SDL_libretro_InputPoll);
	// set_input_state(SDL_libretro_InputState);
	// set_audio_sample(SDL_libretro_AudioSample);
	// set_audio_sample_batch(SDL_libretro_AudioSampleBatch);

    //SDL_libretro_instance->retro_init();

    return true;
}

/**
 * Unloads the given SDL_libretro instance.
 */
void SDL_libretro_UnloadCore() {
    if (SDL_libretro_instance == NULL) {
        return;
    }

    if (SDL_libretro_instance->retro_deinit != NULL) {
        SDL_libretro_instance->retro_deinit();
    }

    if (SDL_libretro_instance->handle != NULL) {
        SDL_UnloadObject(SDL_libretro_instance->handle);
    }

    SDL_free(SDL_libretro_instance);
    SDL_libretro_instance = NULL;
}

SDL_bool SDL_libretro_CoreIsLoaded() {
    return SDL_libretro_instance != NULL;
}

#endif  // SDL_LIBRETRO_IMPLEMENTATION_ONCE
#endif  // SDL_LIBRETRO_IMPLEMENTATION