/**
 * SDL_libretro - core lifecycle implementation
 *
 * @file SDL_libretro_core.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_CORE_IMPL_ONCE)
#define SDL_LIBRETRO_CORE_IMPL_ONCE

#define LOAD_SYM(sym) do { \
    SDL_FunctionPointer fp = SDL_LoadFunction(lr->core.symbols.handle, #sym); \
    SDL_memcpy(&lr->core.symbols.sym, &fp, sizeof(fp)); \
    if (!fp) { \
        SDL_SetError("[SDL_Libretro] Failed to load symbol '%s'", #sym); \
        return false; \
    } \
} while (0)

SDL_Libretro* SDL_Libretro_Create(void) {
    SDL_Libretro* lr = (SDL_Libretro*)SDL_calloc(1, sizeof(SDL_Libretro));
    if (!lr) {
        SDL_SetError("[SDL_Libretro] Failed to allocate context");
        return NULL;
    }

    // Set the initial SDL3 VFS callbacks.
    SDL_Libretro_SetVFS(lr, NULL);

    lr->volume = 1.0f;
    lr->speed = 1.0f;
    lr->rewindMaxBytes = SDL_LIBRETRO_REWIND_DEFAULT_MAX_BYTES;
    SDL_strlcpy(lr->username, "SDL_libretro", sizeof(lr->username));
    SDL_strlcpy(lr->coreDirectory, "cores", sizeof(lr->coreDirectory));
    SDL_strlcpy(lr->saveDirectory, "saves", sizeof(lr->saveDirectory));
    SDL_strlcpy(lr->systemDirectory, "system", sizeof(lr->systemDirectory));
    SDL_strlcpy(lr->coreAssetsDirectory, "assets", sizeof(lr->coreAssetsDirectory));

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

/**
 * Destroys the given libretro context.
 *
 * Will also unload the active game and core if needed.
 */
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

    SDL_Libretro_FreeMessages(lr);
    SDL_Libretro_CloseConfig(lr);

    SDL_free(lr);
}

/**
 * Loads a libretro core.
 */
bool SDL_Libretro_LoadCore(SDL_Libretro* lr, const char* corePath) {
    if (!lr || !corePath) {
        SDL_SetError("[SDL_Libretro] Invalid arguments");
        return false;
    }

    if (SDL_Libretro_active && SDL_Libretro_active != lr) {
        SDL_SetError("[SDL_Libretro] Another context already has a core loaded");
        return false;
    }

    if (lr->core.loaded) {
        SDL_Libretro_UnloadCore(lr);
    }

    SDL_memset(&lr->core, 0, sizeof(lr->core));

    lr->core.symbols.handle = SDL_LoadObject(corePath);
    if (!lr->core.symbols.handle) {
        SDL_SetError("[SDL_Libretro] Failed to load core '%s': %s", corePath, SDL_GetError());
        return false;
    }

    // Verify core API version.
    LOAD_SYM(retro_api_version);
    lr->core.apiVersion = lr->core.symbols.retro_api_version();
    if (lr->core.apiVersion != 1) {
        SDL_UnloadObject(lr->core.symbols.handle);
        SDL_SetError("[SDL_Libretro] Unsupported Core API Version: %d", (int)lr->core.apiVersion);
        SDL_memset(&lr->core, 0, sizeof(lr->core));
        return false;
    }

    LOAD_SYM(retro_init);
    LOAD_SYM(retro_deinit);
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

    // Default the content name to the core's reported name.
    SDL_strlcpy(lr->core.contentName, lr->core.libraryName, sizeof(lr->core.contentName));

    // Config
    SDL_Libretro_LoadCoreConfig(lr);

    lr->core.symbols.retro_init();
    lr->core.loaded = true;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] Core loaded: %s %s", lr->core.libraryName, lr->core.libraryVersion);

    return true;
}

/**
 * Unloads the actively loaded core.
 *
 * Will also unload the game if it's still loaded.
 *
 * @see SD_Libretro_UnloadGame()
 */
void SDL_Libretro_UnloadCore(SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) return;

    SDL_Libretro_UnloadGame(lr);
    SDL_Libretro_SaveCoreConfig(lr);

    lr->core.symbols.retro_deinit();
    if (lr->core.symbols.handle) {
        SDL_UnloadObject(lr->core.symbols.handle);
    }

    SDL_Libretro_CloseSensors(lr);
    SDL_Libretro_CloseMicrophone(lr);
    SDL_Libretro_FreeCoreOptions(lr);
    if (lr->core.inputDescriptors) {
        SDL_free(lr->core.inputDescriptors);
    }
    if (lr->core.controllerInfo) {
        SDL_free(lr->core.controllerInfo);
    }
    SDL_Libretro_FreeMemoryMap(lr);
    SDL_Libretro_FreeContentInfoOverrides(lr);

    SDL_memset(&lr->core, 0, sizeof(lr->core));
    if (SDL_Libretro_active == lr) {
        SDL_Libretro_active = NULL;
    }

    SDL_Log("[SDL_Libretro] Core unloaded");
}

bool SDL_Libretro_IsCoreReady(const SDL_Libretro* lr) {
    return lr && lr->core.loaded;
}

/**
 * Copy the file name portion of a path into a caller-provided buffer.
 *
 * The leading directory components and (optionally) the trailing extension are stripped. Useful for deriving save/state file names from a content path.
 *
 * @param dst the destination buffer to fill (always null-terminated).
 * @param dstSize the size of `dst` in bytes.
 * @param path the source path, may be NULL.
 * @param withExtension if true, keep the file extension; if false, strip it.
 * @returns the length of the resulting string in `dst`, excluding the null terminator (0 on invalid arguments).
 */
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

/**
 * Build a path in the save directory for the currently loaded content.
 *
 * Produces "<saveDirectory>/<contentName><extension>" (or just "<contentName><extension>" when no save directory is set). Handy for deriving SRAM (".srm"), RTC (".rtc"), or save-state file names without hardcoding them.
 *
 * @param lr the libretro context.
 * @param extension the extension to append, including the dot (NULL or "" for none).
 * @param dst the destination buffer (always null-terminated when dstSize > 0).
 * @param dstSize the size of `dst` in bytes.
 * @returns the length of the resulting string, or 0 if no content is loaded or on invalid arguments.
 */
size_t SDL_Libretro_GetSavePath(const SDL_Libretro* lr, const char* extension, char* dst, size_t dstSize) {
    if (!dst || dstSize == 0) return 0;
    dst[0] = '\0';
    if (!lr || lr->core.contentName[0] == '\0') return 0;
    if (!extension) extension = "";
    if (lr->saveDirectory[0] != '\0') {
        SDL_snprintf(dst, dstSize, "%s/%s%s", lr->saveDirectory, lr->core.contentName, extension);
    } else {
        SDL_snprintf(dst, dstSize, "%s%s", lr->core.contentName, extension);
    }
    return SDL_strlen(dst);
}

/**
 * Free the stored content-info overrides, including the deep-copied extension strings, and reset the count.
 *
 * @internal
 */
static void SDL_Libretro_FreeContentInfoOverrides(SDL_Libretro* lr) {
    if (lr->core.contentInfoOverrides) {
        for (unsigned i = 0; i < lr->core.contentInfoOverrideCount; i++) {
            SDL_free((void*)lr->core.contentInfoOverrides[i].extensions);
        }
        SDL_free(lr->core.contentInfoOverrides);
        lr->core.contentInfoOverrides = NULL;
    }
    lr->core.contentInfoOverrideCount = 0;
}

/**
 * Gets the content-info override whose extension list matches `ext`.
 *
 * Cores register overrides via RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE to change need_fullpath / persistent_data for specific content extensions.
 *
 * @returns the matching override index, or -1 if none matches.
 * @internal
 */
static int SDL_Libretro_GetContentInfoOverride(const SDL_Libretro* lr, const char* ext) {
    if (!lr || !ext || ext[0] == '\0') return -1;
    for (unsigned i = 0; i < lr->core.contentInfoOverrideCount; i++) {
        if (SDL_Libretro_ExtensionInList(ext, lr->core.contentInfoOverrides[i].extensions)) {
            return (int)i;
        }
    }
    return -1;
}

/**
 * Whether the core needs the full content path (rather than a loaded data buffer) for the given extension. Defaults to the core's system-info value, overridden per-extension by RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE.
 *
 * @internal
 */
static bool SDL_Libretro_ContentNeedsFullpath(const SDL_Libretro* lr, const char* ext) {
    int i = SDL_Libretro_GetContentInfoOverride(lr, ext);
    return i >= 0 ? lr->core.contentInfoOverrides[i].need_fullpath : lr->core.needFullpath;
}

/**
 * Whether the loaded content buffer must persist until the core is unloaded for the given extension. Only content-info overrides enable this; plain system info has no such concept, so it defaults to false.
 *
 * @internal
 */
static bool SDL_Libretro_ContentPersistData(const SDL_Libretro* lr, const char* ext) {
    int i = SDL_Libretro_GetContentInfoOverride(lr, ext);
    return i >= 0 ? lr->core.contentInfoOverrides[i].persistent_data : false;
}

/**
 * Reset the loaded-content identity to the no-content state.
 *
 * Clear the content path/dir/ext, restore the content name to the core's library-name default (so GetSavePath keeps working), free any persistent content buffer, and invalidate gameInfoExt.
 *
 * @internal
 */
static void SDL_Libretro_ResetContentState(SDL_Libretro* lr) {
    lr->core.contentPath[0] = '\0';
    lr->core.contentDir[0] = '\0';
    lr->core.contentExt[0] = '\0';
    SDL_strlcpy(lr->core.contentName, lr->core.libraryName, sizeof(lr->core.contentName));

    if (lr->core.gameInfoExt.persistent_data) {
        SDL_free((void*)lr->core.gameInfoExt.data);
    }
    SDL_memset(&lr->core.gameInfoExt, 0, sizeof(lr->core.gameInfoExt));
}

/**
 * Loads a game at the given path.
 *
 * A renderer is not required to load a game. If none has been set via
 * SDL_Libretro_SetRenderer(), the game still loads and runs (audio and input
 * work); video texture creation is deferred until a renderer is set, and until
 * then rendered frames are dropped.
 *
 * @see SDL_Libretro_UnloadGame()
 * @see SDL_Libretro_SetRenderer()
 */
bool SDL_Libretro_LoadGame(SDL_Libretro* lr, const char* gamePath) {
    if (!lr || !lr->core.loaded) {
        SDL_SetError("[SDL_Libretro] Core not loaded");
        return false;
    }

    // A core that didn't opt into no-content (SET_SUPPORT_NO_GAME) can't run without a game.
    if (!gamePath && !lr->core.supportNoGame) {
        SDL_SetError("[SDL_Libretro] This core requires content");
        return false;
    }

    // Switching content: unload any game that's already running first.
    if (lr->core.gameLoaded) {
        SDL_Libretro_UnloadGame(lr);
    }

    struct retro_game_info gameInfo = {0};
    void* fileData = NULL;
    bool persistData = false;

    // Cleared here so a no-content load leaves GET_GAME_INFO_EXT invalid.
    SDL_memset(&lr->core.gameInfoExt, 0, sizeof(lr->core.gameInfoExt));

    if (gamePath) {
        SDL_strlcpy(lr->core.contentPath, gamePath, sizeof(lr->core.contentPath));

        // Content base name (no extension) and lower-case extension.
        SDL_Libretro_GetFileName(lr->core.contentName, sizeof(lr->core.contentName), gamePath, false);
        const char* ext = SDL_Libretro_GetContentExtension(lr);
        SDL_strlcpy(lr->core.contentExt, ext, sizeof(lr->core.contentExt));
        for (char* c = lr->core.contentExt; *c; c++)
            *c = (char)SDL_tolower((unsigned char)*c);

        // Directory containing the content file.
        SDL_strlcpy(lr->core.contentDir, gamePath, sizeof(lr->core.contentDir));
        char* sep = SDL_strrchr(lr->core.contentDir, '/');
        if (!sep) sep = SDL_strrchr(lr->core.contentDir, '\\');
        if (sep)
            *sep = '\0';
        else
            lr->core.contentDir[0] = '\0';

        bool needFullpath = SDL_Libretro_ContentNeedsFullpath(lr, ext);
        persistData = SDL_Libretro_ContentPersistData(lr, ext);

        gameInfo.path = lr->core.contentPath;

        if (!needFullpath) {
            size_t fileSize = 0;
            fileData = SDL_LoadFile(gamePath, &fileSize);
            if (!fileData) {
                SDL_SetError("[SDL_Libretro] Failed to load game file '%s'", gamePath);
                SDL_Libretro_ResetContentState(lr);
                return false;
            }
            gameInfo.data = fileData;
            gameInfo.size = fileSize;
        }

        // Update the game info.
        lr->core.gameInfoExt.full_path       = lr->core.contentPath;
        lr->core.gameInfoExt.dir             = lr->core.contentDir;
        lr->core.gameInfoExt.name            = lr->core.contentName;
        lr->core.gameInfoExt.ext             = lr->core.contentExt;
        lr->core.gameInfoExt.data            = gameInfo.data;
        lr->core.gameInfoExt.size            = gameInfo.size;
        lr->core.gameInfoExt.persistent_data = persistData;
    }

    // Set the callbacks.
    lr->core.symbols.retro_set_video_refresh(SDL_Libretro_VideoRefresh);
    lr->core.symbols.retro_set_audio_sample(SDL_Libretro_AudioSample);
    lr->core.symbols.retro_set_audio_sample_batch(SDL_Libretro_AudioSampleBatch);
    lr->core.symbols.retro_set_input_poll(SDL_Libretro_InputPoll);
    lr->core.symbols.retro_set_input_state(SDL_Libretro_InputState);

    bool result = lr->core.symbols.retro_load_game(gamePath ? &gameInfo : NULL);

    // The gameInfoExt.data owns the content buffer. Persistent content keeps it until unload, otherwise the data is only valid for the duration of the load, so free it now and clear the (now dangling) pointer.
    if (fileData && !(persistData && result)) {
        SDL_free(fileData);
        lr->core.gameInfoExt.data = NULL;
        lr->core.gameInfoExt.size = 0;
    }

    if (!result) {
        SDL_Libretro_ResetContentState(lr);
        SDL_SetError("[SDL_Libretro] Core failed to load the game");
        return false;
    }

    lr->core.gameLoaded = true;

    // Grab the Audio/Video data.
    struct retro_system_av_info avInfo = {0};
    lr->core.symbols.retro_get_system_av_info(&avInfo);
    lr->core.width = avInfo.geometry.base_width;
    lr->core.height = avInfo.geometry.base_height;
    lr->core.fps = avInfo.timing.fps;
    lr->core.sampleRate = avInfo.timing.sample_rate;
    lr->core.aspectRatio = avInfo.geometry.aspect_ratio;

    // Failed video initialization should not fail loading the game. It will
    // hopefully initialize itself on the first render.
    if (lr->renderer) {
        if (!SDL_Libretro_InitVideo(lr)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "[SDL_Libretro] Video failed to initialize: %s", SDL_GetError());
        }
    }

    // A missing device shouldn't stop the game from running. Apps can re-init later with SDL_Libretro_InitAudio() or RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO
    if (!SDL_Libretro_InitAudio(lr)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "[SDL_Libretro] Audio failed to initialize: %s", SDL_GetError());
    }

    SDL_Log("[SDL_Libretro] Game loaded: %s [%ux%u @ %.2ffps]", lr->core.contentName,
        lr->core.width, lr->core.height, lr->core.fps);

    // Allocate rewind buffer now that serialize size is known.
    if (lr->rewindEnabled && !lr->rewindReference && lr->rewindCapacity > 0) {
        SDL_Libretro_SetRewindEnabled(lr, true, lr->rewindCapacity, lr->rewindCaptureInterval);
    }

    return true;
}

/**
 * Unloads the actively loaded game.
 *
 * @see SD_Libretro_UnloadCore()
 */
void SDL_Libretro_UnloadGame(SDL_Libretro* lr) {
    if (!lr || !lr->core.gameLoaded) return;

    // Unload the game before closing audio and video.
    lr->core.symbols.retro_unload_game();
    lr->core.gameLoaded = false;
    SDL_Libretro_ResetContentState(lr);

    SDL_Libretro_RewindFree(lr);
    SDL_Libretro_CloseAudio(lr);
    SDL_Libretro_CloseVideo(lr);
    SDL_Log("[SDL_Libretro] Game unloaded");
}

bool SDL_Libretro_IsGameReady(const SDL_Libretro* lr) {
    return lr && lr->core.gameLoaded;
}

bool SDL_Libretro_IsGameRequired(const SDL_Libretro* lr) {
    return lr && !lr->core.supportNoGame;
}

bool SDL_Libretro_Reset(SDL_Libretro* lr) {
    if (!lr || !lr->core.gameLoaded) {
        SDL_SetError("[SDL_Libretro] No game loaded");
        return false;
    }
    lr->core.symbols.retro_reset();

    // Clear any rewind states.
    SDL_Libretro_ClearRewind(lr);

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

    // Report audio buffer occupancy so the core can frame-skip if an underrun looms. Per the libretro spec this fires right before retro_run().
    SDL_Libretro_ReportAudioBufferStatus(lr);

    // Run the frame.
    lr->core.symbols.retro_run();

    // Capture rewind state after each forward tick.
    SDL_Libretro_RewindCapture(lr);

    // Only pump the core's async-audio callback when audio is actually up.
    if (lr->core.audioStream && lr->core.audio_callback.callback) {
        lr->core.audio_callback.callback();
    }
}

void SDL_Libretro_Update(SDL_Libretro* lr) {
    if (!lr || !lr->core.gameLoaded) return;

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

    // Paused: Do nothing when speed is zero.
    if (lr->speed == 0.0f) return;

    // Rewind mode: step backwards when speed is negative.
    if (lr->rewindEnabled && lr->speed < 0.0f) {
        Uint64 nowNS = SDL_GetTicksNS();
        if (lr->lastTickNS == 0) {
            lr->lastTickNS = nowNS;
        }
        double frameTime = (double)(nowNS - lr->lastTickNS) / 1.0e9;
        lr->lastTickNS = nowNS;
        double framePeriod = (lr->core.fps > 0.0) ? (1.0 / lr->core.fps) : (1.0 / 60.0);
        // Each stored snapshot spans captureInterval real frames (a snapshot is taken every Nth frame), so a single rewind step undoes that many frames of game time. Scale the per-step wall-clock cost by the interval; otherwise speed -1 would rewind captureInterval times faster than speed +1 plays forward.
        unsigned interval = lr->rewindCaptureInterval > 0 ? lr->rewindCaptureInterval : 1;
        double stepPeriod = framePeriod * (double)interval;
        lr->speedAccumulator += frameTime * (double)(-lr->speed);
        // Mute audio and neutralize input for the throwaway re-runs that produce the displayed frames while scrubbing backward.
        lr->rewindActive = true;
        while (lr->speedAccumulator >= stepPeriod) {
            lr->speedAccumulator -= stepPeriod;
            if (!SDL_Libretro_RewindStepState(lr)) break;
            lr->core.symbols.retro_run();
        }
        lr->rewindActive = false;
        return;
    }

    // Keep audio consumption locked to speed + nudge the queue toward its target fill. Done here, above all three return paths below, so it runs every frame.
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

bool SDL_Libretro_IsShutdown(const SDL_Libretro* lr) {
    return lr && lr->core.shutdown;
}

// Directory

/**
 * Sets the associated libretro core directory, where the default set of cores will be loaded from.
 */
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

const char* SDL_Libretro_GetCoreDirectory(SDL_Libretro* lr) {
    if (!lr) return NULL;
    return lr->coreDirectory[0] ? lr->coreDirectory : NULL;
}

const char* SDL_Libretro_GetSaveDirectory(SDL_Libretro* lr) {
    if (!lr) return NULL;
    return lr->saveDirectory[0] ? lr->saveDirectory : NULL;
}

const char* SDL_Libretro_GetSystemDirectory(SDL_Libretro* lr) {
    if (!lr) return NULL;
    return lr->systemDirectory[0] ? lr->systemDirectory : NULL;
}

const char* SDL_Libretro_GetCoreAssetsDirectory(SDL_Libretro* lr) {
    if (!lr) return NULL;
    return lr->coreAssetsDirectory[0] ? lr->coreAssetsDirectory : NULL;
}

bool SDL_Libretro_SetUsername(SDL_Libretro* lr, const char* username) {
    if (!lr) return false;
    SDL_strlcpy(lr->username, username ? username : "", sizeof(lr->username));
    return true;
}

const char* SDL_Libretro_GetUsername(SDL_Libretro* lr) {
    if (!lr) return NULL;
    return lr->username;
}

/**
 * Set the audio volume when playing sounds.
 */
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

    if (speed < 0.0f && lr->rewindEnabled) {
        lr->speed = speed;
        if (lr->core.audioStream) {
            lr->core.drcAdjustment = 1.0f;
            lr->core.drcDriftAvg = 0.0;
            SDL_ClearAudioStream(lr->core.audioStream);
            // Pitch and consume the reversed audio at the rewind speed, mirroring
            // the forward path (which sets the ratio to speed * drcAdjustment).
            SDL_SetAudioStreamFrequencyRatio(lr->core.audioStream, -speed);
        }
    } else {
        lr->speed = SDL_max(speed, 0.0f);
    }

    if (lr->core.audioStream && lr->speed > 0.0f) {
        lr->core.drcAdjustment = 1.0f;
        lr->core.drcDriftAvg = 0.0;
        SDL_SetAudioStreamFrequencyRatio(lr->core.audioStream, lr->speed * lr->core.drcAdjustment);
    }
}

float SDL_Libretro_GetSpeed(const SDL_Libretro* lr) {
    return lr ? lr->speed : 1.0f;
}

/**
 * Sets the threshold for logs to be posted.
 *
 * @see SDL_LOG_PRIORITY_INVALID
 */
void SDL_Libretro_SetLogLevel(SDL_Libretro* lr, SDL_LogPriority level) {
    if (!lr) return;
    switch (level) {
        case SDL_LOG_PRIORITY_INFO: lr->logLevel = RETRO_LOG_INFO; break;
        case SDL_LOG_PRIORITY_WARN: lr->logLevel = RETRO_LOG_WARN; break;
        case SDL_LOG_PRIORITY_ERROR: lr->logLevel = RETRO_LOG_ERROR; break;
        case SDL_LOG_PRIORITY_CRITICAL: lr->logLevel = RETRO_LOG_ERROR; break;
        default: lr->logLevel = RETRO_LOG_DEBUG; break;
    }
}

SDL_LogPriority SDL_Libretro_GetLogLevel(const SDL_Libretro* lr) {
    if (!lr) return SDL_LOG_PRIORITY_INVALID;
    switch (lr->logLevel) {
        case RETRO_LOG_DEBUG: return SDL_LOG_PRIORITY_DEBUG;
        case RETRO_LOG_INFO: return SDL_LOG_PRIORITY_INFO;
        case RETRO_LOG_WARN: return SDL_LOG_PRIORITY_WARN;
        case RETRO_LOG_ERROR: return SDL_LOG_PRIORITY_ERROR;
        default: return SDL_LOG_PRIORITY_INVALID;
    }
}

/**
 * Retrieve the name of the libretro core that's actively loaded.
 */
const char* SDL_Libretro_GetCoreName(const SDL_Libretro* lr) {
    return (lr && lr->core.loaded) ? lr->core.libraryName : "";
}

/**
 * Retrieves the version of the libretro core that's actively loaded.
 */
const char* SDL_Libretro_GetCoreVersion(const SDL_Libretro* lr) {
    return (lr && lr->core.loaded) ? lr->core.libraryVersion : "";
}

/**
 * Gets the default set of valid extensions associated with the core, seperated by a "|".
 */
const char* SDL_Libretro_GetValidExtensions(const SDL_Libretro* lr) {
    return (lr && lr->core.loaded) ? lr->core.validExtensions : "";
}

/**
 * Get the performance level the core requested via SET_PERFORMANCE_LEVEL.
 *
 * @param lr the libretro context.
 * @returns the requested performance level, or 0 if none/invalid.
 *
 * @see RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL
 */
unsigned SDL_Libretro_GetPerformanceLevel(const SDL_Libretro* lr) {
    return lr ? lr->core.performanceLevel : 0;
}

/**
 * Get the extension of the loaded content, as it appears in the path.
 *
 * Returns the text after the last '.' of the content path, in its original case (e.g. "SFC" for "game.SFC"), or "" when no content is loaded or the file has no extension. Note this is the raw-case extension; the lower-cased form handed to cores via GET_GAME_INFO_EXT (gameInfoExt.ext) may differ.
 *
 * @param lr the libretro context.
 * @returns the content extension (no leading dot), or "" if none.
 */
const char* SDL_Libretro_GetContentExtension(const SDL_Libretro* lr) {
    if (!lr || lr->core.contentPath[0] == '\0') return "";
    const char* dot = SDL_strrchr(lr->core.contentPath, '.');
    return dot ? dot + 1 : "";
}

static bool SDL_Libretro_ExtensionInList(const char* ext, const char* pipeList) {
    if (!ext || !pipeList) return false;
    size_t extLen = SDL_strlen(ext);
    const char* p = pipeList;
    while (*p) {
        const char* sep = SDL_strchr(p, '|');
        size_t segLen = sep ? (size_t)(sep - p) : SDL_strlen(p);
        if (segLen == extLen && SDL_strncasecmp(p, ext, extLen) == 0) return true;
        if (!sep) break;
        p = sep + 1;
    }
    return false;
}

#undef LOAD_SYM

#endif /* SDL_LIBRETRO_CORE_IMPL_ONCE */
