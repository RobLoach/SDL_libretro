/**
 * SDL_libretro - core options management
 * @file SDL_libretro_options.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_OPTIONS_IMPL_ONCE)
#define SDL_LIBRETRO_OPTIONS_IMPL_ONCE

/* The option's values[] array is sized by our public constant; cores hand us at
 * most RETRO_NUM_CORE_OPTION_VALUES_MAX values, so the two must agree. */
SDL_COMPILE_TIME_ASSERT(option_values_max, SDL_LIBRETRO_OPTION_VALUES_MAX == RETRO_NUM_CORE_OPTION_VALUES_MAX);

static char* SDL_Libretro_Strdup(const char* s) {
    if (!s) return SDL_strdup("");
    return SDL_strdup(s);
}

static void SDL_Libretro_InitCoreOption(SDL_Libretro* lr, const char* key, const char* defaultValue,
    const char* desc, const struct retro_core_option_value* values,
    const char* info, const char* categoryKey) {
    if (!lr || !key) return;

    // Check if already registered
    if (SDL_Libretro_GetOption(lr, key)) {
        return;
    }

    // Grow array if needed
    if (lr->core.optionCount >= lr->core.optionCapacity) {
        unsigned newCap = lr->core.optionCapacity ? lr->core.optionCapacity * 2 : 16;
        SDL_LibretroOption* newOpts = (SDL_LibretroOption*)SDL_realloc(lr->core.options, newCap * sizeof(SDL_LibretroOption));
        if (!newOpts) return;
        lr->core.options = newOpts;
        lr->core.optionCapacity = newCap;
    }

    // Set the option; strings are deep-copied and owned by the context.
    SDL_LibretroOption* slot = &lr->core.options[lr->core.optionCount];
    slot->key = SDL_Libretro_Strdup(key);
    slot->value = SDL_Libretro_Strdup(defaultValue);
    slot->defaultValue = SDL_Libretro_Strdup(defaultValue);
    slot->desc = SDL_Libretro_Strdup(desc);
    slot->info = SDL_Libretro_Strdup(info);
    slot->category = SDL_Libretro_Strdup(categoryKey);
    slot->visible = true;

    // Deep-copy the values; the slot came from SDL_realloc and isn't zeroed, so
    // clear the array first. Copying stops at the { NULL, NULL } terminator.
    SDL_memset(slot->values, 0, sizeof(slot->values));
    slot->valuesCount = 0;
    if (values) {
        for (unsigned v = 0; v < SDL_LIBRETRO_OPTION_VALUES_MAX && values[v].value; v++) {
            slot->values[v].value = SDL_Libretro_Strdup(values[v].value);
            slot->values[v].label = values[v].label ? SDL_Libretro_Strdup(values[v].label) : NULL;
            slot->valuesCount++;
        }
    }

    lr->core.optionCount++;
}

static void SDL_Libretro_InitCoreOptionCategory(SDL_Libretro* lr, const char* key,
    const char* desc, const char* info) {
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
        SDL_LibretroCategory* newCats = (SDL_LibretroCategory*)SDL_realloc(lr->core.optionCategories, newCap * sizeof(SDL_LibretroCategory));
        if (!newCats) return;
        lr->core.optionCategories = newCats;
        lr->core.optionCategoryCapacity = newCap;
    }

    // Set the category; strings are deep-copied and owned by the context.
    SDL_LibretroCategory* cat = &lr->core.optionCategories[lr->core.optionCategoryCount];
    cat->key = SDL_Libretro_Strdup(key);
    cat->desc = SDL_Libretro_Strdup(desc);
    cat->info = SDL_Libretro_Strdup(info);

    lr->core.optionCategoryCount++;
}

static void SDL_Libretro_FreeCoreOptions(SDL_Libretro* lr) {
    if (!lr) return;

    // Options
    if (lr->core.options) {
        for (unsigned i = 0; i < lr->core.optionCount; i++) {
            SDL_LibretroOption* opt = &lr->core.options[i];
            SDL_free((void*)opt->key);
            SDL_free((void*)opt->value);
            SDL_free((void*)opt->defaultValue);
            SDL_free((void*)opt->desc);
            SDL_free((void*)opt->info);
            SDL_free((void*)opt->category);
            for (unsigned v = 0; v < SDL_LIBRETRO_OPTION_VALUES_MAX && opt->values[v].value; v++) {
                SDL_free((void*)opt->values[v].value);
                SDL_free((void*)opt->values[v].label);
            }
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

/**
 * Get the number of core options that have been set.
 */
unsigned SDL_Libretro_GetOptionCount(const SDL_Libretro* lr) {
    return lr ? lr->core.optionCount : 0;
}

/**
 * Retrieve details about a given core option.
 */
const SDL_LibretroOption* SDL_Libretro_GetOption(const SDL_Libretro* lr, const char* key) {
    if (!lr || !key) return NULL;
    for (unsigned i = 0; i < lr->core.optionCount; i++) {
        if (lr->core.options[i].key && SDL_strcmp(lr->core.options[i].key, key) == 0) {
            return &lr->core.options[i];
        }
    }
    return NULL;
}

/**
 * Retrieve a core option by its registration index, or NULL if out of range.
 */
const SDL_LibretroOption* SDL_Libretro_GetOptionByIndex(const SDL_Libretro* lr, unsigned index) {
    if (!lr || index >= lr->core.optionCount) return NULL;
    return &lr->core.options[index];
}

/**
 * Set a core option's value; fails if the value isn't one the core declared.
 */
bool SDL_Libretro_SetOptionValue(SDL_Libretro* lr, const char* key, const char* value) {
    if (!lr || !key || !value) return false;
    SDL_LibretroOption* opt = (SDL_LibretroOption*)SDL_Libretro_GetOption(lr, key);
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
            return SDL_SetError("[SDL_Libretro] '%s' is not a valid value for option '%s'", value, key);
        }
    }

    SDL_free((void*)opt->value);
    opt->value = SDL_strdup(value);
    lr->core.optionsDirty = true;
    return true;
}

/**
 * Get a core option's current value, or NULL if there's no such option.
 */
const char* SDL_Libretro_GetOptionValue(SDL_Libretro* lr, const char* key) {
    const SDL_LibretroOption* opt = SDL_Libretro_GetOption(lr, key);
    if (!opt) return NULL;
    return opt->value;
}

/**
 * Reset a core option to its default value.
 */
bool SDL_Libretro_ResetOption(SDL_Libretro* lr, const char* key) {
    SDL_LibretroOption* opt = (SDL_LibretroOption*)SDL_Libretro_GetOption(lr, key);
    if (!opt) return false;
    SDL_free((void*)opt->value);
    opt->value = SDL_strdup(opt->defaultValue);
    lr->core.optionsDirty = true;
    return true;
}

/**
 * Reset every core option to its default value.
 */
void SDL_Libretro_ResetAllOptions(SDL_Libretro* lr) {
    if (!lr) return;
    for (unsigned i = 0; i < lr->core.optionCount; i++) {
        SDL_LibretroOption* opt = &lr->core.options[i];
        SDL_free((void*)opt->value);
        opt->value = SDL_strdup(opt->defaultValue);
    }
    lr->core.optionsDirty = true;
}

/**
 *
 * Retrieves whether the options have been changed since the last time they were checked.
 */
bool SDL_Libretro_AreOptionsDirty(SDL_Libretro* lr) {
    if (!lr) return false;
    bool dirty = lr->core.optionsDirty;
    lr->core.optionsDirty = false;
    return dirty;
}

bool SDL_Libretro_UpdateOptionVisibility(SDL_Libretro* lr) {
    if (!lr || !lr->core.optionsUpdateDisplayCallback) return false;
    return lr->core.optionsUpdateDisplayCallback();
}

/**
 * Get the number of option categories registered by the core.
 */
unsigned SDL_Libretro_GetCategoryCount(const SDL_Libretro* lr) {
    return lr ? lr->core.optionCategoryCount : 0;
}

/**
 * Retrieve an option category by key, or NULL if there's no such category.
 */
const SDL_LibretroCategory* SDL_Libretro_GetCategory(const SDL_Libretro* lr, const char* key) {
    if (!lr || !key) return NULL;
    for (unsigned i = 0; i < lr->core.optionCategoryCount; i++) {
        if (lr->core.optionCategories[i].key && SDL_strcmp(lr->core.optionCategories[i].key, key) == 0) {
            return &lr->core.optionCategories[i];
        }
    }
    return NULL;
}

/**
 * Retrieve an option category by index, or NULL if out of range.
 */
const SDL_LibretroCategory* SDL_Libretro_GetCategoryByIndex(const SDL_Libretro* lr, unsigned index) {
    if (!lr || index >= lr->core.optionCategoryCount) return NULL;
    return &lr->core.optionCategories[index];
}

#endif /* SDL_LIBRETRO_OPTIONS_IMPL_ONCE */
