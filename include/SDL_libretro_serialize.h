#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_SERIALIZE_IMPL_ONCE)
#define SDL_LIBRETRO_SERIALIZE_IMPL_ONCE

/*
 * SDL_libretro - save states, core memory (SRAM/RTC/...), and rewind
 */

// Cheats

bool SDL_Libretro_SetCheat(SDL_Libretro* lr, unsigned index, bool enabled, const char* code) {
    if (!lr || !lr->core.gameLoaded) return false;
    lr->core.symbols.retro_cheat_set(index, enabled, code);
    return true;
}

void SDL_Libretro_ResetCheats(SDL_Libretro* lr) {
    if (!lr || !lr->core.gameLoaded) return;
    lr->core.symbols.retro_cheat_reset();
}

// Save States

/**
 * Retrieves the size of serialized states.
 */
size_t SDL_Libretro_GetStateSize(const SDL_Libretro* lr) {
    if (!lr || !lr->core.gameLoaded) return 0;
    return lr->core.symbols.retro_serialize_size();
}

bool SDL_Libretro_SaveState_IO(SDL_Libretro* lr, SDL_IOStream* dst, bool closeio) {
    bool ok = false;
    if (!lr || !lr->core.gameLoaded || !dst) {
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
    if (!lr || !lr->core.gameLoaded || !file) {
        SDL_SetError("SDL_libretro: Invalid SaveState arguments");
        return false;
    }

    SDL_IOStream* io = SDL_IOFromFile(file, "wb");
    if (!io) return false;
    return SDL_Libretro_SaveState_IO(lr, io, true);
}

bool SDL_Libretro_LoadState_IO(SDL_Libretro* lr, SDL_IOStream* src, bool closeio) {
    if (!lr || !lr->core.gameLoaded || !src) {
        SDL_SetError("SDL_libretro: Invalid LoadState_IO arguments");
        if (closeio && src) SDL_CloseIO(src);
        return false;
    }
    size_t size = 0;
    void* data = SDL_LoadFile_IO(src, &size, closeio);
    if (!data) return false;
    bool ok = (size > 0) && lr->core.symbols.retro_unserialize(data, size);
    SDL_free(data);
    // A load is a timeline discontinuity: drop rewind history so a subsequent rewind can't walk back across it into the pre-load timeline.
    if (ok && lr->rewindEnabled) SDL_Libretro_ClearRewind(lr);
    return ok;
}

bool SDL_Libretro_LoadState(SDL_Libretro* lr, const char* file) {
    if (!lr || !lr->core.gameLoaded || !file) {
        SDL_SetError("SDL_libretro: Invalid LoadState arguments");
        return false;
    }
    SDL_IOStream* io = SDL_IOFromFile(file, "rb");
    if (!io) return false;
    return SDL_Libretro_LoadState_IO(lr, io, true);
}

// Memory

/**
 * Get a pointer to a core memory region and its size.
 *
 * Returns the core's live buffer for the given RETRO_MEMORY_* type. The pointer is owned by the core: do not free it, it stays valid until the core is unloaded, and writing through it pokes the running game directly. Must not be called concurrently with SDL_Libretro_RunFrame().
 *
 * @param lr the libretro context.
 * @param memoryType one of the RETRO_MEMORY_* constants.
 * @param size receives the region size in bytes (set to 0 when unavailable), or NULL.
 * @returns the live region pointer, or NULL if the core exposes no such memory.
 *
 * @see SDL_Libretro_SetMemoryData()
 */
void* SDL_Libretro_GetMemoryData(const SDL_Libretro* lr, unsigned memoryType, size_t* size) {
    if (!lr || !lr->core.gameLoaded || !lr->core.symbols.retro_get_memory_data || !lr->core.symbols.retro_get_memory_size) {
        if (size) *size = 0;
        return NULL;
    }
    void* ptr = lr->core.symbols.retro_get_memory_data(memoryType);
    if (size) *size = lr->core.symbols.retro_get_memory_size(memoryType);
    return ptr;
}

/**
 * Overwrite a core memory region with caller-provided bytes.
 *
 * Copies up to the region's capacity; any extra bytes are ignored. Writes to the core's live memory, so it must not race SDL_Libretro_RunFrame().
 *
 * @param lr the libretro context.
 * @param memoryType one of the RETRO_MEMORY_* constants.
 * @param data the source bytes.
 * @param size the number of bytes available in `data`.
 * @returns true on success, false on invalid arguments or if the core exposes no such memory.
 *
 * @see SDL_Libretro_GetMemoryData()
 */
bool SDL_Libretro_SetMemoryData(SDL_Libretro* lr, unsigned memoryType, const void* data, size_t size) {
    if (!lr || !data) {
        SDL_SetError("SDL_libretro: Invalid SetMemoryData arguments");
        return false;
    }
    size_t capacity = 0;
    void* dst = SDL_Libretro_GetMemoryData(lr, memoryType, &capacity);
    if (!dst || capacity == 0) {
        SDL_SetError("SDL_libretro: Memory type %u unavailable", memoryType);
        return false;
    }
    size_t copySize = size < capacity ? size : capacity;
    SDL_memcpy(dst, data, copySize);
    return true;
}

/**
 * Free the stored memory-map descriptors, including the deep-copied addrspace
 * label strings, and reset the count.
 *
 * @internal
 */
static void SDL_Libretro_FreeMemoryMap(SDL_Libretro* lr) {
    if (lr->core.memoryMapDescriptors) {
        for (unsigned i = 0; i < lr->core.memoryMapDescriptorCount; i++) {
            SDL_free((void*)lr->core.memoryMapDescriptors[i].addrspace);
        }
        SDL_free(lr->core.memoryMapDescriptors);
        lr->core.memoryMapDescriptors = NULL;
    }
    lr->core.memoryMapDescriptorCount = 0;
}

/**
 * Get the number of memory-map descriptors the core has published.
 *
 * Cores describe their address space via RETRO_ENVIRONMENT_SET_MEMORY_MAPS for
 * the benefit of debuggers, cheat finders, and achievement runtimes. Returns 0
 * if the core published no map.
 *
 * @param lr the libretro context.
 * @returns the descriptor count.
 * @see SDL_Libretro_GetMemoryMapDescriptor()
 * @see RETRO_ENVIRONMENT_SET_MEMORY_MAPS
 */
unsigned SDL_Libretro_GetMemoryMapCount(const SDL_Libretro* lr) {
    return (lr && lr->core.gameLoaded) ? lr->core.memoryMapDescriptorCount : 0;
}

/**
 * Retrieve one memory-map descriptor by index.
 *
 * Maps a region of the emulated address space onto a host pointer. To translate a guest address that falls in this descriptor to a host pointer:
 *
 *     host = (Uint8*)ptr + offset + ((guest & ~disconnect) - start)
 *
 * Any out-parameter may be NULL if not needed. The returned `ptr` and `addrspace` are owned by the context and remain valid until the core is unloaded; do not free them.
 *
 * @param lr the libretro context.
 * @param index the descriptor index, in [0, SDL_Libretro_GetMemoryMapCount()).
 * @param flags receives the RETRO_MEMDESC_* flag bits, or NULL.
 * @param ptr receives the host base pointer of the region, or NULL.
 * @param offset receives the offset from `ptr` to the region start, or NULL.
 * @param start receives the first emulated address mapped here, or NULL.
 * @param select receives the address-decode select mask, or NULL.
 * @param disconnect receives the disconnect (ignored-bits) mask, or NULL.
 * @param len receives the region length in bytes, or NULL.
 * @param addrspace receives the address-space label (may be NULL itself), or NULL.
 * @returns true on success, false if the index is out of range or no core is loaded.
 */
bool SDL_Libretro_GetMemoryMapDescriptor(const SDL_Libretro* lr, unsigned index,
    Uint64* flags, void** ptr, size_t* offset, size_t* start,
    size_t* select, size_t* disconnect, size_t* len, const char** addrspace) {
    if (!lr || !lr->core.gameLoaded || index >= lr->core.memoryMapDescriptorCount) {
        return false;
    }
    const struct retro_memory_descriptor* d = &lr->core.memoryMapDescriptors[index];
    if (flags) *flags = d->flags;
    if (ptr) *ptr = d->ptr;
    if (offset) *offset = d->offset;
    if (start) *start = d->start;
    if (select) *select = d->select;
    if (disconnect) *disconnect = d->disconnect;
    if (len) *len = d->len;
    if (addrspace) *addrspace = d->addrspace;
    return true;
}

/**
 * Translate an emulated (guest) address to a live host pointer via the memory map.
 *
 * Walks the descriptors published by the core (RETRO_ENVIRONMENT_SET_MEMORY_MAPS)
 * and returns a pointer into the core's live memory for `address`, so debuggers,
 * cheat finders, and RAM watchers don't have to reimplement the descriptor math.
 *
 * Resolves an address that lies within a descriptor's primary mapped range
 * (`[start, start + len)`, after folding out `disconnect` bits). Mirror/aliased
 * addresses that rely on a descriptor's `select` mask to wrap back into the
 * region may return NULL rather than a guess; the result is always either a
 * correct pointer or NULL, never a wrong one.
 *
 * @param lr the libretro context.
 * @param address the emulated address to resolve.
 * @param regionRemaining if non-NULL, receives the number of contiguous bytes
 *        from `address` to the end of the matched region (0 when unmatched).
 * @returns a host pointer into the core's live memory, or NULL if no descriptor
 *          maps the address (or no core/game/map is present). Do not free it.
 * @see SDL_Libretro_GetMemoryMapDescriptor()
 * @see RETRO_ENVIRONMENT_SET_MEMORY_MAPS
 */
void* SDL_Libretro_GetMapAddress(const SDL_Libretro* lr, size_t address, size_t* regionRemaining) {
    if (regionRemaining) *regionRemaining = 0;
    if (!lr || !lr->core.gameLoaded) return NULL;

    for (unsigned i = 0; i < lr->core.memoryMapDescriptorCount; i++) {
        const struct retro_memory_descriptor* d = &lr->core.memoryMapDescriptors[i];
        if (!d->ptr || d->len == 0) continue;

        // Does this descriptor's address space contain `address`?
        if (d->select != 0) {
            if (((address ^ d->start) & d->select) != 0) continue;
        } else if (address < d->start || (address - d->start) >= d->len) {
            continue;
        }

        // Fold out disconnected bits, then offset into the host buffer.
        size_t masked = address & ~d->disconnect;
        if (masked < d->start) continue;
        size_t within = masked - d->start;
        if (within >= d->len) continue;

        if (regionRemaining) *regionRemaining = d->len - within;
        return (Uint8*)d->ptr + d->offset + within;
    }
    return NULL;
}

/**
 * Human-readable name for a libretro memory type, used in log/error messages.
 *
 * @internal
 */
static const char* SDL_Libretro_GetMemoryTypeName(unsigned memoryType) {
    switch (memoryType) {
        case RETRO_MEMORY_SAVE_RAM:   return "SRAM";
        case RETRO_MEMORY_RTC:        return "RTC";
        case RETRO_MEMORY_SYSTEM_RAM: return "system RAM";
        case RETRO_MEMORY_VIDEO_RAM:  return "video RAM";
        default:                      return "memory";
    }
}

/**
 * Write a core memory region to a stream.
 *
 * Persists the live contents of the given libretro memory type (e.g. RETRO_MEMORY_SAVE_RAM for battery saves, RETRO_MEMORY_RTC for a real-time clock). A core that exposes no such region is treated as success with nothing written, so callers don't have to special-case it.
 *
 * @param lr the libretro context.
 * @param memoryType one of the RETRO_MEMORY_* constants.
 * @param dst the destination stream.
 * @param closeio if true, close `dst` before returning (even on failure).
 * @returns true on success (including "core has no such memory"), false on a write error or invalid arguments.
 */
bool SDL_Libretro_SaveMemory_IO(SDL_Libretro* lr, unsigned memoryType, SDL_IOStream* dst, bool closeio) {
    bool ok = false;
    if (!lr || !lr->core.gameLoaded || !dst) {
        SDL_SetError("SDL_libretro: Invalid SaveMemory_IO arguments");
    } else {
        size_t size = 0;
        void* mem = SDL_Libretro_GetMemoryData(lr, memoryType, &size);
        if (!mem || size == 0) {
            ok = true; // core has no such memory; nothing to save
        } else {
            ok = (SDL_WriteIO(dst, mem, size) == size);
        }
    }
    if (closeio && dst) SDL_CloseIO(dst);
    return ok;
}

/**
 * Write a core memory region to a file.
 *
 * Convenience wrapper over SDL_Libretro_SaveMemory_IO(). No file is created when
 * the core exposes no such memory region.
 *
 * @param lr the libretro context.
 * @param memoryType one of the RETRO_MEMORY_* constants.
 * @param file the destination file path.
 * @returns true on success, false on error.
 */
bool SDL_Libretro_SaveMemory(SDL_Libretro* lr, unsigned memoryType, const char* file) {
    if (!lr || !lr->core.gameLoaded || !file) {
        SDL_SetError("SDL_libretro: Invalid SaveMemory arguments");
        return false;
    }

    size_t size = 0;
    void* mem = SDL_Libretro_GetMemoryData(lr, memoryType, &size);
    // Nothing to save, so don't create an empty file.
    if (!mem || size == 0) return true;

    SDL_IOStream* io = SDL_IOFromFile(file, "wb");
    if (!io) return false;
    bool ok = SDL_Libretro_SaveMemory_IO(lr, memoryType, io, true);
    if (ok) {
        SDL_Log("SDL_libretro: %s saved to %s (%zu bytes)",
            SDL_Libretro_GetMemoryTypeName(memoryType), file, size);
    }
    return ok;
}

/**
 * Load a core memory region from a stream.
 *
 * Copies stream contents into the live memory region for the given type. When
 * the stream and the region differ in size, only the overlapping bytes are
 * copied, so a save from a slightly different revision still loads what it can.
 *
 * @param lr the libretro context.
 * @param memoryType one of the RETRO_MEMORY_* constants.
 * @param src the source stream.
 * @param closeio if true, close `src` before returning (even on failure).
 * @returns true on success, false if the core exposes no such memory, on a read
 *          error, or on invalid arguments.
 */
bool SDL_Libretro_LoadMemory_IO(SDL_Libretro* lr, unsigned memoryType, SDL_IOStream* src, bool closeio) {
    if (!lr || !lr->core.gameLoaded || !src) {
        SDL_SetError("SDL_libretro: Invalid LoadMemory_IO arguments");
        if (closeio && src) SDL_CloseIO(src);
        return false;
    }

    // Bail before reading the stream when there's nowhere to put the data.
    size_t capacity = 0;
    if (!SDL_Libretro_GetMemoryData(lr, memoryType, &capacity) || capacity == 0) {
        SDL_SetError("SDL_libretro: Core has no %s", SDL_Libretro_GetMemoryTypeName(memoryType));
        if (closeio) SDL_CloseIO(src);
        return false;
    }

    size_t fileSize = 0;
    void* data = SDL_LoadFile_IO(src, &fileSize, closeio);
    if (!data) return false;

    // A size mismatch is the usual cause of a save that loads only partially.
    if (fileSize != capacity) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
            "SDL_libretro: %s size mismatch (file %zu bytes, region %zu bytes); loading %zu",
            SDL_Libretro_GetMemoryTypeName(memoryType), fileSize, capacity,
            fileSize < capacity ? fileSize : capacity);
    }

    // SetMemoryData clamps to the region capacity.
    bool ok = SDL_Libretro_SetMemoryData(lr, memoryType, data, fileSize);
    SDL_free(data);
    return ok;
}

/**
 * Load a core memory region from a file.
 *
 * Convenience wrapper over SDL_Libretro_LoadMemory_IO().
 *
 * @param lr the libretro context.
 * @param memoryType one of the RETRO_MEMORY_* constants.
 * @param file the source file path.
 * @returns true on success, false on error.
 */
bool SDL_Libretro_LoadMemory(SDL_Libretro* lr, unsigned memoryType, const char* file) {
    if (!lr || !lr->core.gameLoaded || !file) {
        SDL_SetError("SDL_libretro: Invalid LoadMemory arguments");
        return false;
    }
    SDL_IOStream* io = SDL_IOFromFile(file, "rb");
    if (!io) return false;
    bool ok = SDL_Libretro_LoadMemory_IO(lr, memoryType, io, true);
    if (ok) {
        SDL_Log("SDL_libretro: %s loaded from %s",
            SDL_Libretro_GetMemoryTypeName(memoryType), file);
    }
    return ok;
}

// SRAM convenience wrappers (RETRO_MEMORY_SAVE_RAM)

bool SDL_Libretro_SaveSRAM_IO(SDL_Libretro* lr, SDL_IOStream* dst, bool closeio) {
    return SDL_Libretro_SaveMemory_IO(lr, RETRO_MEMORY_SAVE_RAM, dst, closeio);
}

bool SDL_Libretro_SaveSRAM(SDL_Libretro* lr, const char* file) {
    return SDL_Libretro_SaveMemory(lr, RETRO_MEMORY_SAVE_RAM, file);
}

bool SDL_Libretro_LoadSRAM_IO(SDL_Libretro* lr, SDL_IOStream* src, bool closeio) {
    return SDL_Libretro_LoadMemory_IO(lr, RETRO_MEMORY_SAVE_RAM, src, closeio);
}

bool SDL_Libretro_LoadSRAM(SDL_Libretro* lr, const char* file) {
    return SDL_Libretro_LoadMemory(lr, RETRO_MEMORY_SAVE_RAM, file);
}

// Rewind

#ifndef SDL_LIBRETRO_REWIND_DEFAULT_MAX_BYTES
/**
 * Default ceiling on the encoded rewind history (delta data only), in bytes.
 *
 * Caps worst-case memory for cores with a large or incompressible serialize size, where the frame-count limit alone could otherwise grow the buffer to gigabytes. Override per build, or at runtime via SDL_Libretro_SetRewindMemoryLimit().
 */
#define SDL_LIBRETRO_REWIND_DEFAULT_MAX_BYTES ((size_t)256 * 1024 * 1024)
#endif

/**
 * Rewind: delta-compressed circular buffer.
 *
 * Instead of storing full serialized states, each entry holds an RLE-compressed XOR delta between consecutive captures. A single full "reference" state is kept; on rewind the delta is XOR'd back to reconstruct the previous state.
 *
 * Encoding format (operates on the XOR of two serialized states):
 *   0x00 LL HH - skip (LL | HH<<8) unchanged bytes (extended, 1-65535)
 *   0x01-0x7F - skip 1-127 unchanged bytes (short)
 *   0x80-0xFF - (tag & 0x7F)+1 literal XOR bytes follow (1-128)
 *
 * A matching run between two changed regions shorter than kMinSkip is folded into the surrounding literal run (emitted as zero XOR bytes) instead of as a skip: breaking a literal costs a skip tag plus a fresh literal header, so a skip only pays for itself once the gap is at least that long. Skips of 128-254 bytes use two short codes (2 bytes) rather than the extended form (3 bytes).
 *
 * @internal
 */
static size_t SDL_Libretro_RewindEncodeDelta(const unsigned char* cur, const unsigned char* ref, size_t len, unsigned char* out, size_t outCap) {
    // A matching gap shorter than this is cheaper to fold into a literal than to emit as a skip. A skip also forces a new literal header afterwards.
    const size_t kMinSkip = 3;

    size_t op = 0, i = 0;
    while (i < len) {
        if (cur[i] == ref[i]) {
            size_t run = 0;
            while (i < len && cur[i] == ref[i]) { i++; run++; }
            while (run > 0) {
                if (run <= 127) {
                    if (out) { if (op >= outCap) return 0; out[op] = (unsigned char)run; }
                    op++;
                    run = 0;
                } else if (run < 255) {
                    // Two short skips (2 bytes) beat one extended skip (3)
                    if (out) { if (op >= outCap) return 0; out[op] = 127; }
                    op++;
                    run -= 127;
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
            // Literal segment: a span of differing bytes that absorbs any matching gaps shorter than kMinSkip (folded as zero XOR bytes).
            size_t segStart = i;
            size_t segEnd = i;
            size_t j = i;
            for (;;) {
                while (j < len && cur[j] != ref[j]) j++;
                segEnd = j;
                if (j >= len) break;
                size_t gapStart = j;
                while (j < len && cur[j] == ref[j]) j++;
                if ((j - gapStart) >= kMinSkip || j >= len) break;
            }
            i = segEnd;
            for (size_t pos = segStart; pos < segEnd; ) {
                size_t chunk = segEnd - pos;
                if (chunk > 128) chunk = 128;
                if (out) {
                    if (op + 1 + chunk > outCap) return 0;
                    out[op] = (unsigned char)(0x80 | (chunk - 1));
                }
                op++;
                for (size_t k = pos; k < pos + chunk; k++) {
                    if (out) out[op] = cur[k] ^ ref[k];
                    op++;
                }
                pos += chunk;
            }
        }
    }
    return op;
}

/**
 * Apply an encoded XOR delta to the `state` in place, reconstructing the prior state.
 *
 * Walks the delta produced by SDL_Libretro_RewindEncodeDelta(): skip codes advance past unchanged bytes, literal codes XOR their bytes into `state`. Given the newer serialized state in `state`, this yields the older one.
 *
 * @returns true on success, false if the delta is malformed (a code runs past the
 *          end of the delta or `state`).
 * @internal
 */
static bool SDL_Libretro_RewindDecodeDelta(const unsigned char* delta, size_t deltaLen, unsigned char* state, size_t stateLen) {
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

/**
 * Enable or disable the rewind system.
 *
 * When enabled, a circular buffer of serialized core states is maintained so that setting a negative speed (via SDL_Libretro_SetSpeed()) rewinds gameplay. The buffer is allocated lazily once a game is loaded and the core's serialize size is known.
 *
 * History is bounded by two independent limits: the snapshot count (`bufferFrames`) and the delta memory budget (see SDL_Libretro_SetRewindMemoryLimit(), which defaults to SDL_LIBRETRO_REWIND_DEFAULT_MAX_BYTES at context creation). Whichever is reached first caps the rewind depth. This call does not change the memory budget, a budget you set beforehand, including 0 (unbounded), is preserved.
 *
 * @param lr the libretro context.
 * @param enabled true to enable, false to disable.
 * @param bufferFrames maximum number of state snapshots to keep (0 for a sensible default of 300, roughly 5 seconds at 60 fps).
 * @param captureInterval capture a snapshot every N frames (0 for the default of 1, i.e. every frame). Provide a larger number in order to allow a longer rewind duration.
 * @returns true on success, false on allocation failure or if the core does not support serialization.
 * @see SDL_Libretro_SetRewindMemoryLimit()
 */
bool SDL_Libretro_SetRewindEnabled(SDL_Libretro* lr, bool enabled, unsigned bufferFrames, unsigned captureInterval) {
    if (!lr) return false;

    SDL_Libretro_RewindFree(lr);

    if (!enabled) {
        lr->rewindEnabled = false;
        return true;
    }

    // Sane defaults.
    if (bufferFrames == 0) bufferFrames = 300;
    if (captureInterval == 0) captureInterval = 1;

    // Allow enabling rewind, when a core isn't loaded.
    if (!lr->core.loaded) {
        lr->rewindEnabled = true;
        lr->rewindCapacity = bufferFrames;
        lr->rewindCaptureInterval = captureInterval;
        return true;
    }

    // Figure out how large the state needs to be.
    size_t slotSize = lr->core.symbols.retro_serialize_size();
    if (slotSize == 0) {
        SDL_SetError("SDL_libretro: Core does not support serialization");
        lr->rewindEnabled = false;
        return false;
    }

    // Cores that flag their state as incomplete warn the frontend not to rely on it for frame-sensitive features (netplay, rerecording).
    if (lr->core.serializationQuirks & RETRO_SERIALIZATION_QUIRK_INCOMPLETE) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
            "SDL_libretro: Core reports incomplete serialization, so rewind may be unreliable");
    }

    // Initialize the rewind slots.
    unsigned char* ref = (unsigned char*)SDL_calloc(1, slotSize);
    unsigned char* scratch = (unsigned char*)SDL_malloc(slotSize);
    SDL_LibretroRewindDelta* entries = (SDL_LibretroRewindDelta*)SDL_calloc(bufferFrames, sizeof(*entries));
    if (!ref || !scratch || !entries) {
        SDL_free(ref);
        SDL_free(scratch);
        SDL_free(entries);
        SDL_SetError("SDL_libretro: Failed to allocate rewind buffers");
        lr->rewindEnabled = false;
        return false;
    }

    // Set the initial state.
    lr->rewindReference = ref;
    lr->rewindScratch = scratch;
    lr->rewindEntries = entries;
    lr->rewindSlotSize = slotSize;
    lr->rewindBytes = 0;
    lr->rewindCapacity = bufferFrames;
    lr->rewindCaptureInterval = captureInterval;
    lr->rewindHead = 0;
    lr->rewindCount = 0;
    lr->rewindFrameCounter = 0;
    lr->rewindEnabled = true;
    lr->rewindHasReference = false;
    lr->rewindActive = false;
    return true;
}

/**
 * Check whether the rewind system is enabled (independent of current direction).
 *
 * @param lr the libretro context.
 * @returns true if rewind capture is enabled.
 */
bool SDL_Libretro_IsRewindEnabled(const SDL_Libretro* lr) {
    return lr && lr->rewindEnabled;
}

/**
 * Check whether the core is currently rewinding.
 *
 * @param lr the libretro context.
 * @returns true if rewind is enabled and the speed is negative.
 */
bool SDL_Libretro_IsRewinding(const SDL_Libretro* lr) {
    return lr && lr->rewindEnabled && lr->speed < 0.0f;
}

/**
 * Calculates the amount of rewind time remaining in the buffer.
 *
 * @param lr the libretro context.
 * @returns seconds of gameplay that can still be rewound, or 0.0 if rewind is disabled or the buffer is empty.
 */
double SDL_Libretro_GetRewindRemaining(const SDL_Libretro* lr) {
    if (!lr || !lr->rewindEnabled || lr->rewindCount == 0) return 0.0;
    double fps = lr->core.fps > 0.0 ? lr->core.fps : 60.0;
    return (double)lr->rewindCount * (double)lr->rewindCaptureInterval / fps;
}

/**
 * Get the approximate memory currently held by the rewind buffer, in bytes.
 *
 * Includes the encoded delta history plus the fixed reference/scratch buffers and the entry table. Useful for debug overlays and tuning the memory limit.
 *
 * @param lr the libretro context.
 * @returns total bytes held by the rewind subsystem, or 0 if disabled.
 * @see SDL_Libretro_SetRewindMemoryLimit()
 */
size_t SDL_Libretro_GetRewindMemoryUsage(const SDL_Libretro* lr) {
    if (!lr || !lr->rewindEnabled) return 0;
    size_t total = lr->rewindBytes;
    if (lr->rewindReference) total += lr->rewindSlotSize;
    if (lr->rewindScratch) total += lr->rewindSlotSize;
    total += (size_t)lr->rewindCapacity * sizeof(SDL_LibretroRewindDelta);
    return total;
}

/**
 * Set the maximum number of bytes of encoded delta history to retain.
 *
 * When the stored history exceeds this budget the oldest snapshots are dropped until it fits (the most recent snapshot is always kept), trading rewind duration for bounded memory. Applies immediately. Pass 0 to remove the byte limit and rely solely on the frame-count capacity.
 *
 * The budget defaults to SDL_LIBRETRO_REWIND_DEFAULT_MAX_BYTES at context creation. Whatever value is set here, including 0 (unbounded), persists across SDL_Libretro_SetRewindEnabled() calls.
 *
 * @param lr the libretro context.
 * @param maxBytes the budget in bytes, or 0 for unbounded.
 * @see SDL_Libretro_SetRewindEnabled()
 */
void SDL_Libretro_SetRewindMemoryLimit(SDL_Libretro* lr, size_t maxBytes) {
    if (!lr) return;
    lr->rewindMaxBytes = maxBytes;
    SDL_Libretro_RewindEvictToBudget(lr);
}

/**
 * Get the current rewind memory budget in bytes (0 if unbounded).
 *
 * @param lr the libretro context.
 * @returns the byte budget, or 0 if unbounded.
 */
size_t SDL_Libretro_GetRewindMemoryLimit(const SDL_Libretro* lr) {
    return lr ? lr->rewindMaxBytes : 0;
}

/**
 * Captures the current instance state into the rewind buffer.
 *
 * @internal
 */
static void SDL_Libretro_RewindCapture(SDL_Libretro* lr) {
    if (!lr->rewindEnabled || !lr->rewindReference) return;

    lr->rewindFrameCounter++;
    if (lr->rewindFrameCounter < lr->rewindCaptureInterval) return;
    lr->rewindFrameCounter = 0;

    // A core may change its serialize size mid-session
    // (RETRO_SERIALIZATION_QUIRK_CORE_VARIABLE_SIZE). The reference/scratch
    // buffers and every stored delta are sized to the old state, so resize the
    // working buffers and drop the now-incompatible history before continuing.
    // Without this, retro_serialize() into an undersized scratch fails and rewind
    // silently stops recording.
    size_t curSize = lr->core.symbols.retro_serialize_size();
    if (curSize == 0) return;
    if (curSize != lr->rewindSlotSize) {
        unsigned char* nref = (unsigned char*)SDL_calloc(1, curSize);
        unsigned char* nscr = (unsigned char*)SDL_malloc(curSize);
        if (!nref || !nscr) {
            SDL_free(nref);
            SDL_free(nscr);
            return;
        }
        SDL_free(lr->rewindReference);
        SDL_free(lr->rewindScratch);
        lr->rewindReference = nref;
        lr->rewindScratch = nscr;
        lr->rewindSlotSize = curSize;
        SDL_Libretro_ClearRewind(lr);
    }

    if (!lr->core.symbols.retro_serialize(lr->rewindScratch, lr->rewindSlotSize)) return;

    if (!lr->rewindHasReference) {
        SDL_memcpy(lr->rewindReference, lr->rewindScratch, lr->rewindSlotSize);
        lr->rewindHasReference = true;
        return;
    }

    size_t encSize = SDL_Libretro_RewindEncodeDelta(
        lr->rewindScratch, lr->rewindReference, lr->rewindSlotSize, NULL, 0);
    if (encSize == 0) return;

    // Reuse the slot's existing allocation when it's already big enough; only (re)allocate when the new delta needs more room. At steady state this stops allocating entirely, avoiding a malloc/free on every captured frame.
    //
    // rewindBytes tracks allocated delta memory (capacity), so it stays accurate whether or not a slot is overwritten in place.
    SDL_LibretroRewindDelta* slot = &lr->rewindEntries[lr->rewindHead];
    if (slot->capacity < encSize) {
        unsigned char* data = (unsigned char*)SDL_realloc(slot->data, encSize);
        if (!data) return;
        lr->rewindBytes += encSize - slot->capacity;
        slot->data = data;
        slot->capacity = encSize;
    }
    SDL_Libretro_RewindEncodeDelta(lr->rewindScratch, lr->rewindReference, lr->rewindSlotSize, slot->data, encSize);
    slot->length = encSize;

    lr->rewindHead = (lr->rewindHead + 1) % lr->rewindCapacity;
    if (lr->rewindCount < lr->rewindCapacity) {
        lr->rewindCount++;
    }

    SDL_memcpy(lr->rewindReference, lr->rewindScratch, lr->rewindSlotSize);

    // Keep total delta memory under the configured budget by dropping the oldest snapshots; this bounds worst-case memory for large/incompressible states.
    SDL_Libretro_RewindEvictToBudget(lr);
}

/**
 * Free a single delta slot and deduct its allocation from the byte total.
 *
 * @internal
 */
static void SDL_Libretro_RewindFreeEntry(SDL_Libretro* lr, SDL_LibretroRewindDelta* entry) {
    lr->rewindBytes -= entry->capacity;
    SDL_free(entry->data);
    entry->data = NULL;
    entry->length = 0;
    entry->capacity = 0;
}

/**
 * Restore the previous snapshot from the rewind buffer (state only, no re-run).
 *
 * XORs the most recent delta back into the reference and hands it to the core via retro_unserialize. The consumed delta is freed. The caller is responsible for running the core afterwards if it needs fresh video/audio for the frame.
 *
 * @internal
 */
static bool SDL_Libretro_RewindStepState(SDL_Libretro* lr) {
    if (!lr->rewindEnabled || !lr->rewindReference || lr->rewindCount == 0) return false;

    lr->rewindHead = (lr->rewindHead == 0) ? (lr->rewindCapacity - 1) : (lr->rewindHead - 1);
    lr->rewindCount--;

    SDL_LibretroRewindDelta* entry = &lr->rewindEntries[lr->rewindHead];
    if (!entry->data || entry->length == 0) return false;

    if (!SDL_Libretro_RewindDecodeDelta(entry->data, entry->length,
            lr->rewindReference, lr->rewindSlotSize)) {
        return false;
    }

    SDL_Libretro_RewindFreeEntry(lr, entry);

    return lr->core.symbols.retro_unserialize(lr->rewindReference, lr->rewindSlotSize);
}

/**
 * Step back a single captured frame and re-run the core to emit its output.
 *
 * Decoupled from the negative-speed path so a frontend can offer manual frame-by-frame reverse stepping. Audio is muted and input neutralized during the throwaway re-run that produces the displayed frame.
 *
 * @param lr the libretro context.
 * @returns true if a frame was rewound, false if rewind is unavailable or the buffer is empty.
 */
bool SDL_Libretro_RewindStep(SDL_Libretro* lr) {
    if (!lr || !lr->core.gameLoaded || !lr->rewindEnabled) {
        SDL_SetError("SDL_libretro: Rewind is not enabled");
        return false;
    }
    if (!SDL_Libretro_RewindStepState(lr)) return false;

    lr->rewindActive = true;
    lr->core.symbols.retro_run();
    lr->rewindActive = false;
    return true;
}

/**
 * Drop the oldest snapshots until the stored delta memory fits the budget.
 *
 * Always retains at least the most recent snapshot so a non-zero budget can't empty the buffer entirely. A zero budget means unbounded (no-op).
 *
 * @internal
 */
static void SDL_Libretro_RewindEvictToBudget(SDL_Libretro* lr) {
    if (!lr->rewindEnabled || lr->rewindMaxBytes == 0 || !lr->rewindEntries) return;

    while (lr->rewindBytes > lr->rewindMaxBytes && lr->rewindCount > 1) {
        unsigned tail = (lr->rewindHead + lr->rewindCapacity - lr->rewindCount) % lr->rewindCapacity;
        SDL_Libretro_RewindFreeEntry(lr, &lr->rewindEntries[tail]);
        lr->rewindCount--;
    }
}

/**
 * Discard all recorded rewind history without disabling rewind.
 *
 * Keeps the buffer allocated; the next captured frame starts a fresh reference. Called both by the public API and internally after a discontinuity (state load, reset, serialize-size change) so rewinding can't walk back across it into a stale timeline.
 *
 * @param lr the libretro context.
 */
void SDL_Libretro_ClearRewind(SDL_Libretro* lr) {
    if (!lr) return;
    if (lr->rewindEntries) {
        for (unsigned i = 0; i < lr->rewindCapacity; i++) {
            SDL_free(lr->rewindEntries[i].data);
            lr->rewindEntries[i].data = NULL;
            lr->rewindEntries[i].length = 0;
            lr->rewindEntries[i].capacity = 0;
        }
    }
    lr->rewindBytes = 0;
    lr->rewindHead = 0;
    lr->rewindCount = 0;
    lr->rewindFrameCounter = 0;
    lr->rewindHasReference = false;
}

/**
 * Frees the entire rewind buffer.
 *
 * @internal
 */
static void SDL_Libretro_RewindFree(SDL_Libretro* lr) {
    // Clear the rewind data.
    SDL_Libretro_ClearRewind(lr);

    // Free all of the allocated memory.
    SDL_free(lr->rewindEntries);
    lr->rewindEntries = NULL;
    SDL_free(lr->rewindReference);
    lr->rewindReference = NULL;
    SDL_free(lr->rewindScratch);
    lr->rewindScratch = NULL;
    lr->rewindSlotSize = 0;
    lr->rewindActive = false;
}

#endif /* SDL_LIBRETRO_SERIALIZE_IMPL_ONCE */
