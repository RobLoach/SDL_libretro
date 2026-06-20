#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_CORE_IMPL_ONCE)
#define SDL_LIBRETRO_CORE_IMPL_ONCE

/*
 * SDL_libretro - core lifecycle implementation
 */

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

    // Set the initial SDL3 VFS callbacks.
    SDL_Libretro_SetVFS(lr, NULL);

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

size_t SDL_Libretro_GetFileName(char* dst, size_t dstSize, const char* path, bool withExtension) {
    if (!dst || dstSize == 0) return 0;
    dst[0] = '\0';
    if (!path) return 0;

    // Skip past the last path separator to the base name.
    const char* base = SDL_strrchr(path, '/');
    if (!base) base = SDL_strrchr(path, '\\');
    base = base ? base + 1 : path;

    SDL_strlcpy(dst, base, dstSize);

    if (!withExtension) {
        char* dot = SDL_strrchr(dst, '.');
        if (dot) *dot = '\0';
    }

    return SDL_strlen(dst);
}

bool SDL_Libretro_LoadGame(SDL_Libretro* lr, const char* gamePath, SDL_Renderer* renderer) {
    if (!lr || !lr->core.loaded || !renderer) {
        SDL_SetError("SDL_libretro: Core not loaded or invalid renderer");
        return false;
    }

    lr->core.renderer = renderer;
    lr->core.window = SDL_GetRenderWindow(renderer);

    struct retro_game_info gameInfo = {0};
    void* fileData = NULL;

    if (gamePath) {
        SDL_strlcpy(lr->core.contentPath, gamePath, sizeof(lr->core.contentPath));

        // Get the Content Name (base name without extension).
        SDL_Libretro_GetFileName(lr->core.contentName, sizeof(lr->core.contentName), gamePath, false);

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

    lr->core.symbols.retro_set_video_refresh(SDL_Libretro_VideoRefresh);
    lr->core.symbols.retro_set_audio_sample(SDL_Libretro_AudioSample);
    lr->core.symbols.retro_set_audio_sample_batch(SDL_Libretro_AudioSampleBatch);
    lr->core.symbols.retro_set_input_poll(SDL_Libretro_InputPoll);
    lr->core.symbols.retro_set_input_state(SDL_Libretro_InputState);

    bool result = lr->core.symbols.retro_load_game(gamePath ? &gameInfo : NULL);

    if (fileData) {
        SDL_free(fileData);
    }

    if (!result) {
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

    // A missing device shouldn't stop the game from running. Apps can re-init later with SDL_Libretro_InitAudio() or RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO
    if (!SDL_Libretro_InitAudio(lr)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "SDL_libretro: Audio unavailable: %s", SDL_GetError());
    }

    SDL_Log("SDL_libretro: Game loaded (%ux%u @ %.2f fps, %.0f Hz)",
        lr->core.width, lr->core.height, lr->core.fps, lr->core.sampleRate);

    return true;
}

void SDL_Libretro_UnloadGame(SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) return;

    // Call retro_unload_game() prior to closing the audio and video.
    if (lr->core.contentPath[0] != '\0') {
        lr->core.symbols.retro_unload_game();
        lr->core.contentPath[0] = '\0';
        lr->core.contentName[0] = '\0';
    }

    SDL_Libretro_CloseAudio(lr);
    SDL_Libretro_CloseVideo(lr);
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

    // Drop the audio queued from before the reset so it doesn't continue into the reset.
    if (lr->core.audioStream) {
        SDL_ClearAudioStream(lr->core.audioStream);
        lr->core.singleSampleCount = 0;
        lr->core.drcDriftAvg = 0.0;
    }
    return true;
}

/**
 * Advance the core a single tick. Also report to the frame-time callback if needed.
 */
static void SDL_Libretro_Tick(SDL_Libretro* lr, retro_usec_t referenceUsec) {
    if (lr->core.runloop_frame_time.callback) {
        retro_usec_t delta = referenceUsec;
        // First tick (or right after a reset) has no measured cadence yet, so fall back to the reference the core declared.
        if (!lr->core.runloop_frame_time_last) {
            delta = lr->core.runloop_frame_time.reference;
        }
        lr->core.runloop_frame_time_last = referenceUsec;
        lr->core.runloop_frame_time.callback(delta);
    }

    // Report audio buffer occupancy so the core can frame-skip if an underrun
    // looms. Per the libretro spec this fires right before retro_run().
    SDL_Libretro_ReportAudioBufferStatus(lr);

    // Run the frame.
    lr->core.symbols.retro_run();

    // Only pump the core's async-audio callback when audio is actually up.
    if (lr->core.audioStream && lr->core.audio_callback.callback) {
        lr->core.audio_callback.callback();
    }
}

void SDL_Libretro_RunFrame(SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) return;

    // Pending Video Driver Reinit
    if (lr->core.videoReinitPending) {
        lr->core.videoReinitPending = false;
        SDL_Libretro_InitVideo(lr);
    }

    // Pending Audio Driver Reinit
    if (lr->core.audioReinitPending) {
        lr->core.audioReinitPending = false;
        SDL_Libretro_InitAudio(lr);
    }

    // Keep audio consumption locked to speed + nudge the queue toward its target
    // fill. Done here, above all three return paths below, so it runs every frame.
    SDL_Libretro_UpdateDRC(lr, lr->speed);

    // Wall-clock delta since the previous RunFrame.
    Uint64 nowNS = SDL_GetTicksNS();
    if (lr->lastTickNS == 0) {
        // First call: seed the clock and run exactly one tick.
        lr->lastTickNS = nowNS;
        lr->speedAccumulator = 0.0;
        SDL_Libretro_Tick(lr, 0);
        return;
    }

    // Calculate the frame time in seconds.
    double frameTime = (double)(nowNS - lr->lastTickNS) / 1.0e9;
    lr->lastTickNS = nowNS;

    // Target frame period from the core's declared fps (default 60).
    double framePeriod = (lr->core.fps > 0.0) ? (1.0 / lr->core.fps) : (1.0 / 60.0);

    // Reference frame-time the core is told about, in microseconds.
    retro_usec_t referenceUsec = (retro_usec_t)(framePeriod * 1.0e6 / (double)lr->speed);

    // At normal speed, when the loop is already paced close to the core's frame rate (e.g. a vsync'd 60 Hz display with a ~60 fps core), run exactly one tick and discard the accumulator. This avoids the beat-frequency judder of occasionally emitting 0 or 2 ticks. Gating on the *measured* cadence keeps the core bounded when vsync is off / FPS uncapped.
    double cadence = (framePeriod > 0.0) ? (frameTime / framePeriod) : 0.0;
    if (lr->speed == 1.0f && cadence > 0.9 && cadence < 1.1) {
        lr->speedAccumulator = 0.0;
        SDL_Libretro_Tick(lr, referenceUsec);
        return;
    }

    lr->speedAccumulator += frameTime * (double)lr->speed;

    // Cap iterations to avoid a spiral of death on slow hardware.
    int maxTicks = (int)(lr->speed + 1.0f);
    if (maxTicks < 1) maxTicks = 1;

    // Clamp the accumulator so a frame-time spike (game load, window drag, menu pause) can't leave a backlog that runs the core fast afterwards.
    double maxAccumulator = framePeriod * (double)maxTicks;
    if (lr->speedAccumulator > maxAccumulator) {
        lr->speedAccumulator = maxAccumulator;
    }

    // Run the required number of ticks to catch up to what's needed.
    while (lr->speedAccumulator >= framePeriod && maxTicks-- > 0) {
        lr->speedAccumulator -= framePeriod;
        SDL_Libretro_Tick(lr, referenceUsec);
    }
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
    if (lr->core.audioStream) {
        SDL_SetAudioStreamGain(lr->core.audioStream, lr->volume);
    }
}

float SDL_Libretro_GetVolume(const SDL_Libretro* lr) {
    return lr ? lr->volume : 0.0f;
}

void SDL_Libretro_SetSpeed(SDL_Libretro* lr, float speed) {
    if (!lr) return;
    lr->speed = SDL_max(speed, 0.1f);

    // Reset DRC so the next steady state re-settles from a neutral ratio, and apply the new rate immediately for instant pitch response.
    if (lr->core.audioStream) {
        lr->core.drcAdjustment = 1.0f;
        lr->core.drcDriftAvg = 0.0;
        SDL_SetAudioStreamFrequencyRatio(lr->core.audioStream, lr->speed * lr->core.drcAdjustment);
    }
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

    // Allow clearing the message.
    if (msg == NULL || msg[0] == '\0') {
        lr->osdEndTimeMs = 0;
        lr->osdMessage[0] = '\0';
        return;
    }

    SDL_strlcpy(lr->osdMessage, msg, sizeof(lr->osdMessage));
    lr->osdEndTimeMs = SDL_GetTicks() + (Uint64)(duration * 1000.0);
}

/**
 * Retrieves the active on-screen message.
 *
 * @return A string for the message to display. NULL if there there is no message to display.
 */
const char* SDL_Libretro_GetMessage(SDL_Libretro* lr) {
    if (!lr || lr->osdMessage[0] == '\0' || lr->osdEndTimeMs == 0) {
        return NULL;
    }

    // Timeout the message if needed.
    if (SDL_GetTicks() > lr->osdEndTimeMs) {
        lr->osdMessage[0] = '\0';
        lr->osdEndTimeMs = 0;
        return NULL;
    }

    return lr->osdMessage;
}

#undef LOAD_SYM

#endif /* SDL_LIBRETRO_CORE_IMPL_ONCE */
