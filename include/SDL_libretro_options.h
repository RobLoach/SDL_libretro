/**
 * SDL_libretro - core options management
 * @file SDL_libretro_options.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_OPTIONS_IMPL_ONCE)
#define SDL_LIBRETRO_OPTIONS_IMPL_ONCE

static char* SDL_Libretro_Strdup(const char* s) {
    if (!s) return SDL_strdup("");
    return SDL_strdup(s);
}

/**
 * Finds a registered option by key, or NULL if there is no such option.
 */
static const SDL_LibretroCoreOption* SDL_Libretro_GetOption(const SDL_Libretro* lr, const char* key) {
    if (!lr || !key) return NULL;
    for (unsigned i = 0; i < lr->core.optionCount; i++) {
        if (lr->core.options[i].key && SDL_strcmp(lr->core.options[i].key, key) == 0) {
            return &lr->core.options[i];
        }
    }
    return NULL;
}

static void SDL_Libretro_InitCoreOption(SDL_Libretro* lr, const char* key, const char* defaultValue,
    const char* label, const struct retro_core_option_value* values,
    const char* info, const char* categoryKey) {
    if (!lr || !key) return;

    // Check if already registered
    if (SDL_Libretro_GetOption(lr, key)) {
        return;
    }

    // Grow array if needed
    if (lr->core.optionCount >= lr->core.optionCapacity) {
        unsigned newCap = lr->core.optionCapacity ? lr->core.optionCapacity * 2 : 16;
        SDL_LibretroCoreOption* newOpts = (SDL_LibretroCoreOption*)SDL_realloc(lr->core.options, newCap * sizeof(SDL_LibretroCoreOption));
        if (!newOpts) return;
        lr->core.options = newOpts;
        lr->core.optionCapacity = newCap;
    }

    // Set the option.
    SDL_LibretroCoreOption* opt = &lr->core.options[lr->core.optionCount];
    opt->key = SDL_Libretro_Strdup(key);
    opt->value = SDL_Libretro_Strdup(defaultValue);
    opt->defaultValue = SDL_Libretro_Strdup(defaultValue);
    opt->label = SDL_Libretro_Strdup(label);

    // Deep-copy the values; the slot came from SDL_realloc and isn't zeroed, so
    // clear the array first. Copying stops at the { NULL, NULL } terminator.
    SDL_memset(opt->values, 0, sizeof(opt->values));
    opt->valuesCount = 0;
    if (values) {
        for (unsigned v = 0; v < RETRO_NUM_CORE_OPTION_VALUES_MAX && values[v].value; v++) {
            opt->values[v].value = SDL_Libretro_Strdup(values[v].value);
            opt->values[v].label = values[v].label ? SDL_Libretro_Strdup(values[v].label) : NULL;
            opt->valuesCount++;
        }
    }

    opt->info = SDL_Libretro_Strdup(info);
    opt->categoryKey = SDL_Libretro_Strdup(categoryKey);
    opt->visible = true;

    lr->core.optionCount++;
}

static void SDL_Libretro_InitCoreOptionCategory(SDL_Libretro* lr, const char* key,
    const char* label, const char* info) {
    if (!lr || !key || key[0] == '\0') return;

    // Check if already registered
    for (unsigned i = 0; i < lr->core.optionCategoryCount; i++) {
        if (lr->core.optionCategories[i].key && SDL_strcmp(lr->core.optionCategories[i].key, key) == 0) {
            return;
        }
    }

    // Grow array if needed
    if (lr->core.optionCategoryCount >= lr->core.optionCategoryCapacity) {
        unsigned newCap = lr->core.optionCategoryCapacity ? lr->core.optionCategoryCapacity * 2 : 8;
        struct retro_core_option_v2_category* newCats = (struct retro_core_option_v2_category*)SDL_realloc(lr->core.optionCategories, newCap * sizeof(struct retro_core_option_v2_category));
        if (!newCats) return;
        lr->core.optionCategories = newCats;
        lr->core.optionCategoryCapacity = newCap;
    }

    // Set the category; strings are deep-copied and owned by the context.
    struct retro_core_option_v2_category* cat = &lr->core.optionCategories[lr->core.optionCategoryCount];
    cat->key = SDL_Libretro_Strdup(key);
    cat->desc = SDL_Libretro_Strdup(label);
    cat->info = SDL_Libretro_Strdup(info);

    lr->core.optionCategoryCount++;
}

static void SDL_Libretro_FreeCoreOptions(SDL_Libretro* lr) {
    if (!lr) return;

    // Options
    if (lr->core.options) {
        for (unsigned i = 0; i < lr->core.optionCount; i++) {
            SDL_free(lr->core.options[i].key);
            SDL_free(lr->core.options[i].value);
            SDL_free(lr->core.options[i].defaultValue);
            SDL_free(lr->core.options[i].label);
            for (unsigned v = 0; v < RETRO_NUM_CORE_OPTION_VALUES_MAX
                    && lr->core.options[i].values[v].value; v++) {
                SDL_free((void*)lr->core.options[i].values[v].value);
                SDL_free((void*)lr->core.options[i].values[v].label);
            }
            SDL_free(lr->core.options[i].info);
            SDL_free(lr->core.options[i].categoryKey);
        }
        SDL_free(lr->core.options);
        lr->core.options = NULL;
    }
    lr->core.optionCount = 0;
    lr->core.optionCapacity = 0;

    // Categories
    if (lr->core.optionCategories) {
        for (unsigned i = 0; i < lr->core.optionCategoryCount; i++) {
            SDL_free((void*)lr->core.optionCategories[i].key);
            SDL_free((void*)lr->core.optionCategories[i].desc);
            SDL_free((void*)lr->core.optionCategories[i].info);
        }
        SDL_free(lr->core.optionCategories);
        lr->core.optionCategories = NULL;
    }
    lr->core.optionCategoryCount = 0;
    lr->core.optionCategoryCapacity = 0;
}

unsigned SDL_Libretro_GetOptionCount(const SDL_Libretro* lr) {
    return lr ? lr->core.optionCount : 0;
}

const char* SDL_Libretro_GetOptionKey(const SDL_Libretro* lr, unsigned index) {
    if (!lr || index >= lr->core.optionCount) return NULL;
    return lr->core.options[index].key;
}

const char* SDL_Libretro_GetOptionValue(const SDL_Libretro* lr, const char* key) {
    const SDL_LibretroCoreOption* opt = SDL_Libretro_GetOption(lr, key);
    return opt ? opt->value : NULL;
}

bool SDL_Libretro_SetOptionValue(SDL_Libretro* lr, const char* key, const char* value) {
    if (!lr || !key || !value) return false;
    SDL_LibretroCoreOption* opt = (SDL_LibretroCoreOption*)SDL_Libretro_GetOption(lr, key);
    if (!opt) return false;

    // Reject values the core never offered, so GET_VARIABLE only ever hands the
    // core a value it declared. Options with no declared values accept anything.
    if (opt->valuesCount > 0) {
        bool valid = false;
        for (unsigned v = 0; v < opt->valuesCount; v++) {
            if (opt->values[v].value && SDL_strcmp(opt->values[v].value, value) == 0) {
                valid = true;
                break;
            }
        }
        if (!valid) {
            return SDL_SetError("SDL_Libretro: '%s' is not a valid value for option '%s'", value, key);
        }
    }

    SDL_free(opt->value);
    opt->value = SDL_strdup(value);
    lr->core.optionsDirty = true;
    return true;
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

/**
 * Retrieves whether the options have been changed since the last time they were checked.
 */
bool SDL_Libretro_AreOptionsDirty(SDL_Libretro* lr) {
    if (!lr) return false;
    bool dirty = lr->core.optionsDirty;
    lr->core.optionsDirty = false;
    return dirty;
}

bool SDL_Libretro_IsOptionVisible(const SDL_Libretro* lr, const char* key) {
    const SDL_LibretroCoreOption* opt = SDL_Libretro_GetOption(lr, key);
    return opt ? opt->visible : false;
}

const char* SDL_Libretro_GetOptionLabel(const SDL_Libretro* lr, const char* key) {
    const SDL_LibretroCoreOption* opt = SDL_Libretro_GetOption(lr, key);
    return opt ? opt->label : NULL;
}

const char* SDL_Libretro_GetOptionInfo(const SDL_Libretro* lr, const char* key) {
    const SDL_LibretroCoreOption* opt = SDL_Libretro_GetOption(lr, key);
    return opt ? opt->info : NULL;
}

const char* SDL_Libretro_GetOptionCategory(const SDL_Libretro* lr, const char* key) {
    const SDL_LibretroCoreOption* opt = SDL_Libretro_GetOption(lr, key);
    return opt ? opt->categoryKey : NULL;
}

unsigned SDL_Libretro_GetOptionValueCount(const SDL_Libretro* lr, const char* key) {
    const SDL_LibretroCoreOption* opt = SDL_Libretro_GetOption(lr, key);
    return opt ? opt->valuesCount : 0;
}

const char* SDL_Libretro_GetOptionValueByIndex(const SDL_Libretro* lr, const char* key, unsigned index) {
    const SDL_LibretroCoreOption* opt = SDL_Libretro_GetOption(lr, key);
    if (!opt || index >= opt->valuesCount) return NULL;
    return opt->values[index].value;
}

/**
 * The human-readable label for the value at index, falling back to the value
 * string itself when the core provided no label (per libretro convention).
 * NULL if the option or index is invalid.
 */
const char* SDL_Libretro_GetOptionValueLabelByIndex(const SDL_Libretro* lr, const char* key, unsigned index) {
    const SDL_LibretroCoreOption* opt = SDL_Libretro_GetOption(lr, key);
    if (!opt || index >= opt->valuesCount) return NULL;
    return opt->values[index].label ? opt->values[index].label : opt->values[index].value;
}

unsigned SDL_Libretro_GetCategoryCount(const SDL_Libretro* lr) {
    return lr ? lr->core.optionCategoryCount : 0;
}

const char* SDL_Libretro_GetCategoryKey(const SDL_Libretro* lr, unsigned index) {
    if (!lr || index >= lr->core.optionCategoryCount) return NULL;
    return lr->core.optionCategories[index].key;
}

const char* SDL_Libretro_GetCategoryLabel(const SDL_Libretro* lr, const char* key) {
    if (!lr || !key) return NULL;
    for (unsigned i = 0; i < lr->core.optionCategoryCount; i++) {
        if (lr->core.optionCategories[i].key && SDL_strcmp(lr->core.optionCategories[i].key, key) == 0) {
            return lr->core.optionCategories[i].desc;
        }
    }
    return NULL;
}

const char* SDL_Libretro_GetCategoryInfo(const SDL_Libretro* lr, const char* key) {
    if (!lr || !key) return NULL;
    for (unsigned i = 0; i < lr->core.optionCategoryCount; i++) {
        if (lr->core.optionCategories[i].key && SDL_strcmp(lr->core.optionCategories[i].key, key) == 0) {
            return lr->core.optionCategories[i].info;
        }
    }
    return NULL;
}

#endif /* SDL_LIBRETRO_OPTIONS_IMPL_ONCE */
