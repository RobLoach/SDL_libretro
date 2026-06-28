/**
 * SDL_libretro - audio subsystem
 *
 * Uses SDL3's push model. The device stream is opened with a NULL callback and samples are pushed directly via SDL_PutAudioStreamData().
 *
 * @file SDL_libretro_audio.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_AUDIO_IMPL_ONCE)
#define SDL_LIBRETRO_AUDIO_IMPL_ONCE

#ifndef SDL_LIBRETRO_AUDIO_DEFAULT_LATENCY_MS
/**
 * The threshold of audio latency in milliseconds.
 *
 * @see SDL_Libretro_GetAudioLatency()
 * @see SDL_Libretro_SetAudioLatency()
 */
#define SDL_LIBRETRO_AUDIO_DEFAULT_LATENCY_MS 100
#endif

#ifndef SDL_LIBRETRO_AUDIO_DEFAULT_SAMPLE_RATE
/**
 * Fallback rate used when a core reports no audio sample rate (<= 0). The real
 * rate, if it arrives later via SET_SYSTEM_AV_INFO, triggers a reopen.
 */
#define SDL_LIBRETRO_AUDIO_DEFAULT_SAMPLE_RATE 48000
#endif

#ifndef SDL_LIBRETRO_AUDIO_DRC_MAX_DELTA
/**
 * The amount to push the DRC to correct audio pitch.
 *
 * The default is good for general use smoothing. Increase when expecting stutter.
 *
 * @see SDL_Libretro_UpdateDRC()
 */
#define SDL_LIBRETRO_AUDIO_DRC_MAX_DELTA 0.005
#endif

static void SDL_Libretro_QueueAudio(SDL_Libretro* lr, const float* samples, int bytes) {
    if (!lr->core.audioStream || bytes <= 0) return;

    int queued = SDL_GetAudioStreamQueued(lr->core.audioStream);
    if (queued >= lr->core.audioQueueThresholdBytes) {
        // Dropping is expected back-pressure when ramping up, so this can be debug noise.
        if (lr->core.audioDropWarnCount < 10) {
            SDL_LogDebug(SDL_LOG_CATEGORY_AUDIO,
                "[SDL_Libretro] Audio queue full with %d bytes queued, dropping %d bytes",
                queued, bytes);
            lr->core.audioDropWarnCount++;
        }
        return;
    }

    SDL_PutAudioStreamData(lr->core.audioStream, samples, bytes);
}

/**
 * Convert and queue int16 stereo frames in reverse frame order.
 *
 * Used while rewinding: each captured frame's audio is emitted back-to-front,
 * and since frames are replayed newest-to-oldest the result is clean
 * time-reversed audio (the classic "rewind" sound) rather than forward chirps.
 * Chunked through the same fixed float scratch as the forward path.
 *
 * @internal
 */
static void SDL_Libretro_QueueAudioReversed(SDL_Libretro* lr, const int16_t* data, size_t frames) {
    float convBuf[512 * 2];
    size_t remaining = frames;
    while (remaining > 0) {
        size_t chunk = remaining > 512 ? 512 : remaining;
        for (size_t i = 0; i < chunk; i++) {
            size_t src = remaining - 1 - i; /* walk backward from the batch tail */
            convBuf[i * 2]     = (float)data[src * 2]     * (1.0f / 32768.0f);
            convBuf[i * 2 + 1] = (float)data[src * 2 + 1] * (1.0f / 32768.0f);
        }
        SDL_Libretro_QueueAudio(lr, convBuf, (int)(chunk * sizeof(float) * 2));
        remaining -= chunk;
    }
}

/**
 * Update the audio stream's consumption rate using dynamic-rate-control (DRC).
 *
 * The frequency ratio scales how fast SDL consumes queued input: `speed` makes
 * fast-forward/slow-mo track pitch and sample consumption, while `drcAdjustment`
 * is a tiny nudge that holds the queue near 50% fill so it never slowly drifts
 * to empty (underrun) or full (drops).
 *
 * DRC nudging is proportional-only and runs solely at normal speed. The end result
 * is smooth audio across potentially laggy frames.
 */
static void SDL_Libretro_UpdateDRC(SDL_Libretro* lr, float speed) {
    if (!lr || !lr->core.audioStream || !lr->core.drcEnabled) return;

    // Without a valid fill target, there's nothing to tweak.
    if (lr->core.audioQueueThresholdBytes <= 0) {
        SDL_SetAudioStreamFrequencyRatio(lr->core.audioStream, speed);
        return;
    }

    int queued = SDL_GetAudioStreamQueued(lr->core.audioStream);
    if (queued < 0) return; // Ignore SDL errors.

    // Hold the queue near 50% fill at every speed. Consumption already tracks `speed` (ratio = speed * adj), so slow-mo and fast-forward stay balanced and the small nudge corrects residual clock drift either way. Gating this to normal speed let the queue drift and underrun (audible in slow motion, where audio bursts are spaced farther apart).
    double drift = (double)queued / (double)lr->core.audioQueueThresholdBytes - 0.5; // > + full, - empty
    lr->core.drcDriftAvg += 0.0625 * (drift - lr->core.drcDriftAvg);
    double adj = 1.0 + SDL_LIBRETRO_AUDIO_DRC_MAX_DELTA * (2.0 * lr->core.drcDriftAvg);
    adj = SDL_clamp(adj, 1.0 - SDL_LIBRETRO_AUDIO_DRC_MAX_DELTA, 1.0 + SDL_LIBRETRO_AUDIO_DRC_MAX_DELTA);

    lr->core.drcAdjustment = (float)adj;
    SDL_SetAudioStreamFrequencyRatio(lr->core.audioStream, speed * (float)adj);
}

/**
 * Recompute the audio queue drop threshold from the current sample rate and requested latency.
 *
 * @see SDL_LIBRETRO_AUDIO_DEFAULT_LATENCY_MS
 *
 * @return The effective latency in milliseconds.
 */
static unsigned SDL_Libretro_UpdateAudioThreshold(SDL_Libretro* lr) {
    unsigned latencyMs = lr->core.minimumAudioLatencyMs;
    if (latencyMs == 0) latencyMs = SDL_LIBRETRO_AUDIO_DEFAULT_LATENCY_MS;

    int threshold = (int)(SDL_Libretro_GetSampleRate(lr) * latencyMs / 1000.0 * (double)(sizeof(float) * 2));

    // Keep at least a few audio batches of headroom so back-pressure + DRC can function; a tiny latency (or a low sample rate, where each batch spans more time) would otherwise size the queue below a single batch and drop almost everything.
    int minThreshold = 4 * (SDL_LIBRETRO_AUDIO_SINGLE_SAMPLE_BUFFER_SIZE * (int)(sizeof(float) * 2));
    lr->core.audioQueueThresholdBytes = threshold > minThreshold ? threshold : minThreshold;
    return latencyMs;
}

/**
 * Reports the audio buffer's occupancy to the core via the callback it
 * registered with RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK.
 *
 * Per the libretro spec this is invoked right before each retro_run(). A core
 * uses it to attempt frame-skipping when an underrun (audible "crackle") looms.
 *
 * @see RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK
 */
static void SDL_Libretro_ReportAudioBufferStatus(SDL_Libretro* lr) {
    if (!lr || !lr->core.audio_buffer_status.callback) return;

    /**
     * Whether the frontend's audio buffer is in use (a stream is open).
     */
    bool active = (lr->core.audioStream != NULL);

    /**
     * Queued bytes as a percentage (0-100) of the drop threshold.
     *
     * It's the frontend's effective buffer cap.
     *
     * @see SDL_Libretro_UpdateAudioThreshold
     */
    unsigned occupancy = 0;

    /**
     * Set when fewer than one frame's worth of samples remain queued, so the device is at risk of starving on the next frame.
     */
    bool underrunLikely = false;

    if (active && lr->core.audioQueueThresholdBytes > 0) {
        int queued = SDL_GetAudioStreamQueued(lr->core.audioStream);
        if (queued < 0) queued = 0;

        double pct = (double)queued * 100.0 / (double)lr->core.audioQueueThresholdBytes;
        occupancy = (unsigned)(pct > 100.0 ? 100.0 : pct);

        // One frame drains sampleRate/fps stereo float samples; if fewer than that are queued, the next frame risks starving the device.
        double fps = lr->core.fps > 0.0 ? lr->core.fps : 60.0;
        int frameBytes = (int)(SDL_Libretro_GetSampleRate(lr) / fps * (double)(sizeof(float) * 2));
        underrunLikely = (queued < frameBytes);
    }

    lr->core.audio_buffer_status.callback(active, occupancy, underrunLikely);
}

bool SDL_Libretro_InitAudio(SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) {
        SDL_SetError("[SDL_Libretro] No core loaded");
        return false;
    }

    SDL_Libretro_CloseAudio(lr);

    unsigned latencyMs = SDL_Libretro_UpdateAudioThreshold(lr);
    double sampleRate = SDL_Libretro_GetSampleRate(lr);

    SDL_AudioSpec spec;
    spec.freq = (int)sampleRate;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;

    lr->core.audioStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        NULL,
        (void*)lr);

    if (!lr->core.audioStream) {
        SDL_SetError("[SDL_Libretro] Failed to open audio device: %s", SDL_GetError());
        return false;
    }

    // Fresh drop-warning budget for this stream (InitAudio may run again on reopen).
    lr->core.audioDropWarnCount = 0;
    lr->core.audioReinitPending = false;

    // Dynamic Rate Control
    lr->core.drcAdjustment = 1.0f;
    lr->core.drcEnabled = true;
    lr->core.drcDriftAvg = 0.0;
    SDL_SetAudioStreamFrequencyRatio(lr->core.audioStream, lr->speed * lr->core.drcAdjustment);

    // Volume
    SDL_SetAudioStreamGain(lr->core.audioStream, lr->volume);
    SDL_ResumeAudioStreamDevice(lr->core.audioStream);

    // Audio Callback
    if (lr->core.audio_callback.set_state) {
        lr->core.audio_callback.set_state(true);
    }

    SDL_Log("[SDL_Libretro] Audio initialized [%d Channels @ %d Hz]", spec.channels, spec.freq);

    return true;
}

void SDL_Libretro_CloseAudio(SDL_Libretro* lr) {
    if (!lr) return;

    // Audio Callback
    if (lr->core.audio_callback.set_state) {
        lr->core.audio_callback.set_state(false);
    }

    if (lr->core.audioStream) {
        SDL_DestroyAudioStream(lr->core.audioStream);
        lr->core.audioStream = NULL;
    }
}

void SDL_Libretro_SetAudioLatency(SDL_Libretro* lr, unsigned latencyMs) {
    if (!lr) return;
    lr->core.minimumAudioLatencyMs = latencyMs;

    // When the Audio Latency changes, make sure to update the threshold too.
    if (lr->core.audioStream) {
        SDL_Libretro_UpdateAudioThreshold(lr);
    }
}

unsigned SDL_Libretro_GetAudioLatency(const SDL_Libretro* lr) {
    if (!lr) return 0;
    return lr->core.minimumAudioLatencyMs ? lr->core.minimumAudioLatencyMs : SDL_LIBRETRO_AUDIO_DEFAULT_LATENCY_MS;
}

double SDL_Libretro_GetSampleRate(const SDL_Libretro* lr) {
    return lr ? lr->core.sampleRate : SDL_LIBRETRO_AUDIO_DEFAULT_SAMPLE_RATE;
}

static void SDL_Libretro_AudioSample(int16_t left, int16_t right) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr) return;

    if (lr->core.singleSampleCount >= SDL_LIBRETRO_AUDIO_SINGLE_SAMPLE_BUFFER_SIZE) {
        SDL_Libretro_FlushSingleSamples(lr);
    }

    size_t idx = lr->core.singleSampleCount * 2;
    lr->core.singleSampleBuffer[idx] = left;
    lr->core.singleSampleBuffer[idx + 1] = right;
    lr->core.singleSampleCount++;
}

static size_t SDL_Libretro_AudioSampleBatch(const int16_t* data, size_t frames) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr || !lr->core.audioStream || frames == 0) return frames;

    if (lr->core.singleSampleCount > 0) {
        SDL_Libretro_FlushSingleSamples(lr);
    }

    // While rewinding, emit this frame's audio reversed for a clean rewind sound.
    if (lr->rewindActive) {
        SDL_Libretro_QueueAudioReversed(lr, data, frames);
        return frames;
    }

    float convBuf[512 * 2];
    size_t written = 0;
    while (written < frames) {
        size_t chunk = frames - written;
        if (chunk > 512) chunk = 512;

        for (size_t i = 0; i < chunk * 2; i++) {
            convBuf[i] = (float)data[written * 2 + i] * (1.0f / 32768.0f);
        }

        SDL_Libretro_QueueAudio(lr, convBuf, (int)(chunk * sizeof(float) * 2));
        written += chunk;
    }

    return frames;
}

static void SDL_Libretro_FlushSingleSamples(SDL_Libretro* lr) {
    if (!lr || lr->core.singleSampleCount == 0) return;

    // While rewinding, emit the buffered frames reversed for a clean rewind sound.
    if (lr->rewindActive) {
        SDL_Libretro_QueueAudioReversed(lr, lr->core.singleSampleBuffer, lr->core.singleSampleCount);
        lr->core.singleSampleCount = 0;
        return;
    }

    float convBuf[SDL_LIBRETRO_AUDIO_SINGLE_SAMPLE_BUFFER_SIZE * 2];
    for (size_t i = 0; i < lr->core.singleSampleCount * 2; i++) {
        convBuf[i] = (float)lr->core.singleSampleBuffer[i] * (1.0f / 32768.0f);
    }

    SDL_Libretro_QueueAudio(lr, convBuf,
        (int)(lr->core.singleSampleCount * sizeof(float) * 2));
    lr->core.singleSampleCount = 0;
}

// Microphone

#ifndef SDL_LIBRETRO_MIC_DEFAULT_RATE
/**
 * The default sample rate for recording microphone input.
 *
 * @see SDL_Libretro_MicOpen()
 */
#define SDL_LIBRETRO_MIC_DEFAULT_RATE 44100
#endif

static retro_microphone_t* SDL_Libretro_MicOpen(const retro_microphone_params_t* params) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr) return NULL;

    if (lr->core.microphone) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "[SDL_libretro] Microphone already open");
        return (retro_microphone_t*)lr->core.microphone;
    }

    unsigned rate = (params && params->rate > 0) ? params->rate : SDL_LIBRETRO_MIC_DEFAULT_RATE;

    SDL_AudioSpec spec;
    spec.freq = (int)rate;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 1;

    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, NULL, (void*)lr);
    if (!stream) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SDL_libretro: Failed to open microphone: %s", SDL_GetError());
        return NULL;
    }

    SDL_LibretroMicrophone* mic = (SDL_LibretroMicrophone*)SDL_calloc(1, sizeof(*mic));
    if (!mic) {
        SDL_DestroyAudioStream(stream);
        SDL_OutOfMemory();
        return NULL;
    }

    mic->stream = stream;
    mic->rate = rate;
    mic->active = false;
    mic->lr = lr;
    lr->core.microphone = mic;

    SDL_Log("SDL_libretro: Microphone opened (%u Hz)", rate);
    return (retro_microphone_t*)mic;
}

static void SDL_Libretro_MicClose(retro_microphone_t* microphone) {
    SDL_LibretroMicrophone* mic = (SDL_LibretroMicrophone*)microphone;
    if (!mic) return;

    // Dereference from the core if it's actively referencing itself.
    if (mic->lr && mic->lr->core.microphone == mic) {
        mic->lr->core.microphone = NULL;
    }

    // Destroy the stream.
    if (mic->stream) {
        SDL_DestroyAudioStream(mic->stream);
        mic->stream = NULL;
    }
    SDL_free(mic);
}

static bool SDL_Libretro_MicGetParams(const retro_microphone_t* microphone, retro_microphone_params_t* params) {
    const SDL_LibretroMicrophone* mic = (const SDL_LibretroMicrophone*)microphone;
    if (!mic || !params) return false;
    params->rate = mic->rate;
    return true;
}

static bool SDL_Libretro_MicSetState(retro_microphone_t* microphone, bool state) {
    SDL_LibretroMicrophone* mic = (SDL_LibretroMicrophone*)microphone;
    if (!mic || !mic->stream) return false;

    bool ok;
    if (state) {
        ok = SDL_ResumeAudioStreamDevice(mic->stream);
    }
    else {
        ok = SDL_PauseAudioStreamDevice(mic->stream);
    }

    // If the state changed successfully, update it accordingly.
    if (ok) {
        mic->active = state;
    }
    return ok;
}

static bool SDL_Libretro_MicGetState(const retro_microphone_t* microphone) {
    const SDL_LibretroMicrophone* mic = (const SDL_LibretroMicrophone*)microphone;
    if (!mic) return false;
    return mic->active;
}

static int SDL_Libretro_MicRead(retro_microphone_t* microphone, int16_t* samples, size_t num_samples) {
    SDL_LibretroMicrophone* mic = (SDL_LibretroMicrophone*)microphone;
    if (!mic || !mic->stream || !samples || num_samples == 0) return 0;

    int bytes = SDL_GetAudioStreamData(mic->stream, samples, (int)(num_samples * sizeof(int16_t)));
    if (bytes < 0) return 0;
    return bytes / (int)sizeof(int16_t);
}

static void SDL_Libretro_CloseMicrophone(SDL_Libretro* lr) {
    if (!lr || !lr->core.microphone) return;
    SDL_Libretro_MicClose((retro_microphone_t*)lr->core.microphone);
    lr->core.microphone = NULL;
}

#endif /* SDL_LIBRETRO_AUDIO_IMPL_ONCE */
