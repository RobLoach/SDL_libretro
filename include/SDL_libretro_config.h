/**
 * SDL_libretro config saving/loading.
 *
 * @file SDL_libretro_config.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_CONFIG_IMPL_ONCE)
#define SDL_LIBRETRO_CONFIG_IMPL_ONCE

/**
 * Cleans up a core name to be used as an .ini section name.
 */
static void SDL_Libretro_SanitizeSectionName(char* dst, size_t dstSize, const char* name) {
    // Replace [], new lines, and spaces with _.
    // This will make `Genesis Plus GX` match with `genesis_plus_gx`
    SDL_strlcpy(dst, name, dstSize);
    for (char* p = dst; *p; ++p) {
        if (*p == '[' || *p == ']' || *p == '\n' || *p == '\r' || *p == ' ') {
            *p = '_';
        }
    }
}

/**
 * Initializes the config system based on the given file.
 *
 * Will load the configuration from the file, and save it when destroying the instance.
 * A missing file is not an error: it starts out empty and is created on save.
 * Any previously initialized config is discarded without being saved.
 *
 * @param lr the libretro context.
 * @param file the path of the .ini file to load from and save to. The string is copied.
 * @returns true when the config was set up, false when lr or file is NULL, or on allocation failure.
 *
 * @see SDL_Libretro_InitConfig()
 */
bool SDL_Libretro_InitConfigFile(SDL_Libretro* lr, const char* file) {
    if (!lr || !file) return false;

    // Clear our any existing config.
    SDL_free(lr->iniFile);
    lr->iniFile = NULL;
    INI_Destroy(lr->ini);
    lr->ini = NULL;

    // Load the file.
    SDL_ini* ini = INI_Load(file);
    if (!ini) {
        ini = INI_Create();
        if (!ini) return false;
    }

    // Capture the ini file path.
    lr->iniFile = SDL_strdup(file);
    if (!lr->iniFile) {
        INI_Destroy(ini);
        return false;
    }
    lr->ini = ini;

    // Get all the configs.
    if (INI_HasValue(ini, NULL, "volume"))
        SDL_Libretro_SetVolume(lr, INI_GetFloat(ini, NULL, "volume", SDL_Libretro_GetVolume(lr)));
    if (INI_HasValue(ini, NULL, "username"))
        SDL_Libretro_SetUsername(lr, INI_GetString(ini, NULL, "username", SDL_Libretro_GetUsername(lr)));
    if (INI_HasValue(ini, NULL, "audiolatency"))
        SDL_Libretro_SetAudioLatency(lr, (unsigned)INI_GetInt(ini, NULL, "audiolatency", SDL_Libretro_GetAudioLatency(lr)));
    if (INI_HasValue(ini, NULL, "fitmode"))
        SDL_Libretro_SetFitMode(lr, (SDL_LibretroFitMode)INI_GetInt(ini, NULL, "fitmode", SDL_Libretro_GetFitMode(lr)));
    if (INI_HasValue(ini, NULL, "scalemode"))
        SDL_Libretro_SetScaleMode(lr, (SDL_ScaleMode)INI_GetInt(ini, NULL, "scalemode", (Sint64)SDL_Libretro_GetScaleMode(lr)));
    if (INI_HasValue(ini, NULL, "savedirectory"))
        SDL_Libretro_SetSaveDirectory(lr, INI_GetString(ini, NULL, "savedirectory", SDL_Libretro_GetSaveDirectory(lr)));
    if (INI_HasValue(ini, NULL, "systemdirectory"))
        SDL_Libretro_SetSystemDirectory(lr, INI_GetString(ini, NULL, "systemdirectory", SDL_Libretro_GetSystemDirectory(lr)));
    if (INI_HasValue(ini, NULL, "coredirectory"))
        SDL_Libretro_SetCoreDirectory(lr, INI_GetString(ini, NULL, "coredirectory", SDL_Libretro_GetCoreDirectory(lr)));
    if (INI_HasValue(ini, NULL, "coreassetsdirectory"))
        SDL_Libretro_SetCoreAssetsDirectory(lr, INI_GetString(ini, NULL, "coreassetsdirectory", SDL_Libretro_GetCoreAssetsDirectory(lr)));
    if (INI_HasValue(ini, NULL, "rewindenabled") || INI_HasValue(ini, NULL, "rewindframes") || INI_HasValue(ini, NULL, "rewindinterval")) {
        // Keep the current snapshot count and capture interval unless saved; 0 means the default.
        bool enabled = INI_GetBoolean(ini, NULL, "rewindenabled", SDL_Libretro_GetRewindEnabled(lr));
        Sint64 frames = INI_GetInt(ini, NULL, "rewindframes", (Sint64)lr->rewindCapacity);
        Sint64 interval = INI_GetInt(ini, NULL, "rewindinterval", (Sint64)lr->rewindCaptureInterval);
        unsigned bufferFrames = (frames > 0 && frames <= SDL_MAX_UINT32) ? (unsigned)frames : 0;
        unsigned captureInterval = (interval > 0 && interval <= SDL_MAX_UINT32) ? (unsigned)interval : 0;
        SDL_Libretro_SetRewindEnabled(lr, enabled, bufferFrames, captureInterval);
        if (!enabled) {
            // Disabling frees the buffers without storing the settings, so keep them for re-enabling later.
            lr->rewindCapacity = bufferFrames;
            lr->rewindCaptureInterval = captureInterval;
        }
    }
    if (INI_HasValue(ini, NULL, "rewindmemorylimit"))
        SDL_Libretro_SetRewindMemoryLimit(lr, (size_t)INI_GetInt(ini, NULL, "rewindmemorylimit", (Sint64)SDL_Libretro_GetRewindMemoryLimit(lr)));
    if (INI_HasValue(ini, NULL, "mouseport"))
        SDL_Libretro_SetMousePort(lr, (unsigned)INI_GetInt(ini, NULL, "mouseport", (Sint64)SDL_Libretro_GetMousePort(lr)));
    if (INI_HasValue(ini, NULL, "language")) {
        // -1 (or anything out of range) detects it from the system locale.
        Sint64 language = INI_GetInt(ini, NULL, "language", (Sint64)lr->language);
        SDL_Libretro_SetLanguage(lr, (language >= 0 && language < RETRO_LANGUAGE_LAST) ? (int)language : -1);
    }
    if (INI_HasValue(ini, NULL, "filebrowserdirectory"))
        SDL_strlcpy(lr->fileBrowserStartDirectory, INI_GetString(ini, NULL, "filebrowserdirectory", ""), sizeof(lr->fileBrowserStartDirectory));

    // Player 1 keyboard bindings, stored as SDL scancode names.
    for (int button = 0; button < SDL_LIBRETRO_MAX_JOYPAD_BUTTONS; button++) {
        if (!INI_HasValue(ini, "keyboard", SDL_Libretro_JoypadButtonNames[button])) {
            continue;
        }
        SDL_Scancode scancode = SDL_GetScancodeFromName(INI_GetString(ini, "keyboard", SDL_Libretro_JoypadButtonNames[button], ""));
        if (scancode != SDL_SCANCODE_UNKNOWN) {
            SDL_Libretro_SetKeyboardMapping(lr, button, scancode);
        }
    }

    // Per-port gamepad bindings ([gamepad1]..[gamepadN]), stored as SDL
    // gamepad button names, with the analog-to-digital stick by mode name.
    for (unsigned port = 0; port < SDL_LIBRETRO_MAX_USERS; port++) {
        char section[16];
        SDL_snprintf(section, sizeof(section), "gamepad%u", port + 1);
        for (int button = 0; button < SDL_LIBRETRO_MAX_JOYPAD_BUTTONS; button++) {
            if (!INI_HasValue(ini, section, SDL_Libretro_JoypadButtonNames[button])) {
                continue;
            }
            SDL_GamepadButton pad = SDL_GetGamepadButtonFromString(INI_GetString(ini, section, SDL_Libretro_JoypadButtonNames[button], ""));
            if (pad != SDL_GAMEPAD_BUTTON_INVALID) {
                SDL_Libretro_SetGamepadMapping(lr, port, button, pad);
            }
        }

        const char* analog = INI_GetString(ini, section, "AnalogToDigital", NULL);
        if (analog != NULL) {
            for (int mode = 0; mode < SDL_LIBRETRO_ANALOG_TO_DIGITAL_COUNT; mode++) {
                if (SDL_strcasecmp(analog, SDL_Libretro_AnalogToDigitalNames[mode]) == 0) {
                    SDL_Libretro_SetAnalogToDigital(lr, port, (SDL_LibretroAnalogToDigital)mode);
                }
            }
        }
    }

    return true;
}

/**
 * Sets up the config file to be the default path, accoring to the organization and app name.
 *
 * The file is "<app>.cfg" inside SDL_GetPrefPath(org, app). Any previously
 * initialized config is discarded without being saved.
 *
 * @param lr the libretro context.
 * @param org the organization name, or NULL/empty for "SDL_libretro".
 * @param app the application name, or NULL/empty for "SDL_libretro".
 * @returns true when the config was set up, false when lr is NULL, no pref path is available, or SDL_Libretro_InitConfigFile() fails.
 *
 * @see SDL_Libretro_InitConfigFile()
 * @see SDL_GetPrefPath()
 * @see https://wiki.libsdl.org/SDL3/SDL_GetPrefPath
 */
bool SDL_Libretro_InitConfig(SDL_Libretro* lr, const char* org, const char* app) {
    if (!lr) return false;
    if (!org || org[0] == '\0') org = "SDL_libretro";
    if (!app || app[0] == '\0') app = "SDL_libretro";
    char* prefPath = SDL_GetPrefPath(org, app);
    if (!prefPath) return false;
    char path[SDL_LIBRETRO_MAX_PATH];
    SDL_snprintf(path, sizeof(path), "%s%s.cfg", prefPath, app);
    SDL_free(prefPath);
    return SDL_Libretro_InitConfigFile(lr, path);
}

/**
 * Callback to set the given config option as a core option.
 */
static void SDL_Libretro_SetCoreConfigOption(void* userdata, const SDL_ini* ini, const char* section, const char* key, const char* value) {
    (void)ini;
    (void)section;
    SDL_Libretro_SetOptionValue((SDL_Libretro*)userdata, key, value);
}

/**
 * Loads the core options from the config, if it exists.
 */
static bool SDL_Libretro_LoadCoreConfig(SDL_Libretro* lr) {
    if (!lr || !lr->ini || !SDL_Libretro_IsCoreReady(lr)) return false;

    // Set the library name as the section name.
    char section[128];
    SDL_Libretro_SanitizeSectionName(section, sizeof(section), lr->core.libraryName);

    // Ensure there's a valid section name to enumerate.
    if (section[0] == '\0') {
        return false;
    }
    INI_EnumerateKeys(lr->ini, section, &SDL_Libretro_SetCoreConfigOption, (void*)lr);
    return true;
}

/**
 * Get the saved config value for a core option, or NULL if there isn't one.
 *
 * Lets options registered after SDL_Libretro_LoadCoreConfig() ran, such as in retro_init() or retro_load_game(), pick up their saved value.
 */
static const char* SDL_Libretro_GetSavedCoreOption(const SDL_Libretro* lr, const char* key) {
    if (!lr || !lr->ini || !key || lr->core.libraryName[0] == '\0') return NULL;
    char section[128];
    SDL_Libretro_SanitizeSectionName(section, sizeof(section), lr->core.libraryName);
    if (section[0] == '\0') return NULL;
    return INI_GetString(lr->ini, section, key, NULL);
}

/**
 * Set the core options directly into the config.
 */
static bool SDL_Libretro_SaveCoreConfig(SDL_Libretro* lr) {
    if (!SDL_Libretro_IsCoreReady(lr) || !lr->ini) return false;
    char section[128];
    SDL_Libretro_SanitizeSectionName(section, sizeof(section), lr->core.libraryName);
    if (section[0] == '\0') {
        return false;
    }
    int count = SDL_Libretro_GetOptionCount(lr);
    for (int i = 0; i < count; i++) {
        const SDL_LibretroOption* option = SDL_Libretro_GetOptionByIndex(lr, i);
        if (!option) continue;
        const char* val = option->value ? option->value : option->defaultValue;
        // Save the value in the config even if it matches the default.
        INI_SetString(lr->ini, section, option->key, val);
    }
    return true;
}

/**
 * Save the configuration to the file system.
 */
static bool SDL_Libretro_SaveConfig(SDL_Libretro* lr) {
    if (!lr || !lr->ini || !lr->iniFile) return false;

    // Save the core options if needed.
    SDL_Libretro_SaveCoreConfig(lr);

    INI_SetFloat(lr->ini, NULL, "volume", SDL_Libretro_GetVolume(lr));
    INI_SetString(lr->ini, NULL, "username", SDL_Libretro_GetUsername(lr));
    INI_SetInt(lr->ini, NULL, "audiolatency", (Sint64)SDL_Libretro_GetAudioLatency(lr));
    INI_SetInt(lr->ini, NULL, "fitmode", (Sint64)SDL_Libretro_GetFitMode(lr));
    INI_SetInt(lr->ini, NULL, "scalemode", (Sint64)SDL_Libretro_GetScaleMode(lr));
    INI_SetString(lr->ini, NULL, "savedirectory", SDL_Libretro_GetSaveDirectory(lr));
    INI_SetString(lr->ini, NULL, "systemdirectory", SDL_Libretro_GetSystemDirectory(lr));
    INI_SetString(lr->ini, NULL, "coredirectory", SDL_Libretro_GetCoreDirectory(lr));
    INI_SetString(lr->ini, NULL, "coreassetsdirectory", SDL_Libretro_GetCoreAssetsDirectory(lr));
    INI_SetBoolean(lr->ini, NULL, "rewindenabled", SDL_Libretro_GetRewindEnabled(lr));
    INI_SetInt(lr->ini, NULL, "rewindframes", (Sint64)lr->rewindCapacity);
    INI_SetInt(lr->ini, NULL, "rewindinterval", (Sint64)lr->rewindCaptureInterval);
    INI_SetInt(lr->ini, NULL, "rewindmemorylimit", (Sint64)SDL_Libretro_GetRewindMemoryLimit(lr));
    INI_SetInt(lr->ini, NULL, "mouseport", (Sint64)SDL_Libretro_GetMousePort(lr));
    // The raw override, so -1 keeps following the system locale.
    INI_SetInt(lr->ini, NULL, "language", (Sint64)lr->language);
    INI_SetString(lr->ini, NULL, "filebrowserdirectory", lr->fileBrowserStartDirectory);

    for (int button = 0; button < SDL_LIBRETRO_MAX_JOYPAD_BUTTONS; button++) {
        INI_SetString(lr->ini, "keyboard", SDL_Libretro_JoypadButtonNames[button], SDL_GetScancodeName(lr->keyboardPlayer1[button]));
    }

    // Per-port gamepad bindings ([gamepad1]..[gamepadN]).
    for (unsigned port = 0; port < SDL_LIBRETRO_MAX_USERS; port++) {
        char section[16];
        SDL_snprintf(section, sizeof(section), "gamepad%u", port + 1);
        for (int button = 0; button < SDL_LIBRETRO_MAX_JOYPAD_BUTTONS; button++) {
            const char* padName = SDL_GetGamepadStringForButton(lr->gamepadButtons[port][button]);
            INI_SetString(lr->ini, section, SDL_Libretro_JoypadButtonNames[button], padName != NULL ? padName : "");
        }
        INI_SetString(lr->ini, section, "AnalogToDigital", SDL_Libretro_AnalogToDigitalNames[lr->analogToDigital[port]]);
    }

    // Write through a temporary file, so a failed save can't truncate the config.
    char tempPath[SDL_LIBRETRO_MAX_PATH];
    SDL_IOStream* io = SDL_Libretro_OpenSaveFile(lr->iniFile, tempPath, sizeof(tempPath));
    if (!io) return false;
    return SDL_Libretro_CommitSaveFile(io, INI_Save_IO(lr->ini, io, false), tempPath, lr->iniFile);
}

/**
 * Called when closing the instance to save the config and close.
 */
static bool SDL_Libretro_CloseConfig(SDL_Libretro* lr) {
    if (!lr) return false;
    bool ok = SDL_Libretro_SaveConfig(lr);
    SDL_free(lr->iniFile);
    lr->iniFile = NULL;
    INI_Destroy(lr->ini);
    lr->ini = NULL;
    return ok;
}

#endif
