/**
 * SDL_libretro environment callback dispatch
 *
 * @file SDL_libretro_env.h
 */

 #if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_ENV_IMPL_ONCE)
#define SDL_LIBRETRO_ENV_IMPL_ONCE

#include <stdarg.h>

static void SDL_Libretro_Logger(enum retro_log_level level, const char* fmt, ...) {
    if (SDL_Libretro_active && (int)level < SDL_Libretro_active->logLevel) {
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
        case RETRO_LOG_DEBUG: SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] %s", buf); break;
        case RETRO_LOG_INFO:  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] %s", buf); break;
        case RETRO_LOG_WARN:  SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] %s", buf); break;
        case RETRO_LOG_ERROR: SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] %s", buf); break;
        default:              SDL_Log("[SDL_Libretro] %s", buf); break;
    }
}

static retro_time_t SDL_Libretro_GetTimeUSEC(void) {
    return (retro_time_t)(SDL_GetPerformanceCounter() * 1000000 / SDL_GetPerformanceFrequency());
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
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "PERF: counter registry full (%u), dropping '%s'",
                    lr->core.perfCounterCount, counter->ident ? counter->ident : "?");
        return;
    }

    lr->core.perfCounters[lr->core.perfCounterCount++] = counter;
    counter->registered = true;
}

static void SDL_Libretro_PerfStart(struct retro_perf_counter* counter) {
    if (counter) counter->start = (retro_perf_tick_t)SDL_GetPerformanceCounter();
}

static void SDL_Libretro_PerfStop(struct retro_perf_counter* counter) {
    if (counter) counter->total += (retro_perf_tick_t)SDL_GetPerformanceCounter() - counter->start;
}

/**
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
        SDL_Log("[PERF] %s: %llu calls, %llu ticks (%.4f ms)",
                counter->ident ? counter->ident : "?",
                (unsigned long long)counter->call_cnt,
                (unsigned long long)counter->total,
                ms);
    }
}

static bool SDL_Libretro_SetRumbleState(unsigned port, enum retro_rumble_effect effect, uint16_t strength) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr || port >= SDL_LIBRETRO_RUMBLE_PORTS) return false;

    float normalized = (float)strength / 65535.0f;
    if (effect == RETRO_RUMBLE_STRONG) {
        lr->core.rumbleStrong[port] = normalized;
    } else {
        lr->core.rumbleWeak[port] = normalized;
    }

    if (port < 16 && lr->gamepads[port]) {
        Uint16 lo = (Uint16)(lr->core.rumbleWeak[port] * 65535.0f);
        Uint16 hi = (Uint16)(lr->core.rumbleStrong[port] * 65535.0f);
        SDL_RumbleGamepad(lr->gamepads[port], lo, hi, 100);
    }

    return true;
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
 * The refresh rate the frontend is presenting at, in Hz. Taken from the
 * current display mode of the core's window, defaulting to 60 when unknown.
 */
static float SDL_Libretro_GetTargetRefreshRate(SDL_Libretro* lr) {
    float rate = 60.0f;
    if (lr->core.window) {
        SDL_DisplayID display = SDL_GetDisplayForWindow(lr->core.window);
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

static bool SDL_Libretro_EnvironmentCallback(unsigned cmd, void* data) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr) return false;

    switch (cmd) {
        case RETRO_ENVIRONMENT_SET_ROTATION: {
            if (!data) return false;
            lr->core.rotation = (int)*(const unsigned*)data;
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
            return true;
        }

        case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL: {
            if (!data) return false;
            lr->core.performanceLevel = *(const unsigned*)data;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
            if (!data) return false;
            *(const char**)data = SDL_Libretro_GetDirectory(lr, RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY);
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
            if (!desc) return false;
            unsigned count = 0;
            for (const struct retro_input_descriptor* d = desc; d->description; d++) count++;
            SDL_free(lr->core.inputDescriptors);
            lr->core.inputDescriptors = NULL;
            lr->core.inputDescriptorCount = 0;
            if (count > 0) {
                lr->core.inputDescriptors = (struct retro_input_descriptor*)SDL_malloc(count * sizeof(*desc));
                if (lr->core.inputDescriptors) {
                    SDL_memcpy(lr->core.inputDescriptors, desc, count * sizeof(*desc));
                    lr->core.inputDescriptorCount = count;
                }
            }
            return true;
        }

        case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK: {
            if (!data) return false;
            const struct retro_keyboard_callback* cb = (const struct retro_keyboard_callback*)data;
            lr->core.keyboard_event = cb->callback;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE: {
            const struct retro_disk_control_callback* cb = (const struct retro_disk_control_callback*)data;
            if (!cb) return false;
            SDL_memset(&lr->core.disk_control, 0, sizeof(lr->core.disk_control));
            lr->core.disk_control.set_eject_state = cb->set_eject_state;
            lr->core.disk_control.get_eject_state = cb->get_eject_state;
            lr->core.disk_control.get_image_index = cb->get_image_index;
            lr->core.disk_control.set_image_index = cb->set_image_index;
            lr->core.disk_control.get_num_images = cb->get_num_images;
            lr->core.disk_control.replace_image_index = cb->replace_image_index;
            lr->core.disk_control.add_image_index = cb->add_image_index;
            lr->core.diskControlActive = true;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_HW_RENDER: {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] SET_HW_RENDER not supported");
            return false;
        }

        case RETRO_ENVIRONMENT_GET_VARIABLE: {
            if (!data) return false;
            struct retro_variable* var = (struct retro_variable*)data;
            if (!var->key) return false;
            const SDL_LibretroOption* o = SDL_Libretro_GetOption(lr, var->key);
            if (o) {
                var->value = o->value;
                return true;
            }
            return false;
        }

        case RETRO_ENVIRONMENT_SET_VARIABLE: {
            if (!data) return true;
            const struct retro_variable* var = (const struct retro_variable*)data;
            if (!var->key || !var->value) return false;
            return SDL_Libretro_SetOptionValue(lr, var->key, var->value);
        }

        case RETRO_ENVIRONMENT_SET_VARIABLES: {
            if (!data) return false;
            const struct retro_variable* var = (const struct retro_variable*)data;
            for (; var->key; var++) {
                char defaultVal[512] = {0};
                char label[512] = {0};
                char optsBuf[512] = {0};
                // v0/v1 variables carry no per-value labels; only value strings.
                struct retro_core_option_value values[RETRO_NUM_CORE_OPTION_VALUES_MAX] = {0};
                if (var->value) {
                    const char* semi = SDL_strchr(var->value, ';');
                    if (semi) {
                        size_t labelLen = (size_t)(semi - var->value);
                        while (labelLen > 0 && var->value[labelLen - 1] == ' ') {
                            labelLen--;
                        }
                        if (labelLen >= sizeof(label)) {
                            labelLen = sizeof(label) - 1;
                        }
                        SDL_memcpy(label, var->value, labelLen);

                        const char* opts = semi + 1;
                        while (*opts == ' ') {
                            opts++;
                        }

                        const char* pipe = SDL_strchr(opts, '|');
                        size_t len = pipe ? (size_t)(pipe - opts) : SDL_strlen(opts);
                        if (len >= sizeof(defaultVal)) {
                            len = sizeof(defaultVal) - 1;
                        }
                        SDL_memcpy(defaultVal, opts, len);

                        // Parse the pipe-separated values into the values array.
                        // optsBuf is the mutable backing store the pointers reference;
                        // InitCoreOption deep-copies, so it only needs to live until that call.
                        SDL_strlcpy(optsBuf, opts, sizeof(optsBuf));
                        unsigned vcount = 0;
                        char* tok = optsBuf;
                        while (tok && *tok && vcount < RETRO_NUM_CORE_OPTION_VALUES_MAX - 1) {
                            char* nextPipe = SDL_strchr(tok, '|');
                            if (nextPipe) *nextPipe = '\0';
                            values[vcount].value = tok;
                            values[vcount].label = NULL;
                            vcount++;
                            tok = nextPipe ? nextPipe + 1 : NULL;
                        }
                    }
                }
                SDL_Libretro_InitCoreOption(lr, var->key,
                    defaultVal,
                    label,
                    values,
                    NULL, NULL);
            }
            return true;
        }

        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
            if (!data) return false;
            *(bool*)data = lr->core.optionsDirty;
            lr->core.optionsDirty = false;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: {
            if (!data) return false;
            lr->core.supportNoGame = *(const bool*)data;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH: {
            if (!data) return false;
            *(const char**)data = lr->core.corePath[0] ? lr->core.corePath : NULL;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK: {
            if (!data) return false;
            lr->core.runloop_frame_time = *(const struct retro_frame_time_callback*)data;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK: {
            if (!data) return false;
            lr->core.audio_callback = *(const struct retro_audio_callback*)data;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE: {
            if (!data) return false;
            struct retro_rumble_interface* rumble = (struct retro_rumble_interface*)data;
            rumble->set_rumble_state = SDL_Libretro_SetRumbleState;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES: {
            if (!data) return false;
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
            if (!data) return false;
            struct retro_log_callback* cb = (struct retro_log_callback*)data;
            cb->log = &SDL_Libretro_Logger;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_PERF_INTERFACE: {
            if (!data) return false;
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

        case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY: {
            if (!data) return false;
            *(const char**)data = SDL_Libretro_GetDirectory(lr, RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY);
            return true;
        }

        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: {
            if (!data) return false;
            *(const char**)data = SDL_Libretro_GetDirectory(lr, RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY);
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
            return true;
        }

        case RETRO_ENVIRONMENT_SET_GEOMETRY: {
            if (!data) return false;
            const struct retro_game_geometry* geom = (const struct retro_game_geometry*)data;
            // Geometry updates during runtime are applied in SDL_Libretro_VideoRefresh().
            lr->core.aspectRatio = geom->aspect_ratio;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_USERNAME: {
            if (!data) return false;
            *(const char**)data = lr->username;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_LANGUAGE: {
            if (!data) return false;
            *(unsigned*)data = RETRO_LANGUAGE_ENGLISH;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS: {
            if (!data) return false;
            lr->core.serializationQuirks = *(const uint64_t*)data;
            return true;
        }

        case 25:
        case RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE: {
            if (!data) return false;
            struct retro_sensor_interface* sensor = (struct retro_sensor_interface*)data;
            sensor->set_sensor_state = SDL_Libretro_SetSensorState;
            sensor->get_sensor_input = SDL_Libretro_GetSensorInput;
            return true;
        }

        case 47:
        case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE: {
            if (!data) return false;
            *(int*)data = RETRO_AV_ENABLE_VIDEO | RETRO_AV_ENABLE_AUDIO;
            return true;
        }

        case 49:
        case RETRO_ENVIRONMENT_GET_FASTFORWARDING: {
            if (!data) return false;
            *(bool*)data = SDL_Libretro_GetSpeed(lr) > 1.0f;
            return true;
        }

        case 50:
        case RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE: {
            if (!data) return false;
            *(float*)data = SDL_Libretro_GetTargetRefreshRate(lr);
            return true;
        }

        case 51:
        case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS: {
            return true;
        }

        case 71:
        case RETRO_ENVIRONMENT_GET_THROTTLE_STATE: {
            if (!data) return false;
            struct retro_throttle_state* throttle = (struct retro_throttle_state*)data;

            double fps = lr->core.fps > 0.0 ? lr->core.fps : 60.0;
            float speed = lr->speed;
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
                if (lr->core.renderer && SDL_GetRenderVSync(lr->core.renderer, &vsync) && vsync != SDL_RENDERER_VSYNC_DISABLED) {
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

        case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO: {
            const struct retro_controller_info* info = (const struct retro_controller_info*)data;
            if (!info) return false;
            SDL_free(lr->core.controllerInfo);
            lr->core.controllerInfo = NULL;
            lr->core.controllerPortCount = 0;
            unsigned count = 0;
            for (unsigned i = 0; info[i].types; i++) count++;
            if (count > 0) {
                lr->core.controllerInfo = (struct retro_controller_info*)SDL_malloc(count * sizeof(*info));
                if (lr->core.controllerInfo) {
                    SDL_memcpy(lr->core.controllerInfo, info, count * sizeof(*info));
                    lr->core.controllerPortCount = count;
                }
            }
            return true;
        }

        case 36:
        case RETRO_ENVIRONMENT_SET_MEMORY_MAPS: {
            if (!data) return false;
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
                        if (as) lr->core.memoryMapDescriptors[i].addrspace = SDL_strdup(as);
                    }
                }
            }
            return true;
        }

        case RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK: {
            // NULL data unregisters the callback.
            // @see SDL_Libretro_ReportAudioBufferStatus
            const struct retro_audio_buffer_status_callback* cb =
                (const struct retro_audio_buffer_status_callback*)data;
            lr->core.audio_buffer_status.callback = cb ? cb->callback : NULL;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY: {
            if (!data) return false;
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
            if (!data) return false;
            *(unsigned*)data = 2;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS: {
            if (!data) return false;
            const struct retro_core_option_definition* defs = (const struct retro_core_option_definition*)data;
            unsigned count = 0;
            while (defs[count].key) count++;
            struct retro_core_option_v2_definition* v2defs =
                (struct retro_core_option_v2_definition*)SDL_calloc(count + 1, sizeof(*v2defs));
            if (!v2defs) return false;
            for (unsigned i = 0; i < count; i++) {
                v2defs[i].key           = defs[i].key;
                v2defs[i].desc          = defs[i].desc;
                v2defs[i].info          = defs[i].info;
                v2defs[i].default_value = defs[i].default_value;
                SDL_memcpy(v2defs[i].values, defs[i].values, sizeof(defs[i].values));
            }
            struct retro_core_options_v2 opts = { NULL, v2defs };
            bool result = SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &opts);
            SDL_free(v2defs);
            return result;
        }

        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL: {
            if (!data) return false;
            const struct retro_core_options_intl* intl = (const struct retro_core_options_intl*)data;
            const struct retro_core_option_definition* defs = intl->us;
            if (!defs) return false;
            unsigned count = 0;
            while (defs[count].key) count++;
            struct retro_core_option_v2_definition* v2defs =
                (struct retro_core_option_v2_definition*)SDL_calloc(count + 1, sizeof(*v2defs));
            if (!v2defs) return false;
            for (unsigned i = 0; i < count; i++) {
                v2defs[i].key           = defs[i].key;
                v2defs[i].desc          = defs[i].desc;
                v2defs[i].info          = defs[i].info;
                v2defs[i].default_value = defs[i].default_value;
                SDL_memcpy(v2defs[i].values, defs[i].values, sizeof(defs[i].values));
            }
            struct retro_core_options_v2 opts = { NULL, v2defs };
            bool result = SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &opts);
            SDL_free(v2defs);
            return result;
        }

        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY: {
            if (!data) return false;
            const struct retro_core_option_display* opt = (const struct retro_core_option_display*)data;
            if (!opt->key) return false;
            for (unsigned i = 0; i < lr->core.optionCount; i++) {
                if (lr->core.options[i].key && SDL_strcmp(lr->core.options[i].key, opt->key) == 0) {
                    lr->core.options[i].visible = opt->visible;
                    lr->core.optionsDirty = true;
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
                    SDL_Libretro_InitCoreOptionCategory(lr, opts->categories[c].key,
                        opts->categories[c].desc, opts->categories[c].info);
                }
            }

            for (unsigned i = 0; opts->definitions[i].key; i++) {
                const struct retro_core_option_v2_definition* def = &opts->definitions[i];
                const char* defaultVal = def->default_value ? def->default_value : "";
                // Prefer the *_categorized text when the option belongs to a category.
                const char* desc = SDL_Libretro_PickCoreOptionText(def->desc, def->desc_categorized, def->category_key);
                const char* info = SDL_Libretro_PickCoreOptionText(def->info, def->info_categorized, def->category_key);
                SDL_Libretro_InitCoreOption(lr, def->key,
                    defaultVal,
                    desc,
                    def->values,
                    info,
                    def->category_key);
            }
            return true;
        }

        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL: {
            if (!data) return false;
            const struct retro_core_options_v2_intl* intl = (const struct retro_core_options_v2_intl*)data;
            // Frontend language is English, so the US definitions are used directly. The `us` member is already a retro_core_options_v2, so forward it as-is.
            if (!intl->us) return false;
            return SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, intl->us);
        }

        case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION: {
            if (!data) return false;
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
            if (!msg || !msg->msg) return false;
            double seconds = msg->duration / 1000.0;

            if (msg->target == RETRO_MESSAGE_TARGET_LOG || msg->target == RETRO_MESSAGE_TARGET_ALL) {
                switch (msg->level) {
                    case RETRO_LOG_DEBUG: SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Core] %s", msg->msg); break;
                    case RETRO_LOG_WARN:  SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Core] %s", msg->msg); break;
                    case RETRO_LOG_ERROR: SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[Core] %s", msg->msg); break;
                    default:              SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Core] %s", msg->msg); break;
                }
            }

            if (msg->target != RETRO_MESSAGE_TARGET_LOG) {
                SDL_Libretro_OsdPush(lr, msg->msg, seconds, msg->priority, msg->type, msg->progress);
            }
            return true;
        }

        case RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE: {
            if (!data) return false;
            lr->core.disk_control = *(const struct retro_disk_control_ext_callback*)data;
            lr->core.diskControlActive = true;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT: {
            // A non-NULL full_path means LoadGame populated this for content.
            if (!data || !lr->core.gameInfoExt.full_path) return false;
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
            if (!data) return false;
            struct retro_vfs_interface_info* info = (struct retro_vfs_interface_info*)data;
            if (info->required_interface_version > 4) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "[SDL_Libretro] GET_VFS_INTERFACE: unsupported required_interface_version %u",
                            info->required_interface_version);
                return false;
            }

            // Let the core know which version of the VFS we support.
            info->required_interface_version = 4;
            info->iface = &lr->vfs_interface;
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
            if (!data) return false;
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
            if (!data) return false;
            SDL_AudioSpec spec;
            if (lr->core.audioStream && SDL_GetAudioStreamFormat(lr->core.audioStream, NULL, &spec) && spec.freq > 0) {
                // Active Device
                *(unsigned*)data = (unsigned)spec.freq;
            } else if (SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL) && spec.freq > 0) {
                // Default Device
                *(unsigned*)data = (unsigned)spec.freq;
            } else {
                // Desired Frequency
                *(unsigned*)data = SDL_LIBRETRO_AUDIO_DEFAULT_SAMPLE_RATE;
            }
            return true;
        }

        // Unimplemented
        case 26:
        case RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE:
        case RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE:
        case RETRO_ENVIRONMENT_SET_PROC_ADDRESS_CALLBACK:
            return false;

        case 41:
        case RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE:
        case 42:
        case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
        case 43:
        case RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE:
        case RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT:
        case 46:
        case RETRO_ENVIRONMENT_GET_LED_INTERFACE:
        case RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION:
            return false;

        default: {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] Unhandled environment callback: %u", cmd);
            return false;
        }
    }
}

#endif /* SDL_LIBRETRO_ENV_IMPL_ONCE */
