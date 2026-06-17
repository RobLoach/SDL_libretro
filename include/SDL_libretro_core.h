#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_CORE_IMPL_ONCE)
#define SDL_LIBRETRO_CORE_IMPL_ONCE

/*
 * SDL_libretro - core lifecycle implementation
 */


#include <string.h>


#define LOAD_SYM(sym) do { \
    SDL_FunctionPointer fp = SDL_LoadFunction(lr->core.symbols.handle, #sym); \
    SDL_memcpy(&lr->core.symbols.sym, &fp, sizeof(fp)); \
    if (!fp) { \
        SDL_SetError("SDL_libretro: Failed to load symbol '%s'", #sym); \
        return false; \
    } \
} while (0)

SDL_Libretro* SDL_Libretro_Create(void) {
    SDL_Libretro* lr = (SDL_Libretro*)SDL_calloc(1, sizeof(SDL_Libretro));
    if (!lr) {
        SDL_SetError("SDL_libretro: Failed to allocate context");
        return NULL;
    }

    lr->volume = 1.0f;
    lr->speed = 1.0f;
    SDL_strlcpy(lr->username, "SDL_libretro", sizeof(lr->username));

    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_B]      = SDL_SCANCODE_Z;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_Y]      = SDL_SCANCODE_A;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_SELECT]  = SDL_SCANCODE_RSHIFT;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_START]   = SDL_SCANCODE_RETURN;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_UP]      = SDL_SCANCODE_UP;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_DOWN]    = SDL_SCANCODE_DOWN;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_LEFT]    = SDL_SCANCODE_LEFT;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_RIGHT]   = SDL_SCANCODE_RIGHT;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A]       = SDL_SCANCODE_X;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_X]       = SDL_SCANCODE_S;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_L]       = SDL_SCANCODE_Q;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_R]       = SDL_SCANCODE_W;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_L2]      = SDL_SCANCODE_E;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_R2]      = SDL_SCANCODE_R;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_L3]      = SDL_SCANCODE_D;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_R3]      = SDL_SCANCODE_F;

    return lr;
}

void SDL_Libretro_Destroy(SDL_Libretro* lr) {
    if (!lr) return;

    SDL_Libretro_UnloadGame(lr);
    SDL_Libretro_UnloadCore(lr);

    for (unsigned i = 0; i < 16; i++) {
        if (lr->gamepads[i]) {
            SDL_CloseGamepad(lr->gamepads[i]);
            lr->gamepads[i] = NULL;
        }
    }

    SDL_free(lr);
}

bool SDL_Libretro_LoadCore(SDL_Libretro* lr, const char* corePath) {
    if (!lr || !corePath) {
        SDL_SetError("SDL_libretro: Invalid arguments");
        return false;
    }

    if (SDL_Libretro_active && SDL_Libretro_active != lr) {
        SDL_SetError("SDL_libretro: Another context already has a core loaded");
        return false;
    }

    if (lr->core.loaded) {
        SDL_Libretro_UnloadCore(lr);
    }

    SDL_memset(&lr->core, 0, sizeof(lr->core));

    lr->core.symbols.handle = SDL_LoadObject(corePath);
    if (!lr->core.symbols.handle) {
        SDL_SetError("SDL_libretro: Failed to load core '%s': %s", corePath, SDL_GetError());
        return false;
    }

    LOAD_SYM(retro_init);
    LOAD_SYM(retro_deinit);
    LOAD_SYM(retro_api_version);
    LOAD_SYM(retro_set_environment);
    LOAD_SYM(retro_set_video_refresh);
    LOAD_SYM(retro_set_audio_sample);
    LOAD_SYM(retro_set_audio_sample_batch);
    LOAD_SYM(retro_set_input_poll);
    LOAD_SYM(retro_set_input_state);
    LOAD_SYM(retro_get_system_info);
    LOAD_SYM(retro_get_system_av_info);
    LOAD_SYM(retro_set_controller_port_device);
    LOAD_SYM(retro_reset);
    LOAD_SYM(retro_run);
    LOAD_SYM(retro_serialize_size);
    LOAD_SYM(retro_serialize);
    LOAD_SYM(retro_unserialize);
    LOAD_SYM(retro_cheat_reset);
    LOAD_SYM(retro_cheat_set);
    LOAD_SYM(retro_load_game);
    LOAD_SYM(retro_load_game_special);
    LOAD_SYM(retro_unload_game);
    LOAD_SYM(retro_get_region);
    LOAD_SYM(retro_get_memory_data);
    LOAD_SYM(retro_get_memory_size);

    SDL_strlcpy(lr->core.corePath, corePath, sizeof(lr->core.corePath));

    SDL_Libretro_active = lr;

    lr->core.symbols.retro_set_environment(SDL_Libretro_EnvironmentCallback);

    struct retro_system_info sysinfo = {0};
    lr->core.symbols.retro_get_system_info(&sysinfo);
    SDL_strlcpy(lr->core.libraryName, sysinfo.library_name ? sysinfo.library_name : "", sizeof(lr->core.libraryName));
    SDL_strlcpy(lr->core.libraryVersion, sysinfo.library_version ? sysinfo.library_version : "", sizeof(lr->core.libraryVersion));
    SDL_strlcpy(lr->core.validExtensions, sysinfo.valid_extensions ? sysinfo.valid_extensions : "", sizeof(lr->core.validExtensions));
    lr->core.needFullpath = sysinfo.need_fullpath;
    lr->core.blockExtract = sysinfo.block_extract;

    lr->core.symbols.retro_init();
    lr->core.loaded = true;

    lr->core.apiVersion = lr->core.symbols.retro_api_version();

    SDL_Log("SDL_libretro: Loaded core '%s' v%s (API %u)",
        lr->core.libraryName, lr->core.libraryVersion, lr->core.apiVersion);

    return true;
}

void SDL_Libretro_UnloadCore(SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) return;

    SDL_Libretro_UnloadGame(lr);

    lr->core.symbols.retro_deinit();

    if (lr->core.symbols.handle) {
        SDL_UnloadObject(lr->core.symbols.handle);
    }

    SDL_Libretro_FreeCoreOptions(lr);

    if (lr->core.inputDescriptors) {
        SDL_free(lr->core.inputDescriptors);
    }
    if (lr->core.controllerInfo) {
        SDL_free(lr->core.controllerInfo);
    }
    if (lr->core.memoryMapDescriptors) {
        SDL_free(lr->core.memoryMapDescriptors);
    }
    if (lr->core.persistentGameData) {
        SDL_free(lr->core.persistentGameData);
    }

    SDL_memset(&lr->core, 0, sizeof(lr->core));

    if (SDL_Libretro_active == lr) {
        SDL_Libretro_active = NULL;
    }

    SDL_Log("SDL_libretro: Core unloaded");
}

bool SDL_Libretro_IsCoreReady(const SDL_Libretro* lr) {
    return lr && lr->core.loaded;
}

/*
 * Shared post-load sequence used by both SDL_Libretro_LoadGame and
 * SDL_Libretro_LoadGame_IO: wire up the retro callbacks, hand the game info to
 * the core, then read back and store the AV info and initialize video.
 *
 * Buffer ownership for info->data is the CALLER's responsibility; this helper
 * never frees it (the core copies the data during retro_load_game when it does
 * not require a full path).
 */
static bool SDL_Libretro_FinishLoadGame(SDL_Libretro* lr, const struct retro_game_info* info,
                                        SDL_Renderer* renderer) {
    lr->core.renderer = renderer;
    lr->core.window = SDL_GetRenderWindow(renderer);

    lr->core.symbols.retro_set_video_refresh(SDL_Libretro_VideoRefresh);
    lr->core.symbols.retro_set_audio_sample(SDL_Libretro_AudioSample);
    lr->core.symbols.retro_set_audio_sample_batch(SDL_Libretro_AudioSampleBatch);
    lr->core.symbols.retro_set_input_poll(SDL_Libretro_InputPoll);
    lr->core.symbols.retro_set_input_state(SDL_Libretro_InputState);

    if (!lr->core.symbols.retro_load_game(info)) {
        SDL_SetError("SDL_libretro: Core rejected the game");
        return false;
    }

    struct retro_system_av_info avInfo = {0};
    lr->core.symbols.retro_get_system_av_info(&avInfo);
    lr->core.width = avInfo.geometry.base_width;
    lr->core.height = avInfo.geometry.base_height;
    lr->core.fps = avInfo.timing.fps;
    lr->core.sampleRate = avInfo.timing.sample_rate;
    lr->core.aspectRatio = avInfo.geometry.aspect_ratio;

    if (!SDL_Libretro_InitVideo(lr)) {
        lr->core.symbols.retro_unload_game();
        return false;
    }

    SDL_Log("SDL_libretro: Game loaded (%ux%u @ %.2f fps, %.0f Hz)",
        lr->core.width, lr->core.height, lr->core.fps, lr->core.sampleRate);

    return true;
}

bool SDL_Libretro_LoadGame(SDL_Libretro* lr, const char* gamePath, SDL_Renderer* renderer) {
    if (!lr || !lr->core.loaded || !renderer) {
        SDL_SetError("SDL_libretro: Core not loaded or invalid renderer");
        return false;
    }

    struct retro_game_info gameInfo = {0};
    void* fileData = NULL;

    if (gamePath) {
        SDL_strlcpy(lr->core.contentPath, gamePath, sizeof(lr->core.contentPath));
        gameInfo.path = gamePath;

        if (!lr->core.needFullpath) {
            size_t fileSize = 0;
            fileData = SDL_LoadFile(gamePath, &fileSize);
            if (!fileData) {
                SDL_SetError("SDL_libretro: Failed to load game file '%s'", gamePath);
                return false;
            }
            gameInfo.data = fileData;
            gameInfo.size = fileSize;
        }
    }

    bool result = SDL_Libretro_FinishLoadGame(lr, gamePath ? &gameInfo : NULL, renderer);

    if (fileData) {
        SDL_free(fileData);
    }

    return result;
}

bool SDL_Libretro_LoadGame_IO(SDL_Libretro* lr, SDL_IOStream* stream,
                              const char* contentPath, SDL_Renderer* renderer) {
    if (!lr || !lr->core.loaded || !renderer || !stream) {
        SDL_SetError("SDL_libretro: Invalid arguments");
        return false;
    }

    /* A stream has no real path on disk, so cores that require a full path
     * cannot be served from one. */
    if (lr->core.needFullpath) {
        SDL_SetError("SDL_libretro: Core '%s' requires a full content path and cannot load from a stream",
            lr->core.libraryName);
        return false;
    }

    if (contentPath) {
        SDL_strlcpy(lr->core.contentPath, contentPath, sizeof(lr->core.contentPath));
    }

    /* Read the whole stream into memory; pass false so the CALLER keeps
     * ownership of the stream (and is responsible for closing it). */
    size_t size = 0;
    void* data = SDL_LoadFile_IO(stream, &size, false);
    if (!data) {
        /* SDL_SetError already set by SDL_LoadFile_IO. */
        return false;
    }

    struct retro_game_info gameInfo = {0};
    gameInfo.path = contentPath;
    gameInfo.data = data;
    gameInfo.size = size;
    gameInfo.meta = NULL;

    bool result = SDL_Libretro_FinishLoadGame(lr, &gameInfo, renderer);

    /* The core copies the content during retro_load_game (need_fullpath is
     * false here), so the temporary buffer can be released now. */
    SDL_free(data);

    return result;
}

void SDL_Libretro_UnloadGame(SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) return;

    SDL_Libretro_CloseAudio(lr);
    SDL_Libretro_CloseVideo(lr);

    if (lr->core.contentPath[0] != '\0') {
        lr->core.symbols.retro_unload_game();
        lr->core.contentPath[0] = '\0';
    }
}

bool SDL_Libretro_IsGameReady(const SDL_Libretro* lr) {
    return lr && lr->core.loaded && lr->core.renderer != NULL;
}

bool SDL_Libretro_IsGameRequired(const SDL_Libretro* lr) {
    return lr && !lr->core.supportNoGame;
}

bool SDL_Libretro_Reset(SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) {
        SDL_SetError("SDL_libretro: No core loaded");
        return false;
    }
    lr->core.symbols.retro_reset();
    return true;
}

void SDL_Libretro_RunFrame(SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) return;
    lr->core.symbols.retro_run();
}

bool SDL_Libretro_ShouldClose(const SDL_Libretro* lr) {
    return lr && lr->core.shutdown;
}

/* Directory setters */
bool SDL_Libretro_SetCoreDirectory(SDL_Libretro* lr, const char* path) {
    if (!lr) return false;
    SDL_strlcpy(lr->coreDirectory, path ? path : "", sizeof(lr->coreDirectory));
    return true;
}

bool SDL_Libretro_SetSaveDirectory(SDL_Libretro* lr, const char* path) {
    if (!lr) return false;
    SDL_strlcpy(lr->saveDirectory, path ? path : "", sizeof(lr->saveDirectory));
    return true;
}

bool SDL_Libretro_SetSystemDirectory(SDL_Libretro* lr, const char* path) {
    if (!lr) return false;
    SDL_strlcpy(lr->systemDirectory, path ? path : "", sizeof(lr->systemDirectory));
    return true;
}

bool SDL_Libretro_SetCoreAssetsDirectory(SDL_Libretro* lr, const char* path) {
    if (!lr) return false;
    SDL_strlcpy(lr->coreAssetsDirectory, path ? path : "", sizeof(lr->coreAssetsDirectory));
    return true;
}

bool SDL_Libretro_SetUsername(SDL_Libretro* lr, const char* username) {
    if (!lr) return false;
    SDL_strlcpy(lr->username, username ? username : "", sizeof(lr->username));
    return true;
}

/* Volume / Speed */
void SDL_Libretro_SetVolume(SDL_Libretro* lr, float volume) {
    if (!lr) return;
    lr->volume = SDL_clamp(volume, 0.0f, 1.0f);
}

float SDL_Libretro_GetVolume(const SDL_Libretro* lr) {
    return lr ? lr->volume : 0.0f;
}

void SDL_Libretro_SetSpeed(SDL_Libretro* lr, float speed) {
    if (!lr) return;
    lr->speed = SDL_max(speed, 0.1f);
}

float SDL_Libretro_GetSpeed(const SDL_Libretro* lr) {
    return lr ? lr->speed : 1.0f;
}

/* Metadata */
const char* SDL_Libretro_GetCoreName(const SDL_Libretro* lr) {
    return (lr && lr->core.loaded) ? lr->core.libraryName : "";
}

const char* SDL_Libretro_GetCoreVersion(const SDL_Libretro* lr) {
    return (lr && lr->core.loaded) ? lr->core.libraryVersion : "";
}

const char* SDL_Libretro_GetValidExtensions(const SDL_Libretro* lr) {
    return (lr && lr->core.loaded) ? lr->core.validExtensions : "";
}

/* OSD */
void SDL_Libretro_SetMessage(SDL_Libretro* lr, const char* msg, double duration) {
    if (!lr) return;
    SDL_strlcpy(lr->osdMessage, msg ? msg : "", sizeof(lr->osdMessage));
    lr->osdEndTimeMs = SDL_GetTicks() + (Uint64)(duration * 1000.0);
}

/* VFS */
bool SDL_Libretro_SetVFS(SDL_Libretro* lr, const SDL_Libretro_VFSCallbacks* vfs) {
    if (!lr) return false;
    if (vfs) {
        lr->vfs = *vfs;
        lr->vfsActive = true;
    } else {
        SDL_memset(&lr->vfs, 0, sizeof(lr->vfs));
        lr->vfsActive = false;
    }
    return true;
}

#undef LOAD_SYM

#endif /* SDL_LIBRETRO_CORE_IMPL_ONCE */
