#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_SERIALIZE_IMPL_ONCE)
#define SDL_LIBRETRO_SERIALIZE_IMPL_ONCE

/*
 * SDL_libretro - save state and SRAM
 */

size_t SDL_Libretro_GetStateSize(const SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) return 0;
    return lr->core.symbols.retro_serialize_size();
}

bool SDL_Libretro_SetCheat(SDL_Libretro* lr, unsigned index, bool enabled, const char* code) {
    if (!lr || !lr->core.loaded) return false;
    lr->core.symbols.retro_cheat_set(index, enabled, code);
    return true;
}

void SDL_Libretro_ResetCheats(SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) return;
    lr->core.symbols.retro_cheat_reset();
}

/* Save state */

bool SDL_Libretro_SaveState_IO(SDL_Libretro* lr, SDL_IOStream* dst, bool closeio) {
    bool ok = false;
    if (!lr || !lr->core.loaded || !dst) {
        SDL_SetError("SDL_libretro: Invalid SaveState_IO arguments");
    } else {
        size_t size = lr->core.symbols.retro_serialize_size();
        if (size == 0) {
            SDL_SetError("SDL_libretro: Core does not support save states");
        } else {
            void* data = SDL_malloc(size);
            if (data) {
                if (lr->core.symbols.retro_serialize(data, size)) {
                    ok = (SDL_WriteIO(dst, data, size) == size);
                }
                SDL_free(data);
            }
        }
    }
    if (closeio && dst) SDL_CloseIO(dst);
    return ok;
}

bool SDL_Libretro_SaveState(SDL_Libretro* lr, const char* file) {
    if (!lr || !lr->core.loaded || !file) {
        SDL_SetError("SDL_libretro: Invalid SaveState arguments");
        return false;
    }

    SDL_IOStream* io = SDL_IOFromFile(file, "wb");
    if (!io) return false;
    return SDL_Libretro_SaveState_IO(lr, io, true);
}

bool SDL_Libretro_LoadState_IO(SDL_Libretro* lr, SDL_IOStream* src, bool closeio) {
    if (!lr || !lr->core.loaded || !src) {
        SDL_SetError("SDL_libretro: Invalid LoadState_IO arguments");
        if (closeio && src) SDL_CloseIO(src);
        return false;
    }
    size_t size = 0;
    void* data = SDL_LoadFile_IO(src, &size, closeio);
    if (!data) return false;
    bool ok = (size > 0) && lr->core.symbols.retro_unserialize(data, size);
    SDL_free(data);
    return ok;
}

bool SDL_Libretro_LoadState(SDL_Libretro* lr, const char* file) {
    if (!lr || !lr->core.loaded || !file) {
        SDL_SetError("SDL_libretro: Invalid LoadState arguments");
        return false;
    }
    SDL_IOStream* io = SDL_IOFromFile(file, "rb");
    if (!io) return false;
    return SDL_Libretro_LoadState_IO(lr, io, true);
}

bool SDL_Libretro_SaveSRAM_IO(SDL_Libretro* lr, SDL_IOStream* dst, bool closeio) {
    bool ok = false;
    if (!lr || !lr->core.loaded || !dst) {
        SDL_SetError("SDL_libretro: Invalid SaveSRAM_IO arguments");
    } else {
        void* sram = lr->core.symbols.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        size_t sramSize = lr->core.symbols.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
        if (!sram || sramSize == 0) {
            ok = true; /* core has no SRAM; nothing to save */
        } else {
            ok = (SDL_WriteIO(dst, sram, sramSize) == sramSize);
        }
    }
    if (closeio && dst) SDL_CloseIO(dst);
    return ok;
}

bool SDL_Libretro_SaveSRAM(SDL_Libretro* lr, const char* file) {
    if (!lr || !lr->core.loaded || !file) {
        SDL_SetError("SDL_libretro: Invalid SaveSRAM arguments");
        return false;
    }

    void* sram = lr->core.symbols.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    size_t sramSize = lr->core.symbols.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    if (!sram || sramSize == 0) return true; /* nothing to save; don't create an empty file */

    SDL_IOStream* io = SDL_IOFromFile(file, "wb");
    if (!io) return false;
    bool ok = SDL_Libretro_SaveSRAM_IO(lr, io, true);
    if (ok) {
        SDL_Log("SDL_libretro: SRAM saved to %s (%zu bytes)", file, sramSize);
    }
    return ok;
}

bool SDL_Libretro_LoadSRAM_IO(SDL_Libretro* lr, SDL_IOStream* src, bool closeio) {
    if (!lr || !lr->core.loaded || !src) {
        SDL_SetError("SDL_libretro: Invalid LoadSRAM_IO arguments");
        if (closeio && src) SDL_CloseIO(src);
        return false;
    }

    void* sram = lr->core.symbols.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    size_t sramSize = lr->core.symbols.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    if (!sram || sramSize == 0) {
        SDL_SetError("SDL_libretro: Core has no SRAM");
        if (closeio) SDL_CloseIO(src);
        return false;
    }

    size_t fileSize = 0;
    void* data = SDL_LoadFile_IO(src, &fileSize, closeio);
    if (!data) return false;

    size_t copySize = (fileSize < sramSize) ? fileSize : sramSize;
    SDL_memcpy(sram, data, copySize);
    SDL_free(data);
    return true;
}

bool SDL_Libretro_LoadSRAM(SDL_Libretro* lr, const char* file) {
    if (!lr || !lr->core.loaded || !file) {
        SDL_SetError("SDL_libretro: Invalid LoadSRAM arguments");
        return false;
    }
    SDL_IOStream* io = SDL_IOFromFile(file, "rb");
    if (!io) return false;
    bool ok = SDL_Libretro_LoadSRAM_IO(lr, io, true);
    if (ok) {
        SDL_Log("SDL_libretro: SRAM loaded from %s", file);
    }
    return ok;
}

/* Rewind — delta-compressed circular buffer
 *
 * Instead of storing full serialized states, each entry holds an RLE-compressed
 * XOR delta between consecutive captures.  A single full "reference" state is
 * kept; on rewind the delta is XOR'd back to reconstruct the previous state.
 *
 * Encoding format (operates on the XOR of two serialized states):
 *   0x00 LL HH — skip (LL | HH<<8) unchanged bytes  (extended, 1-65535)
 *   0x01-0x7F  — skip 1-127 unchanged bytes           (short)
 *   0x80-0xFF  — (tag & 0x7F)+1 literal XOR bytes follow (1-128)
 */

static size_t SDL_Libretro_RewindEncodeDelta(
    const unsigned char* cur, const unsigned char* ref,
    size_t len, unsigned char* out, size_t outCap)
{
    size_t op = 0, i = 0;
    while (i < len) {
        if (cur[i] == ref[i]) {
            size_t start = i;
            while (i < len && cur[i] == ref[i]) i++;
            size_t run = i - start;
            while (run > 0) {
                if (run <= 127) {
                    if (out) { if (op >= outCap) return 0; out[op] = (unsigned char)run; }
                    op++;
                    run = 0;
                } else {
                    size_t chunk = run > 65535 ? 65535 : run;
                    if (out) {
                        if (op + 3 > outCap) return 0;
                        out[op]     = 0x00;
                        out[op + 1] = (unsigned char)(chunk & 0xFF);
                        out[op + 2] = (unsigned char)(chunk >> 8);
                    }
                    op += 3;
                    run -= chunk;
                }
            }
        } else {
            size_t start = i;
            while (i < len && cur[i] != ref[i] && (i - start) < 128) i++;
            size_t run = i - start;
            if (out) {
                if (op + 1 + run > outCap) return 0;
                out[op] = (unsigned char)(0x80 | (run - 1));
            }
            op++;
            for (size_t j = start; j < start + run; j++) {
                if (out) out[op] = cur[j] ^ ref[j];
                op++;
            }
        }
    }
    return op;
}

static bool SDL_Libretro_RewindDecodeDelta(
    const unsigned char* delta, size_t deltaLen,
    unsigned char* state, size_t stateLen)
{
    size_t dp = 0, sp = 0;
    while (dp < deltaLen && sp < stateLen) {
        unsigned char tag = delta[dp++];
        if (tag == 0x00) {
            if (dp + 2 > deltaLen) return false;
            size_t skip = delta[dp] | ((size_t)delta[dp + 1] << 8);
            dp += 2;
            sp += skip;
        } else if (tag <= 0x7F) {
            sp += tag;
        } else {
            size_t count = (tag & 0x7F) + 1;
            if (dp + count > deltaLen || sp + count > stateLen) return false;
            for (size_t j = 0; j < count; j++)
                state[sp++] ^= delta[dp++];
        }
    }
    return (sp <= stateLen);
}

bool SDL_Libretro_SetRewindEnabled(SDL_Libretro* lr, bool enabled, unsigned bufferFrames, unsigned captureInterval) {
    if (!lr) return false;

    SDL_Libretro_RewindFree(lr);

    if (!enabled) {
        lr->rewindEnabled = false;
        return true;
    }

    if (bufferFrames == 0) bufferFrames = 300;
    if (captureInterval == 0) captureInterval = 1;

    if (!lr->core.loaded) {
        lr->rewindEnabled = true;
        lr->rewindCapacity = bufferFrames;
        lr->rewindCaptureInterval = captureInterval;
        return true;
    }

    size_t slotSize = lr->core.symbols.retro_serialize_size();
    if (slotSize == 0) {
        SDL_SetError("SDL_libretro: Core does not support serialization");
        return false;
    }

    unsigned char* ref = (unsigned char*)SDL_calloc(1, slotSize);
    unsigned char* scratch = (unsigned char*)SDL_malloc(slotSize);
    SDL_LibretroRewindDelta* entries = (SDL_LibretroRewindDelta*)SDL_calloc(bufferFrames, sizeof(*entries));
    if (!ref || !scratch || !entries) {
        SDL_free(ref);
        SDL_free(scratch);
        SDL_free(entries);
        SDL_SetError("SDL_libretro: Failed to allocate rewind buffers");
        return false;
    }

    lr->rewindReference = ref;
    lr->rewindScratch = scratch;
    lr->rewindEntries = entries;
    lr->rewindSlotSize = slotSize;
    lr->rewindCapacity = bufferFrames;
    lr->rewindCaptureInterval = captureInterval;
    lr->rewindHead = 0;
    lr->rewindCount = 0;
    lr->rewindFrameCounter = 0;
    lr->rewindEnabled = true;
    lr->rewindHasReference = false;
    return true;
}

bool SDL_Libretro_IsRewinding(const SDL_Libretro* lr) {
    return lr && lr->rewindEnabled && lr->speed < 0.0f;
}

static void SDL_Libretro_RewindCapture(SDL_Libretro* lr) {
    if (!lr->rewindEnabled || !lr->rewindReference) return;

    lr->rewindFrameCounter++;
    if (lr->rewindFrameCounter < lr->rewindCaptureInterval) return;
    lr->rewindFrameCounter = 0;

    if (!lr->core.symbols.retro_serialize(lr->rewindScratch, lr->rewindSlotSize)) return;

    if (!lr->rewindHasReference) {
        SDL_memcpy(lr->rewindReference, lr->rewindScratch, lr->rewindSlotSize);
        lr->rewindHasReference = true;
        return;
    }

    size_t encSize = SDL_Libretro_RewindEncodeDelta(
        lr->rewindScratch, lr->rewindReference, lr->rewindSlotSize, NULL, 0);
    if (encSize == 0) return;

    unsigned char* data = (unsigned char*)SDL_malloc(encSize);
    if (!data) return;
    SDL_Libretro_RewindEncodeDelta(
        lr->rewindScratch, lr->rewindReference, lr->rewindSlotSize, data, encSize);

    SDL_free(lr->rewindEntries[lr->rewindHead].data);
    lr->rewindEntries[lr->rewindHead].data = data;
    lr->rewindEntries[lr->rewindHead].length = encSize;

    lr->rewindHead = (lr->rewindHead + 1) % lr->rewindCapacity;
    if (lr->rewindCount < lr->rewindCapacity) {
        lr->rewindCount++;
    }

    SDL_memcpy(lr->rewindReference, lr->rewindScratch, lr->rewindSlotSize);
}

static bool SDL_Libretro_RewindStep(SDL_Libretro* lr) {
    if (!lr->rewindEnabled || !lr->rewindReference || lr->rewindCount == 0) return false;

    lr->rewindHead = (lr->rewindHead == 0) ? (lr->rewindCapacity - 1) : (lr->rewindHead - 1);
    lr->rewindCount--;

    SDL_LibretroRewindDelta* entry = &lr->rewindEntries[lr->rewindHead];
    if (!entry->data || entry->length == 0) return false;

    if (!SDL_Libretro_RewindDecodeDelta(entry->data, entry->length,
            lr->rewindReference, lr->rewindSlotSize)) {
        return false;
    }

    SDL_free(entry->data);
    entry->data = NULL;
    entry->length = 0;

    return lr->core.symbols.retro_unserialize(lr->rewindReference, lr->rewindSlotSize);
}

static void SDL_Libretro_RewindFree(SDL_Libretro* lr) {
    if (lr->rewindEntries) {
        for (unsigned i = 0; i < lr->rewindCapacity; i++) {
            SDL_free(lr->rewindEntries[i].data);
        }
        SDL_free(lr->rewindEntries);
        lr->rewindEntries = NULL;
    }
    SDL_free(lr->rewindReference);
    lr->rewindReference = NULL;
    SDL_free(lr->rewindScratch);
    lr->rewindScratch = NULL;
    lr->rewindSlotSize = 0;
    lr->rewindHead = 0;
    lr->rewindCount = 0;
    lr->rewindFrameCounter = 0;
    lr->rewindHasReference = false;
}

#endif /* SDL_LIBRETRO_SERIALIZE_IMPL_ONCE */
