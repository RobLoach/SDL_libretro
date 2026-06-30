/**
 * SDL_libretro config saving/loading.
 *
 * @file SDL_libretro_config.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_INI_IMPL_ONCE)
#define SDL_LIBRETRO_INI_IMPL_ONCE

/**
 * Set a configuration path to save/load core options from.
 */
bool SDL_Libretro_SetConfig(SDL_Libretro* lr, const char* file) {
    #ifndef SDL_INI_VERSION
        (void)lr;
        (void)file;
        return false;
    #else
        if (!lr || !file) return false;

        lr->iniFile = SDL_strdup(file);
        if (!lr->iniFile) return false;

        SDL_ini* ini = INI_Load(lr->iniFile);
        if (!ini) {
            ini = INI_Create();
            if (!ini) {
                SDL_free((void*)lr->iniFile);
                lr->iniFile = NULL;
                return false;
            }
        }

        SDL_Libretro_SetVolume(lr, INI_GetFloat(ini, NULL, "volume", 1.0f));
        SDL_Libretro_SetUsername(lr, INI_GetString(ini, NULL, "username", "SDL_libretro"));

        lr->ini = ini;
        return true;
    #endif
}

#ifdef SDL_INI_VERSION
static void SDL_Libretro_SetCoreConfigOption(void *userdata, const SDL_ini *ini, const char* section, const char *key, const char *value) {
    (void)ini;
    (void)section;
    SDL_Libretro_SetOptionValue((SDL_Libretro*)userdata, key, value);
}
#endif

static bool SDL_Libretro_LoadCoreConfig(SDL_Libretro* lr) {
    #ifndef SDL_INI_VERSION
        (void)lr;
        return false;
    #else
        INI_EnumerateKeys(lr->ini, lr->core.libraryName, &SDL_Libretro_SetCoreConfigOption, (void*)lr);
        return false;
    #endif
}

static bool SDL_Libretro_SaveCoreConfig(SDL_Libretro* lr) {
    #ifndef SDL_INI_VERSION
        (void)lr;
        return false;
    #else
        SDL_ini* ini = lr->ini;
        if (!ini) return false;
        int count = SDL_Libretro_GetOptionCount(lr);
        for (int i = 0; i < count; i++) {
            const SDL_LibretroOption* option = SDL_Libretro_GetOptionByIndex(lr, i);
            if (!option) continue;
            INI_SetString(ini, lr->core.libraryName, option->key, option->value ? option->value : option->defaultValue);
        }
        return false;
    #endif
}

static bool SDL_Libretro_SaveConfig(SDL_Libretro* lr) {
    #ifndef SDL_INI_VERSION
        (void)lr;
        return false;
    #else
        if (!lr->ini || !lr->iniFile) return false;
        SDL_ini* ini = lr->ini;

        INI_SetFloat(ini, NULL, "volume", SDL_Libretro_GetVolume(lr));
        INI_SetString(ini, NULL, "username", SDL_Libretro_GetUsername(lr));
        INI_Save(ini, lr->iniFile);

        SDL_free((void*)lr->iniFile);
        lr->iniFile = NULL;

        INI_Destroy(ini);
        lr->ini = NULL;

        return true;
    #endif
}

#endif
