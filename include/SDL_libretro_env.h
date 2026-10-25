/**
 * SDL_libretro environment callback dispatch
 *
 * @file SDL_libretro_env.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_ENV_IMPL_ONCE)
#define SDL_LIBRETRO_ENV_IMPL_ONCE

#include <stdarg.h>

/**
 * @internal
 * Callback for when the libretro core writes to the log.
 */
static void SDL_Libretro_Logger(enum retro_log_level level, const char* fmt, ...) {
    if (SDL_Libretro_active && level < SDL_Libretro_active->logLevel) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    char buf[2048];
    SDL_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* Strip trailing newline */
    size_t len = SDL_strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';

    switch (level) {
        case RETRO_LOG_INFO: SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] %s", buf); break;
        case RETRO_LOG_WARN: SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] %s", buf); break;
        case RETRO_LOG_ERROR: SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] %s", buf); break;
        case RETRO_LOG_DEBUG:
        default: SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] %s", buf); break;
    }
}

static retro_time_t SDL_Libretro_GetTimeUSEC(void) {
    // SDL_GetTicksNS() avoids the overflow of scaling the raw performance counter by 1000000.
    return (retro_time_t)(SDL_GetTicksNS() / SDL_NS_PER_US);
}

static uint64_t SDL_Libretro_GetCPUFeatures(void) {
    uint64_t features = 0;
    if (SDL_HasSSE()) features |= RETRO_SIMD_SSE;
    if (SDL_HasSSE2()) features |= RETRO_SIMD_SSE2;
    if (SDL_HasSSE3()) features |= RETRO_SIMD_SSE3;
    if (SDL_HasSSE41()) features |= RETRO_SIMD_SSE4;
    if (SDL_HasSSE42()) features |= RETRO_SIMD_SSE42;
    if (SDL_HasAVX()) features |= RETRO_SIMD_AVX;
    if (SDL_HasAVX2()) features |= RETRO_SIMD_AVX2;
    if (SDL_HasAVX512F()) features |= RETRO_SIMD_AVX512;
    if (SDL_HasMMX()) features |= RETRO_SIMD_MMX;
    if (SDL_HasAltiVec()) features |= RETRO_SIMD_VMX;
    if (SDL_HasNEON()) {
        features |= RETRO_SIMD_NEON;
#if defined(__aarch64__) || defined(_M_ARM64)
        // AArch64's Advanced SIMD is NEON; report both, as RetroArch does.
        features |= RETRO_SIMD_ASIMD;
#endif
    }
    return features;
}

static retro_perf_tick_t SDL_Libretro_GetPerfCounter(void) {
    return (retro_perf_tick_t)SDL_GetPerformanceCounter();
}

static void SDL_Libretro_PerfRegister(struct retro_perf_counter* counter) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr || !counter) return;

    // Avoid registering the same counter twice.
    for (unsigned i = 0; i < lr->core.perfCounterCount; i++) {
        if (lr->core.perfCounters[i] == counter) {
            counter->registered = true;
            return;
        }
    }

    if (lr->core.perfCounterCount >= SDL_arraysize(lr->core.perfCounters)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "PERF: counter registry full (%u), dropping '%s'", lr->core.perfCounterCount, counter->ident ? counter->ident : "?");
        return;
    }

    lr->core.perfCounters[lr->core.perfCounterCount++] = counter;
    counter->registered = true;
}

static void SDL_Libretro_PerfStart(struct retro_perf_counter* counter) {
    if (!counter) return;
    counter->call_cnt++;
    counter->start = (retro_perf_tick_t)SDL_GetPerformanceCounter();
}

static void SDL_Libretro_PerfStop(struct retro_perf_counter* counter) {
    if (counter) counter->total += (retro_perf_tick_t)SDL_GetPerformanceCounter() - counter->start;
}

/**
 * @internal
 * Displays the active performance timers in the log.
 */
static void SDL_Libretro_PerfLog(void) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr) return;

    // SDL_GetPerformanceCounter() is the tick source used by the perf counters, so its frequency converts accumulated ticks to seconds. */
    Uint64 freq = SDL_GetPerformanceFrequency();

    for (unsigned i = 0; i < lr->core.perfCounterCount; i++) {
        const struct retro_perf_counter* counter = lr->core.perfCounters[i];
        if (!counter) continue;

        double ms = freq ? ((double)counter->total / (double)freq) * 1000.0 : 0.0;
        SDL_Log("[PERF] %s: %llu calls, %llu ticks (%.4f ms)", counter->ident ? counter->ident : "?", (unsigned long long)counter->call_cnt, (unsigned long long)counter->total, ms);
    }
}

/**
 * @internal
 * Push a port's rumble state to its gamepad.
 *
 * Libretro rumble persists until the core sets it to 0, while SDL rumble expires, so this is re-applied every frame from SDL_Libretro_Update(). The duration only bounds how long it keeps going if the frontend stops updating.
 */
static void SDL_Libretro_ApplyRumble(SDL_Libretro* lr, unsigned port) {
    if (!lr || port >= SDL_LIBRETRO_MAX_USERS || !lr->gamepads[port]) return;
    // SDL's low-frequency motor is the large, strong one.
    Uint16 lo = (Uint16)(lr->core.rumbleStrong[port] * 65535.0f);
    Uint16 hi = (Uint16)(lr->core.rumbleWeak[port] * 65535.0f);
    SDL_RumbleGamepad(lr->gamepads[port], lo, hi, (lo || hi) ? 1000 : 0);
}

/**
 * @internal
 * Stop rumble on every port and forget the core's rumble state.
 */
static void SDL_Libretro_StopRumble(SDL_Libretro* lr) {
    if (!lr) return;
    for (unsigned port = 0; port < SDL_LIBRETRO_MAX_USERS; port++) {
        bool active = lr->core.rumbleStrong[port] > 0.0f || lr->core.rumbleWeak[port] > 0.0f;
        lr->core.rumbleStrong[port] = 0.0f;
        lr->core.rumbleWeak[port] = 0.0f;
        if (active) SDL_Libretro_ApplyRumble(lr, port);
    }
}

static bool SDL_Libretro_SetRumbleState(unsigned port, enum retro_rumble_effect effect, uint16_t strength) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr || port >= SDL_LIBRETRO_MAX_USERS) return false;

    float normalized = (float)strength / 65535.0f;
    if (effect == RETRO_RUMBLE_STRONG) {
        lr->core.rumbleStrong[port] = normalized;
    }
    else {
        lr->core.rumbleWeak[port] = normalized;
    }

    SDL_Libretro_ApplyRumble(lr, port);

    return true;
}

static void SDL_Libretro_SetLEDState(int led, int state) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr) return;
    Uint8 v = state ? 255 : 0;
    if (led >= 0 && led < SDL_LIBRETRO_MAX_USERS) {
        if (lr->gamepads[led]) SDL_SetGamepadLED(lr->gamepads[led], v, v, v);
    }
    else {
        for (unsigned i = 0; i < SDL_LIBRETRO_MAX_USERS; i++) {
            if (lr->gamepads[i]) SDL_SetGamepadLED(lr->gamepads[i], v, v, v);
        }
    }
}

static const char* SDL_Libretro_GetDirectory(SDL_Libretro* lr, unsigned cmd) {
    switch (cmd) {
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
            return lr->systemDirectory[0] ? lr->systemDirectory : NULL;
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
            return lr->saveDirectory[0] ? lr->saveDirectory : NULL;
        case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY:
            return lr->coreAssetsDirectory[0] ? lr->coreAssetsDirectory : NULL;
        case RETRO_ENVIRONMENT_GET_PLAYLIST_DIRECTORY:
            return lr->playlistDirectory[0] ? lr->playlistDirectory : NULL;
        case RETRO_ENVIRONMENT_GET_FILE_BROWSER_START_DIRECTORY:
            return lr->fileBrowserStartDirectory[0] ? lr->fileBrowserStartDirectory : NULL;
        default:
            return NULL;
    }
}

/**
 * @internal
 * The refresh rate the frontend is presenting at, in Hz. Taken from the
 * current display mode of the core's window, defaulting to 60 when unknown.
 */
static float SDL_Libretro_GetTargetRefreshRate(SDL_Libretro* lr) {
    float rate = 60.0f;
    if (lr->window) {
        SDL_DisplayID display = SDL_GetDisplayForWindow(lr->window);
        if (display) {
            const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
            if (mode && mode->refresh_rate > 0.0f) {
                rate = mode->refresh_rate;
            }
        }
    }
    return rate;
}

/**
 * @internal
 * Picks the text a categorizing frontend should show. libretro provides a
 * `*_categorized` variant for use when an option sits inside a category; it falls
 * back to the base text when there's no category or the variant is empty.
 */
static const char* SDL_Libretro_PickCoreOptionText(const char* base, const char* categorized, const char* categoryKey) {
    if (categoryKey && categoryKey[0] && categorized && categorized[0]) {
        return categorized;
    }
    return base;
}

/**
 * @internal
 * Map an ISO 639 language code and ISO 3166 country code (as found in SDL_Locale) to a RETRO_LANGUAGE_* value.
 *
 * @param language The language code, such as "pt". NULL or unknown gives RETRO_LANGUAGE_ENGLISH.
 * @param country The country code, such as "BR". May be NULL.
 */
static int SDL_Libretro_LanguageFromLocale(const char* language, const char* country) {
    if (!language || !language[0]) return RETRO_LANGUAGE_ENGLISH;
    if (!country) country = "";

    // Catalan's Valencian variant isn't a country, but some platforms tack it on as "ca-ES-valencia" or "ca_ES@valencia".
    if (SDL_strcasecmp(language, "ca") == 0) {
        bool valencia = SDL_strcasestr(country, "valencia") != NULL || SDL_strcasestr(language, "valencia") != NULL;
        return valencia ? RETRO_LANGUAGE_CATALAN_VALENCIA : RETRO_LANGUAGE_CATALAN;
    }
    if (SDL_strcasecmp(language, "en") == 0) {
        return SDL_strcasecmp(country, "GB") == 0 ? RETRO_LANGUAGE_BRITISH_ENGLISH : RETRO_LANGUAGE_ENGLISH;
    }
    if (SDL_strcasecmp(language, "pt") == 0) {
        return SDL_strcasecmp(country, "PT") == 0 ? RETRO_LANGUAGE_PORTUGUESE_PORTUGAL : RETRO_LANGUAGE_PORTUGUESE_BRAZIL;
    }
    if (SDL_strcasecmp(language, "zh") == 0) {
        bool traditional = SDL_strcasecmp(country, "TW") == 0 || SDL_strcasecmp(country, "HK") == 0 || SDL_strcasecmp(country, "MO") == 0;
        return traditional ? RETRO_LANGUAGE_CHINESE_TRADITIONAL : RETRO_LANGUAGE_CHINESE_SIMPLIFIED;
    }

    static const struct {
        const char* code;
        int language;
    } languages[] = {
        {"ja", RETRO_LANGUAGE_JAPANESE},
        {"fr", RETRO_LANGUAGE_FRENCH},
        {"es", RETRO_LANGUAGE_SPANISH},
        {"de", RETRO_LANGUAGE_GERMAN},
        {"it", RETRO_LANGUAGE_ITALIAN},
        {"nl", RETRO_LANGUAGE_DUTCH},
        {"ru", RETRO_LANGUAGE_RUSSIAN},
        {"ko", RETRO_LANGUAGE_KOREAN},
        {"eo", RETRO_LANGUAGE_ESPERANTO},
        {"pl", RETRO_LANGUAGE_POLISH},
        {"vi", RETRO_LANGUAGE_VIETNAMESE},
        {"ar", RETRO_LANGUAGE_ARABIC},
        {"el", RETRO_LANGUAGE_GREEK},
        {"tr", RETRO_LANGUAGE_TURKISH},
        {"sk", RETRO_LANGUAGE_SLOVAK},
        {"fa", RETRO_LANGUAGE_PERSIAN},
        {"he", RETRO_LANGUAGE_HEBREW},
        {"iw", RETRO_LANGUAGE_HEBREW}, // Legacy code for Hebrew
        {"ast", RETRO_LANGUAGE_ASTURIAN},
        {"fi", RETRO_LANGUAGE_FINNISH},
        {"id", RETRO_LANGUAGE_INDONESIAN},
        {"in", RETRO_LANGUAGE_INDONESIAN}, // Legacy code for Indonesian
        {"sv", RETRO_LANGUAGE_SWEDISH},
        {"uk", RETRO_LANGUAGE_UKRAINIAN},
        {"cs", RETRO_LANGUAGE_CZECH},
        {"hu", RETRO_LANGUAGE_HUNGARIAN},
        {"be", RETRO_LANGUAGE_BELARUSIAN},
        {"gl", RETRO_LANGUAGE_GALICIAN},
        {"no", RETRO_LANGUAGE_NORWEGIAN},
        {"nb", RETRO_LANGUAGE_NORWEGIAN},
        {"nn", RETRO_LANGUAGE_NORWEGIAN},
        {"ga", RETRO_LANGUAGE_IRISH},
        {"th", RETRO_LANGUAGE_THAI},
    };
    for (size_t i = 0; i < SDL_arraysize(languages); i++) {
        if (SDL_strcasecmp(language, languages[i].code) == 0) {
            return languages[i].language;
        }
    }
    return RETRO_LANGUAGE_ENGLISH;
}

/**
 * @internal
 * Detect the RETRO_LANGUAGE_* value from the user's most preferred locale.
 */
static int SDL_Libretro_DetectLanguage(void) {
    int count = 0;
    SDL_Locale** locales = SDL_GetPreferredLocales(&count);
    int language = RETRO_LANGUAGE_ENGLISH;
    if (locales && count > 0 && locales[0]) {
        language = SDL_Libretro_LanguageFromLocale(locales[0]->language, locales[0]->country);
    }
    SDL_free(locales);
    return language;
}

void SDL_Libretro_SetLanguage(SDL_Libretro* lr, int language) {
    if (!lr) return;
    lr->language = (language >= 0 && language < RETRO_LANGUAGE_LAST) ? language : -1;
}

int SDL_Libretro_GetLanguage(const SDL_Libretro* lr) {
    if (!lr) return RETRO_LANGUAGE_ENGLISH;
    return lr->language >= 0 ? lr->language : SDL_Libretro_DetectLanguage();
}

/**
 * @internal
 * Pick the translated text when the core provides it, otherwise the US English text.
 */
static const char* SDL_Libretro_PickLocalText(const char* us, const char* local) {
    return (local && local[0]) ? local : us;
}

/**
 * @internal
 * Convert a NULL-key-terminated v1 option definition list into a newly allocated v2 list (also NULL-key-terminated). Strings still point into the core's data.
 */
static struct retro_core_option_v2_definition* SDL_Libretro_CoreOptionsV1ToV2(const struct retro_core_option_definition* defs) {
    unsigned count = 0;
    while (defs[count].key) count++;
    struct retro_core_option_v2_definition* v2defs =
        (struct retro_core_option_v2_definition*)SDL_calloc(count + 1, sizeof(*v2defs));
    if (!v2defs) return NULL;
    for (unsigned i = 0; i < count; i++) {
        v2defs[i].key = defs[i].key;
        v2defs[i].desc = defs[i].desc;
        v2defs[i].info = defs[i].info;
        v2defs[i].default_value = defs[i].default_value;
        SDL_memcpy(v2defs[i].values, defs[i].values, sizeof(defs[i].values));
    }
    return v2defs;
}

static bool SDL_Libretro_EnvironmentCallback(unsigned cmd, void* data) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr) return false;

    switch (cmd) {
        case RETRO_ENVIRONMENT_SET_ROTATION: {
            if (!data) return false;
            // Restrict rotation to 0-3
            lr->core.rotation = (int)(*(const unsigned*)data & 3);
            SDL_Log("[SDL_Libretro] SET_ROTATION: %d (%d deg)", lr->core.rotation, lr->core.rotation * 90);
            return true;
        }

        case RETRO_ENVIRONMENT_GET_OVERSCAN: {
            return false;
        }

        case RETRO_ENVIRONMENT_GET_CAN_DUPE: {
            if (!data) return false;
            *(bool*)data = true;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_MESSAGE: {
            const struct retro_message* msg = (const struct retro_message*)data;
            if (!msg || !msg->msg || msg->msg[0] == '\0') return msg != NULL;
            double seconds = msg->frames / (lr->core.fps > 0 ? lr->core.fps : 60.0);
            SDL_Libretro_SetMessage(lr, msg->msg, seconds);
            return true;
        }

        case RETRO_ENVIRONMENT_SHUTDOWN: {
            SDL_Log("[SDL_Libretro] Shutdown requested");
            lr->core.shutdown = true;
            SDL_Libretro_PushEvent(lr, SDL_EVENT_LIBRETRO_SHUTDOWN, NULL);
            return true;
        }

        case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL: {
            if (!data) return false;
            lr->core.performanceLevel = *(const unsigned*)data;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY:
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        case RETRO_ENVIRONMENT_GET_PLAYLIST_DIRECTORY:
        case RETRO_ENVIRONMENT_GET_FILE_BROWSER_START_DIRECTORY: {
            // If data is NULL, it's checking if the environment callback is available.
            if (!data) return true;
            *(const char**)data = SDL_Libretro_GetDirectory(lr, cmd);
            return true;
        }

        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
            if (!data) return false;
            enum retro_pixel_format fmt = *(const enum retro_pixel_format*)data;
            switch (fmt) {
                case RETRO_PIXEL_FORMAT_0RGB1555:
                case RETRO_PIXEL_FORMAT_XRGB8888:
                case RETRO_PIXEL_FORMAT_RGB565: {
                    bool changed = (fmt != lr->core.pixelFormat);
                    lr->core.pixelFormat = fmt;
                    // The texture's format is fixed at creation. The load path builds it from this field, a runtime change (texture already exists) defers a rebuild to the next RunFrame.
                    if (changed && lr->core.texture) {
                        lr->core.videoReinitPending = true;
                    }
                    return true;
                }
                default:
                    // Unsupported. Signal false without clobbering the current format.
                    return false;
            }
        }

        case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS: {
            const struct retro_input_descriptor* desc = (const struct retro_input_descriptor*)data;
            if (!desc) return true;

            // Clear out the existing input descriptors first
            SDL_Libretro_FreeInputDescriptors(lr);

            // Count how many we are adding.
            unsigned count = 0;
            for (const struct retro_input_descriptor* d = desc; d->description; d++) count++;
            if (count == 0) {
                return true;
            }

            // Copy the input descriptors into our structure
            lr->core.inputDescriptors = (struct retro_input_descriptor*)SDL_malloc(count * sizeof(*desc));
            if (!lr->core.inputDescriptors) {
                return SDL_SetError("[SDL_Libretro] Out of memory copying input descriptors");
            }
            SDL_memcpy(lr->core.inputDescriptors, desc, count * sizeof(*desc));
            lr->core.inputDescriptorsCount = count;
            // Deep copy the description strings. SDL_strdup returns NULL on failure, which FreeInputDescriptors tolerates.
            for (unsigned i = 0; i < count; i++) {
                lr->core.inputDescriptors[i].description = SDL_strdup(desc[i].description);
            }

            return true;
        }

        case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK: {
            if (!data) return true;
            const struct retro_keyboard_callback* cb = (const struct retro_keyboard_callback*)data;
            lr->core.keyboard_event = cb->callback;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE: {
            const struct retro_disk_control_callback* cb = (const struct retro_disk_control_callback*)data;
            if (!cb) return true;
            SDL_memset(&lr->core.disk_control, 0, sizeof(lr->core.disk_control));
            lr->core.disk_control.set_eject_state = cb->set_eject_state;
            lr->core.disk_control.get_eject_state = cb->get_eject_state;
            lr->core.disk_control.get_image_index = cb->get_image_index;
            lr->core.disk_control.set_image_index = cb->set_image_index;
            lr->core.disk_control.get_num_images = cb->get_num_images;
            lr->core.disk_control.replace_image_index = cb->replace_image_index;
            lr->core.disk_control.add_image_index = cb->add_image_index;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_HW_RENDER: {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] SET_HW_RENDER not supported");
            return false;
        }

        case RETRO_ENVIRONMENT_GET_VARIABLE: {
            // When NULL data is passed, the core is checking if the environment callback is available.
            if (!data) return true;
            // An unknown key still succeeds, with a NULL value.
            struct retro_variable* var = (struct retro_variable*)data;
            const SDL_LibretroOption* o = var->key ? SDL_Libretro_GetOption(lr, var->key) : NULL;
            var->value = o ? o->value : NULL;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_VARIABLE: {
            // When NULL data is passed, it's just checking if the callback is available.
            if (!data) return true;
            const struct retro_variable* var = (const struct retro_variable*)data;
            if (!var->key || !var->value) return false;
            return SDL_Libretro_SetOptionValue(lr, var->key, var->value);
        }

        case RETRO_ENVIRONMENT_SET_VARIABLES: {
            // When NULL data is passed, it's just checking if the callback is available.
            if (!data) return true;
            const struct retro_variable* var = (const struct retro_variable*)data;
            for (; var->key; var++) {
                const char* label = "";
                const char* defaultVal = "";
                // Copy the value so that we can manipulate it as needed.
                // Only needed until the end of this call.
                char* valueBuffer = var->value ? SDL_strdup(var->value) : NULL;
                struct retro_core_option_value* values = NULL;
                unsigned vcount = 0;
                if (valueBuffer) {
                    char* semicolon = SDL_strchr(valueBuffer, ';');
                    if (semicolon) {
                        // Split off the label, trimming trailing spaces.
                        *semicolon = '\0';
                        char* labelEnd = semicolon;
                        while (labelEnd > valueBuffer && labelEnd[-1] == ' ') {
                            *(--labelEnd) = '\0';
                        }
                        label = valueBuffer;

                        char* opts = semicolon + 1;
                        while (*opts == ' ') {
                            opts++;
                        }

                        // Count the pipe-separated values so the array is sized exactly.
                        unsigned count = *opts ? 1 : 0;
                        for (const char* p = opts; *p; p++) {
                            if (*p == '|') count++;
                        }
                        if (count > 0) {
                            // Extra trailing entry keeps a NULL sentinel for InitCoreOption.
                            values = (struct retro_core_option_value*)SDL_calloc(count + 1, sizeof(*values));
                        }
                        if (values) {
                            // Version 1 and 2 of the variables API carry no per-value labels, only value strings.
                            char* tok = opts;
                            while (tok && *tok) {
                                char* nextPipe = SDL_strchr(tok, '|');
                                if (nextPipe) *nextPipe = '\0';
                                values[vcount].value = tok;
                                values[vcount].label = NULL;
                                vcount++;
                                tok = nextPipe ? nextPipe + 1 : NULL;
                            }
                        }
                        // The first value is the default (terminate it in place).
                        char* firstPipe = SDL_strchr(opts, '|');
                        if (firstPipe) *firstPipe = '\0';
                        defaultVal = opts;
                    }
                }
                // The legacy list has no size limit, but always ends in a NULL value.
                SDL_Libretro_InitCoreOption(lr, var->key, defaultVal, label, values, SDL_MAX_UINT32, NULL, NULL);
                SDL_free(values);
                SDL_free(valueBuffer);
            }
            return true;
        }

        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
            if (!data) return true;
            *(bool*)data = lr->core.optionsDirtyCore;
            lr->core.optionsDirtyCore = false;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: {
            if (!data) return true;
            lr->core.supportNoGame = *(const bool*)data;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH: {
            if (!data) return true;
            *(const char**)data = lr->core.corePath[0] ? lr->core.corePath : NULL;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK: {
            if (!data) return true;
            lr->core.runloop_frame_time = *(const struct retro_frame_time_callback*)data;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK: {
            // When data is NULL, the core is checking if the environment callback is available.
            if (!data) return true;
            lr->core.audio_callback = *(const struct retro_audio_callback*)data;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE: {
            if (!data) return true;
            struct retro_rumble_interface* rumble = (struct retro_rumble_interface*)data;
            rumble->set_rumble_state = SDL_Libretro_SetRumbleState;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES: {
            if (!data) return true;
            *(uint64_t*)data =
                (1 << RETRO_DEVICE_JOYPAD) |
                (1 << RETRO_DEVICE_MOUSE) |
                (1 << RETRO_DEVICE_KEYBOARD) |
                (1 << RETRO_DEVICE_LIGHTGUN) |
                (1 << RETRO_DEVICE_ANALOG) |
                (1 << RETRO_DEVICE_POINTER);
            return true;
        }

        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
            if (!data) return true;
            struct retro_log_callback* cb = (struct retro_log_callback*)data;
            cb->log = &SDL_Libretro_Logger;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_PERF_INTERFACE: {
            if (!data) return true;
            struct retro_perf_callback* perf = (struct retro_perf_callback*)data;
            perf->get_time_usec = &SDL_Libretro_GetTimeUSEC;
            perf->get_cpu_features = &SDL_Libretro_GetCPUFeatures;
            perf->get_perf_counter = &SDL_Libretro_GetPerfCounter;
            perf->perf_register = &SDL_Libretro_PerfRegister;
            perf->perf_start = &SDL_Libretro_PerfStart;
            perf->perf_stop = &SDL_Libretro_PerfStop;
            perf->perf_log = &SDL_Libretro_PerfLog;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: {
            if (!data) return false;
            const struct retro_system_av_info* av = (const struct retro_system_av_info*)data;
            lr->core.fps = av->timing.fps;
            lr->core.aspectRatio = av->geometry.aspect_ratio;

            // A base-resolution change is picked up by VideoRefresh on the next frame; aspect is applied at render time. Only the audio stream (frequency fixed at open) needs an explicit reopen, deferred to RunFrame.
            if (av->timing.sample_rate != lr->core.sampleRate) {
                lr->core.sampleRate = av->timing.sample_rate;
                if (lr->core.audioStream) {
                    lr->core.audioReinitPending = true;
                }
            }
            SDL_Libretro_PushEvent(lr, SDL_EVENT_LIBRETRO_GEOMETRY_CHANGED, NULL);
            return true;
        }

        case RETRO_ENVIRONMENT_SET_GEOMETRY: {
            if (!data) return true;
            const struct retro_game_geometry* geom = (const struct retro_game_geometry*)data;
            // Geometry updates during runtime are applied in SDL_Libretro_VideoRefresh().
            // Cores may call this every frame, so only report actual changes.
            if (geom->aspect_ratio != lr->core.aspectRatio) {
                lr->core.aspectRatio = geom->aspect_ratio;
                SDL_Libretro_PushEvent(lr, SDL_EVENT_LIBRETRO_GEOMETRY_CHANGED, NULL);
            }
            return true;
        }

        case RETRO_ENVIRONMENT_GET_USERNAME: {
            if (!data) return true;
            *(const char**)data = lr->username;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_LANGUAGE: {
            if (!data) return true;
            *(unsigned*)data = (unsigned)SDL_Libretro_GetLanguage(lr);
            return true;
        }

        // SET_SERIALIZATION_QUIRKS_2 is a newer number for the same command,
        // with the same data; cores use either.
        case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS:
        case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS_2: {
            if (!data) return true;
            lr->core.serializationQuirks = *(const uint64_t*)data;
            // Report that we support variable-size states so cores that flag
            // RETRO_SERIALIZATION_QUIRK_CORE_VARIABLE_SIZE know we won't break.
            *(uint64_t*)data |= RETRO_SERIALIZATION_QUIRK_FRONT_VARIABLE_SIZE;
            return true;
        }

        case 72:
        case RETRO_ENVIRONMENT_GET_SAVESTATE_CONTEXT: {
            if (!data) return false;
            *(unsigned*)data = (unsigned)lr->core.savestateContext;
            return true;
        }

        case 25:
        case RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE: {
            if (!data) return true;
            struct retro_sensor_interface* sensor = (struct retro_sensor_interface*)data;
            sensor->set_sensor_state = SDL_Libretro_SetSensorState;
            sensor->get_sensor_input = SDL_Libretro_GetSensorInput;
            return true;
        }

        case 47:
        case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE: {
            if (!data) return true;
            *(int*)data = RETRO_AV_ENABLE_VIDEO | RETRO_AV_ENABLE_AUDIO;
            return true;
        }

        case 49:
        case RETRO_ENVIRONMENT_GET_FASTFORWARDING: {
            if (!data) return true;
            *(bool*)data = SDL_Libretro_GetSpeed(lr) > 1.0f;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE: {
            if (!data) return true;
            lr->core.fastforwardOverride = *(const struct retro_fastforwarding_override*)data;
            lr->core.fastforwardOverrideActive = lr->core.fastforwardOverride.fastforward;
            // Bypass SDL_Libretro_SetSpeed so the inhibit_toggle guard doesn't block the core's own request.
            if (lr->core.fastforwardOverride.fastforward) {
                float ratio = lr->core.fastforwardOverride.ratio;
                lr->core.speed = ratio > 1.0f ? ratio : 10.0f;
            }
            else {
                lr->core.speed = 1.0f;
            }
            if (lr->core.fastforwardOverride.notification) {
                SDL_Libretro_SetMessage(lr, lr->core.fastforwardOverride.fastforward ? "Fast-Forward" : "Normal Speed", 3.0);
            }
            return true;
        }

        case 50:
        case RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE: {
            if (!data) return true;
            *(float*)data = SDL_Libretro_GetTargetRefreshRate(lr);
            return true;
        }

        case 51:
        case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS: {
            return true;
        }

        case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER: {
            if (!data) return false;
            // Software rendering only; reporting RETRO_HW_CONTEXT_NONE lets
            // GL/Vulkan-capable cores fall back to their software path.
            *(unsigned*)data = RETRO_HW_CONTEXT_NONE;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION: {
            if (!data) return false;
            *(unsigned*)data = 1;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_INPUT_MAX_USERS: {
            if (!data) return false;
            *(unsigned*)data = SDL_LIBRETRO_MAX_USERS;
            return true;
        }

        case 71:
        case RETRO_ENVIRONMENT_GET_THROTTLE_STATE: {
            if (!data) return false;
            struct retro_throttle_state* throttle = (struct retro_throttle_state*)data;

            double fps = lr->core.fps > 0.0 ? lr->core.fps : 60.0;
            float speed = lr->core.speed;
            if (speed == 0.0f) {
                throttle->mode = RETRO_THROTTLE_FRAME_STEPPING;
                throttle->rate = 0.0f;
            }
            else if (speed > 1.0f) {
                throttle->mode = RETRO_THROTTLE_FAST_FORWARD;
                throttle->rate = (float)(fps * speed);
            }
            else if (speed > 0.0f && speed < 1.0f) {
                throttle->mode = RETRO_THROTTLE_SLOW_MOTION;
                throttle->rate = (float)(fps * speed);
            }
            else if (speed < 0.0f) {
                throttle->mode = RETRO_THROTTLE_REWINDING;
                throttle->rate = (float)(fps * -speed);
            }
            else {
                throttle->mode = RETRO_THROTTLE_NONE;
                throttle->rate = (float)fps;

                // VSYNC
                int vsync = SDL_RENDERER_VSYNC_DISABLED;
                if (lr->renderer && SDL_GetRenderVSync(lr->renderer, &vsync) && vsync != SDL_RENDERER_VSYNC_DISABLED) {
                    float interval = (vsync == SDL_RENDERER_VSYNC_ADAPTIVE) ? 1.0f : (float)vsync;
                    float vsyncRate = SDL_Libretro_GetTargetRefreshRate(lr) / interval;
                    if (vsyncRate < fps) {
                        throttle->mode = RETRO_THROTTLE_VSYNC;
                        throttle->rate = vsyncRate;
                    }
                }
            }

            return true;
        }

        case RETRO_ENVIRONMENT_GET_JIT_CAPABLE: {
            if (!data) return false;
#if defined(SDL_PLATFORM_EMSCRIPTEN)
            *(bool*)data = false;
#else
            *(bool*)data = true;
#endif
            return true;
        }

        case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO: {
            if (!data) return true;
            const struct retro_subsystem_info* info = (const struct retro_subsystem_info*)data;
            SDL_Libretro_FreeSubsystems(lr);
            unsigned count = 0;
            while (info[count].desc) count++;
            if (count > 0) {
                struct retro_subsystem_info* subs = (struct retro_subsystem_info*)SDL_calloc(count, sizeof(*subs));
                if (subs) {
                    lr->core.subsystems = subs;
                    lr->core.subsystemCount = count;
                    for (unsigned i = 0; i < count; i++) {
                        subs[i].desc = SDL_strdup(info[i].desc ? info[i].desc : "");
                        subs[i].ident = SDL_strdup(info[i].ident ? info[i].ident : "");
                        subs[i].id = info[i].id;
                        if (info[i].num_roms > 0 && info[i].roms) {
                            struct retro_subsystem_rom_info* roms = (struct retro_subsystem_rom_info*)SDL_calloc(
                                info[i].num_roms,
                                sizeof(*roms));
                            subs[i].roms = roms;
                            if (roms) {
                                // Only publish the count once the array exists, so an allocation failure leaves num_roms at 0.
                                subs[i].num_roms = info[i].num_roms;
                                for (unsigned r = 0; r < info[i].num_roms; r++) {
                                    roms[r].desc = SDL_strdup(info[i].roms[r].desc ? info[i].roms[r].desc : "");
                                    roms[r].valid_extensions = SDL_strdup(
                                        info[i].roms[r].valid_extensions ? info[i].roms[r].valid_extensions : "");
                                    roms[r].need_fullpath = info[i].roms[r].need_fullpath;
                                    roms[r].block_extract = info[i].roms[r].block_extract;
                                    roms[r].required = info[i].roms[r].required;
                                    if (info[i].roms[r].num_memory > 0 && info[i].roms[r].memory) {
                                        struct retro_subsystem_memory_info* memory = (struct retro_subsystem_memory_info*)SDL_calloc(
                                            info[i].roms[r].num_memory,
                                            sizeof(*memory));
                                        roms[r].memory = memory;
                                        if (memory) {
                                            roms[r].num_memory = info[i].roms[r].num_memory;
                                            for (unsigned m = 0; m < info[i].roms[r].num_memory; m++) {
                                                memory[m].extension = SDL_strdup(
                                                    info[i].roms[r].memory[m].extension ? info[i].roms[r].memory[m].extension : "");
                                                memory[m].type = info[i].roms[r].memory[m].type;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return true;
        }

        case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO: {
            const struct retro_controller_info* info = (const struct retro_controller_info*)data;
            if (!info) return true;
            SDL_Libretro_FreeControllerInfo(lr);
            unsigned count = 0;
            for (unsigned i = 0; info[i].types; i++) count++;
            if (count == 0) {
                return true;
            }

            lr->core.controllerInfo = (struct retro_controller_info*)SDL_calloc(count, sizeof(*info));
            if (lr->core.controllerInfo) {
                lr->core.controllerInfoCount = count;
                // Deep copy the data into our own data struture
                for (unsigned i = 0; i < count; i++) {
                    if (info[i].num_types == 0) continue;
                    struct retro_controller_description* types = (struct retro_controller_description*)SDL_calloc(info[i].num_types, sizeof(*types));
                    if (types) {
                        // Only publish the count once the array exists, so an allocation failure leaves num_types at 0.
                        lr->core.controllerInfo[i].num_types = info[i].num_types;
                        for (unsigned t = 0; t < info[i].num_types; t++) {
                            types[t].id = info[i].types[t].id;
                            types[t].desc = info[i].types[t].desc ? SDL_strdup(info[i].types[t].desc) : NULL;
                        }
                    }
                    lr->core.controllerInfo[i].types = types;
                }
            }
            return true;
        }

        case 36:
        case RETRO_ENVIRONMENT_SET_MEMORY_MAPS: {
            if (!data) return true;
            const struct retro_memory_map* map = (const struct retro_memory_map*)data;
            SDL_Libretro_FreeMemoryMap(lr);
            if (map->num_descriptors > 0) {
                size_t sz = map->num_descriptors * sizeof(struct retro_memory_descriptor);
                lr->core.memoryMapDescriptors = (struct retro_memory_descriptor*)SDL_malloc(sz);
                if (lr->core.memoryMapDescriptors) {
                    SDL_memcpy(lr->core.memoryMapDescriptors, map->descriptors, sz);
                    lr->core.memoryMapDescriptorCount = map->num_descriptors;
                    // The core only guarantees the addrspace label strings for the duration of this call, so deep-copy them.
                    for (unsigned i = 0; i < lr->core.memoryMapDescriptorCount; i++) {
                        const char* as = lr->core.memoryMapDescriptors[i].addrspace;
                        if (!as) continue;
                        lr->core.memoryMapDescriptors[i].addrspace = SDL_strdup(as);
                        if (!lr->core.memoryMapDescriptors[i].addrspace) {
                            // Clear the not-yet-copied labels (still the core's temporary pointers) before unwinding.
                            for (unsigned j = i + 1; j < lr->core.memoryMapDescriptorCount; j++) {
                                lr->core.memoryMapDescriptors[j].addrspace = NULL;
                            }
                            SDL_Libretro_FreeMemoryMap(lr);
                            return SDL_SetError("[SDL_Libretro] Out of memory copying memory map");
                        }
                    }
                }
            }
            return true;
        }

        case RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK: {
            // NULL data unregisters the callback.
            // @see SDL_Libretro_ReportAudioBufferStatus
            const struct retro_audio_buffer_status_callback* cb = (const struct retro_audio_buffer_status_callback*)data;
            lr->core.audio_buffer_status.callback = cb ? cb->callback : NULL;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY: {
            if (!data) return true;
            lr->core.minimumAudioLatencyMs = *(const unsigned*)data;
            // Apply the new latency target live if audio is already running.
            if (lr->core.audioStream) {
                SDL_Libretro_UpdateAudioThreshold(lr);
            }
            return true;
        }

        case RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE: {
            if (!data) return false;
            const struct retro_system_content_info_override* overrides =
                (const struct retro_system_content_info_override*)data;
            SDL_Libretro_FreeContentInfoOverrides(lr);
            unsigned count = 0;
            while (overrides[count].extensions) count++;
            if (count > 0) {
                lr->core.contentInfoOverrides = (struct retro_system_content_info_override*)
                    SDL_calloc(count, sizeof(struct retro_system_content_info_override));
                if (lr->core.contentInfoOverrides) {
                    for (unsigned i = 0; i < count; i++) {
                        lr->core.contentInfoOverrides[i] = overrides[i];
                        // The core only guarantees the extensions string for this call, so deep-copy it.
                        lr->core.contentInfoOverrides[i].extensions = SDL_strdup(overrides[i].extensions);
                    }
                    lr->core.contentInfoOverrideCount = count;
                }
            }
            return true;
        }

        case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: {
            if (!data) return true;
            *(unsigned*)data = 2;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS: {
            if (!data) return true;
            const struct retro_core_option_definition* defs = (const struct retro_core_option_definition*)data;
            struct retro_core_option_v2_definition* v2defs = SDL_Libretro_CoreOptionsV1ToV2(defs);
            if (!v2defs) return false;
            struct retro_core_options_v2 opts = {NULL, v2defs};
            bool result = SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &opts);
            SDL_free(v2defs);
            return result;
        }

        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL: {
            if (!data) return false;
            const struct retro_core_options_intl* intl = (const struct retro_core_options_intl*)data;
            if (!intl->us) return false;
            // Convert both sets to v2 and let the v2 INTL path merge the translations.
            struct retro_core_option_v2_definition* usDefs = SDL_Libretro_CoreOptionsV1ToV2(intl->us);
            if (!usDefs) return false;
            struct retro_core_option_v2_definition* localDefs = intl->local ? SDL_Libretro_CoreOptionsV1ToV2(intl->local) : NULL;
            struct retro_core_options_v2 us = {NULL, usDefs};
            struct retro_core_options_v2 local = {NULL, localDefs};
            struct retro_core_options_v2_intl v2intl = {&us, localDefs ? &local : NULL};
            bool result = SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL, &v2intl);
            SDL_free(localDefs);
            SDL_free(usDefs);
            return result;
        }

        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY: {
            if (!data) return false;
            const struct retro_core_option_display* opt = (const struct retro_core_option_display*)data;
            if (!opt->key) return false;
            for (unsigned i = 0; i < lr->core.optionCount; i++) {
                if (lr->core.options[i].key && SDL_strcmp(lr->core.options[i].key, opt->key) == 0) {
                    // Display callbacks re-send every option's visibility, so
                    // only an actual flip counts as a change. Only the app needs
                    // to refresh; the option's value is unchanged, so don't make
                    // GET_VARIABLE_UPDATE report true to the core.
                    if (lr->core.options[i].visible != opt->visible) {
                        lr->core.options[i].visible = opt->visible;
                        SDL_Libretro_MarkOptionsChanged(lr);
                    }
                    return true;
                }
            }
            return false;
        }

        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2: {
            if (!data) return false;
            const struct retro_core_options_v2* opts = (const struct retro_core_options_v2*)data;
            if (!opts->definitions) {
                return true; // There arn't any options.
            }

            // Register the option categories (if any) before the options that reference them.
            if (opts->categories) {
                for (unsigned c = 0; opts->categories[c].key; c++) {
                    SDL_Libretro_InitCoreOptionCategory(lr, opts->categories[c].key, opts->categories[c].desc, opts->categories[c].info);
                }
            }

            for (unsigned i = 0; opts->definitions[i].key; i++) {
                const struct retro_core_option_v2_definition* def = &opts->definitions[i];
                const char* defaultVal = def->default_value ? def->default_value : "";
                // Prefer the *_categorized text when the option belongs to a category.
                const char* desc = SDL_Libretro_PickCoreOptionText(def->desc, def->desc_categorized, def->category_key);
                const char* info = SDL_Libretro_PickCoreOptionText(def->info, def->info_categorized, def->category_key);
                SDL_Libretro_InitCoreOption(lr, def->key, defaultVal, desc, def->values, RETRO_NUM_CORE_OPTION_VALUES_MAX, info, def->category_key);
            }
            return true;
        }

        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL: {
            if (!data) return false;
            const struct retro_core_options_v2_intl* intl = (const struct retro_core_options_v2_intl*)data;
            if (!intl->us) return false;
            const struct retro_core_options_v2* us = intl->us;
            const struct retro_core_options_v2* local = intl->local;
            // Without translations the `us` member is already a retro_core_options_v2, so forward it as-is.
            if (!local || !us->definitions || (!local->definitions && !local->categories)) {
                return SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, (void*)us);
            }

            // Merge onto a copy of `us`: keys, option values, defaults and category membership come from `us`; any non-empty text in `local` (matched by key, and value labels by value) replaces the English text. Options missing from `local` keep their `us` text.
            unsigned defCount = 0;
            while (us->definitions[defCount].key) defCount++;
            unsigned catCount = 0;
            while (us->categories && us->categories[catCount].key) catCount++;
            struct retro_core_option_v2_definition* defs =
                (struct retro_core_option_v2_definition*)SDL_calloc(defCount + 1, sizeof(*defs));
            struct retro_core_option_v2_category* cats = catCount > 0
                                                             ? (struct retro_core_option_v2_category*)SDL_calloc(catCount + 1, sizeof(*cats))
                                                             : NULL;
            if (!defs || (catCount > 0 && !cats)) {
                SDL_free(defs);
                SDL_free(cats);
                return false;
            }

            for (unsigned c = 0; c < catCount; c++) {
                cats[c] = us->categories[c];
                for (unsigned l = 0; local->categories && local->categories[l].key; l++) {
                    if (SDL_strcmp(local->categories[l].key, cats[c].key) == 0) {
                        cats[c].desc = SDL_Libretro_PickLocalText(cats[c].desc, local->categories[l].desc);
                        cats[c].info = SDL_Libretro_PickLocalText(cats[c].info, local->categories[l].info);
                        break;
                    }
                }
            }

            for (unsigned i = 0; i < defCount; i++) {
                defs[i] = us->definitions[i];
                const struct retro_core_option_v2_definition* loc = NULL;
                for (unsigned l = 0; local->definitions && local->definitions[l].key; l++) {
                    if (SDL_strcmp(local->definitions[l].key, defs[i].key) == 0) {
                        loc = &local->definitions[l];
                        break;
                    }
                }
                if (!loc) continue;
                defs[i].desc = SDL_Libretro_PickLocalText(defs[i].desc, loc->desc);
                defs[i].desc_categorized = SDL_Libretro_PickLocalText(defs[i].desc_categorized, loc->desc_categorized);
                defs[i].info = SDL_Libretro_PickLocalText(defs[i].info, loc->info);
                defs[i].info_categorized = SDL_Libretro_PickLocalText(defs[i].info_categorized, loc->info_categorized);
                for (unsigned v = 0; v < RETRO_NUM_CORE_OPTION_VALUES_MAX && defs[i].values[v].value; v++) {
                    for (unsigned lv = 0; lv < RETRO_NUM_CORE_OPTION_VALUES_MAX && loc->values[lv].value; lv++) {
                        if (SDL_strcmp(loc->values[lv].value, defs[i].values[v].value) == 0) {
                            defs[i].values[v].label = SDL_Libretro_PickLocalText(defs[i].values[v].label, loc->values[lv].label);
                            break;
                        }
                    }
                }
            }

            struct retro_core_options_v2 merged = {cats, defs};
            bool result = SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &merged);
            SDL_free(cats);
            SDL_free(defs);
            return result;
        }

        case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION: {
            if (!data) return true;
            *(unsigned*)data = 1;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK: {
            if (!data) {
                lr->core.optionsUpdateDisplayCallback = NULL;
                return true;
            }
            const struct retro_core_options_update_display_callback* cb =
                (const struct retro_core_options_update_display_callback*)data;
            lr->core.optionsUpdateDisplayCallback = cb->callback;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_MESSAGE_EXT: {
            const struct retro_message_ext* msg = (const struct retro_message_ext*)data;
            if (!msg || !msg->msg) return true;
            double seconds = msg->duration / 1000.0;

            if (msg->target == RETRO_MESSAGE_TARGET_LOG || msg->target == RETRO_MESSAGE_TARGET_ALL) {
                switch (msg->level) {
                    case RETRO_LOG_DEBUG: SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Core] %s", msg->msg); break;
                    case RETRO_LOG_WARN: SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Core] %s", msg->msg); break;
                    case RETRO_LOG_ERROR: SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[Core] %s", msg->msg); break;
                    default: SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Core] %s", msg->msg); break;
                }
            }

            if (msg->target != RETRO_MESSAGE_TARGET_LOG) {
                SDL_Libretro_OsdPush(lr, msg->msg, seconds, msg->priority, msg->type, msg->progress);
            }
            return true;
        }

        case RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE: {
            if (!data) return true;
            lr->core.disk_control = *(const struct retro_disk_control_ext_callback*)data;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT: {
            if (!data) return false;
            // Subsystem loads get an array with one entry per content file.
            if (lr->core.gameInfoExtArray) {
                *(const struct retro_game_info_ext**)data = lr->core.gameInfoExtArray;
                return true;
            }
            // Single-file loads get a 1-entry array. A non-NULL full_path means LoadGame populated it for content.
            if (!lr->core.gameInfoExt.full_path) return false;
            *(const struct retro_game_info_ext**)data = &lr->core.gameInfoExt;
            return true;
        }

        case 40:
        case RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER: {
            if (!data || !lr->core.texture) return false;
            struct retro_framebuffer* fb = (struct retro_framebuffer*)data;
            if (fb->width != lr->core.width || fb->height != lr->core.height) return false;
            // The lock is write-only (discards prior contents), so a core asking to read back can't use it.
            if (fb->access_flags & RETRO_MEMORY_ACCESS_READ) return false;

            // Drop any lock still held from an earlier acquire this frame (or a frame that never reached video_cb) before re-locking.
            SDL_Libretro_ReleaseSoftwareFramebuffer(lr);

            void* pixels = NULL;
            int pitch = 0;
            if (!SDL_LockTexture(lr->core.texture, NULL, &pixels, &pitch)) return false;

            fb->data = pixels;
            fb->pitch = (size_t)pitch;
            fb->format = lr->core.pixelFormat;
            fb->memory_flags = RETRO_MEMORY_TYPE_CACHED;
            lr->core.softwareFramebufferPixels = pixels;
            return true;
        }

        case 45:
        case RETRO_ENVIRONMENT_GET_VFS_INTERFACE: {
            if (!data) return true;
            // No VFS to hand out (SDL_LIBRETRO_DISABLE_VFS without a custom one set).
            if (!lr->vfs_interface.open) return false;
            struct retro_vfs_interface_info* info = (struct retro_vfs_interface_info*)data;
            if (info->required_interface_version > 4) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] GET_VFS_INTERFACE: unsupported required_interface_version %u", info->required_interface_version);
                return false;
            }

            // Let the core know which version of the VFS we support.
            info->required_interface_version = 4;
            info->iface = &lr->vfs_interface;
            lr->core.usedVFS = true;
            return true;
        }

        case 75:
        case RETRO_ENVIRONMENT_GET_MICROPHONE_INTERFACE: {
            if (!data) return false;
            struct retro_microphone_interface* mic = (struct retro_microphone_interface*)data;
            mic->interface_version = RETRO_MICROPHONE_INTERFACE_VERSION;
            mic->open_mic = SDL_Libretro_MicOpen;
            mic->close_mic = SDL_Libretro_MicClose;
            mic->get_params = SDL_Libretro_MicGetParams;
            mic->set_mic_state = SDL_Libretro_MicSetState;
            mic->get_mic_state = SDL_Libretro_MicGetState;
            mic->read_mic = SDL_Libretro_MicRead;
            return true;
        }

        case 77:
        case RETRO_ENVIRONMENT_GET_DEVICE_POWER: {
            if (!data) return true;
            struct retro_device_power* power = (struct retro_device_power*)data;
            int percent;
            SDL_PowerState sdl_state = SDL_GetPowerInfo(&power->seconds, &percent);
            switch (sdl_state) {
                case SDL_POWERSTATE_ON_BATTERY:
                    power->state = RETRO_POWERSTATE_DISCHARGING;
                    break;
                case SDL_POWERSTATE_CHARGING:
                    power->state = RETRO_POWERSTATE_CHARGING;
                    break;
                case SDL_POWERSTATE_CHARGED:
                    power->state = RETRO_POWERSTATE_CHARGED;
                    break;
                case SDL_POWERSTATE_NO_BATTERY:
                    power->state = RETRO_POWERSTATE_PLUGGED_IN;
                    break;
                default:
                    return false;
            }
            power->percent = (int8_t)percent;
            return true;
        }

        case 81:
        case RETRO_ENVIRONMENT_GET_TARGET_SAMPLE_RATE: {
            if (!data) return true;
            SDL_AudioSpec spec;
            if (lr->core.audioStream && SDL_GetAudioStreamFormat(lr->core.audioStream, NULL, &spec) && spec.freq > 0) {
                // Active Device
                *(unsigned*)data = (unsigned)spec.freq;
            }
            else if (SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL) && spec.freq > 0) {
                // Default Device
                *(unsigned*)data = (unsigned)spec.freq;
            }
            else {
                // Desired Frequency
                *(unsigned*)data = SDL_LIBRETRO_AUDIO_DEFAULT_SAMPLE_RATE;
            }
            return true;
        }

        case 46:
        case RETRO_ENVIRONMENT_GET_LED_INTERFACE: {
            if (!data) return true;
            struct retro_led_interface* iface = (struct retro_led_interface*)data;
            iface->set_led_state = SDL_Libretro_SetLEDState;
            return true;
        }

        // Anything else, including commands the library chooses not to
        // implement (camera, location, hardware rendering, achievements):
        // let the application implement the command through an event watch
        // on SDL_EVENT_LIBRETRO_ENVIRONMENT(cmd).
        default: {
            if (SDL_Libretro_PushEnvEvent(lr, cmd, data)) {
                return true;
            }
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] Unhandled environment callback: %u", cmd);
            return false;
        }
    }
}

#endif /* SDL_LIBRETRO_ENV_IMPL_ONCE */
