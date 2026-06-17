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

#endif /* SDL_LIBRETRO_SERIALIZE_IMPL_ONCE */
