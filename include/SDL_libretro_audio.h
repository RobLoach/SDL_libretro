#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_AUDIO_IMPL_ONCE)
#define SDL_LIBRETRO_AUDIO_IMPL_ONCE

/*
 * SDL_libretro - audio subsystem
 *
 * Uses SDL3's push model. The device stream is opened with a NULL callback
 * and samples are pushed directly via SDL_PutAudioStreamData().
 */

#define SDL_LIBRETRO_AUDIO_DEFAULT_LATENCY_MS 100

static void SDL_Libretro_QueueAudio(SDL_Libretro* lr, const float* samples, int bytes) {
    if (!lr->core.audioStream || bytes <= 0) return;

    int queued = SDL_GetAudioStreamQueued(lr->core.audioStream);
    if (queued >= lr->core.audioQueueThresholdBytes) {
        if (lr->core.audioDropWarnCount < 5) {
            SDL_Log("SDL_libretro: Audio queue full (%d bytes queued), dropping %d bytes",
                queued, bytes);
            lr->core.audioDropWarnCount++;
        }
        return;
    }

    SDL_PutAudioStreamData(lr->core.audioStream, samples, bytes);
}

bool SDL_Libretro_InitAudio(SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) {
        SDL_SetError("SDL_libretro: No core loaded");
        return false;
    }

    SDL_Libretro_CloseAudio(lr);

    unsigned latencyMs = lr->core.minimumAudioLatencyMs;
    if (latencyMs == 0) latencyMs = SDL_LIBRETRO_AUDIO_DEFAULT_LATENCY_MS;
    lr->core.audioQueueThresholdBytes = (int)(lr->core.sampleRate * latencyMs / 1000.0)
        * (int)(sizeof(float) * 2);

    SDL_AudioSpec spec;
    spec.freq = (int)lr->core.sampleRate;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;

    lr->core.audioStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        NULL,
        NULL);

    if (!lr->core.audioStream) {
        SDL_SetError("SDL_libretro: Failed to open audio device: %s", SDL_GetError());
        return false;
    }

    SDL_SetAudioStreamGain(lr->core.audioStream, lr->volume);
    SDL_ResumeAudioStreamDevice(lr->core.audioStream);

    SDL_Log("SDL_libretro: Audio initialized (%.0f Hz, push model, %u ms latency threshold)",
        lr->core.sampleRate, latencyMs);

    return true;
}

void SDL_Libretro_CloseAudio(SDL_Libretro* lr) {
    if (!lr) return;

    if (lr->core.audioStream) {
        SDL_DestroyAudioStream(lr->core.audioStream);
        lr->core.audioStream = NULL;
    }
}

static void SDL_Libretro_AudioSample(int16_t left, int16_t right) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr) return;

    size_t idx = lr->core.singleSampleCount * 2;
    lr->core.singleSampleBuffer[idx] = left;
    lr->core.singleSampleBuffer[idx + 1] = right;
    lr->core.singleSampleCount++;

    if (lr->core.singleSampleCount >= SDL_LIBRETRO_AUDIO_SINGLE_SAMPLE_BUFFER_SIZE) {
        SDL_Libretro_FlushSingleSamples(lr);
    }
}

static size_t SDL_Libretro_AudioSampleBatch(const int16_t* data, size_t frames) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr || !lr->core.audioStream || frames == 0) return frames;

    if (lr->core.singleSampleCount > 0) {
        SDL_Libretro_FlushSingleSamples(lr);
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

    float convBuf[SDL_LIBRETRO_AUDIO_SINGLE_SAMPLE_BUFFER_SIZE * 2];
    for (size_t i = 0; i < lr->core.singleSampleCount * 2; i++) {
        convBuf[i] = (float)lr->core.singleSampleBuffer[i] * (1.0f / 32768.0f);
    }

    SDL_Libretro_QueueAudio(lr, convBuf,
        (int)(lr->core.singleSampleCount * sizeof(float) * 2));
    lr->core.singleSampleCount = 0;
}

#endif /* SDL_LIBRETRO_AUDIO_IMPL_ONCE */
