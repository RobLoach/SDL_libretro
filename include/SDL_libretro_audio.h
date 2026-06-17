#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_AUDIO_IMPL_ONCE)
#define SDL_LIBRETRO_AUDIO_IMPL_ONCE

/*
 * SDL_libretro - audio subsystem
 */


static void SDL_Libretro_AudioCallback(void* userdata, SDL_AudioStream* stream,
                                        int additional_amount, int total_amount) {
    SDL_Libretro* lr = (SDL_Libretro*)userdata;
    (void)total_amount;

    if (!lr || additional_amount <= 0) return;

    int framesRequested = additional_amount / (int)(sizeof(float) * 2);
    int available = SDL_GetAtomicInt(&lr->core.audioRingAvailable);
    int framesToRead = (framesRequested < available) ? framesRequested : available;

    if (framesToRead <= 0) {
        /* Underrun: push silence */
        float* silence = (float*)SDL_calloc((size_t)framesRequested * 2, sizeof(float));
        if (silence) {
            SDL_PutAudioStreamData(stream, silence, framesRequested * (int)(sizeof(float) * 2));
            SDL_free(silence);
        }
        return;
    }

    int writePos = SDL_GetAtomicInt(&lr->core.audioRingWritePos);
    int ringSize = (int)lr->core.audioRingBufferSize;
    int readPos = (writePos - available + ringSize) % ringSize;

    /* Read from ring buffer, handling wrap-around */
    int firstChunk = ringSize - readPos;
    if (firstChunk > framesToRead) firstChunk = framesToRead;
    int secondChunk = framesToRead - firstChunk;

    SDL_PutAudioStreamData(stream, &lr->core.audioRingBuffer[readPos * 2],
        firstChunk * (int)(sizeof(float) * 2));
    if (secondChunk > 0) {
        SDL_PutAudioStreamData(stream, lr->core.audioRingBuffer,
            secondChunk * (int)(sizeof(float) * 2));
    }

    SDL_AddAtomicInt(&lr->core.audioRingAvailable, -framesToRead);

    /* Pad remaining with silence */
    int remaining = framesRequested - framesToRead;
    if (remaining > 0) {
        float* silence = (float*)SDL_calloc((size_t)remaining * 2, sizeof(float));
        if (silence) {
            SDL_PutAudioStreamData(stream, silence, remaining * (int)(sizeof(float) * 2));
            SDL_free(silence);
        }
    }
}

bool SDL_Libretro_InitAudio(SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) {
        SDL_SetError("SDL_libretro: No core loaded");
        return false;
    }

    SDL_Libretro_CloseAudio(lr);

    size_t ringSize = SDL_LIBRETRO_AUDIO_RING_BUFFER_SIZE;
    if (lr->core.minimumAudioLatencyMs > 0 && lr->core.sampleRate > 0) {
        size_t minFrames = (size_t)(lr->core.sampleRate * lr->core.minimumAudioLatencyMs / 1000.0);
        if (minFrames > ringSize) ringSize = minFrames;
    }

    /* Round up to next power of 2 for efficient modulo */
    size_t po2 = 1;
    while (po2 < ringSize) po2 <<= 1;
    ringSize = po2;

    lr->core.audioRingBuffer = (float*)SDL_calloc(ringSize * 2, sizeof(float));
    if (!lr->core.audioRingBuffer) {
        SDL_SetError("SDL_libretro: Failed to allocate audio ring buffer");
        return false;
    }
    lr->core.audioRingBufferSize = ringSize;
    SDL_SetAtomicInt(&lr->core.audioRingWritePos, 0);
    SDL_SetAtomicInt(&lr->core.audioRingAvailable, 0);

    SDL_AudioSpec spec;
    spec.freq = (int)lr->core.sampleRate;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;

    lr->core.audioStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        SDL_Libretro_AudioCallback,
        lr);

    if (!lr->core.audioStream) {
        SDL_SetError("SDL_libretro: Failed to open audio device: %s", SDL_GetError());
        SDL_free(lr->core.audioRingBuffer);
        lr->core.audioRingBuffer = NULL;
        return false;
    }

    /* Apply the current volume as stream gain so output is scaled. */
    SDL_SetAudioStreamGain(lr->core.audioStream, lr->volume);

    SDL_ResumeAudioStreamDevice(lr->core.audioStream);

    SDL_Log("SDL_libretro: Audio initialized (%.0f Hz, ring buffer %zu frames)",
        lr->core.sampleRate, ringSize);

    return true;
}

void SDL_Libretro_CloseAudio(SDL_Libretro* lr) {
    if (!lr) return;

    if (lr->core.audioStream) {
        SDL_DestroyAudioStream(lr->core.audioStream);
        lr->core.audioStream = NULL;
    }
    if (lr->core.audioRingBuffer) {
        SDL_free(lr->core.audioRingBuffer);
        lr->core.audioRingBuffer = NULL;
        lr->core.audioRingBufferSize = 0;
    }
}

static void SDL_Libretro_WriteToRingBuffer(SDL_Libretro* lr, const float* samples, size_t frames) {
    if (!lr->core.audioRingBuffer || frames == 0) return;

    size_t ringSize = lr->core.audioRingBufferSize;
    int available = SDL_GetAtomicInt(&lr->core.audioRingAvailable);

    if ((size_t)available + frames > ringSize) {
        if (lr->core.audioDropWarnCount < 5) {
            SDL_Log("SDL_libretro: Audio ring buffer full, dropping %zu frames", frames);
            lr->core.audioDropWarnCount++;
        }
        return;
    }

    int writePos = SDL_GetAtomicInt(&lr->core.audioRingWritePos);

    size_t firstChunk = ringSize - (size_t)writePos;
    if (firstChunk > frames) firstChunk = frames;
    size_t secondChunk = frames - firstChunk;

    SDL_memcpy(&lr->core.audioRingBuffer[writePos * 2], samples,
        firstChunk * sizeof(float) * 2);
    if (secondChunk > 0) {
        SDL_memcpy(lr->core.audioRingBuffer, &samples[firstChunk * 2],
            secondChunk * sizeof(float) * 2);
    }

    int newWritePos = ((size_t)writePos + frames) % ringSize;
    SDL_SetAtomicInt(&lr->core.audioRingWritePos, (int)newWritePos);
    SDL_AddAtomicInt(&lr->core.audioRingAvailable, (int)frames);
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
    if (!lr || !lr->core.audioRingBuffer || frames == 0) return frames;

    /* Flush any accumulated single samples first */
    if (lr->core.singleSampleCount > 0) {
        SDL_Libretro_FlushSingleSamples(lr);
    }

    /* Convert int16 stereo to float and write to ring buffer in chunks */
    float convBuf[512 * 2];
    size_t written = 0;
    while (written < frames) {
        size_t chunk = frames - written;
        if (chunk > 512) chunk = 512;

        for (size_t i = 0; i < chunk * 2; i++) {
            convBuf[i] = (float)data[written * 2 + i] * (1.0f / 32768.0f);
        }

        SDL_Libretro_WriteToRingBuffer(lr, convBuf, chunk);
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

    SDL_Libretro_WriteToRingBuffer(lr, convBuf, lr->core.singleSampleCount);
    lr->core.singleSampleCount = 0;
}

#endif /* SDL_LIBRETRO_AUDIO_IMPL_ONCE */
