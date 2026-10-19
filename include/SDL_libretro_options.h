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
 * Whether the value is one the option declared. Options with no declared values accept anything.
 */
static bool SDL_Libretro_IsOptionValueValid(const SDL_LibretroOption* opt, const char* value) {
    if (!opt || !value) return false;
    if (opt->valuesCount == 0) return true;
    for (unsigned v = 0; v < opt->valuesCount; v++) {
        if (opt->values[v].value && SDL_strcmp(opt->values[v].value, value) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * Free an option's declared values.
 */
static void SDL_Libretro_FreeOptionValues(SDL_LibretroOptionValue* values, unsigned count) {
    if (!values) return;
    for (unsigned v = 0; v < count; v++) {
        SDL_free((void*)values[v].value);
        SDL_free((void*)values[v].label);
    }
    SDL_free(values);
}

/**
 * Deep-copy the core's NULL-terminated value list into an owned array.
 *
 * @returns false when out of memory.
 */
static bool SDL_Libretro_CopyOptionValues(const struct retro_core_option_value* values, SDL_LibretroOptionValue** outValues, unsigned* outCount) {
    *outValues = NULL;
    *outCount = 0;
    if (!values) return true;

    // Count how many values there are available for the option.
    unsigned count = 0;
    while (values[count].value) count++;
    if (count == 0) return true;

    SDL_LibretroOptionValue* copy = (SDL_LibretroOptionValue*)SDL_calloc(count, sizeof(SDL_LibretroOptionValue));
    if (!copy) return false;
    for (unsigned v = 0; v < count; v++) {
        copy[v].value = SDL_strdup(values[v].value);
        copy[v].label = values[v].label ? SDL_strdup(values[v].label) : NULL;
        if (!copy[v].value || (values[v].label && !copy[v].label)) {
            SDL_Libretro_FreeOptionValues(copy, v + 1);
            return false;
        }
    }
    *outValues = copy;
    *outCount = count;
    return true;
}

static void SDL_Libretro_InitCoreOption(SDL_Libretro* lr, const char* key, const char* defaultValue, const char* desc, const struct retro_core_option_value* values, const char* info, const char* categoryKey) {
    if (!lr || !key) return;

    SDL_LibretroOption* slot = (SDL_LibretroOption*)SDL_Libretro_GetOption(lr, key);
    bool reregistered = slot != NULL;

    // Grow array if needed
    if (!reregistered && lr->core.optionCount >= lr->core.optionCapacity) {
        unsigned newCap = lr->core.optionCapacity ? lr->core.optionCapacity * 2 : 16;
        SDL_LibretroOption* newOpts = (SDL_LibretroOption*)SDL_realloc(lr->core.options, newCap * sizeof(SDL_LibretroOption));
        if (!newOpts) return;
        lr->core.options = newOpts;
        lr->core.optionCapacity = newCap;
    }

    // Copy everything up front, so running out of memory leaves any existing
    // option untouched. Strings are deep-copied and owned by the context.
    SDL_LibretroOption copy = {0};
    copy.key = reregistered ? NULL : SDL_Libretro_Strdup(key);
    copy.defaultValue = SDL_Libretro_Strdup(defaultValue);
    copy.desc = SDL_Libretro_Strdup(desc);
    copy.info = SDL_Libretro_Strdup(info);
    copy.category = SDL_Libretro_Strdup(categoryKey);
    bool ok = (reregistered || copy.key) && copy.defaultValue && copy.desc && copy.info && copy.category;
    ok = ok && SDL_Libretro_CopyOptionValues(values, &copy.values, &copy.valuesCount);
    copy.valuesCapacity = copy.valuesCount;

    // Pick the value: a re-registered option keeps its current value while
    // it's still valid. Otherwise use the saved value, since cores may register
    // options after the saved core config was loaded (in retro_init() or
    // retro_load_game()), falling back to the default.
    const char* current = reregistered ? slot->value : NULL;
    bool keepCurrent = current && SDL_Libretro_IsOptionValueValid(&copy, current);
    if (ok && !keepCurrent) {
        const char* saved = SDL_Libretro_GetSavedCoreOption(lr, key);
        copy.value = SDL_strdup((saved && SDL_Libretro_IsOptionValueValid(&copy, saved)) ? saved : copy.defaultValue);
        ok = copy.value != NULL;
    }

    if (!ok) {
        SDL_free((void*)copy.key);
        SDL_free((void*)copy.value);
        SDL_free((void*)copy.defaultValue);
        SDL_free((void*)copy.desc);
        SDL_free((void*)copy.info);
        SDL_free((void*)copy.category);
        SDL_Libretro_FreeOptionValues(copy.values, copy.valuesCount);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] Out of memory registering option '%s'", key);
        return;
    }

    if (!reregistered) {
        // The core hasn't read a new option yet, so it isn't flagged as updated.
        copy.visible = true;
        lr->core.options[lr->core.optionCount++] = copy;
        return;
    }

    // Re-registration: the core may have changed the values, default, or text.
    SDL_free((void*)slot->defaultValue);
    SDL_free((void*)slot->desc);
    SDL_free((void*)slot->info);
    SDL_free((void*)slot->category);
    SDL_Libretro_FreeOptionValues(slot->values, slot->valuesCount);
    slot->defaultValue = copy.defaultValue;
    slot->desc = copy.desc;
    slot->info = copy.info;
    slot->category = copy.category;
    slot->values = copy.values;
    slot->valuesCount = copy.valuesCount;
    slot->valuesCapacity = copy.valuesCapacity;
    if (!keepCurrent) {
        SDL_free((void*)slot->value);
        slot->value = copy.value;
        lr->core.optionsDirtyCore = true;
    }
    lr->core.optionsDirtyApp = true;
}

static void SDL_Libretro_InitCoreOptionCategory(SDL_Libretro* lr, const char* key, const char* desc, const char* info) {
    if (!lr || !key || key[0] == '\0') return;

    // Check if already registered
    for (unsigned i = 0; i < lr->core.optionCategoriesCount; i++) {
        if (lr->core.optionCategories[i].key && SDL_strcmp(lr->core.optionCategories[i].key, key) == 0) {
            return;
        }
    }

    // Grow array if needed
    if (lr->core.optionCategoriesCount >= lr->core.optionCategoriesCapacity) {
        unsigned newCap = lr->core.optionCategoriesCapacity ? lr->core.optionCategoriesCapacity * 2 : 8;
        SDL_LibretroCategory* newCats = (SDL_LibretroCategory*)SDL_realloc(lr->core.optionCategories, newCap * sizeof(SDL_LibretroCategory));
        if (!newCats) return;
        lr->core.optionCategories = newCats;
        lr->core.optionCategoriesCapacity = newCap;
    }

    // Set the category; strings are deep-copied and owned by the context.
    SDL_LibretroCategory* cat = &lr->core.optionCategories[lr->core.optionCategoriesCount];
    cat->key = SDL_Libretro_Strdup(key);
    cat->desc = SDL_Libretro_Strdup(desc);
    cat->info = SDL_Libretro_Strdup(info);
    if (!cat->key || !cat->desc || !cat->info) {
        SDL_free((void*)cat->key);
        SDL_free((void*)cat->desc);
        SDL_free((void*)cat->info);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] Out of memory registering option category '%s'", key);
        return;
    }

    lr->core.optionCategoriesCount++;
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
            SDL_Libretro_FreeOptionValues(opt->values, opt->valuesCount);
        }
        SDL_free(lr->core.options);
        lr->core.options = NULL;
    }
    lr->core.optionCount = 0;
    lr->core.optionCapacity = 0;

    // Categories
    if (lr->core.optionCategories) {
        for (unsigned i = 0; i < lr->core.optionCategoriesCount; i++) {
            SDL_free((void*)lr->core.optionCategories[i].key);
            SDL_free((void*)lr->core.optionCategories[i].desc);
            SDL_free((void*)lr->core.optionCategories[i].info);
        }
        SDL_free(lr->core.optionCategories);
        lr->core.optionCategories = NULL;
    }
    lr->core.optionCategoriesCount = 0;
    lr->core.optionCategoriesCapacity = 0;
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
    if (!lr || !key || !value) {
        SDL_SetError("[SDL_Libretro] Invalid SetOptionValue arguments");
        return false;
    }
    SDL_LibretroOption* opt = (SDL_LibretroOption*)SDL_Libretro_GetOption(lr, key);
    if (!opt) {
        return SDL_SetError("[SDL_Libretro] No such option '%s'", key);
    }

    // Reject values the core never offered, so GET_VARIABLE only ever hands the
    // core a value it declared. Options with no declared values accept anything.
    if (!SDL_Libretro_IsOptionValueValid(opt, value)) {
        return SDL_SetError("[SDL_Libretro] '%s' is not a valid value for option '%s'", value, key);
    }

    // Allocate before freeing so a failed allocation never leaves opt->value
    // NULL (which GET_VARIABLE would then hand to the core).
    char* dup = SDL_strdup(value);
    if (!dup) {
        return SDL_SetError("[SDL_Libretro] Out of memory setting option '%s'", key);
    }
    SDL_free((void*)opt->value);
    opt->value = dup;
    lr->core.optionsDirtyCore = true;
    lr->core.optionsDirtyApp = true;
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
 * Get the human-readable label for a core option's current value.
 *
 * Falls back to the raw value when the core supplied no label, like in
 * core option variables API version 0 or 1.
 */
const char* SDL_Libretro_GetOptionValueLabel(SDL_Libretro* lr, const char* key) {
    const SDL_LibretroOption* opt = SDL_Libretro_GetOption(lr, key);
    if (!opt || !opt->value) return NULL;
    for (unsigned v = 0; v < opt->valuesCount; v++) {
        if (opt->values[v].value && SDL_strcmp(opt->values[v].value, opt->value) == 0) {
            return opt->values[v].label ? opt->values[v].label : opt->values[v].value;
        }
    }
    return opt->value;
}

/**
 * Advance a core option to the next (+1) or previous (-1) value.
 */
bool SDL_Libretro_CycleOptionValue(SDL_Libretro* lr, const char* key, int direction) {
    if (direction == 0) {
        return SDL_InvalidParamError("direction");
    }
    SDL_LibretroOption* opt = (SDL_LibretroOption*)SDL_Libretro_GetOption(lr, key);
    if (!opt) {
        return SDL_SetError("[SDL_Libretro] No such option '%s'", key);
    }
    if (opt->valuesCount == 0) {
        return SDL_SetError("[SDL_Libretro] Option '%s' has no declared values to cycle", key);
    }

    // Find the current value's index; default to 0 if it isn't in the list.
    unsigned current = 0;
    for (unsigned v = 0; v < opt->valuesCount; v++) {
        if (opt->values[v].value && opt->value && SDL_strcmp(opt->values[v].value, opt->value) == 0) {
            current = v;
            break;
        }
    }

    unsigned count = opt->valuesCount;
    unsigned step = (direction > 0) ? 1 : count - 1;
    unsigned next = (current + step) % count;
    return SDL_Libretro_SetOptionValue(lr, key, opt->values[next].value);
}

/**
 * Reset a core option to its default value.
 *
 * @param lr the libretro context.
 * @param key the key to revert to the default value.
 */
bool SDL_Libretro_ResetOption(SDL_Libretro* lr, const char* key) {
    SDL_LibretroOption* opt = (SDL_LibretroOption*)SDL_Libretro_GetOption(lr, key);
    if (!opt) {
        return SDL_InvalidParamError("opt");
    }
    char* dupicateValue = SDL_Libretro_Strdup(opt->defaultValue);
    if (!dupicateValue) {
        return SDL_SetError("[SDL_Libretro] Out of memory resetting option '%s'", key);
    }
    SDL_free((void*)opt->value);
    opt->value = dupicateValue;
    lr->core.optionsDirtyCore = true;
    lr->core.optionsDirtyApp = true;
    return true;
}

/**
 * Reset every core option to its default value.
 */
void SDL_Libretro_ResetAllOptions(SDL_Libretro* lr) {
    if (!lr) return;
    for (unsigned i = 0; i < lr->core.optionCount; i++) {
        SDL_LibretroOption* opt = &lr->core.options[i];
        char* dupicateValue = SDL_Libretro_Strdup(opt->defaultValue);
        if (!dupicateValue) continue; // keep the previous value rather than nulling it
        SDL_free((void*)opt->value);
        opt->value = dupicateValue;
    }
    lr->core.optionsDirtyCore = true;
    lr->core.optionsDirtyApp = true;
}

/**
 * Retrieves whether the options have been changed since the last time they were checked.
 */
bool SDL_Libretro_AreOptionsDirty(SDL_Libretro* lr) {
    if (!lr) return false;
    bool dirty = lr->core.optionsDirtyApp;
    lr->core.optionsDirtyApp = false;
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
    return lr ? lr->core.optionCategoriesCount : 0;
}

/**
 * Retrieve an option category by key, or NULL if there's no such category.
 */
const SDL_LibretroCategory* SDL_Libretro_GetCategory(const SDL_Libretro* lr, const char* key) {
    if (!lr || !key) return NULL;
    for (unsigned i = 0; i < lr->core.optionCategoriesCount; i++) {
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
    if (!lr || index >= lr->core.optionCategoriesCount) return NULL;
    return &lr->core.optionCategories[index];
}

#endif /* SDL_LIBRETRO_OPTIONS_IMPL_ONCE */
