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

/* Rewind */

bool SDL_Libretro_SetRewindEnabled(SDL_Libretro* lr, bool enabled, unsigned bufferFrames, unsigned captureInterval) {
    if (!lr) return false;

    SDL_Libretro_RewindFree(lr);

    if (!enabled || bufferFrames == 0 || captureInterval == 0) {
        lr->rewindEnabled = false;
        return true;
    }

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

    unsigned char* buf = (unsigned char*)SDL_malloc(slotSize * bufferFrames);
    if (!buf) {
        SDL_SetError("SDL_libretro: Failed to allocate rewind buffer");
        return false;
    }

    lr->rewindBuffer = buf;
    lr->rewindSlotSize = slotSize;
    lr->rewindCapacity = bufferFrames;
    lr->rewindCaptureInterval = captureInterval;
    lr->rewindHead = 0;
    lr->rewindCount = 0;
    lr->rewindFrameCounter = 0;
    lr->rewindEnabled = true;
    return true;
}

bool SDL_Libretro_IsRewinding(const SDL_Libretro* lr) {
    return lr && lr->rewindEnabled && lr->speed < 0.0f;
}

static void SDL_Libretro_RewindCapture(SDL_Libretro* lr) {
    if (!lr->rewindEnabled || !lr->rewindBuffer) return;

    lr->rewindFrameCounter++;
    if (lr->rewindFrameCounter < lr->rewindCaptureInterval) return;
    lr->rewindFrameCounter = 0;

    unsigned char* slot = lr->rewindBuffer + (lr->rewindHead * lr->rewindSlotSize);
    if (!lr->core.symbols.retro_serialize(slot, lr->rewindSlotSize)) return;

    lr->rewindHead = (lr->rewindHead + 1) % lr->rewindCapacity;
    if (lr->rewindCount < lr->rewindCapacity) {
        lr->rewindCount++;
    }
}

static bool SDL_Libretro_RewindStep(SDL_Libretro* lr) {
    if (!lr->rewindEnabled || !lr->rewindBuffer || lr->rewindCount == 0) return false;

    lr->rewindHead = (lr->rewindHead == 0) ? (lr->rewindCapacity - 1) : (lr->rewindHead - 1);
    lr->rewindCount--;

    unsigned char* slot = lr->rewindBuffer + (lr->rewindHead * lr->rewindSlotSize);
    return lr->core.symbols.retro_unserialize(slot, lr->rewindSlotSize);
}

static void SDL_Libretro_RewindFree(SDL_Libretro* lr) {
    if (lr->rewindBuffer) {
        SDL_free(lr->rewindBuffer);
        lr->rewindBuffer = NULL;
    }
    lr->rewindSlotSize = 0;
    lr->rewindCapacity = 0;
    lr->rewindHead = 0;
    lr->rewindCount = 0;
    lr->rewindFrameCounter = 0;
}

#endif /* SDL_LIBRETRO_SERIALIZE_IMPL_ONCE */
