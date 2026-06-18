#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_SERIALIZE_IMPL_ONCE)
#define SDL_LIBRETRO_SERIALIZE_IMPL_ONCE

/*
 * SDL_libretro - save state and SRAM
 */


#include <string.h>

size_t SDL_Libretro_GetSerializeSize(const SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) return 0;
    return lr->core.symbols.retro_serialize_size();
}

bool SDL_Libretro_Serialize(SDL_Libretro* lr, void* data, size_t size) {
    if (!lr || !lr->core.loaded || !data || size == 0) {
        SDL_SetError("SDL_libretro: Invalid serialize arguments");
        return false;
    }
    return lr->core.symbols.retro_serialize(data, size);
}

bool SDL_Libretro_Unserialize(SDL_Libretro* lr, const void* data, size_t size) {
    if (!lr || !lr->core.loaded || !data || size == 0) {
        SDL_SetError("SDL_libretro: Invalid unserialize arguments");
        return false;
    }
    return lr->core.symbols.retro_unserialize(data, size);
}

void* SDL_Libretro_GetSRAMData(const SDL_Libretro* lr, size_t* size) {
    if (!lr || !lr->core.loaded) {
        if (size) *size = 0;
        return NULL;
    }
    void* data = lr->core.symbols.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    size_t sz = lr->core.symbols.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    if (size) *size = sz;
    return data;
}

bool SDL_Libretro_SetSRAMData(SDL_Libretro* lr, const void* data, size_t size) {
    if (!lr || !lr->core.loaded || !data) {
        SDL_SetError("SDL_libretro: Invalid SRAM arguments");
        return false;
    }
    void* sram = lr->core.symbols.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    size_t sramSize = lr->core.symbols.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    if (!sram || sramSize == 0) {
        SDL_SetError("SDL_libretro: Core has no SRAM");
        return false;
    }
    size_t copySize = (size < sramSize) ? size : sramSize;
    SDL_memcpy(sram, data, copySize);
    return true;
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

/* IOStream-based save state helpers */

bool SDL_Libretro_SaveState_IO(SDL_Libretro* lr, SDL_IOStream* stream) {
    if (!lr || !lr->core.loaded || !stream) {
        SDL_SetError("SDL_libretro: Invalid SaveState_IO arguments");
        return false;
    }
    size_t size = SDL_Libretro_GetSerializeSize(lr);
    if (size == 0) {
        SDL_SetError("SDL_libretro: Core does not support save states");
        return false;
    }
    void* data = SDL_malloc(size);
    if (!data) return false;

    bool ok = SDL_Libretro_Serialize(lr, data, size);
    if (ok) {
        ok = (SDL_WriteIO(stream, data, size) == size);
    }
    SDL_free(data);
    return ok;
}

bool SDL_Libretro_LoadState_IO(SDL_Libretro* lr, SDL_IOStream* stream) {
    if (!lr || !lr->core.loaded || !stream) {
        SDL_SetError("SDL_libretro: Invalid LoadState_IO arguments");
        return false;
    }
    size_t size = SDL_Libretro_GetSerializeSize(lr);
    if (size == 0) {
        SDL_SetError("SDL_libretro: Core does not support save states");
        return false;
    }
    void* data = SDL_malloc(size);
    if (!data) return false;

    size_t bytesRead = SDL_ReadIO(stream, data, size);
    bool ok = (bytesRead > 0) && SDL_Libretro_Unserialize(lr, data, bytesRead);
    SDL_free(data);
    return ok;
}

/* SRAM file persistence */

static bool SDL_Libretro_GetSRAMPath(const SDL_Libretro* lr, char* out, size_t outSize) {
    if (!lr || lr->saveDirectory[0] == '\0' || lr->core.contentName[0] == '\0') {
        return false;
    }
    SDL_snprintf(out, outSize, "%s/%s.srm", lr->saveDirectory, lr->core.contentName);
    return true;
}

bool SDL_Libretro_SaveSRAM(SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) return false;

    size_t sramSize = 0;
    void* sram = SDL_Libretro_GetSRAMData(lr, &sramSize);
    if (!sram || sramSize == 0) return true;

    char path[SDL_LIBRETRO_MAX_PATH];
    if (!SDL_Libretro_GetSRAMPath(lr, path, sizeof(path))) return false;

    if (!SDL_CreateDirectory(lr->saveDirectory)) {
        if (SDL_GetPathInfo(lr->saveDirectory, NULL) == false) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SDL_libretro: Cannot create save directory '%s'", lr->saveDirectory);
            return false;
        }
    }

    bool ok = SDL_SaveFile(path, sram, sramSize);
    if (ok) {
        SDL_Log("SDL_libretro: SRAM saved to %s (%zu bytes)", path, sramSize);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_libretro: Failed to save SRAM to %s", path);
    }
    return ok;
}

bool SDL_Libretro_LoadSRAM(SDL_Libretro* lr) {
    if (!lr || !lr->core.loaded) return false;

    char path[SDL_LIBRETRO_MAX_PATH];
    if (!SDL_Libretro_GetSRAMPath(lr, path, sizeof(path))) return false;

    size_t fileSize = 0;
    void* data = SDL_LoadFile(path, &fileSize);
    if (!data) return true;

    bool ok = SDL_Libretro_SetSRAMData(lr, data, fileSize);
    SDL_free(data);
    if (ok) {
        SDL_Log("SDL_libretro: SRAM loaded from %s (%zu bytes)", path, fileSize);
    }
    return ok;
}

void SDL_Libretro_SetSRAMAutoSave(SDL_Libretro* lr, bool enabled) {
    if (lr) lr->sramAutoSave = enabled;
}

#endif /* SDL_LIBRETRO_SERIALIZE_IMPL_ONCE */
