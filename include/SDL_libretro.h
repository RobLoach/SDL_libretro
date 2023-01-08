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
SDL_bool SDL_libretro_LoadGame(const char* filename);
void SDL_libretro_UnloadCore();
void SDL_libretro_UnloadGame();
SDL_bool SDL_libretro_CoreIsLoaded();
SDL_bool SDL_libretro_GameIsLoaded();
void SDL_libretro_Update();
SDL_Texture* SDL_libretro_GetTexture(SDL_Renderer* renderer);
SDL_PixelFormatEnum SDL_libretro_GetPixelFormat();
int SDL_libretro_GetWidth();
int SDL_libretro_GetHeight();
void SDL_libretro_Render(SDL_Renderer* renderer);

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
    struct retro_system_info system_info;
    struct retro_system_timing system_timing;
    bool gameLoaded;
    bool supports_no_game;
    const uint8_t* inputKeyboardState;
    unsigned inputJoypadState[RETRO_DEVICE_ID_JOYPAD_R3+1];

    int width;
    int height;

    SDL_Texture* texture;
    SDL_AudioDeviceID audioDevice;

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

void SDL_libretro_Log(enum retro_log_level level, const char *fmt, ...) {
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

int SDL_libretro_GetWidth() {
    if (SDL_libretro_instance != NULL) {
        return SDL_libretro_instance->width;
    }

    return 0;
}

int SDL_libretro_GetHeight() {
    if (SDL_libretro_instance != NULL) {
        return SDL_libretro_instance->height;
    }

    return 0;
}

SDL_PixelFormatEnum SDL_libretro_GetPixelFormat() {
	switch (SDL_libretro_instance->pixel_format) {
        case RETRO_PIXEL_FORMAT_0RGB1555:
            return SDL_PIXELFORMAT_ARGB1555;
        case RETRO_PIXEL_FORMAT_XRGB8888:
            return SDL_PIXELFORMAT_ARGB8888;
        case RETRO_PIXEL_FORMAT_RGB565:
            return SDL_PIXELFORMAT_RGB565;
        case RETRO_PIXEL_FORMAT_UNKNOWN:
            return SDL_PIXELFORMAT_ARGB1555;
	}

    return SDL_PIXELFORMAT_ARGB1555;
}

void SDL_libretro_AudioDeinit() {
    if (SDL_libretro_instance->audioDevice <= 0) {
        return;
    }

    SDL_CloseAudioDevice(SDL_libretro_instance->audioDevice);
    SDL_libretro_instance->audioDevice = 0;
}

SDL_bool SDL_libretro_AudioInit() {
    SDL_libretro_AudioDeinit();
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;

    SDL_zero(desired);
    SDL_zero(obtained);

    desired.format = AUDIO_S16;
    desired.freq   = SDL_libretro_instance->system_timing.sample_rate;
    desired.channels = 2;
    desired.samples = 4096;

    SDL_libretro_instance->audioDevice = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (!SDL_libretro_instance->audioDevice) {
        SDL_libretro_Log(RETRO_LOG_ERROR, "[libretro] Failed to open playback device: %s", SDL_GetError());
        return false;
    }

    SDL_PauseAudioDevice(SDL_libretro_instance->audioDevice, 0);

    // Let the core know that the audio device has been initialized.
    if (SDL_libretro_instance->audio_callback.set_state) {
        SDL_libretro_instance->audio_callback.set_state(true);
    }

    return true;
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

        return false;
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

        return false;
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

        switch (*fmt){
            case RETRO_PIXEL_FORMAT_RGB565:
                SDL_libretro_Log(RETRO_LOG_INFO, "Set pixel format to: RETRO_PIXEL_FORMAT_RGB565");
                break;
            case RETRO_PIXEL_FORMAT_0RGB1555:
                SDL_libretro_Log(RETRO_LOG_INFO, "Set pixel format to: RETRO_PIXEL_FORMAT_0RGB1555");
                break;
            case RETRO_PIXEL_FORMAT_XRGB8888:
                SDL_libretro_Log(RETRO_LOG_INFO, "Set pixel format to: RETRO_PIXEL_FORMAT_XRGB8888");
                break;
            case RETRO_PIXEL_FORMAT_UNKNOWN:
            default:
                SDL_libretro_Log(RETRO_LOG_INFO, "Pixel format can't be RETRO_PIXEL_FORMAT_UNKNOWN");
                return false;
        }

        SDL_libretro_instance->pixel_format = *fmt;
        // Invalidate the texture so it's recreated if needed.
        if (SDL_libretro_instance->texture != NULL) {
            SDL_DestroyTexture(SDL_libretro_instance->texture);
            SDL_libretro_instance->texture = NULL;
        }

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

void SDL_libretro_VideoRefresh(const void *data, unsigned width, unsigned height, size_t pitch) {

    if (SDL_libretro_instance == NULL) {
        return;
    }
    // TODO: Update the SDL texture for the video refresh.
    if (SDL_libretro_instance->texture == NULL) {
        return;
    }

    // Lock the texture from reading
    void* pix;
    int pitch2;
    if (SDL_LockTexture(SDL_libretro_instance->texture, NULL, (void**)&pix, &pitch2) != 0) {
        SDL_libretro_Log(RETRO_LOG_ERROR, "Failed to lock the texture: %s", SDL_GetError());
    }

    // TODO: Is there a faster way to update the pixel data than memcpy?
    SDL_memcpy(pix, data, height * pitch);

    // Update the texture pixels using SDL_UpdateTexture()
    // if (SDL_UpdateTexture(SDL_libretro_instance->texture, NULL, data, pitch2) != 0) {
    //     SDL_libretro_Log(RETRO_LOG_ERROR, "Failed to update the texture pixels: %s", SDL_GetError());
    // }

    // Unlock the texture for reading
    SDL_UnlockTexture(SDL_libretro_instance->texture);

    //SDL_Log("Creating text");
}

void SDL_libretro_Render(SDL_Renderer* renderer) {
    SDL_Texture* texture = SDL_libretro_GetTexture(renderer);
    if (!texture) {
        return;
    }

    // Find the aspect ratio.
    float aspect = SDL_libretro_instance->game_geometry.aspect_ratio;
    if (aspect <= 0) {
        aspect = (float)SDL_libretro_instance->width / (float)SDL_libretro_instance->height;
    }

    int screenWidth, screenHeight;
    if (SDL_GetRendererOutputSize(renderer, &screenWidth, &screenHeight) != 0) {
        SDL_libretro_Log(RETRO_LOG_ERROR, "Failed to get output size: %s", SDL_GetError());
        return;
    }

    // Calculate the optimal width/height to display in the screen size.
    int height = screenHeight;
    int width = height * aspect;
    if (width > screenWidth) {
        height = (float)screenWidth / aspect;
        width = screenWidth;
    }

    // Draw the texture in the middle of the screen.
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;
    SDL_Rect destRect = {x, y, width, height};
    SDL_RenderCopy(renderer, texture, NULL, &destRect);
}

void SDL_libretro_InputPoll(void) {
    // TODO: Add Joypad
    // TODO: Add Mouse Support
    // TODO: Add multiplayer support

    // Keyboard
    SDL_libretro_instance->inputKeyboardState = SDL_GetKeyboardState(NULL);
    SDL_libretro_instance->inputJoypadState[RETRO_DEVICE_ID_JOYPAD_A] = SDL_libretro_instance->inputKeyboardState[SDL_SCANCODE_X];
    SDL_libretro_instance->inputJoypadState[RETRO_DEVICE_ID_JOYPAD_B] = SDL_libretro_instance->inputKeyboardState[SDL_SCANCODE_Z];
    SDL_libretro_instance->inputJoypadState[RETRO_DEVICE_ID_JOYPAD_Y] = SDL_libretro_instance->inputKeyboardState[SDL_SCANCODE_A];
    SDL_libretro_instance->inputJoypadState[RETRO_DEVICE_ID_JOYPAD_X] = SDL_libretro_instance->inputKeyboardState[SDL_SCANCODE_S];
    SDL_libretro_instance->inputJoypadState[RETRO_DEVICE_ID_JOYPAD_UP] = SDL_libretro_instance->inputKeyboardState[SDL_SCANCODE_UP];
    SDL_libretro_instance->inputJoypadState[RETRO_DEVICE_ID_JOYPAD_DOWN] = SDL_libretro_instance->inputKeyboardState[SDL_SCANCODE_DOWN];
    SDL_libretro_instance->inputJoypadState[RETRO_DEVICE_ID_JOYPAD_LEFT] = SDL_libretro_instance->inputKeyboardState[SDL_SCANCODE_LEFT];
    SDL_libretro_instance->inputJoypadState[RETRO_DEVICE_ID_JOYPAD_RIGHT] = SDL_libretro_instance->inputKeyboardState[SDL_SCANCODE_RIGHT];
    SDL_libretro_instance->inputJoypadState[RETRO_DEVICE_ID_JOYPAD_START] = SDL_libretro_instance->inputKeyboardState[SDL_SCANCODE_RETURN];
    SDL_libretro_instance->inputJoypadState[RETRO_DEVICE_ID_JOYPAD_SELECT] = SDL_libretro_instance->inputKeyboardState[SDL_SCANCODE_BACKSPACE];
    SDL_libretro_instance->inputJoypadState[RETRO_DEVICE_ID_JOYPAD_L] = SDL_libretro_instance->inputKeyboardState[SDL_SCANCODE_Q];
    SDL_libretro_instance->inputJoypadState[RETRO_DEVICE_ID_JOYPAD_R] = SDL_libretro_instance->inputKeyboardState[SDL_SCANCODE_W];
}

int16_t SDL_libretro_InputState(unsigned port, unsigned device, unsigned index, unsigned id) {
    // TODO: Add Mouse, Keyboard, and Gamepad support
	if (port || index || device != RETRO_DEVICE_JOYPAD)
		return 0;

	return SDL_libretro_instance->inputJoypadState[id];
}

size_t SDL_libretro_AudioWrite(const int16_t *buf, unsigned frames) {
    if (SDL_libretro_instance->audioDevice > 0) {
        SDL_QueueAudio(SDL_libretro_instance->audioDevice, buf, sizeof(*buf) * frames * 2);
    }
    return frames;
}

SDL_Texture* SDL_libretro_GetTexture(SDL_Renderer* renderer) {
    if (SDL_libretro_instance == NULL || renderer == NULL) {
        return NULL;
    }

    // If the texture exists already, return it.
    if (SDL_libretro_instance->texture != NULL) {
        return SDL_libretro_instance->texture;
    }

    // Create the texture
    SDL_libretro_instance->texture = SDL_CreateTexture(renderer, SDL_libretro_GetPixelFormat(), SDL_TEXTUREACCESS_STREAMING, SDL_libretro_instance->width, SDL_libretro_instance->height);
    if (SDL_libretro_instance->texture == NULL) {
        SDL_libretro_Log(RETRO_LOG_ERROR, "Failed to create texture from renderer: %s", SDL_GetError());
    }

    return SDL_libretro_instance->texture;
}

void SDL_libretro_AudioSample(int16_t left, int16_t right) {
	int16_t buf[2] = {left, right};
	SDL_libretro_AudioWrite(buf, 1);
}

size_t SDL_libretro_AudioSampleBatch(const int16_t *data, size_t frames) {
	return SDL_libretro_AudioWrite(data, frames);
}

void SDL_libretro_ResizeToAspect(double ratio, int sw, int sh, int *dw, int *dh) {
	*dw = sw;
	*dh = sh;

	if (ratio <= 0) {
		ratio = (double)sw / sh;
    }

	if ((float)sw / (float)sh < 1.0f) {
		*dw = *dh * ratio;
    }
	else {
		*dh = *dw / ratio;
    }
}

void SDL_libretro_VideoConfigure() {
    if (SDL_libretro_instance == NULL) {
        return;
    }

    if (SDL_libretro_instance->retro_get_system_av_info == NULL) {
        return;
    }

    // if (SDL_libretro_instance->surface != NULL) {
    //     SDL_FreeSurface(SDL_libretro_instance->surface);
    // }

    // Load the audio video details.
    struct retro_system_av_info av;
    SDL_libretro_instance->retro_get_system_av_info(&av);
    SDL_libretro_instance->game_geometry.base_width = av.geometry.base_width;
    SDL_libretro_instance->game_geometry.base_height = av.geometry.base_height;
    SDL_libretro_instance->game_geometry.max_width = av.geometry.max_width;
    SDL_libretro_instance->game_geometry.max_height = av.geometry.max_height;
    SDL_libretro_instance->game_geometry.aspect_ratio = av.geometry.aspect_ratio;

    SDL_libretro_instance->system_timing.fps = av.timing.fps;
    SDL_libretro_instance->system_timing.sample_rate = av.timing.sample_rate;

	SDL_libretro_ResizeToAspect(av.geometry.aspect_ratio, av.geometry.base_width * 1, av.geometry.base_height * 1, &SDL_libretro_instance->width, &SDL_libretro_instance->height);
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

    // Ensure everything starts as nothing..
    SDL_memset(SDL_libretro_instance, 0, sizeof(SDL_libretro));

    // Load the core
    SDL_libretro_instance->handle = SDL_LoadObject(coreFile);
    if (SDL_libretro_instance->handle == NULL) {
        SDL_libretro_Log(RETRO_LOG_ERROR, "Failed to load core: %s", SDL_GetError());
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
        SDL_libretro_Log(RETRO_LOG_ERROR, "Only libretro API 1 is supported");
        SDL_libretro_UnloadCore();
        return false;
    }

    set_environment(SDL_libretro_Environment);
	set_video_refresh(SDL_libretro_VideoRefresh);
	set_input_poll(SDL_libretro_InputPoll);
	set_input_state(SDL_libretro_InputState);
	set_audio_sample(SDL_libretro_AudioSample);
	set_audio_sample_batch(SDL_libretro_AudioSampleBatch);

    SDL_libretro_instance->retro_get_system_info(&SDL_libretro_instance->system_info);

    SDL_libretro_instance->retro_init();

    return true;
}

SDL_bool SDL_libretro_LoadGame(const char* filename) {
    if (SDL_libretro_instance == NULL) {
        SDL_libretro_Log(RETRO_LOG_ERROR, "Core not loaded");
        return false;
    }

	struct retro_game_info info;
    info.path = filename;
    info.meta = "";
    info.data = NULL;
    info.size = 0;

    if (filename) {
        // Check if we take the buffer of data.
        if (!SDL_libretro_instance->system_info.need_fullpath) {
            SDL_RWops *file = SDL_RWFromFile(filename, "rb");
            Sint64 size;

            if (!file) {
                SDL_libretro_Log(RETRO_LOG_ERROR, "Failed to load %s: %s", filename, SDL_GetError());
                return false;
            }

            size = SDL_RWsize(file);

            if (size < 0) {
                SDL_libretro_Log(RETRO_LOG_ERROR, "Failed to get file size of %s: %s", filename, SDL_GetError());
                SDL_RWclose(file);
                return false;
            }

            info.size = size;
            info.data = SDL_malloc(info.size);

            if (!info.data){
                SDL_libretro_Log(RETRO_LOG_ERROR, "Failed to allocate memory for file data");
                SDL_RWclose(file);
                return false;
            }

            if (SDL_RWread(file, (void*)info.data, info.size, 1) == 0) {
                SDL_libretro_Log(RETRO_LOG_ERROR, "Failed to read data for file %s: %s", filename, SDL_GetError());
                SDL_free((void*)info.data);
                SDL_RWclose(file);
                return false;
            }

            SDL_RWclose(file);
        }
    }

    // Load the game
    if (!SDL_libretro_instance->retro_load_game(&info)) {
        SDL_libretro_Log(RETRO_LOG_ERROR, "Failed to load game");
        if (info.data != NULL) {
            SDL_free((void*)info.data);
        }
        return false;
    }

    // Unload the file data now that it's loaded.
    if (info.data != NULL) {
        SDL_free((void*)info.data);
    }

    SDL_libretro_VideoConfigure();
    SDL_libretro_AudioInit();

    // TODO: Allow changing the joypad device?
    SDL_libretro_instance->retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
    return SDL_libretro_instance->gameLoaded = true;
}

void SDL_libretro_Update() {
    if (SDL_libretro_instance == NULL) {
        return;
    }
    // Update the game loop timer.
    // if (runloop_frame_time.callback) {
    //     retro_time_t current = cpu_features_get_time_usec();
    //     retro_time_t delta = current - runloop_frame_time_last;

    //     if (!runloop_frame_time_last)
    //         delta = runloop_frame_time.reference;
    //     runloop_frame_time_last = current;
    //     runloop_frame_time.callback(delta);
    // }

    // Ask the core to emit the audio.
    if (SDL_libretro_instance->audio_callback.callback) {
        SDL_libretro_instance->audio_callback.callback();
    }

    // Run the core
    SDL_libretro_instance->retro_run();
}

void SDL_libretro_UnloadGame() {
    if (SDL_libretro_instance == NULL) {
        return;
    }

    // retro_unload_game
    if (SDL_libretro_GameIsLoaded() && SDL_libretro_instance->retro_unload_game != NULL) {
        SDL_libretro_instance->retro_unload_game();
    }

    SDL_libretro_instance->gameLoaded = false;
}

/**
 * Unloads the given SDL_libretro instance.
 */
void SDL_libretro_UnloadCore() {
    if (SDL_libretro_instance == NULL) {
        return;
    }

    // Game
    SDL_libretro_UnloadGame();

    // retro_deinit
    if (SDL_libretro_instance->retro_deinit != NULL) {
        SDL_libretro_instance->retro_deinit();
    }

    // Unload the handle
    SDL_Log("SDL_UnloadObject");
    if (SDL_libretro_instance->handle != NULL) {
        SDL_UnloadObject(SDL_libretro_instance->handle);
    }

    // Audio
    SDL_Log("SDL_libretro_AudioDeinit");
    SDL_libretro_AudioDeinit();

    // Texture
    SDL_Log("SDL_DestroyTexture");
    if (SDL_libretro_instance->texture != NULL) {
        //SDL_DestroyTexture(SDL_libretro_instance->texture);
        SDL_libretro_instance->texture = NULL;
    }
    SDL_Log("SDL_free");

    SDL_free(SDL_libretro_instance);
    SDL_libretro_instance = NULL;
}

SDL_bool SDL_libretro_CoreIsLoaded() {
    return SDL_libretro_instance != NULL;
}

SDL_bool SDL_libretro_GameIsLoaded() {
    if (SDL_libretro_instance == NULL) {
        return false;
    }

    return SDL_libretro_instance->gameLoaded;
}

#endif  // SDL_LIBRETRO_IMPLEMENTATION_ONCE
#endif  // SDL_LIBRETRO_IMPLEMENTATION
