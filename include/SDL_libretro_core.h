/**
 * SDL_libretro - core lifecycle implementation
 *
 * @file SDL_libretro_core.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_CORE_IMPL_ONCE)
#define SDL_LIBRETRO_CORE_IMPL_ONCE

/**
 * The clock that paces SDL_Libretro_Update() and the frame-time callback, in nanoseconds.
 *
 * Define before including the implementation to substitute a custom clock, e.g. a fake one in tests.
 */
#ifndef SDL_LIBRETRO_GET_TICKS_NS
#define SDL_LIBRETRO_GET_TICKS_NS() SDL_GetTicksNS()
#endif

/**
 * Reset all controller ports to the libretro default device.
 *
 * Libretro assumes ports are \c RETRO_DEVICE_JOYPAD, so we start with that.
 */
static void SDL_Libretro_ResetPortDevices(SDL_Libretro* lr) {
    for (unsigned i = 0; i < SDL_LIBRETRO_MAX_USERS; i++) {
        lr->core.portDeviceMap[i] = RETRO_DEVICE_JOYPAD;
    }
}

/**
 * Load a required core symbol in \c lr->core.
 *
 * On failure, unload the shared object and reset lr->core so
 * no handle leaks and no half-initialized state is left behind.
 */
#define LOAD_SYM(sym)                                                             \
    do {                                                                          \
        SDL_FunctionPointer fp = SDL_LoadFunction(lr->core.symbols.handle, #sym); \
        SDL_memcpy(&lr->core.symbols.sym, &fp, sizeof(fp));                       \
        if (!fp) {                                                                \
            SDL_SetError("[SDL_Libretro] Failed to load symbol '%s'", #sym);      \
            SDL_UnloadObject(lr->core.symbols.handle);                            \
            SDL_memset(&lr->core, 0, sizeof(lr->core));                           \
            SDL_Libretro_ResetPortDevices(lr);                                    \
            return false;                                                         \
        }                                                                         \
    } while (0)

/**
 * Builds a libretro context.
 *
 * @see SDL_Libretro_LoadCore()
 */
/**
 * @internal
 * Reserves the fixed SDL_EVENT_LIBRETRO range with SDL_RegisterEvents(), once
 * per process, so events the application registers afterwards can't collide
 * with it. SDL hands out user events sequentially from SDL_EVENT_USER, so
 * this registers everything up to SDL_EVENT_LIBRETRO_LAST, and warns when
 * earlier registrations already reached into the range.
 */
static void SDL_Libretro_ReserveEvents(void) {
    static SDL_AtomicInt reserved;
    if (!SDL_CompareAndSwapAtomicInt(&reserved, 0, 1)) {
        return;
    }

    // Probe where SDL's next user event sits, then claim up to the end of
    // the range in one block.
    const Uint32 next = SDL_RegisterEvents(1);
    if (next == 0 || next >= SDL_EVENT_LIBRETRO) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
            "[SDL_Libretro] Registered user events reach the SDL_EVENT_LIBRETRO range (0x%x-0x%x); they may collide",
            (unsigned)SDL_EVENT_LIBRETRO, (unsigned)SDL_EVENT_LIBRETRO_LAST);
        return;
    }
    const int count = (int)(SDL_EVENT_LIBRETRO_LAST - next);
    const Uint32 base = SDL_RegisterEvents(count);
    if (base == 0 || base > SDL_EVENT_LIBRETRO) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
            "[SDL_Libretro] Failed to reserve the SDL_EVENT_LIBRETRO range (0x%x-0x%x); registered user events may collide",
            (unsigned)SDL_EVENT_LIBRETRO, (unsigned)SDL_EVENT_LIBRETRO_LAST);
    }
}

/**
 * Creates a new libretro context.
 *
 * The context starts with default settings: full volume, normal speed, the
 * default keyboard and gamepad mappings, and "SDL_libretro" as the username.
 * The first call in a process also reserves the SDL_EVENT_LIBRETRO event range.
 * Only one context can have a core loaded at a time.
 *
 * @returns the new context, or NULL on allocation failure; call SDL_GetError() for more information. Free it with SDL_Libretro_Destroy().
 *
 * @see SDL_Libretro_Destroy()
 * @see SDL_Libretro_LoadCore()
 */
SDL_Libretro* SDL_Libretro_Create(void) {
    SDL_Libretro* lr = (SDL_Libretro*)SDL_calloc(1, sizeof(SDL_Libretro));
    if (!lr) {
        SDL_SetError("[SDL_Libretro] Failed to allocate context");
        return NULL;
    }

    SDL_Libretro_ReserveEvents();

    // Initial state (the calloc above already zeroed the struct).
    lr->rewindMaxBytes = SDL_LIBRETRO_REWIND_DEFAULT_MAX_BYTES;
    SDL_Libretro_ResetPortDevices(lr);
    SDL_Libretro_SetVFS(lr, NULL);
    SDL_Libretro_SetVolume(lr, 1.0f);
    SDL_Libretro_SetSpeed(lr, 1.0f);
    SDL_Libretro_SetUsername(lr, "SDL_libretro");
    SDL_Libretro_SetLanguage(lr, -1);

    // Keyboard Mappings
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_B] = SDL_SCANCODE_Z;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_Y] = SDL_SCANCODE_A;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_SELECT] = SDL_SCANCODE_RSHIFT;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_START] = SDL_SCANCODE_RETURN;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_UP] = SDL_SCANCODE_UP;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_DOWN] = SDL_SCANCODE_DOWN;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_LEFT] = SDL_SCANCODE_LEFT;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_RIGHT] = SDL_SCANCODE_RIGHT;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] = SDL_SCANCODE_X;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_X] = SDL_SCANCODE_S;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_L] = SDL_SCANCODE_Q;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_R] = SDL_SCANCODE_W;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_L2] = SDL_SCANCODE_E;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_R2] = SDL_SCANCODE_R;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_L3] = SDL_SCANCODE_D;
    lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_R3] = SDL_SCANCODE_F;

    // Gamepad Mappings; analogToDigital stays zeroed (None) from the calloc.
    for (unsigned port = 0; port < SDL_LIBRETRO_MAX_USERS; port++) {
        SDL_memcpy(lr->gamepadButtons[port], SDL_Libretro_DefaultGamepadButtons, sizeof(lr->gamepadButtons[port]));
    }

    return lr;
}

/**
 * Destroys the given libretro context.
 *
 * Will also unload the active game and core if needed, save the config set up
 * with SDL_Libretro_InitConfig(), and drop any SDL_libretro events still in
 * the queue, since they point into the context.
 *
 * @param lr the libretro context to destroy. NULL is ignored.
 *
 * @see SDL_Libretro_Create()
 */
void SDL_Libretro_Destroy(SDL_Libretro* lr) {
    if (!lr) return;

    SDL_Libretro_UnloadGame(lr);
    SDL_Libretro_UnloadCore(lr);

    // Tear down PhysFS if it was initialized.
    SDL_Libretro_PhysFS_Quit(lr);

    for (unsigned i = 0; i < SDL_LIBRETRO_MAX_USERS; i++) {
        if (lr->gamepads[i]) {
            SDL_CloseGamepad(lr->gamepads[i]);
            lr->gamepads[i] = NULL;
        }
    }

    SDL_Libretro_FreeCoreLibrary(lr);
    SDL_Libretro_FreeMessages(lr);
    SDL_Libretro_CloseConfig(lr);

    // Queued SDL_libretro events (including the unloads pushed above) carry
    // pointers into this context; drop them before it goes away.
    SDL_FlushEvents(SDL_EVENT_LIBRETRO, SDL_EVENT_LIBRETRO_LAST);

    SDL_free(lr);
}

/**
 * Retrieve the core path based on its core name from the library.
 *
 * @param lr the libretro context.
 * @param coreName the name of the core to retrieve the path for.
 * @returns the path to the core, or NULL if it wasn't found or the coreName is a path. When several cores share a name, the first in sorted order wins.
 */
static const char* SDL_Libretro_GetCorePathFromName(const SDL_Libretro* lr, const char* coreName) {
    if (!lr || !coreName || !lr->coreLibrary || SDL_strrchr(coreName, '.') != NULL) {
        return NULL;
    }
    for (unsigned i = 0; i < lr->coreLibraryCount; i++) {
        if (lr->coreLibrary[i].corename && SDL_strcasecmp(lr->coreLibrary[i].corename, coreName) == 0) {
            return lr->coreLibrary[i].path;
        }
    }
    return NULL;
}

/**
 * Loads a libretro core.
 *
 * Only one core may be loaded at a time, as the library keeps a single global
 * active context, and the API is not thread-safe; call it from the main SDL
 * thread.
 *
 * @param lr the libretro context.
 * @param core Either the path to the core to load, or a name of the core within the core directory.
 * @returns true when the core was loaded and initialized, false on invalid arguments, when another context already has a core loaded, or when the core fails to load; call SDL_GetError() for more information.
 *
 * @see SDL_Libretro_Create()
 * @see SDL_Libretro_UnloadCore()
 */
bool SDL_Libretro_LoadCore(SDL_Libretro* lr, const char* core) {
    if (!lr || !core) {
        SDL_SetError("[SDL_Libretro] Invalid arguments");
        return false;
    }

    if (SDL_Libretro_active && SDL_Libretro_active != lr) {
        SDL_SetError("[SDL_Libretro] Another context already has a core loaded");
        return false;
    }

    // Make sure the old core is unloaded.
    SDL_Libretro_UnloadCore(lr);

    // Ensure the Saves and System directories exist, since the core may need them.
    if (lr->saveDirectory[0] != '\0') {
        SDL_CreateDirectory(lr->saveDirectory);
    }
    if (lr->systemDirectory[0] != '\0') {
        SDL_CreateDirectory(lr->systemDirectory);
    }

    // If the corePath is just a name, see if it lives in the loaded coreLibrary.
    const char* path = SDL_Libretro_GetCorePathFromName(lr, core);
    if (path) {
        core = path;
    }

    // Load the core handle.
    lr->core.symbols.handle = SDL_LoadObject(core);
    if (!lr->core.symbols.handle) {
        SDL_SetError("[SDL_Libretro] Failed to load core '%s': %s", core, SDL_GetError());
        return false;
    }

    // Verify core API version.
    LOAD_SYM(retro_api_version);
    lr->core.apiVersion = lr->core.symbols.retro_api_version();
    if (lr->core.apiVersion != 1) {
        SDL_UnloadObject(lr->core.symbols.handle);
        SDL_SetError("[SDL_Libretro] Unsupported Core API Version: %d", (int)lr->core.apiVersion);
        SDL_memset(&lr->core, 0, sizeof(lr->core));
        SDL_Libretro_ResetPortDevices(lr);
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

    SDL_strlcpy(lr->core.corePath, core, sizeof(lr->core.corePath));

    // Default to normal speed for the freshly loaded core (lr->core was zeroed
    // above, so speed does not carry over from a previously loaded core).
    lr->core.speed = 1.0f;

    SDL_Libretro_active = lr;

    // Tell the core that we should call our own enviornment callback.
    lr->core.symbols.retro_set_environment(SDL_Libretro_EnvironmentCallback);

    // Grab the initial system info from the core.
    struct retro_system_info sysinfo = {0};
    lr->core.symbols.retro_get_system_info(&sysinfo);
    SDL_strlcpy(lr->core.libraryName, sysinfo.library_name ? sysinfo.library_name : "", sizeof(lr->core.libraryName));
    SDL_strlcpy(lr->core.libraryVersion, sysinfo.library_version ? sysinfo.library_version : "", sizeof(lr->core.libraryVersion));
    SDL_strlcpy(lr->core.validExtensions, sysinfo.valid_extensions ? sysinfo.valid_extensions : "", sizeof(lr->core.validExtensions));
    lr->core.needFullpath = sysinfo.need_fullpath;
    lr->core.blockExtract = sysinfo.block_extract;

    // Default the content name to the core's reported name.
    SDL_strlcpy(lr->core.contentName, lr->core.libraryName, sizeof(lr->core.contentName));

    // Config
    SDL_Libretro_LoadCoreConfig(lr);

    // Initialize the core
    lr->core.symbols.retro_init();

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] Core loaded: %s %s", lr->core.libraryName, lr->core.libraryVersion);

    SDL_Libretro_PushEvent(lr, SDL_EVENT_LIBRETRO_CORE_LOADED, lr->core.libraryName);

    return true;
}

/**
 * Unloads the actively loaded core.
 *
 * Will also unload the game if it's still loaded.
 *
 * @param lr the libretro context. NULL is ignored.
 *
 * @see SDL_Libretro_UnloadGame()
 */
void SDL_Libretro_UnloadCore(SDL_Libretro* lr) {
    if (!lr) return;
    bool hadCore = SDL_Libretro_IsCoreReady(lr);

    SDL_Libretro_UnloadGame(lr);

    // Tear down the loaded core module, if one is present.
    if (SDL_Libretro_IsCoreReady(lr)) {
        SDL_Libretro_SaveCoreConfig(lr);
        lr->core.symbols.retro_deinit();
        SDL_UnloadObject(lr->core.symbols.handle);
    }

    // Unload any frontend resources associated with the core.
    SDL_Libretro_CloseSensors(lr);
    SDL_Libretro_CloseMicrophone(lr);
    SDL_Libretro_FreeCoreOptions(lr);
    SDL_Libretro_FreeInputDescriptors(lr);
    SDL_Libretro_FreeControllerInfo(lr);
    SDL_Libretro_FreeMemoryMap(lr);
    SDL_Libretro_FreeContentInfoOverrides(lr);
    SDL_Libretro_FreeSubsystems(lr);
    SDL_Libretro_StopRumble(lr);

    SDL_memset(&lr->core, 0, sizeof(lr->core));
    SDL_Libretro_ResetPortDevices(lr);
    if (SDL_Libretro_active == lr) {
        SDL_Libretro_active = NULL;
    }
    lr->coreGeneration++;

    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] Core unloaded");

    if (hadCore) {
        SDL_Libretro_PushEvent(lr, SDL_EVENT_LIBRETRO_CORE_UNLOADED, NULL);
    }
}

/**
 * Indicates whether a core is loaded.
 *
 * @param lr the libretro context.
 * @returns true when a core is loaded, false otherwise or when lr is NULL.
 *
 * @see SDL_Libretro_LoadCore()
 * @see SDL_Libretro_IsGameReady()
 */
bool SDL_Libretro_IsCoreReady(const SDL_Libretro* lr) {
    return lr && lr->core.symbols.handle != NULL;
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
 * @internal
 * The extension of a path, without the dot ("" if none).
 */
static const char* SDL_Libretro_GetExtension(const char* path) {
    const char* dot = SDL_strrchr(path, '.');
    return dot ? dot + 1 : "";
}

/**
 * Build a path in the save directory for the currently loaded content.
 *
 * Produces "<saveDirectory>/<contentName><extension>" (or just "<contentName><extension>" when no save directory is set). Handy for deriving SRAM (".srm"), RTC (".rtc"), or save-state (".state") file names without hardcoding them.
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

    if (!extension) {
        extension = "";
    }

    if (lr->saveDirectory[0] != '\0') {
        SDL_snprintf(dst, dstSize, "%s/%s%s", lr->saveDirectory, lr->core.contentName, extension);
    }
    else {
        SDL_snprintf(dst, dstSize, "%s%s", lr->core.contentName, extension);
    }

    return SDL_strlen(dst);
}

/**
 * @internal
 * Free the stored content-info overrides, including the deep-copied extension strings, and reset the count.
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
 * Frees the loaded subsystem data.
 */
static void SDL_Libretro_FreeSubsystems(SDL_Libretro* lr) {
    if (!lr || !lr->core.subsystems) return;
    for (unsigned i = 0; i < lr->core.subsystemCount; i++) {
        // Subsystem Data
        SDL_free((void*)lr->core.subsystems[i].desc);
        SDL_free((void*)lr->core.subsystems[i].ident);

        // Roms
        if (lr->core.subsystems[i].roms) {
            for (unsigned r = 0; r < lr->core.subsystems[i].num_roms; r++) {
                SDL_free((void*)lr->core.subsystems[i].roms[r].desc);
                SDL_free((void*)lr->core.subsystems[i].roms[r].valid_extensions);

                // Rom Memory
                if (lr->core.subsystems[i].roms[r].memory) {
                    for (unsigned m = 0; m < lr->core.subsystems[i].roms[r].num_memory; m++) {
                        SDL_free((void*)lr->core.subsystems[i].roms[r].memory[m].extension);
                    }
                    SDL_free((void*)lr->core.subsystems[i].roms[r].memory);
                }
            }
            SDL_free((void*)lr->core.subsystems[i].roms);
        }
    }
    SDL_free(lr->core.subsystems);
    lr->core.subsystems = NULL;
    lr->core.subsystemCount = 0;
}

/**
 * Frees the retained input descriptors, including the deep-copied description strings.
 */
static void SDL_Libretro_FreeInputDescriptors(SDL_Libretro* lr) {
    if (!lr || !lr->core.inputDescriptors) return;
    for (unsigned i = 0; i < lr->core.inputDescriptorsCount; i++) {
        SDL_free((void*)lr->core.inputDescriptors[i].description);
    }
    SDL_free(lr->core.inputDescriptors);
    lr->core.inputDescriptors = NULL;
    lr->core.inputDescriptorsCount = 0;
}

/**
 * Frees the retained controller info, including the deep-copied type arrays and their strings.
 */
static void SDL_Libretro_FreeControllerInfo(SDL_Libretro* lr) {
    if (!lr || !lr->core.controllerInfo) return;
    for (unsigned i = 0; i < lr->core.controllerInfoCount; i++) {
        if (lr->core.controllerInfo[i].types) {
            for (unsigned t = 0; t < lr->core.controllerInfo[i].num_types; t++) {
                SDL_free((void*)lr->core.controllerInfo[i].types[t].desc);
            }
            SDL_free((void*)lr->core.controllerInfo[i].types);
        }
    }
    SDL_free(lr->core.controllerInfo);
    lr->core.controllerInfo = NULL;
    lr->core.controllerInfoCount = 0;
}

/**
 * Retrieves the given subsystem by subsystem ID.
 */
static const struct retro_subsystem_info* SDL_Libretro_GetSubsystemById(const SDL_Libretro* lr, unsigned subsystemId) {
    if (!lr) return NULL;
    for (unsigned i = 0; i < lr->core.subsystemCount; i++) {
        if (lr->core.subsystems[i].id == subsystemId) {
            return &lr->core.subsystems[i];
        }
    }
    return NULL;
}

/**
 * Retrieves the subsystem that matches either the ident, or description.
 */
static const struct retro_subsystem_info* SDL_Libretro_GetSubsystemByName(const SDL_Libretro* lr, const char* name) {
    if (!lr || !name) return NULL;
    for (unsigned i = 0; i < lr->core.subsystemCount; i++) {
        const struct retro_subsystem_info* subsys = &lr->core.subsystems[i];
        if ((subsys->ident && SDL_strcasecmp(subsys->ident, name) == 0) ||
            (subsys->desc && SDL_strcasecmp(subsys->desc, name) == 0)) {
            return subsys;
        }
    }
    return NULL;
}

/**
 * @internal
 * Gets the content-info override whose extension list matches `ext`.
 *
 * Cores register overrides via RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE to change need_fullpath / persistent_data for specific content extensions.
 *
 * @returns the matching override index, or -1 if none matches.
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
 * @internal
 * Whether the core needs the full content path (rather than a loaded data buffer) for the given extension. Defaults to the core's system-info value, overridden per-extension by RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE.
 */
static bool SDL_Libretro_ContentNeedsFullpath(const SDL_Libretro* lr, const char* ext) {
    int i = SDL_Libretro_GetContentInfoOverride(lr, ext);
    return i >= 0 ? lr->core.contentInfoOverrides[i].need_fullpath : lr->core.needFullpath;
}

/**
 * @internal
 * Whether the loaded content buffer must persist until the core is unloaded for the given extension. Only content-info overrides enable this; plain system info has no such concept, so it defaults to false.
 */
static bool SDL_Libretro_ContentPersistData(const SDL_Libretro* lr, const char* ext) {
    int i = SDL_Libretro_GetContentInfoOverride(lr, ext);
    return i >= 0 ? lr->core.contentInfoOverrides[i].persistent_data : false;
}

/**
 * @internal
 * Free the per-file subsystem GET_GAME_INFO_EXT entries.
 */
static void SDL_Libretro_FreeGameInfoExtArray(SDL_Libretro* lr) {
    for (unsigned i = 0; i < lr->core.gameInfoExtCount; i++) {
        struct retro_game_info_ext* entry = &lr->core.gameInfoExtArray[i];
        SDL_free((void*)entry->full_path);
        SDL_free((void*)entry->dir);
        SDL_free((void*)entry->name);
        SDL_free((void*)entry->ext);
    }
    SDL_free(lr->core.gameInfoExtArray);
    lr->core.gameInfoExtArray = NULL;
    lr->core.gameInfoExtCount = 0;
}

/**
 * @internal
 * Fill a GET_GAME_INFO_EXT entry's path, directory, base name and lower-case extension from a content path. The strings are allocated, and freed by SDL_Libretro_FreeGameInfoExtArray().
 */
static void SDL_Libretro_FillGameInfoExt(struct retro_game_info_ext* entry, const char* path) {
    if (!path) return;
    entry->full_path = SDL_strdup(path);

    char buffer[SDL_LIBRETRO_MAX_PATH];
    SDL_Libretro_GetFileName(buffer, sizeof(buffer), path, false);
    entry->name = SDL_strdup(buffer);

    SDL_Libretro_GetFileName(buffer, sizeof(buffer), path, true);
    const char* dot = SDL_strrchr(buffer, '.');
    char* ext = SDL_strdup(dot ? dot + 1 : "");
    if (ext) {
        for (char* c = ext; *c; c++) *c = (char)SDL_tolower((unsigned char)*c);
    }
    entry->ext = ext;

    SDL_strlcpy(buffer, path, sizeof(buffer));
    char* sep = SDL_strrchr(buffer, '/');
    char* backslash = SDL_strrchr(buffer, '\\');
    if (!sep || (backslash && backslash > sep)) sep = backslash;
    if (sep) *sep = '\0';
    else buffer[0] = '\0';
    entry->dir = SDL_strdup(buffer);
}

/**
 * @internal
 * Reset the loaded-content identity to the no-content state.
 *
 * Clear the content path/dir/ext, restore the content name to the core's library-name default (so GetSavePath keeps working), free any persistent content buffer, and invalidate gameInfoExt.
 */
static void SDL_Libretro_ResetContentState(SDL_Libretro* lr) {
    lr->core.contentPath[0] = '\0';
    lr->core.contentDir[0] = '\0';
    lr->core.contentExt[0] = '\0';
    SDL_strlcpy(lr->core.contentName, lr->core.libraryName, sizeof(lr->core.contentName));

    // Remove the file SDL_Libretro_LoadGame_IO() spilled for a need_fullpath core.
    if (lr->core.contentTempPath[0] != '\0') {
        SDL_RemovePath(lr->core.contentTempPath);
        lr->core.contentTempPath[0] = '\0';
    }

    // Remove an archive extracted for a need_fullpath core, with everything in it.
    if (lr->core.contentTempDir[0] != '\0') {
        SDL_Libretro_RemoveTree(lr->core.contentTempDir);
        lr->core.contentTempDir[0] = '\0';
    }

    if (lr->core.gameInfoExt.persistent_data) {
        SDL_free((void*)lr->core.gameInfoExt.data);
    }
    SDL_memset(&lr->core.gameInfoExt, 0, sizeof(lr->core.gameInfoExt));
    SDL_Libretro_FreeGameInfoExtArray(lr);

    // Images kept by SDL_Libretro_AddDiskImage() and _IO() belong to the game.
    SDL_Libretro_FreeDiskImages(lr);

    // Restart the frame-time clock, so the first tick of new content reports the core's reference.
    lr->core.runloop_frame_time_last = 0;

    // The disk control interface is core-lifetime state (registered from
    // retro_set_environment/retro_init), so it's kept until SDL_Libretro_UnloadCore().
}

/**
 * @internal
 * Populate the content identity from a path.
 *
 * This will fill in the core's path, base name, lower-case ext, directory,
 * etc. It gets it from a content file path, and point gameInfoExt's
 * string fields at them.
 *
 * `path` is the file handed to the core (contentPath / full_path). The name,
 * directory and extension come from `identityPath` instead when it's not NULL,
 * so content spilled to a temp file keeps the name it was given.
 *
 * The caller must have cleared gameInfoExt beforehand and is responsible for
 * gameInfoExt.data / .size / .persistent_data.
 *
 * @returns the raw-case extension (a pointer into contentPath) for
 *          need_fullpath / persistent-data lookups.
 */
static const char* SDL_Libretro_SetContentIdentity(SDL_Libretro* lr, const char* path, const char* identityPath) {
    SDL_strlcpy(lr->core.contentPath, path, sizeof(lr->core.contentPath));
    if (!identityPath) identityPath = path;

    // Content base name (no extension) and lower-case extension.
    SDL_Libretro_GetFileName(lr->core.contentName, sizeof(lr->core.contentName), identityPath, false);
    const char* ext = SDL_Libretro_GetContentExtension(lr);
    SDL_strlcpy(lr->core.contentExt, SDL_Libretro_GetExtension(identityPath), sizeof(lr->core.contentExt));
    for (char* c = lr->core.contentExt; *c; c++)
        *c = (char)SDL_tolower((unsigned char)*c);

    // Directory containing the content file.
    SDL_strlcpy(lr->core.contentDir, identityPath, sizeof(lr->core.contentDir));
    char* sep = SDL_strrchr(lr->core.contentDir, '/');
    if (!sep) sep = SDL_strrchr(lr->core.contentDir, '\\');
    if (sep)
        *sep = '\0';
    else
        lr->core.contentDir[0] = '\0';

    // Initial gameInfoExt.
    lr->core.gameInfoExt.full_path = lr->core.contentPath;
    lr->core.gameInfoExt.dir = lr->core.contentDir;
    lr->core.gameInfoExt.name = lr->core.contentName;
    lr->core.gameInfoExt.ext = lr->core.contentExt;

    return ext;
}

/**
 * Given a game path, load the core that supported its file extension.
 *
 * Cores are tried in core library order (sorted by core name, then path; see
 * SDL_Libretro_SetCoreDirectory()), and the first match in sorted order that
 * loads wins.
 *
 * @return True if the core was successfully loaded.
 */
static bool SDL_Libretro_LoadCoreForGame(SDL_Libretro* lr, const char* gamePath) {
    if (!lr || !gamePath) return false;

    // The game's file extension.
    const char* extension = SDL_Libretro_GetExtension(gamePath);
    if (extension[0] == '\0') {
        SDL_SetError("[SDL_Libretro] Game path '%s' has no file extension", gamePath);
        return false;
    }

    // Try to load cores that match the extension.
    for (unsigned i = 0; i < lr->coreLibraryCount; i++) {
        // Check if the extension is in the lr->coreLibrary[i] list.
        if (!SDL_Libretro_ExtensionInList(extension, lr->coreLibrary[i].supported_extensions)) {
            continue;
        }

        // A candidate core claims this extension, try loading it.
        if (SDL_Libretro_LoadCore(lr, lr->coreLibrary[i].path)) {
            return true;
        }
    }

    SDL_SetError("[SDL_Libretro] No core found for extension '%s'", extension);
    return false;
}

/**
 * @internal
 * Wire the core's audio/video/input callbacks.
 *
 * The symbols must be loaded prior.
 */
static void SDL_Libretro_SetCoreCallbacks(SDL_Libretro* lr) {
    if (!lr) return;
    if (lr->core.symbols.retro_set_video_refresh) lr->core.symbols.retro_set_video_refresh(SDL_Libretro_VideoRefresh);
    if (lr->core.symbols.retro_set_audio_sample) lr->core.symbols.retro_set_audio_sample(SDL_Libretro_AudioSample);
    if (lr->core.symbols.retro_set_audio_sample_batch) lr->core.symbols.retro_set_audio_sample_batch(SDL_Libretro_AudioSampleBatch);
    if (lr->core.symbols.retro_set_input_poll) lr->core.symbols.retro_set_input_poll(SDL_Libretro_InputPoll);
    if (lr->core.symbols.retro_set_input_state) lr->core.symbols.retro_set_input_state(SDL_Libretro_InputState);
}

/**
 * Once the core is loaded, we can finalize loading.
 *
 * Set up the Audio Video information, initialize it if available, and
 * allocate the rewind buffer.
 */
static bool SDL_Libretro_FinishGameLoad(SDL_Libretro* lr) {
    lr->core.gameLoaded = true;

    // Re-apply the app's port devices; cores expect them after retro_load_game().
    for (unsigned port = 0; port < SDL_LIBRETRO_MAX_USERS; port++) {
        if (lr->core.portDeviceMap[port] != RETRO_DEVICE_JOYPAD) {
            lr->core.symbols.retro_set_controller_port_device(port, lr->core.portDeviceMap[port]);
        }
    }

    // Restore the game's battery save before it first runs.
    SDL_Libretro_AutoLoadSRAM(lr);

    // Grab the Audio/Video data.
    struct retro_system_av_info avInfo = {0};
    lr->core.symbols.retro_get_system_av_info(&avInfo);
    lr->core.width = avInfo.geometry.base_width;
    lr->core.height = avInfo.geometry.base_height;
    lr->core.fps = avInfo.timing.fps;
    lr->core.sampleRate = avInfo.timing.sample_rate;
    lr->core.aspectRatio = avInfo.geometry.aspect_ratio;

    // Failed video initialization should not fail loading the game. Video is
    // deferred until a renderer is set via SDL_Libretro_SetRenderer().
    if (lr->renderer && !SDL_Libretro_InitVideo(lr)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "[SDL_Libretro] Video failed to initialize: %s", SDL_GetError());
    }

    // A missing device shouldn't stop the game from running; it plays silently
    // until the game is loaded again.
    if (!SDL_Libretro_InitAudio(lr)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "[SDL_Libretro] Audio failed to initialize: %s", SDL_GetError());
    }

    SDL_Log("[SDL_Libretro] Game loaded: %s [%ux%u @ %.2ffps]", lr->core.contentName, lr->core.width, lr->core.height, lr->core.fps);

    // Allocate the rewind buffer now that the serialize size is known. A core
    // that doesn't report one yet gets it on a later capture instead.
    if (lr->rewindEnabled && !lr->rewindReference && lr->core.symbols.retro_serialize_size() > 0) {
        SDL_Libretro_RewindAllocate(lr);
    }

    SDL_Libretro_PushEvent(lr, SDL_EVENT_LIBRETRO_GAME_LOADED, lr->core.contentName);

    return true;
}

/**
 * @internal
 * Shared tail of SDL_Libretro_LoadGame() and SDL_Libretro_LoadGame_IO().
 *
 * Takes ownership of `fileData` (content bytes already read from an IOStream;
 * NULL to read them from `gamePath` when the core wants a data buffer). The
 * content identity (name, dir, ext) comes from `identityPath` when it's not
 * NULL, otherwise from `gamePath`. The core must already be loaded.
 */
/**
 * @internal
 * Whether the content at `path` exists, as the core would see it.
 */
static bool SDL_Libretro_ContentExists(const SDL_Libretro* lr, const char* path) {
    // Look through the VFS the core will use, which also sees PhysFS mounts.
    if (lr->vfs_interface.stat) {
        return (lr->vfs_interface.stat(path, NULL) & RETRO_VFS_STAT_IS_VALID) != 0;
    }
    return SDL_GetPathInfo(path, NULL);
}

static bool SDL_Libretro_LoadGameCommon(SDL_Libretro* lr, const char* gamePath, const char* identityPath, void* fileData, size_t fileSize) {
    // Check what can be checked before unloading anything, so a request that's
    // bound to fail leaves the running game alone.

    // A core that didn't opt into no-content (SET_SUPPORT_NO_GAME) can't run without a game.
    if (!gamePath && SDL_Libretro_IsGameRequired(lr)) {
        SDL_free(fileData);
        SDL_SetError("[SDL_Libretro] This core requires content");
        return false;
    }

    // The content has to be there: read the bytes for cores that want them,
    // or make sure the file exists for cores that want its path.
    if (gamePath && !fileData) {
        if (!SDL_Libretro_ContentNeedsFullpath(lr, SDL_Libretro_GetExtension(identityPath ? identityPath : gamePath))) {
            fileData = SDL_LoadFile(gamePath, &fileSize);
            if (!fileData) {
                SDL_SetError("[SDL_Libretro] Failed to load game file '%s'", gamePath);
                return false;
            }
        }
        else if (!SDL_Libretro_ContentExists(lr, gamePath)) {
            SDL_SetError("[SDL_Libretro] Game file '%s' not found", gamePath);
            return false;
        }
    }

    // Switching content: unload any game that's already running first.
    SDL_Libretro_UnloadGame(lr);

    struct retro_game_info gameInfo = {0};
    bool persistData = false;

    // Cleared here so a no-content load leaves GET_GAME_INFO_EXT invalid.
    SDL_memset(&lr->core.gameInfoExt, 0, sizeof(lr->core.gameInfoExt));

    if (gamePath) {
        const char* ext = SDL_Libretro_SetContentIdentity(lr, gamePath, identityPath);

        bool needFullpath = SDL_Libretro_ContentNeedsFullpath(lr, ext);
        persistData = SDL_Libretro_ContentPersistData(lr, ext);

        gameInfo.path = lr->core.contentPath;

        if (!needFullpath) {
            if (!fileData) {
                fileData = SDL_LoadFile(gamePath, &fileSize);
                if (!fileData) {
                    SDL_SetError("[SDL_Libretro] Failed to load game file '%s'", gamePath);
                    SDL_Libretro_ResetContentState(lr);
                    return false;
                }
            }
            gameInfo.data = fileData;
            gameInfo.size = fileSize;
        }
        else if (fileData) {
            // The core wants a path, not bytes; drop a pre-read buffer.
            SDL_free(fileData);
            fileData = NULL;
        }

        // The string fields are set above; fill in the content buffer.
        lr->core.gameInfoExt.data = gameInfo.data;
        lr->core.gameInfoExt.size = gameInfo.size;
        lr->core.gameInfoExt.persistent_data = persistData;
    }
    else if (fileData) {
        SDL_free(fileData);
        fileData = NULL;
    }

    SDL_Libretro_SetCoreCallbacks(lr);

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

    SDL_Libretro_FinishGameLoad(lr);
    return true;
}

/**
 * Plain file loader: load a game straight from an OS path, no archive handling.
 *
 * This is the non-dispatching core of SDL_Libretro_LoadGame(). The archive
 * loader (SDL_Libretro_PhysFS_LoadGame) falls back here rather than into the
 * public entry point, so archive routing can never recurse.
 */
static bool SDL_Libretro_LoadGameFile(SDL_Libretro* lr, const char* gamePath) {
    // Try to find what can be loaded with it.
    if (!SDL_Libretro_IsCoreReady(lr)) {
        if (!SDL_Libretro_LoadCoreForGame(lr, gamePath)) {
            SDL_SetError("[SDL_Libretro] Core not loaded");
            return false;
        }
    }

    return SDL_Libretro_LoadGameCommon(lr, gamePath, NULL, NULL, 0);
}

/**
 * Loads a game at the given path.
 *
 * When SDL_LIBRETRO_ENABLE_PHYSFS is enabled, a `.zip` path is routed through
 * SDL_Libretro_PhysFS_LoadGame() automatically.
 *
 * Pass a NULL gamePath to start the loaded core without content. This only
 * works for cores that report RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME (see
 * SDL_Libretro_IsGameRequired()); otherwise it fails and SDL_GetError() says
 * the core requires content. A core must already be loaded, as there is no
 * extension to pick one by.
 *
 * @param lr the libretro context.
 * @param gamePath the path of the content to load, or NULL for no content.
 * @returns true when the game is running; call SDL_GetError() for more information on failure.
 *
 * @see SDL_Libretro_IsGameRequired()
 * @see SDL_Libretro_UnloadGame()
 * @see SDL_Libretro_SetRenderer()
 * @see SDL_Libretro_PhysFS_LoadGame()
 */
bool SDL_Libretro_LoadGame(SDL_Libretro* lr, const char* gamePath) {
    if (!lr) return false;

#if defined(SDL_LIBRETRO_ENABLE_PHYSFS) && !defined(SDL_LIBRETRO_DISABLE_PHYSFS)
    // Route archives through the PhysFS loader. It only ever falls back to
    // SDL_Libretro_LoadGameFile(), never this dispatcher, so there is no loop.
    if (gamePath && SDL_strcasecmp(SDL_Libretro_GetExtension(gamePath), "zip") == 0) {
        return SDL_Libretro_PhysFS_LoadGame(lr, gamePath);
    }
#endif

    return SDL_Libretro_LoadGameFile(lr, gamePath);
}

/**
 * @internal
 * Get the frontend's temp directory, creating it if needed.
 *
 * It's "<save directory>/tmp/", falling back to "<pref path>/tmp/", then
 * "<base path>/tmp/".
 *
 * @returns true when the directory exists, with its path in `dir`.
 */
static bool SDL_Libretro_GetTempDirectory(SDL_Libretro* lr, char* dir, size_t dirSize) {
    const char* saveDir = SDL_Libretro_GetSaveDirectory(lr);
    if (saveDir && saveDir[0]) {
        SDL_snprintf(dir, dirSize, "%s/tmp/", saveDir);
    }
    else if (SDL_Libretro_GetPrefDirectory(dir, dirSize)) {
        SDL_strlcat(dir, "/tmp/", dirSize);
    }
    else {
        const char* base = SDL_GetBasePath();
        SDL_snprintf(dir, dirSize, "%stmp/", base ? base : "./");
    }
    if (!SDL_CreateDirectory(dir)) {
        return SDL_SetError("[SDL_Libretro] Failed to create temp directory '%s': %s", dir, SDL_GetError());
    }
    return true;
}

/**
 * @internal
 * SDL_EnumerateDirectory() callback for SDL_Libretro_RemoveTree().
 */
static SDL_EnumerationResult SDLCALL SDL_Libretro_RemoveTreeEntry(void* userdata, const char* dirname, const char* fname) {
    (void)userdata;
    char path[SDL_LIBRETRO_MAX_PATH];
    size_t len = SDL_strlen(dirname);
    bool slash = len > 0 && (dirname[len - 1] == '/' || dirname[len - 1] == '\\');
    SDL_snprintf(path, sizeof(path), "%s%s%s", dirname, slash ? "" : "/", fname);
    SDL_PathInfo info;
    if (SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_DIRECTORY) {
        SDL_EnumerateDirectory(path, SDL_Libretro_RemoveTreeEntry, NULL);
    }
    SDL_RemovePath(path);
    return SDL_ENUM_CONTINUE;
}

/**
 * @internal
 * Remove a directory and everything in it.
 */
static void SDL_Libretro_RemoveTree(const char* path) {
    SDL_EnumerateDirectory(path, SDL_Libretro_RemoveTreeEntry, NULL);
    SDL_RemovePath(path);
}

/**
 * @internal
 * Spill a stream to a new file in the frontend's temp directory.
 *
 * The temp directory comes from SDL_Libretro_GetTempDirectory(). The file keeps
 * the file name of `path` (uniquified with a "-<n>" suffix if it's taken), so
 * cores that look at the name or extension see the original. The caller owns
 * the file and must remove it once it's no longer needed.
 *
 * @param lr the libretro context.
 * @param src the stream to read.
 * @param path the path the content is known by; only its file name is used.
 * @param closeio if true, close `src` before returning, even on failure.
 * @param tempPath receives the path of the written file.
 * @param tempPathSize the size of `tempPath` in bytes.
 * @returns true when the file was written.
 */
static bool SDL_Libretro_SpillToTempFile(SDL_Libretro* lr, SDL_IOStream* src, const char* path, bool closeio, char* tempPath, size_t tempPathSize) {
    // Read the content first, so the stream is handled on every path.
    size_t fileSize = 0;
    void* fileData = SDL_LoadFile_IO(src, &fileSize, closeio);
    if (!fileData) return false;

    char name[SDL_LIBRETRO_MAX_PATH];
    if (!path || SDL_Libretro_GetFileName(name, sizeof(name), path, true) == 0) {
        SDL_strlcpy(name, "content.bin", sizeof(name));
    }

    // Pick the temp directory and make sure it exists.
    char dir[SDL_LIBRETRO_MAX_PATH];
    if (!SDL_Libretro_GetTempDirectory(lr, dir, sizeof(dir))) {
        SDL_free(fileData);
        return false;
    }

    // Find a file name in the directory that doesn't exist yet.
    SDL_snprintf(tempPath, tempPathSize, "%s%s", dir, name);
    const char* dot = SDL_strrchr(name, '.');
    int stemLen = dot ? (int)(dot - name) : (int)SDL_strlen(name);
    for (int attempt = 1; SDL_GetPathInfo(tempPath, NULL); attempt++) {
        if (attempt > 99) {
            SDL_SetError("[SDL_Libretro] Could not find a free file name for '%s' in '%s'", name, dir);
            SDL_free(fileData);
            return false;
        }
        SDL_snprintf(tempPath, tempPathSize, "%s%.*s-%d%s", dir, stemLen, name, attempt, dot ? dot : "");
    }

    bool saved = SDL_SaveFile(tempPath, fileData, fileSize);
    SDL_free(fileData);
    if (!saved) {
        SDL_SetError("[SDL_Libretro] Failed to save content to '%s'", tempPath);
        return false;
    }
    return true;
}

/**
 * Loads a game from an SDL_IOStream.
 *
 * Stream content into memory, and hand it to the core. The `path` only
 * provides the content identity (name, extension, save path, etc). When
 * the core requires a full path (need_fullpath), the stream is saved to a
 * temp directory ("<save directory>/tmp") and loaded from there, while the
 * content keeps the identity of `path`. The file is removed when the game
 * unloads.
 *
 * @param lr the libretro context.
 * @param src the stream to read the content from.
 * @param path the path the content is known by. Could be a virtual path
 * if the loading from an archive.
 * @param closeio if true, close `src` before returning, even on failure.
 * @returns true when the game is running; call SDL_GetError() for more information on failure.
 *
 * @see SDL_Libretro_LoadGame()
 */
bool SDL_Libretro_LoadGame_IO(SDL_Libretro* lr, SDL_IOStream* src, const char* path, bool closeio) {
    if (!lr || !src || !path) {
        if (src && closeio) SDL_CloseIO(src);
        return SDL_InvalidParamError("src, path");
    }

    // Try to find what can be loaded with it.
    if (!SDL_Libretro_IsCoreReady(lr)) {
        if (!SDL_Libretro_LoadCoreForGame(lr, path)) {
            if (closeio) SDL_CloseIO(src);
            SDL_SetError("[SDL_Libretro] Core not loaded");
            return false;
        }
    }

    // The raw-case extension drives the need_fullpath resolution.
    if (!SDL_Libretro_ContentNeedsFullpath(lr, SDL_Libretro_GetExtension(path))) {
        // Byte-oriented content: hand the stream contents to the core.
        size_t fileSize = 0;
        void* fileData = SDL_LoadFile_IO(src, &fileSize, closeio);
        if (!fileData) return false;
        return SDL_Libretro_LoadGameCommon(lr, path, NULL, fileData, fileSize);
    }

    // The core wants a real file: spill the stream to the temp directory and
    // load from there. The content keeps the identity of `path`, so save files
    // are named after it rather than the temp file.
    char tempPath[SDL_LIBRETRO_MAX_PATH];
    if (!SDL_Libretro_SpillToTempFile(lr, src, path, closeio, tempPath, sizeof(tempPath))) {
        return false;
    }

    // The temp path is recorded only after the load, so the UnloadGame inside
    // SDL_Libretro_LoadGameCommon() doesn't remove the file it's about to use.
    bool result = SDL_Libretro_LoadGameCommon(lr, tempPath, path, NULL, 0);
    if (result) {
        SDL_strlcpy(lr->core.contentTempPath, tempPath, sizeof(lr->core.contentTempPath));
    }
    else {
        SDL_RemovePath(tempPath);
    }
    return result;
}

/**
 * Load multi-ROM subsystem content by its numeric subsystem id.
 *
 * Equivalent to SDL_Libretro_LoadGameSpecial() but selects the subsystem by the
 * id the core assigned it rather than by ident or description.
 *
 * @param lr the libretro instance.
 * @param subsystemId the id of the subsystem to load.
 * @param paths the content file paths, one per ROM the subsystem expects.
 * @param numPaths the number of entries in `paths`; must be greater than 0.
 * @returns true on success; false if the id is unknown, the arguments are
 *          invalid, the paths don't match the ROMs the subsystem expects, a
 *          file fails to load, or the core rejects the content. Only a
 *          rejection by the core unloads the game that was running.
 *
 * @see SDL_Libretro_LoadGameSpecial
 */
bool SDL_Libretro_LoadGameSpecialById(SDL_Libretro* lr, unsigned subsystemId, const char** paths, unsigned numPaths) {
    if (!SDL_Libretro_IsCoreReady(lr) || !paths || numPaths == 0) {
        SDL_SetError("[SDL_Libretro] Invalid arguments for LoadGameSpecial");
        return false;
    }

    // Find the subsystem info to determine per-ROM need_fullpath.
    const struct retro_subsystem_info* subsys = SDL_Libretro_GetSubsystemById(lr, subsystemId);
    if (!subsys) {
        SDL_SetError("[SDL_Libretro] Unknown subsystem id %u", subsystemId);
        return false;
    }

    // The paths must fit the ROMs the subsystem declares, with every required
    // one given, or the core only rejects them with no hint why.
    unsigned requiredCount = 0;
    bool missingRequired = false;
    for (unsigned i = 0; i < subsys->num_roms; i++) {
        if (subsys->roms[i].required) {
            requiredCount++;
            missingRequired = missingRequired || i >= numPaths || !paths[i];
        }
    }
    if (numPaths > subsys->num_roms || missingRequired) {
        SDL_SetError("[SDL_Libretro] Subsystem '%s' expects %u ROM(s) (%u required), got %u",
            subsys->desc ? subsys->desc : (subsys->ident ? subsys->ident : ""), subsys->num_roms, requiredCount, numPaths);
        return false;
    }

    // Build the game info with the given data. The files are read before the
    // running game is unloaded, so a missing one leaves it running.
    struct retro_game_info* gameInfos = (struct retro_game_info*)SDL_calloc(numPaths, sizeof(struct retro_game_info));
    void** fileBuffers = (void**)SDL_calloc(numPaths, sizeof(void*));
    if (!gameInfos || !fileBuffers) {
        SDL_free(gameInfos);
        SDL_free(fileBuffers);
        SDL_SetError("[SDL_Libretro] Allocation failed");
        return false;
    }

    for (unsigned i = 0; i < numPaths; i++) {
        if (!paths[i]) continue;
        gameInfos[i].path = paths[i];

        bool needFullpath = lr->core.needFullpath;
        if (i < subsys->num_roms) {
            needFullpath = subsys->roms[i].need_fullpath;
        }

        if (!needFullpath) {
            size_t fileSize = 0;
            fileBuffers[i] = SDL_LoadFile(paths[i], &fileSize);
            if (!fileBuffers[i]) {
                SDL_SetError("[SDL_Libretro] Failed to load file '%s'", paths[i]);
                for (unsigned j = 0; j < i; j++) SDL_free(fileBuffers[j]);
                SDL_free(fileBuffers);
                SDL_free(gameInfos);
                return false;
            }
            gameInfos[i].data = fileBuffers[i];
            gameInfos[i].size = fileSize;
        }
        else if (!SDL_Libretro_ContentExists(lr, paths[i])) {
            SDL_SetError("[SDL_Libretro] File '%s' not found", paths[i]);
            for (unsigned j = 0; j < i; j++) SDL_free(fileBuffers[j]);
            SDL_free(fileBuffers);
            SDL_free(gameInfos);
            return false;
        }
    }

    SDL_Libretro_UnloadGame(lr);

    // Populate the game info from the primary path, so
    // GET_GAME_INFO_EXT is valid while loading the game.
    SDL_memset(&lr->core.gameInfoExt, 0, sizeof(lr->core.gameInfoExt));
    if (paths[0]) {
        SDL_Libretro_SetContentIdentity(lr, paths[0], NULL);
        lr->core.gameInfoExt.data = gameInfos[0].data;
        lr->core.gameInfoExt.size = gameInfos[0].size;
        lr->core.gameInfoExt.persistent_data = false;
    }
    else {
        SDL_strlcpy(lr->core.contentName, lr->core.libraryName, sizeof(lr->core.contentName));
    }

    // GET_GAME_INFO_EXT hands subsystem cores one entry per content file, in subsystem order. Files that weren't given keep a NULL full_path.
    SDL_Libretro_FreeGameInfoExtArray(lr);
    lr->core.gameInfoExtArray = (struct retro_game_info_ext*)SDL_calloc(numPaths, sizeof(struct retro_game_info_ext));
    if (lr->core.gameInfoExtArray) {
        lr->core.gameInfoExtCount = numPaths;
        for (unsigned i = 0; i < numPaths; i++) {
            SDL_Libretro_FillGameInfoExt(&lr->core.gameInfoExtArray[i], paths[i]);
            lr->core.gameInfoExtArray[i].data = gameInfos[i].data;
            lr->core.gameInfoExtArray[i].size = gameInfos[i].size;
            lr->core.gameInfoExtArray[i].persistent_data = false;
        }
    }

    SDL_Libretro_SetCoreCallbacks(lr);

    bool result = lr->core.symbols.retro_load_game_special(subsystemId, gameInfos, numPaths);

    // The frontend owns the ROM buffers for the load only, free them
    // and drop gameInfoExt.
    for (unsigned i = 0; i < numPaths; i++) SDL_free(fileBuffers[i]);
    SDL_free(fileBuffers);
    SDL_free(gameInfos);
    lr->core.gameInfoExt.data = NULL;
    lr->core.gameInfoExt.size = 0;
    for (unsigned i = 0; i < lr->core.gameInfoExtCount; i++) {
        lr->core.gameInfoExtArray[i].data = NULL;
        lr->core.gameInfoExtArray[i].size = 0;
    }

    if (!result) {
        SDL_Libretro_ResetContentState(lr);
        SDL_SetError("[SDL_Libretro] Core failed to load the special game");
        return false;
    }

    SDL_Libretro_FinishGameLoad(lr);
    return true;
}

/**
 * Loads multi-ROM subsystem content (e.g. Super Game Boy, Sufami Turbo).
 *
 * The subsystem is chosen by matching `subsystem` against each registered
 * subsystem's ident or human-readable description. The `paths` map positionally
 * to that subsystem's ROMs; each ROM whose descriptor sets need_fullpath is
 * passed by path, otherwise the file is read into memory and handed to the core.
 *
 * @param lr the libretro instance.
 * @param subsystem the subsystem ident or description to load.
 * @param paths the content file paths, one per ROM the subsystem expects.
 * @param numPaths the number of entries in `paths`; must be greater than 0.
 * @returns true on success; false if the subsystem is unknown, the arguments
 *          are invalid, a file fails to load, or the core rejects the content.
 *
 * @see SDL_Libretro_LoadGameSpecialById
 */
bool SDL_Libretro_LoadGameSpecial(SDL_Libretro* lr, const char* subsystem, const char** paths, unsigned numPaths) {
    // Match by ident or human-readable description.
    const struct retro_subsystem_info* subsys = SDL_Libretro_GetSubsystemByName(lr, subsystem);
    if (!subsys) {
        SDL_SetError("[SDL_Libretro] Unknown subsystem '%s'", subsystem ? subsystem : "(null)");
        return false;
    }
    return SDL_Libretro_LoadGameSpecialById(lr, subsys->id, paths, numPaths);
}

/**
 * Unloads the actively loaded game.
 *
 * The core stays loaded.
 *
 * @param lr the libretro context. NULL, or no game loaded, is ignored.
 *
 * @see SDL_Libretro_UnloadCore()
 */
void SDL_Libretro_UnloadGame(SDL_Libretro* lr) {
    if (!lr || !lr->core.gameLoaded) return;

    // Keep the game's battery save while the core still has it.
    SDL_Libretro_AutoSaveSRAM(lr);

    // Unload the game before closing audio and video.
    lr->core.symbols.retro_unload_game();
    lr->core.gameLoaded = false;
    // A shutdown request was for the game that's now gone.
    lr->core.shutdown = false;
    SDL_Libretro_ResetContentState(lr);
    SDL_Libretro_StopRumble(lr);

    SDL_Libretro_RewindFree(lr);
    SDL_Libretro_CloseAudio(lr);
    SDL_Libretro_CloseVideo(lr);
    SDL_Libretro_PhysFS_ClearMount(lr);
    SDL_Log("[SDL_Libretro] Game unloaded");

    SDL_Libretro_PushEvent(lr, SDL_EVENT_LIBRETRO_GAME_UNLOADED, NULL);
}

/**
 * Indicates whether a game is loaded and running.
 *
 * This is also true for a core started without content.
 *
 * @param lr the libretro context.
 * @returns true when a game is loaded, false otherwise or when lr is NULL.
 *
 * @see SDL_Libretro_LoadGame()
 * @see SDL_Libretro_IsCoreReady()
 */
bool SDL_Libretro_IsGameReady(const SDL_Libretro* lr) {
    return lr && lr->core.gameLoaded;
}

/**
 * Indicates whether the loaded core needs content to run.
 *
 * Cores opt into running without content with
 * RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, after which SDL_Libretro_LoadGame()
 * accepts a NULL path. The core's own report is used rather than the
 * supports_no_game flag in its .info file.
 *
 * @param lr the libretro context.
 * @returns false when the loaded core can start without content, true otherwise (including when no core is loaded).
 *
 * @see SDL_Libretro_LoadGame()
 */
bool SDL_Libretro_IsGameRequired(const SDL_Libretro* lr) {
    return lr && !lr->core.supportNoGame;
}

/**
 * Resets the loaded game, like pressing the console's reset button.
 *
 * Also clears the rewind buffer and any queued audio.
 *
 * @param lr the libretro context.
 * @returns true when the game was reset, false when no game is loaded; call SDL_GetError() for more information.
 */
bool SDL_Libretro_Reset(SDL_Libretro* lr) {
    if (!lr || !lr->core.gameLoaded) {
        SDL_SetError("[SDL_Libretro] No game loaded");
        return false;
    }
    lr->core.symbols.retro_reset();

    // Restart the frame-time clock, so the first tick after the reset reports the core's reference.
    lr->core.runloop_frame_time_last = 0;

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
 * @internal
 * The longest wall-clock gap between tick batches that's reported to the
 * frame-time callback as elapsed time. A longer gap means the application
 * stopped calling SDL_Libretro_Update() for a while (its menu was open, or the
 * machine slept), which is a pause rather than one long frame.
 */
#define SDL_LIBRETRO_FRAME_TIME_MAX_GAP_NS (250 * SDL_NS_PER_MS)

/**
 * Work out the delta handed to the frame-time callback for each of the `ticks` ticks about to run, and stamp the time.
 *
 * Mirrors RetroArch: the first tick (or the first after a load, reset, pause or rewind) has no measured cadence, so it gets the reference the core declared. So does the first after a gap longer than SDL_LIBRETRO_FRAME_TIME_MAX_GAP_NS, which is the application pausing by not calling SDL_Libretro_Update(). At 1x speed the wall-clock time since the previous tick batch is reported. Off 1x speed (fast-forward, slow-motion) the idealized `referenceUsec` is reported instead, since the measured time doesn't describe game time. When one Update runs several catch-up ticks, the measured time is split evenly across them, so each tick still reports its share rather than the first getting it all and the rest ~0.
 */
static retro_usec_t SDL_Libretro_FrameTimeDelta(SDL_Libretro* lr, retro_usec_t referenceUsec, int ticks) {
    Uint64 nowNS = SDL_LIBRETRO_GET_TICKS_NS();
    Uint64 lastNS = lr->core.runloop_frame_time_last;
    lr->core.runloop_frame_time_last = nowNS;
    if (!lastNS || referenceUsec <= 0 || nowNS - lastNS > SDL_LIBRETRO_FRAME_TIME_MAX_GAP_NS) {
        return lr->core.runloop_frame_time.reference;
    }
    if (lr->core.speed != 1.0f) {
        return referenceUsec;
    }
    retro_usec_t measured = (retro_usec_t)((nowNS - lastNS) / SDL_NS_PER_US);
    return ticks > 1 ? measured / ticks : measured;
}

/**
 * Advance the core a single tick. Also report `frameTimeUsec` to the frame-time callback if needed.
 *
 * @see SDL_Libretro_FrameTimeDelta
 */
static void SDL_Libretro_Tick(SDL_Libretro* lr, retro_usec_t frameTimeUsec) {
    if (lr->core.runloop_frame_time.callback) {
        lr->core.runloop_frame_time.callback(frameTimeUsec);
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

/**
 * Runs the loaded game; call it once per frame of the application's loop.
 *
 * The core is paced to its own frame rate scaled by the speed, so this may run
 * zero, one, or several core frames per call. Pending video and audio
 * reinitializations happen here. Nothing runs while the speed is 0, or once
 * the core has requested shutdown, and a negative speed steps backwards
 * through the rewind buffer when rewind is enabled.
 *
 * @param lr the libretro context. Does nothing when lr is NULL or no game is loaded.
 *
 * @see SDL_Libretro_SetSpeed()
 * @see SDL_Libretro_Render()
 */
void SDL_Libretro_Update(SDL_Libretro* lr) {
    // Once the core asks to shut down, it doesn't expect to run again.
    if (!lr || !lr->core.gameLoaded || lr->core.shutdown) return;

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

    // Paused: Do nothing when speed is zero. The frame-time clock restarts on resume, so the pause isn't reported as one long frame.
    if (lr->core.speed == 0.0f) {
        lr->core.runloop_frame_time_last = 0;
        return;
    }

    // Keep any active rumble going until the core sets it back to 0.
    for (unsigned port = 0; port < SDL_LIBRETRO_MAX_USERS; port++) {
        if (lr->core.rumbleStrong[port] > 0.0f || lr->core.rumbleWeak[port] > 0.0f) {
            SDL_Libretro_ApplyRumble(lr, port);
        }
    }

    // Rewind mode: step backwards when speed is negative.
    if (lr->rewindEnabled && lr->core.speed < 0.0f) {
        Uint64 nowNS = SDL_LIBRETRO_GET_TICKS_NS();
        if (lr->core.lastTickNS == 0) {
            lr->core.lastTickNS = nowNS;
        }
        double frameTime = (double)(nowNS - lr->core.lastTickNS) / 1.0e9;
        lr->core.lastTickNS = nowNS;
        double framePeriod = (lr->core.fps > 0.0) ? (1.0 / lr->core.fps) : (1.0 / 60.0);
        // Each stored snapshot spans captureInterval real frames (a snapshot is taken every Nth frame), so a single rewind step undoes that many frames of game time. Scale the per-step wall-clock cost by the interval; otherwise speed -1 would rewind captureInterval times faster than speed +1 plays forward.
        unsigned interval = lr->rewindCaptureInterval > 0 ? lr->rewindCaptureInterval : 1;
        double stepPeriod = framePeriod * (double)interval;
        lr->core.speedAccumulator += frameTime * (double)(-lr->core.speed);

        // Protect against infinite rewinds, and clamp it to 1 tick.
        int maxTicks = (int)(-lr->core.speed + 1.0f);
        if (maxTicks < 1) maxTicks = 1;

        // Protect with a maximum accumulator, so frame spikes don't drain the rewind buffer.
        double maxAccumulator = stepPeriod * (double)maxTicks;
        if (lr->core.speedAccumulator > maxAccumulator) {
            lr->core.speedAccumulator = maxAccumulator;
        }

        // Mute audio and neutralize input for the throwaway re-runs that produce the displayed frames while scrubbing backward.
        lr->rewindActive = true;
        while (lr->core.speedAccumulator >= stepPeriod && maxTicks-- > 0 && !lr->core.shutdown) {
            lr->core.speedAccumulator -= stepPeriod;
            if (!SDL_Libretro_RewindStepState(lr)) break;
            lr->core.symbols.retro_run();
        }
        lr->rewindActive = false;
        // Rewind steps bypass the frame-time callback, so restart its clock for when forward play resumes.
        lr->core.runloop_frame_time_last = 0;
        return;
    }

    // Keep audio consumption locked to speed + nudge the queue toward its target fill. Done here, above all three return paths below, so it runs every frame.
    SDL_Libretro_UpdateDRC(lr, lr->core.speed);

    // Wall-clock delta since the previous RunFrame.
    Uint64 nowNS = SDL_LIBRETRO_GET_TICKS_NS();
    if (lr->core.lastTickNS == 0) {
        // First call: seed the clock and run exactly one tick.
        lr->core.lastTickNS = nowNS;
        lr->core.speedAccumulator = 0.0;
        SDL_Libretro_Tick(lr, SDL_Libretro_FrameTimeDelta(lr, 0, 1));
        return;
    }

    // Calculate the frame time in seconds.
    double frameTime = (double)(nowNS - lr->core.lastTickNS) / 1.0e9;
    lr->core.lastTickNS = nowNS;

    // Target frame period from the core's declared fps (default 60).
    double framePeriod = (lr->core.fps > 0.0) ? (1.0 / lr->core.fps) : (1.0 / 60.0);

    // Reference frame-time the core is told about, in microseconds.
    retro_usec_t referenceUsec = (retro_usec_t)(framePeriod * 1.0e6 / (double)lr->core.speed);

    // At normal speed, when the loop is already paced close to the core's frame rate (e.g. a vsync'd 60 Hz display with a ~60 fps core), run exactly one tick and discard the accumulator. This avoids the beat-frequency judder of occasionally emitting 0 or 2 ticks. Gating on the *measured* cadence keeps the core bounded when vsync is off / FPS uncapped.
    double cadence = (framePeriod > 0.0) ? (frameTime / framePeriod) : 0.0;
    if (lr->core.speed == 1.0f && cadence > 0.9 && cadence < 1.1) {
        lr->core.speedAccumulator = 0.0;
        SDL_Libretro_Tick(lr, SDL_Libretro_FrameTimeDelta(lr, referenceUsec, 1));
        return;
    }

    lr->core.speedAccumulator += frameTime * (double)lr->core.speed;

    // Cap iterations to avoid a spiral of death on slow hardware.
    int maxTicks = (int)(lr->core.speed + 1.0f);
    if (maxTicks < 1) maxTicks = 1;

    // Clamp the accumulator so a frame-time spike (game load, window drag, menu pause) can't leave a backlog that runs the core fast afterwards.
    double maxAccumulator = framePeriod * (double)maxTicks;
    if (lr->core.speedAccumulator > maxAccumulator) {
        lr->core.speedAccumulator = maxAccumulator;
    }

    // Count the ticks needed to catch up, so the measured frame time can be shared between them.
    int ticks = 0;
    while (lr->core.speedAccumulator >= framePeriod && ticks < maxTicks) {
        lr->core.speedAccumulator -= framePeriod;
        ticks++;
    }
    if (ticks == 0) return;

    // Run the required number of ticks.
    retro_usec_t frameTimeUsec = SDL_Libretro_FrameTimeDelta(lr, referenceUsec, ticks);
    for (int i = 0; i < ticks && !lr->core.shutdown; i++) {
        SDL_Libretro_Tick(lr, frameTimeUsec);
    }
}

/**
 * Indicates whether or not the core has requested to shutdown.
 *
 * The request lasts until the game unloads, so loading another game clears it.
 *
 * @param lr the libretro context.
 * @returns true when the core requested to shut down, false otherwise or when lr is NULL.
 *
 * @see RETRO_ENVIRONMENT_SHUTDOWN
 * @see SDL_EVENT_LIBRETRO_SHUTDOWN
 */
bool SDL_Libretro_ShouldQuit(const SDL_Libretro* lr) {
    return lr && lr->core.shutdown;
}

/**
 * Get the version of SDL_libretro the implementation was compiled with.
 *
 * @returns the version as SDL_LIBRETRO_VERSION, packed with SDL_VERSIONNUM().
 *
 * @see SDL_LIBRETRO_VERSION
 */
int SDL_Libretro_GetVersion(void) {
    return SDL_LIBRETRO_VERSION;
}

/**
 * @internal
 * Pushes an SDL_libretro event onto the SDL event queue: data1 is the
 * context, data2 the event-specific payload (a name, the menu, or NULL).
 */
static void SDL_Libretro_PushEvent(SDL_Libretro* lr, Uint32 type, void* data) {
    SDL_Event event;
    SDL_zero(event);
    event.user.type = type;
    event.user.data1 = lr;
    event.user.data2 = data;
    SDL_PushEvent(&event);
}

/**
 * @internal
 * Event filter for SDL_FilterEvents() that removes exactly the queued copy of
 * the env event in userdata, matched by type, context, and the sequence tag
 * SDL_Libretro_PushEnvEvent() stored in user.reserved.
 */
static bool SDLCALL SDL_Libretro_KeepUnclaimedEnvEvent(void* userdata, SDL_Event* event) {
    const SDL_UserEvent* pushed = (const SDL_UserEvent*)userdata;
    return !(event->type == pushed->type &&
        event->user.data1 == pushed->data1 &&
        event->user.reserved == pushed->reserved);
}

/**
 * @internal
 * Reports an environment command the library doesn't handle itself as
 * SDL_EVENT_LIBRETRO_ENVIRONMENT(cmd). data2 is the command's data pointer.
 * Commands that type can't encode aren't reported.
 *
 * Each push is tagged with a process-wide sequence number in user.reserved,
 * which applications don't read, so a claimed push can remove exactly its own
 * queued copy even when earlier copies of the same command are still queued.
 *
 * @return true when an event watch or the event filter claimed the command by
 *         setting a non-zero user.code; both run synchronously inside
 *         SDL_PushEvent(), which is how an application implements an
 *         environment command while the core waits. A filter may also drop
 *         the event by returning false; the claim still counts.
 */
static bool SDL_Libretro_PushEnvEvent(SDL_Libretro* lr, unsigned cmd, void* data) {
    // Only commands that map onto their own event type are reported, so an
    // out-of-range number can't pose as another command or a library event.
    unsigned flags = cmd & (unsigned)(RETRO_ENVIRONMENT_EXPERIMENTAL | RETRO_ENVIRONMENT_PRIVATE);
    if ((cmd & ~flags) >= 0x400 || flags == (unsigned)(RETRO_ENVIRONMENT_EXPERIMENTAL | RETRO_ENVIRONMENT_PRIVATE)) {
        return false;
    }

    static SDL_AtomicInt sequence;
    SDL_Event event;
    SDL_zero(event);
    event.user.type = SDL_EVENT_LIBRETRO_ENVIRONMENT(cmd);
    event.user.reserved = (Uint32)SDL_AddAtomicInt(&sequence, 1) + 1;
    event.user.data1 = lr;
    event.user.data2 = data;
    const SDL_UserEvent pushed = event.user;

    // SDL_PushEvent() returns false when the filter drops the event, which a
    // filter that claimed the command may do, so only user.code decides.
    const bool queued = SDL_PushEvent(&event);
    if (event.user.code == 0) {
        return false;
    }

    // A watch implemented the command, so each event is delivered exactly
    // once: the queued copy would only re-report it with an expired data
    // pointer, so drop it. Unclaimed commands stay queued for observation.
    if (queued) {
        SDL_FilterEvents(SDL_Libretro_KeepUnclaimedEnvEvent, (void*)&pushed);
    }
    return true;
}

// Directory

static void SDL_Libretro_FreeCoreLibrary(SDL_Libretro* lr) {
    if (lr->coreLibrary) {
        for (unsigned i = 0; i < lr->coreLibraryCount; i++) {
            SDL_free(lr->coreLibrary[i].corename);
            SDL_free(lr->coreLibrary[i].supported_extensions);
            SDL_free(lr->coreLibrary[i].path);
        }
        SDL_free(lr->coreLibrary);
        lr->coreLibrary = NULL;
    }
    lr->coreLibraryCount = 0;
}

// The file extension for the cores on this platform
#if defined(SDL_PLATFORM_WINDOWS)
#define SDL_LIBRETRO_CORE_EXTENSION ".dll"
#elif defined(SDL_PLATFORM_APPLE)
#define SDL_LIBRETRO_CORE_EXTENSION ".dylib"
#elif defined(SDL_PLATFORM_EMSCRIPTEN)
#define SDL_LIBRETRO_CORE_EXTENSION ".wasm"
#else
#define SDL_LIBRETRO_CORE_EXTENSION ".so"
#endif

static SDL_EnumerationResult SDLCALL SDL_Libretro_SetCoreDirectory_Iterator(void* userdata, const char* dirname, const char* fname) {
    SDL_Libretro* lr = (SDL_Libretro*)userdata;

    // Find all .info files in the core directory.
    const char* dot = SDL_strrchr(fname, '.');
    if (!dot || SDL_strcasecmp(dot, ".info") != 0) {
        return SDL_ENUM_CONTINUE;
    }

    // Get the path.
    char infoPath[SDL_LIBRETRO_MAX_PATH];
    SDL_snprintf(infoPath, sizeof(infoPath), "%s%s", dirname, fname);

    // Load the .ini file.
    SDL_ini* ini = INI_Load(infoPath);
    if (!ini) {
        return SDL_ENUM_CONTINUE;
    }

    // Make sure it's a valid info file.
    const char* corename = INI_GetString(ini, NULL, "corename", NULL);
    if (!corename) {
        INI_Destroy(ini);
        return SDL_ENUM_CONTINUE;
    }

    // Grow the core library by one entry.
    SDL_Libretro_CoreInfo* grown = (SDL_Libretro_CoreInfo*)SDL_realloc(
        lr->coreLibrary,
        (lr->coreLibraryCount + 1) * sizeof(SDL_Libretro_CoreInfo));
    if (!grown) {
        INI_Destroy(ini);
        SDL_SetError("[SDL_Libretro] Out of memory building the core library");
        return SDL_ENUM_FAILURE;
    }
    lr->coreLibrary = grown;

    // Build the SDL_Libretro_CoreInfo structure.
    SDL_Libretro_CoreInfo* entry = &lr->coreLibrary[lr->coreLibraryCount];
    const char* extensions = INI_GetString(ini, NULL, "supported_extensions", NULL);
    entry->corename = SDL_strdup(corename);
    entry->supported_extensions = extensions ? SDL_strdup(extensions) : NULL;
    entry->needs_fullpath = INI_GetBoolean(ini, NULL, "needs_fullpath", false);
    entry->supports_no_game = INI_GetBoolean(ini, NULL, "supports_no_game", false);
    entry->block_extract = INI_GetBoolean(ini, NULL, "block_extract", false);

    // Build the path for the core.
    char base[SDL_LIBRETRO_MAX_PATH];
    size_t baseLen = (size_t)(dot - fname);
    SDL_strlcpy(base, fname, (baseLen < sizeof(base)) ? baseLen + 1 : sizeof(base));
    char corePath[SDL_LIBRETRO_MAX_PATH];
    SDL_snprintf(corePath, sizeof(corePath), "%s%s%s", dirname, base, SDL_LIBRETRO_CORE_EXTENSION);
    entry->path = SDL_strdup(corePath);

    INI_Destroy(ini);

    // Only publish the entry once every string copied, so the library never holds NULL names or paths.
    if (!entry->corename || !entry->path || (extensions && !entry->supported_extensions)) {
        SDL_free(entry->corename);
        SDL_free(entry->supported_extensions);
        SDL_free(entry->path);
        SDL_SetError("[SDL_Libretro] Out of memory building the core library");
        return SDL_ENUM_FAILURE;
    }
    lr->coreLibraryCount++;
    return SDL_ENUM_CONTINUE;
}

/**
 * @internal
 * Orders core library entries by case-insensitive core name, then by path, so
 * the library is the same on every machine regardless of directory order.
 */
static int SDLCALL SDL_Libretro_CompareCoreInfo(const void* a, const void* b) {
    const SDL_Libretro_CoreInfo* left = (const SDL_Libretro_CoreInfo*)a;
    const SDL_Libretro_CoreInfo* right = (const SDL_Libretro_CoreInfo*)b;
    int result = SDL_strcasecmp(left->corename, right->corename);
    return result != 0 ? result : SDL_strcmp(left->path, right->path);
}

/**
 * Sets the associated libretro core directory, where the default set of cores will be loaded from.
 *
 * The discovered cores are sorted by case-insensitive core name, then by path,
 * so lookups that take the first match (such as picking a core for a game's
 * extension) behave the same on every machine.
 *
 * The path is stored even when it can't be scanned, so it can still be shown or saved.
 *
 * @param lr the libretro context.
 * @param path the core directory, or NULL/empty to clear it.
 * @returns true when the directory was scanned (or cleared), false when it doesn't exist, isn't a directory, or the scan failed; call SDL_GetError() for more information.
 */
bool SDL_Libretro_SetCoreDirectory(SDL_Libretro* lr, const char* path) {
    if (!lr) return false;
    const char* newPath = path ? path : "";
    SDL_strlcpy(lr->coreDirectory, newPath, sizeof(lr->coreDirectory));

    // When the core directory changed, rebuild the core library.
    SDL_Libretro_FreeCoreLibrary(lr);
    if (lr->coreDirectory[0] == '\0') return true;

    SDL_PathInfo info;
    if (!SDL_GetPathInfo(lr->coreDirectory, &info) || info.type != SDL_PATHTYPE_DIRECTORY) {
        SDL_SetError("[SDL_Libretro] Core directory '%s' is not a directory", lr->coreDirectory);
        return false;
    }

    if (!SDL_EnumerateDirectory(lr->coreDirectory, &SDL_Libretro_SetCoreDirectory_Iterator, (void*)lr)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] Failed to scan core directory '%s': %s", lr->coreDirectory, SDL_GetError());
        return false;
    }

    // Directory order varies by platform and filesystem, so sort for a stable pick.
    if (lr->coreLibraryCount > 1) {
        SDL_qsort(lr->coreLibrary, lr->coreLibraryCount, sizeof(SDL_Libretro_CoreInfo), SDL_Libretro_CompareCoreInfo);
    }
    return true;
}

/**
 * @internal
 * The application's writable pref path, without the trailing separator.
 *
 * Keyed by the app metadata (SDL_SetAppMetadata): CREATOR_STRING as the
 * organization and NAME_STRING as the application, each falling back to
 * "SDL_libretro". SDL_GetPrefPath() creates the directory.
 *
 * @returns true when `dst` received the path, false when no pref path is
 * available (leaving `dst` untouched).
 */
static bool SDL_Libretro_GetPrefDirectory(char* dst, size_t dstSize) {
    const char* appName = SDL_GetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING);
    const char* orgName = SDL_GetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING);
    char* prefPath = SDL_GetPrefPath(orgName && orgName[0] ? orgName : "SDL_libretro", appName && appName[0] ? appName : "SDL_libretro");
    if (!prefPath) return false;

    // Ensure it's a clean path; directory paths are composed as "<dir>/<file>".
    size_t len = SDL_strlen(prefPath);
    while (len > 0 && (prefPath[len - 1] == '/' || prefPath[len - 1] == '\\')) {
        prefPath[--len] = '\0';
    }
    SDL_strlcpy(dst, prefPath, dstSize);
    SDL_free(prefPath);
    return dst[0] != '\0';
}

/**
 * Sets where content will be saved.
 *
 * The directory is created when a core is loaded.
 *
 * @param lr The libretro context.
 * @param path The path to where save data should be placed, NULL for the
 * default ("saves" inside the app's pref path), or "" for none.
 * @returns true when the directory was set, false when lr is NULL.
 *
 * @see SDL_Libretro_GetSaveDirectory()
 */
bool SDL_Libretro_SetSaveDirectory(SDL_Libretro* lr, const char* path) {
    if (!lr) return false;

    if (!path) {
        // NULL resets to the default: "saves" inside the app's pref path.
        char pref[SDL_LIBRETRO_MAX_PATH];
        if (SDL_Libretro_GetPrefDirectory(pref, sizeof(pref))) {
            SDL_snprintf(lr->saveDirectory, sizeof(lr->saveDirectory), "%s/saves", pref);
        }
        else {
            lr->saveDirectory[0] = '\0';
        }
        return true;
    }

    SDL_strlcpy(lr->saveDirectory, path, sizeof(lr->saveDirectory));
    return true;
}

/**
 * Sets where cores look for system files (BIOS, firmware, etc).
 *
 * The directory is created when a core is loaded.
 *
 * @param lr The libretro context.
 * @param path The system directory, NULL for the default ("system" inside the app's pref path), or "" for none.
 * @returns true when the directory was set, false when lr is NULL.
 *
 * @see SDL_Libretro_GetSystemDirectory()
 */
bool SDL_Libretro_SetSystemDirectory(SDL_Libretro* lr, const char* path) {
    if (!lr) return false;

    if (!path) {
        // NULL resets to the default: "system" inside the app's pref path.
        char pref[SDL_LIBRETRO_MAX_PATH];
        if (SDL_Libretro_GetPrefDirectory(pref, sizeof(pref))) {
            SDL_snprintf(lr->systemDirectory, sizeof(lr->systemDirectory), "%s/system", pref);
        }
        else {
            lr->systemDirectory[0] = '\0';
        }
        return true;
    }

    SDL_strlcpy(lr->systemDirectory, path, sizeof(lr->systemDirectory));
    return true;
}

/**
 * Sets where cores look for their assets, reported through RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY.
 *
 * @param lr the libretro context.
 * @param path the core assets directory, or NULL/empty for none. The string is copied.
 * @returns true when the directory was set, false when lr is NULL.
 *
 * @see SDL_Libretro_GetCoreAssetsDirectory()
 */
bool SDL_Libretro_SetCoreAssetsDirectory(SDL_Libretro* lr, const char* path) {
    if (!lr) return false;
    SDL_strlcpy(lr->coreAssetsDirectory, path ? path : "", sizeof(lr->coreAssetsDirectory));
    return true;
}

/**
 * Get the core directory.
 *
 * @param lr the libretro context.
 * @returns the core directory, or NULL when none is set or lr is NULL. The string is owned by the context and stays valid until the directory changes or the context is destroyed.
 *
 * @see SDL_Libretro_SetCoreDirectory()
 */
const char* SDL_Libretro_GetCoreDirectory(SDL_Libretro* lr) {
    if (!lr) return NULL;
    return lr->coreDirectory[0] ? lr->coreDirectory : NULL;
}

/**
 * Get the save directory.
 *
 * @param lr the libretro context.
 * @returns the save directory, or NULL when none is set or lr is NULL. The string is owned by the context and stays valid until the directory changes or the context is destroyed.
 *
 * @see SDL_Libretro_SetSaveDirectory()
 */
const char* SDL_Libretro_GetSaveDirectory(SDL_Libretro* lr) {
    if (!lr) return NULL;
    return lr->saveDirectory[0] ? lr->saveDirectory : NULL;
}

/**
 * Get the system directory.
 *
 * @param lr the libretro context.
 * @returns the system directory, or NULL when none is set or lr is NULL. The string is owned by the context and stays valid until the directory changes or the context is destroyed.
 *
 * @see SDL_Libretro_SetSystemDirectory()
 */
const char* SDL_Libretro_GetSystemDirectory(SDL_Libretro* lr) {
    if (!lr) return NULL;
    return lr->systemDirectory[0] ? lr->systemDirectory : NULL;
}

/**
 * Get the core assets directory.
 *
 * @param lr the libretro context.
 * @returns the core assets directory, or NULL when none is set or lr is NULL. The string is owned by the context and stays valid until the directory changes or the context is destroyed.
 *
 * @see SDL_Libretro_SetCoreAssetsDirectory()
 */
const char* SDL_Libretro_GetCoreAssetsDirectory(SDL_Libretro* lr) {
    if (!lr) return NULL;
    return lr->coreAssetsDirectory[0] ? lr->coreAssetsDirectory : NULL;
}

/**
 * Sets the username reported to cores through RETRO_ENVIRONMENT_GET_USERNAME.
 *
 * Defaults to "SDL_libretro". Names longer than 63 bytes are truncated.
 *
 * @param lr the libretro context.
 * @param username the username, or NULL for an empty one. The string is copied.
 * @returns true when the username was set, false when lr is NULL.
 *
 * @see SDL_Libretro_GetUsername()
 */
bool SDL_Libretro_SetUsername(SDL_Libretro* lr, const char* username) {
    if (!lr) return false;
    SDL_strlcpy(lr->username, username ? username : "", sizeof(lr->username));
    return true;
}

/**
 * Get the username reported to cores.
 *
 * @param lr the libretro context.
 * @returns the username, or NULL when lr is NULL. The string is owned by the context and stays valid until the username changes or the context is destroyed.
 *
 * @see SDL_Libretro_SetUsername()
 */
const char* SDL_Libretro_GetUsername(SDL_Libretro* lr) {
    if (!lr) return NULL;
    return lr->username;
}

/**
 * Set the audio volume when playing sounds.
 *
 * The volume is kept across core and game loads, and applied to the audio stream as soon as one is open.
 *
 * @param lr the libretro context. NULL is ignored.
 * @param volume the volume, from 0.0 (silent) to 1.0 (full); values outside that range are clamped.
 *
 * @see SDL_Libretro_GetVolume()
 */
void SDL_Libretro_SetVolume(SDL_Libretro* lr, float volume) {
    if (!lr) return;
    lr->volume = SDL_clamp(volume, 0.0f, 1.0f);
    if (lr->core.audioStream) {
        SDL_SetAudioStreamGain(lr->core.audioStream, lr->volume);
    }
}

/**
 * Get the audio volume.
 *
 * @param lr the libretro context.
 * @returns the volume, from 0.0f (muted) to 1.0f (full), or 0.0f when lr is NULL.
 *
 * @see SDL_Libretro_SetVolume()
 */
float SDL_Libretro_GetVolume(const SDL_Libretro* lr) {
    return lr ? lr->volume : 0.0f;
}

/**
 * Sets how fast the game runs, as a multiple of its normal speed.
 *
 * 1.0f is normal speed, values above it fast-forward (e.g. 2.0f), values
 * between 0 and 1 run in slow motion, and 0.0f pauses. A negative value
 * rewinds at that rate (-1.0f is real time) when rewind is enabled with
 * SDL_Libretro_SetRewindEnabled() and the game's state can be captured;
 * otherwise it is clamped to 0.0f (paused).
 * Audio is pitched to match. The speed resets to 1.0f whenever a core is
 * loaded, and the call is ignored while the core has a fast-forward override
 * active that inhibits toggling.
 *
 * @param lr the libretro context.
 * @param speed the speed multiplier.
 *
 * @see SDL_Libretro_GetSpeed()
 * @see SDL_Libretro_IsFastforwardOverrideActive()
 * @see SDL_Libretro_Update()
 */
void SDL_Libretro_SetSpeed(SDL_Libretro* lr, float speed) {
    if (!lr) return;

    if (lr->core.fastforwardOverrideActive && lr->core.fastforwardOverride.inhibit_toggle) return;

    if (speed < 0.0f && lr->rewindEnabled && lr->rewindReference) {
        lr->core.speed = speed;
        if (lr->core.audioStream) {
            lr->core.drcAdjustment = 1.0f;
            lr->core.drcDriftAvg = 0.0;
            SDL_ClearAudioStream(lr->core.audioStream);
            // Pitch and consume the reversed audio at the rewind speed, mirroring
            // the forward path (which sets the ratio to speed * drcAdjustment).
            SDL_SetAudioStreamFrequencyRatio(lr->core.audioStream, -speed);
        }
    }
    else {
        lr->core.speed = SDL_max(speed, 0.0f);
    }

    if (lr->core.audioStream && lr->core.speed > 0.0f) {
        lr->core.drcAdjustment = 1.0f;
        lr->core.drcDriftAvg = 0.0;
        SDL_SetAudioStreamFrequencyRatio(lr->core.audioStream, lr->core.speed * lr->core.drcAdjustment);
    }
}

/**
 * Get how fast the game runs, as a multiple of its normal speed.
 *
 * This includes a speed set by the core's fast-forward override.
 *
 * @param lr the libretro context.
 * @returns the speed multiplier (0.0f paused, negative rewinding), or 1.0f when lr is NULL.
 *
 * @see SDL_Libretro_SetSpeed()
 */
float SDL_Libretro_GetSpeed(const SDL_Libretro* lr) {
    return lr ? lr->core.speed : 1.0f;
}

/**
 * Indicates whether the core is forcing fast-forward through RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE.
 *
 * While active, the core controls the speed; if it also inhibits toggling,
 * SDL_Libretro_SetSpeed() is ignored.
 *
 * @param lr the libretro context.
 * @returns true while the core's fast-forward override is active, false otherwise or when lr is NULL.
 *
 * @see SDL_Libretro_SetSpeed()
 */
bool SDL_Libretro_IsFastforwardOverrideActive(const SDL_Libretro* lr) {
    return lr && lr->core.fastforwardOverrideActive;
}

/**
 * Sets the threshold for logs to be posted.
 *
 * @param lr the libretro context.
 * @param level the minimum priority of core log messages to post. Priorities below SDL_LOG_PRIORITY_INFO (and unknown ones) post everything from debug up; SDL_LOG_PRIORITY_CRITICAL behaves like SDL_LOG_PRIORITY_ERROR.
 *
 * @see SDL_Libretro_GetLogLevel()
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

/**
 * Retrieve the threshold for logs that will be posted.
 *
 * @param lr the libretro context.
 * @returns the minimum priority of core log messages that are posted, or SDL_LOG_PRIORITY_INVALID when lr is NULL.
 *
 * @see SDL_Libretro_SetLogLevel()
 */
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
 *
 * @param lr the libretro context.
 * @returns the core's library name, or "" when no core is loaded. The string is owned by the context and stays valid until the core is unloaded.
 */
const char* SDL_Libretro_GetCoreName(const SDL_Libretro* lr) {
    return SDL_Libretro_IsCoreReady(lr) ? lr->core.libraryName : "";
}

/**
 * Retrieve the human-readable name of the loaded game.
 *
 * This is the content file's base name without its extension. When the core
 * runs without content, it falls back to the core's name.
 *
 * @param lr the libretro context.
 * @returns the game name, or "" when no game is loaded. The string is owned by the context and stays valid until the game is unloaded.
 */
const char* SDL_Libretro_GetGameName(const SDL_Libretro* lr) {
    return SDL_Libretro_IsGameReady(lr) ? lr->core.contentName : "";
}

/**
 * Retrieves the version of the libretro core that's actively loaded.
 *
 * @param lr the libretro context.
 * @returns the core's library version, or "" when no core is loaded. The string is owned by the context and stays valid until the core is unloaded.
 */
const char* SDL_Libretro_GetCoreVersion(const SDL_Libretro* lr) {
    return SDL_Libretro_IsCoreReady(lr) ? lr->core.libraryVersion : "";
}

/**
 * Gets the default set of valid extensions associated with the core, seperated by a "|".
 *
 * @param lr the libretro context.
 * @returns the extensions without leading dots (e.g. "nes|fds"), or "" when no core is loaded. The string is owned by the context and stays valid until the core is unloaded.
 */
const char* SDL_Libretro_GetValidExtensions(const SDL_Libretro* lr) {
    return SDL_Libretro_IsCoreReady(lr) ? lr->core.validExtensions : "";
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
 * Get the savestate context reported to cores through RETRO_ENVIRONMENT_GET_SAVESTATE_CONTEXT.
 *
 * @param lr the libretro context.
 * @returns the context set with SDL_Libretro_SetSavestateContext(), or RETRO_SAVESTATE_CONTEXT_NORMAL when none was set or lr is NULL.
 *
 * @see SDL_Libretro_SetSavestateContext()
 */
enum retro_savestate_context SDL_Libretro_GetSavestateContext(const SDL_Libretro* lr) {
    return lr ? lr->core.savestateContext : RETRO_SAVESTATE_CONTEXT_NORMAL;
}

/**
 * Sets the savestate context reported to cores through RETRO_ENVIRONMENT_GET_SAVESTATE_CONTEXT.
 *
 * Tells the core what upcoming save states are for, such as
 * RETRO_SAVESTATE_CONTEXT_RUNAHEAD_SAME_INSTANCE for run-ahead or
 * RETRO_SAVESTATE_CONTEXT_ROLLBACK_NETPLAY for netplay, so it can skip work
 * that isn't needed. It stays set until changed, and resets to
 * RETRO_SAVESTATE_CONTEXT_NORMAL when the core is unloaded.
 *
 * @param lr the libretro context. NULL is ignored.
 * @param context the savestate context.
 *
 * @see SDL_Libretro_GetSavestateContext()
 * @see SDL_Libretro_SaveState()
 */
void SDL_Libretro_SetSavestateContext(SDL_Libretro* lr, enum retro_savestate_context context) {
    if (lr) lr->core.savestateContext = context;
}

/**
 * Retrieves whether the core expects archives to be loaded directly.
 *
 * @param lr the libretro context.
 * @returns true if the core handles archive files itself.
 */
bool SDL_Libretro_GetBlockExtract(const SDL_Libretro* lr) {
    return SDL_Libretro_IsCoreReady(lr) && lr->core.blockExtract;
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
    return SDL_Libretro_GetExtension(lr->core.contentPath);
}

/**
 * @internal
 * Gets whether or not the given extension appears within the provided piped list.
 *
 * @param ext The extension to check, without the dot.
 * @param pipeList A piped list of extensions. For example: nes|rom|bin
 */
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
