#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_ENV_IMPL_ONCE)
#define SDL_LIBRETRO_ENV_IMPL_ONCE

/*
 * SDL_libretro - environment callback dispatch
 */


#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static void SDL_Libretro_Logger(enum retro_log_level level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[2048];
    SDL_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* Strip trailing newline */
    size_t len = SDL_strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';

    switch (level) {
        case RETRO_LOG_DEBUG: SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "CORE: %s", buf); break;
        case RETRO_LOG_INFO:  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "CORE: %s", buf); break;
        case RETRO_LOG_WARN:  SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "CORE: %s", buf); break;
        case RETRO_LOG_ERROR: SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "CORE: %s", buf); break;
        default:              SDL_Log("CORE: %s", buf); break;
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

static bool SDL_Libretro_EnvironmentCallback(unsigned cmd, void* data) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr) return false;

    unsigned baseCmd = cmd & ~RETRO_ENVIRONMENT_EXPERIMENTAL;

    switch (baseCmd) {
        case RETRO_ENVIRONMENT_SET_ROTATION: {
            if (!data) return false;
            lr->core.rotation = (int)*(const unsigned*)data;
            SDL_Log("SDL_libretro: SET_ROTATION: %d (%d deg)", lr->core.rotation, lr->core.rotation * 90);
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
            SDL_Log("SDL_libretro: SHUTDOWN requested");
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
                    /* The texture's format is fixed at creation. The load path
                     * builds it from this field; a runtime change (texture already
                     * exists) defers a rebuild to the next RunFrame. */
                    if (changed && lr->core.texture) {
                        lr->core.videoReinitPending = true;
                    }
                    return true;
                }
                default:
                    /* Unsupported: signal false without clobbering the current format. */
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
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SDL_libretro: SET_HW_RENDER not supported");
            return false;
        }

        case RETRO_ENVIRONMENT_GET_VARIABLE: {
            if (!data) return false;
            struct retro_variable* var = (struct retro_variable*)data;
            if (!var->key) return false;
            const char* value = SDL_Libretro_GetOptionValue(lr, var->key);
            if (value) {
                var->value = value;
                return true;
            }
            return false;
        }

        case RETRO_ENVIRONMENT_SET_VARIABLES: {
            if (!data) return false;
            const struct retro_variable* var = (const struct retro_variable*)data;
            for (; var->key; var++) {
                char defaultVal[512] = {0};
                char label[512] = {0};
                char valuesList[512] = {0};
                if (var->value) {
                    const char* semi = SDL_strchr(var->value, ';');
                    if (semi) {
                        size_t labelLen = (size_t)(semi - var->value);
                        while (labelLen > 0 && var->value[labelLen - 1] == ' ') labelLen--;
                        if (labelLen >= sizeof(label)) labelLen = sizeof(label) - 1;
                        SDL_memcpy(label, var->value, labelLen);

                        const char* opts = semi + 1;
                        while (*opts == ' ') opts++;
                        SDL_strlcpy(valuesList, opts, sizeof(valuesList));

                        const char* pipe = SDL_strchr(opts, '|');
                        size_t len = pipe ? (size_t)(pipe - opts) : SDL_strlen(opts);
                        if (len >= sizeof(defaultVal)) len = sizeof(defaultVal) - 1;
                        SDL_memcpy(defaultVal, opts, len);
                    }
                }
                SDL_Libretro_InitCoreOption(lr, var->key, defaultVal, label, valuesList, valuesList, "", "");
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
            cb->log = SDL_Libretro_Logger;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_PERF_INTERFACE: {
            if (!data) return false;
            struct retro_perf_callback* perf = (struct retro_perf_callback*)data;
            perf->get_time_usec = SDL_Libretro_GetTimeUSEC;
            perf->get_cpu_features = SDL_Libretro_GetCPUFeatures;
            perf->get_perf_counter = SDL_Libretro_GetPerfCounter;
            perf->perf_register = SDL_Libretro_PerfRegister;
            perf->perf_start = SDL_Libretro_PerfStart;
            perf->perf_stop = SDL_Libretro_PerfStop;
            perf->perf_log = SDL_Libretro_PerfLog;
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
            // A base-resolution change is picked up by VideoRefresh on the next
            // frame; aspect is applied at render time. Only the audio stream
            // (frequency fixed at open) needs an explicit reopen, deferred to RunFrame.
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

        case 47:
        case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE: {
            if (!data) return false;
            *(int*)data = RETRO_AV_ENABLE_VIDEO | RETRO_AV_ENABLE_AUDIO;
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

        case RETRO_ENVIRONMENT_SET_MEMORY_MAPS: {
            if (!data) return false;
            const struct retro_memory_map* map = (const struct retro_memory_map*)data;
            SDL_free(lr->core.memoryMapDescriptors);
            lr->core.memoryMapDescriptors = NULL;
            lr->core.memoryMapDescriptorCount = 0;
            if (map->num_descriptors > 0) {
                size_t sz = map->num_descriptors * sizeof(struct retro_memory_descriptor);
                lr->core.memoryMapDescriptors = (struct retro_memory_descriptor*)SDL_malloc(sz);
                if (lr->core.memoryMapDescriptors) {
                    SDL_memcpy(lr->core.memoryMapDescriptors, map->descriptors, sz);
                    lr->core.memoryMapDescriptorCount = map->num_descriptors;
                }
            }
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
            lr->core.contentInfoOverrideCount = 0;
            for (unsigned i = 0; overrides[i].extensions && i < SDL_LIBRETRO_MAX_CONTENT_INFO_OVERRIDES; i++) {
                SDL_strlcpy(lr->core.contentInfoOverrideExts[i], overrides[i].extensions,
                    SDL_LIBRETRO_CONTENT_INFO_OVERRIDE_EXTS_LEN);
                lr->core.contentInfoOverrideNeedFullpath[i] = overrides[i].need_fullpath;
                lr->core.contentInfoOverridePersistent[i] = overrides[i].persistent_data;
                lr->core.contentInfoOverrideCount++;
            }
            return true;
        }

        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2: {
            if (!data) return false;
            const struct retro_core_options_v2* opts = (const struct retro_core_options_v2*)data;
            if (opts->definitions) {
                for (unsigned i = 0; opts->definitions[i].key; i++) {
                    const struct retro_core_option_v2_definition* def = &opts->definitions[i];
                    const char* defaultVal = def->default_value ? def->default_value : "";

                    /* Build pipe-separated values list */
                    char valuesList[512] = {0};
                    size_t pos = 0;
                    for (unsigned v = 0; v < RETRO_NUM_CORE_OPTION_VALUES_MAX && def->values[v].value; v++) {
                        if (v > 0 && pos < sizeof(valuesList) - 1) valuesList[pos++] = '|';
                        pos += SDL_strlcpy(valuesList + pos, def->values[v].value, sizeof(valuesList) - pos);
                    }

                    SDL_Libretro_InitCoreOption(lr, def->key, defaultVal,
                        def->desc ? def->desc : "",
                        valuesList, valuesList,
                        def->info ? def->info : "",
                        def->category_key ? def->category_key : "");
                }
            }
            return true;
        }

        case RETRO_ENVIRONMENT_SET_MESSAGE_EXT: {
            const struct retro_message_ext* msg = (const struct retro_message_ext*)data;
            if (!msg || !msg->msg) return false;
            double seconds = msg->duration / (lr->core.fps > 0 ? lr->core.fps : 60.0);
            SDL_Libretro_SetMessage(lr, msg->msg, seconds);
            return true;
        }

        case RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE: {
            if (!data) return false;
            lr->core.disk_control = *(const struct retro_disk_control_ext_callback*)data;
            lr->core.diskControlActive = true;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT: {
            if (!data || !lr->core.gameInfoExtValid) return false;
            *(const struct retro_game_info_ext**)data = &lr->core.gameInfoExt;
            return true;
        }

        /* Unimplemented - return false */
        case RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE:
        case RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE:
        case RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE:
        case RETRO_ENVIRONMENT_SET_PROC_ADDRESS_CALLBACK:
        case RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER:
        case RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE:
        case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
        case RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE:
        case RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT:
        case RETRO_ENVIRONMENT_GET_LED_INTERFACE:
        case RETRO_ENVIRONMENT_GET_VFS_INTERFACE:
            return false;

        default: {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SDL_libretro: Unhandled env cmd %u", baseCmd);
            return false;
        }
    }
}

#endif /* SDL_LIBRETRO_ENV_IMPL_ONCE */
