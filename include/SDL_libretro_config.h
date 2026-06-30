/**
 * SDL_libretro config saving/loading.
 *
 * @file SDL_libretro_config.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_INI_IMPL_ONCE)
#define SDL_LIBRETRO_INI_IMPL_ONCE

#ifdef SDL_LIBRETRO_NO_CONFIG

bool SDL_Libretro_LoadConfig(SDL_Libretro* lr, const char* file) {
    (void)lr; (void)file;
    return false;
}

bool SDL_Libretro_LoadDefaultConfig(SDL_Libretro* lr) {
    (void)lr;
    return false;
}

bool SDL_Libretro_SaveConfig(SDL_Libretro* lr) {
    (void)lr;
    return false;
}

bool SDL_Libretro_UnloadConfig(SDL_Libretro* lr) {
    (void)lr;
    return false;
}

const char* SDL_Libretro_GetConfig(const SDL_Libretro* lr) {
    (void)lr;
    return NULL;
}

static bool SDL_Libretro_LoadCoreConfig(SDL_Libretro* lr) {
    (void)lr;
    return false;
}

static bool SDL_Libretro_SaveCoreConfig(SDL_Libretro* lr) {
    (void)lr;
    return false;
}

#else /* !SDL_LIBRETRO_NO_CONFIG */

static void SDL_Libretro_SanitizeSectionName(char* dst, size_t dstSize, const char* name) {
    SDL_strlcpy(dst, name, dstSize);
    for (char* p = dst; *p; ++p) {
        if (*p == '[' || *p == ']' || *p == '\n' || *p == '\r') {
            *p = '_';
        }
    }
}

bool SDL_Libretro_LoadConfig(SDL_Libretro* lr, const char* file) {
    if (!lr || !file) return false;

    SDL_free(lr->iniFile);
    lr->iniFile = NULL;
    INI_Destroy(lr->ini);
    lr->ini = NULL;

    SDL_ini* ini = INI_Load(file);
    if (!ini) {
        ini = INI_Create();
        if (!ini) return false;
    }

    if (INI_HasKey(ini, NULL, "volume"))
        SDL_Libretro_SetVolume(lr, INI_GetFloat(ini, NULL, "volume", 1.0f));
    if (INI_HasKey(ini, NULL, "username"))
        SDL_Libretro_SetUsername(lr, INI_GetString(ini, NULL, "username", "SDL_libretro"));
    if (INI_HasKey(ini, NULL, "audiolatency"))
        SDL_Libretro_SetAudioLatency(lr, (unsigned)INI_GetInt(ini, NULL, "audiolatency", 100));
    if (INI_HasKey(ini, NULL, "scalemode")) {
        const char* scaleStr = INI_GetString(ini, NULL, "scalemode", "aspect");
        if (SDL_strcasecmp(scaleStr, "integer") == 0) {
            SDL_Libretro_SetScaleMode(lr, SDL_LIBRETRO_SCALE_INTEGER);
        } else {
            SDL_Libretro_SetScaleMode(lr, SDL_LIBRETRO_SCALE_ASPECT);
        }
    }
    if (INI_HasKey(ini, NULL, "savedirectory"))
        SDL_Libretro_SetSaveDirectory(lr, INI_GetString(ini, NULL, "savedirectory", ""));
    if (INI_HasKey(ini, NULL, "systemdirectory"))
        SDL_Libretro_SetSystemDirectory(lr, INI_GetString(ini, NULL, "systemdirectory", ""));
    if (INI_HasKey(ini, NULL, "coredirectory"))
        SDL_Libretro_SetCoreDirectory(lr, INI_GetString(ini, NULL, "coredirectory", ""));
    if (INI_HasKey(ini, NULL, "coreassetsdirectory"))
        SDL_Libretro_SetCoreAssetsDirectory(lr, INI_GetString(ini, NULL, "coreassetsdirectory", ""));

    lr->iniFile = SDL_strdup(file);
    if (!lr->iniFile) {
        INI_Destroy(ini);
        return false;
    }

    lr->ini = ini;
    return true;
}

bool SDL_Libretro_LoadDefaultConfig(SDL_Libretro* lr) {
    if (!lr) return false;
    char* prefPath = SDL_GetPrefPath("SDL_libretro", "SDL_libretro");
    if (!prefPath) return false;
    char path[SDL_LIBRETRO_MAX_PATH];
    SDL_snprintf(path, sizeof(path), "%sconfig.cfg", prefPath);
    SDL_free(prefPath);
    return SDL_Libretro_LoadConfig(lr, path);
}

const char* SDL_Libretro_GetConfig(const SDL_Libretro* lr) {
    if (!lr) return NULL;
    return lr->iniFile;
}

static void SDL_Libretro_SetCoreConfigOption(void *userdata, const SDL_ini *ini, const char* section, const char *key, const char *value) {
    (void)ini;
    (void)section;
    SDL_Libretro_SetOptionValue((SDL_Libretro*)userdata, key, value);
}

static bool SDL_Libretro_LoadCoreConfig(SDL_Libretro* lr) {
    if (!lr->ini) return false;
    char section[256];
    SDL_Libretro_SanitizeSectionName(section, sizeof(section), lr->core.libraryName);
    INI_EnumerateKeys(lr->ini, section, &SDL_Libretro_SetCoreConfigOption, (void*)lr);
    return true;
}

static bool SDL_Libretro_SaveCoreConfig(SDL_Libretro* lr) {
    if (!lr->ini) return false;
    char section[256];
    SDL_Libretro_SanitizeSectionName(section, sizeof(section), lr->core.libraryName);
    int count = SDL_Libretro_GetOptionCount(lr);
    for (int i = 0; i < count; i++) {
        const SDL_LibretroOption* option = SDL_Libretro_GetOptionByIndex(lr, i);
        if (!option) continue;
        const char* val = option->value ? option->value : option->defaultValue;
        if (option->defaultValue && SDL_strcmp(val, option->defaultValue) == 0) {
            INI_RemoveKey(lr->ini, section, option->key);
            continue;
        }
        INI_SetString(lr->ini, section, option->key, val);
    }
    return true;
}

bool SDL_Libretro_SaveConfig(SDL_Libretro* lr) {
    if (!lr || !lr->ini || !lr->iniFile) return false;

    if (lr->core.loaded) {
        SDL_Libretro_SaveCoreConfig(lr);
    }

    INI_SetFloat(lr->ini, NULL, "volume", SDL_Libretro_GetVolume(lr));
    INI_SetString(lr->ini, NULL, "username", SDL_Libretro_GetUsername(lr));
    INI_SetInt(lr->ini, NULL, "audiolatency", (Sint64)SDL_Libretro_GetAudioLatency(lr));
    INI_SetString(lr->ini, NULL, "scalemode",
        SDL_Libretro_GetScaleMode(lr) == SDL_LIBRETRO_SCALE_INTEGER ? "integer" : "aspect");

    if (lr->saveDirectory[0])
        INI_SetString(lr->ini, NULL, "savedirectory", lr->saveDirectory);
    if (lr->systemDirectory[0])
        INI_SetString(lr->ini, NULL, "systemdirectory", lr->systemDirectory);
    if (lr->coreDirectory[0])
        INI_SetString(lr->ini, NULL, "coredirectory", lr->coreDirectory);
    if (lr->coreAssetsDirectory[0])
        INI_SetString(lr->ini, NULL, "coreassetsdirectory", lr->coreAssetsDirectory);

    return INI_Save(lr->ini, lr->iniFile);
}

bool SDL_Libretro_UnloadConfig(SDL_Libretro* lr) {
    bool ok = SDL_Libretro_SaveConfig(lr);
    SDL_free(lr->iniFile);
    lr->iniFile = NULL;
    INI_Destroy(lr->ini);
    lr->ini = NULL;
    return ok;
}

#endif /* SDL_LIBRETRO_NO_CONFIG */

#endif
