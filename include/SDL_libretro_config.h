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
    if (INI_HasValue(ini, NULL, "rewindenabled"))
        SDL_Libretro_SetRewindEnabled(lr, INI_GetBoolean(ini, NULL, "rewindenabled", false), 0, 0);

    return true;
}

/**
 * Sets up the config file to be the default path, accoring to the organization and app name.
 *
 * Will automatically call SDL_Libretro_CloseConfig()
 *
 * @see SDL_Libretro_InitConfigFile
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
    // Only an explicit scale mode is saved; a default one must stay eligible for the nearest to pixelart upgrade on reload.
    if (lr->scaleModeExplicit)
        INI_SetInt(lr->ini, NULL, "scalemode", (Sint64)SDL_Libretro_GetScaleMode(lr));
    INI_SetString(lr->ini, NULL, "savedirectory", SDL_Libretro_GetSaveDirectory(lr));
    INI_SetString(lr->ini, NULL, "systemdirectory", SDL_Libretro_GetSystemDirectory(lr));
    INI_SetString(lr->ini, NULL, "coredirectory", SDL_Libretro_GetCoreDirectory(lr));
    INI_SetString(lr->ini, NULL, "coreassetsdirectory", SDL_Libretro_GetCoreAssetsDirectory(lr));
    INI_SetBoolean(lr->ini, NULL, "rewindenabled", SDL_Libretro_GetRewindEnabled(lr));

    return INI_Save(lr->ini, lr->iniFile);
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
