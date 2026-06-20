#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_OPTIONS_IMPL_ONCE)
#define SDL_LIBRETRO_OPTIONS_IMPL_ONCE

/*
 * SDL_libretro - core options management
 */


static char* SDL_Libretro_Strdup(const char* s) {
    if (!s) return SDL_strdup("");
    return SDL_strdup(s);
}

static void SDL_Libretro_InitCoreOption(SDL_Libretro* lr, const char* key, const char* defaultValue,
    const char* label, const char* valuesList, const char* displayList,
    const char* tooltip, const char* categoryKey) {
    if (!lr || !key) return;

    // Check if already registered
    for (unsigned i = 0; i < lr->core.optionCount; i++) {
        if (lr->core.options[i].key && SDL_strcmp(lr->core.options[i].key, key) == 0) {
            return;
        }
    }

    // Grow array if needed
    if (lr->core.optionCount >= lr->core.optionCapacity) {
        unsigned newCap = lr->core.optionCapacity ? lr->core.optionCapacity * 2 : 32;
        SDL_LibretroCoreOption* newOpts = (SDL_LibretroCoreOption*)SDL_realloc(
            lr->core.options, newCap * sizeof(SDL_LibretroCoreOption));
        if (!newOpts) return;
        lr->core.options = newOpts;
        lr->core.optionCapacity = newCap;
    }

    SDL_LibretroCoreOption* opt = &lr->core.options[lr->core.optionCount];
    opt->key = SDL_Libretro_Strdup(key);
    opt->value = SDL_Libretro_Strdup(defaultValue);
    opt->defaultValue = SDL_Libretro_Strdup(defaultValue);
    opt->label = SDL_Libretro_Strdup(label);
    opt->valuesList = SDL_Libretro_Strdup(valuesList);
    opt->displayList = SDL_Libretro_Strdup(displayList);
    opt->tooltip = SDL_Libretro_Strdup(tooltip);
    opt->categoryKey = SDL_Libretro_Strdup(categoryKey);
    opt->visible = true;

    lr->core.optionCount++;
    lr->core.optionsVisibilityDirty = true;
}

static void SDL_Libretro_FreeCoreOptions(SDL_Libretro* lr) {
    if (!lr || !lr->core.options) return;

    for (unsigned i = 0; i < lr->core.optionCount; i++) {
        SDL_free(lr->core.options[i].key);
        SDL_free(lr->core.options[i].value);
        SDL_free(lr->core.options[i].defaultValue);
        SDL_free(lr->core.options[i].label);
        SDL_free(lr->core.options[i].valuesList);
        SDL_free(lr->core.options[i].displayList);
        SDL_free(lr->core.options[i].tooltip);
        SDL_free(lr->core.options[i].categoryKey);
    }
    SDL_free(lr->core.options);
    lr->core.options = NULL;
    lr->core.optionCount = 0;
    lr->core.optionCapacity = 0;
}

unsigned SDL_Libretro_GetOptionCount(const SDL_Libretro* lr) {
    return lr ? lr->core.optionCount : 0;
}

const char* SDL_Libretro_GetOptionKey(const SDL_Libretro* lr, unsigned index) {
    if (!lr || index >= lr->core.optionCount) return NULL;
    return lr->core.options[index].key;
}

const char* SDL_Libretro_GetOptionValue(const SDL_Libretro* lr, const char* key) {
    if (!lr || !key) return NULL;
    for (unsigned i = 0; i < lr->core.optionCount; i++) {
        if (lr->core.options[i].key && SDL_strcmp(lr->core.options[i].key, key) == 0) {
            return lr->core.options[i].value;
        }
    }
    return NULL;
}

bool SDL_Libretro_SetOptionValue(SDL_Libretro* lr, const char* key, const char* value) {
    if (!lr || !key || !value) return false;
    for (unsigned i = 0; i < lr->core.optionCount; i++) {
        if (lr->core.options[i].key && SDL_strcmp(lr->core.options[i].key, key) == 0) {
            SDL_free(lr->core.options[i].value);
            lr->core.options[i].value = SDL_strdup(value);
            lr->core.optionsDirty = true;
            return true;
        }
    }
    return false;
}

bool SDL_Libretro_ResetOption(SDL_Libretro* lr, const char* key) {
    if (!lr || !key) return false;
    for (unsigned i = 0; i < lr->core.optionCount; i++) {
        if (lr->core.options[i].key && SDL_strcmp(lr->core.options[i].key, key) == 0) {
            SDL_free(lr->core.options[i].value);
            lr->core.options[i].value = SDL_strdup(lr->core.options[i].defaultValue);
            lr->core.optionsDirty = true;
            return true;
        }
    }
    return false;
}

void SDL_Libretro_ResetAllOptions(SDL_Libretro* lr) {
    if (!lr) return;
    for (unsigned i = 0; i < lr->core.optionCount; i++) {
        SDL_free(lr->core.options[i].value);
        lr->core.options[i].value = SDL_strdup(lr->core.options[i].defaultValue);
    }
    lr->core.optionsDirty = true;
}

bool SDL_Libretro_AreOptionsDirty(SDL_Libretro* lr) {
    if (!lr) return false;
    bool dirty = lr->core.optionsDirty;
    lr->core.optionsDirty = false;
    return dirty;
}

#endif /* SDL_LIBRETRO_OPTIONS_IMPL_ONCE */
