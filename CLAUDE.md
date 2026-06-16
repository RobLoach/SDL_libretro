# CLAUDE.md — SDL_libretro

An SDL3-based [libretro](https://www.libretro.com/) frontend library. Spiritual successor to [raylib-libretro](https://github.com/RobLoach/raylib-libretro), redesigned with SDL3-idiomatic conventions. C99, zlib/libpng license.

## Architecture

### Context-based design

All state lives in an opaque `SDL_Libretro*` context, unlike raylib-libretro's global static. Internally split into:
- `struct SDL_Libretro` — persistent settings (directories, volume, speed, keyboard map, OSD)
- `SDL_LibretroCoreData` — per-core state, zeroed on unload (symbols, texture, audio, options)

Since libretro C callbacks have no userdata parameter, a file-static `SDL_Libretro_active` pointer is set when a core loads. **Only one active context per process.**

### File layout

- `include/SDL_libretro.h` — public API header
- `src/SDL_libretro.c` — lifecycle (create, destroy, load core/game)
- `src/SDL_libretro_video.c` — texture creation, pixel format conversion, video refresh callback
- `src/SDL_libretro_audio.c` — ring buffer with atomics, SDL_AudioStream, sample callbacks
- `src/SDL_libretro_input.c` — gamepad/keyboard/mouse polling, RETROK→SDL_Scancode table
- `src/SDL_libretro_env.c` — environment callback dispatch (the big switch)
- `src/SDL_libretro_options.c` — core variables/options (dynamic arrays, not fixed-size)
- `src/SDL_libretro_serialize.c` — save state, SRAM, cheats
- `src/SDL_libretro_internal.h` — shared struct definitions and internal function declarations

### Core loading

Uses `SDL_LoadObject()` / `SDL_LoadFunction()` instead of libretro-common's `dylib.h`. Resolves all 25+ libretro symbols on load.

### Pixel formats

- `RETRO_PIXEL_FORMAT_RGB565` → `SDL_PIXELFORMAT_RGB565` (direct)
- `RETRO_PIXEL_FORMAT_XRGB8888` → `SDL_PIXELFORMAT_XRGB8888` (direct — no conversion needed, unlike raylib)
- `RETRO_PIXEL_FORMAT_0RGB1555` → software convert to RGB565

### Audio

Ring buffer with `SDL_AtomicInt` for thread safety (SDL3 audio callbacks run on a separate thread). SPSC pattern — main thread writes, audio thread reads. int16→float conversion at write time.

## Build

```sh
git submodule update --init
mkdir build && cd build && cmake .. && cmake --build .
```

SDL3 is fetched via FetchContent if not system-installed. Only `libretro.h` is needed from the libretro-common submodule.

## Coding conventions

- **Language:** C99
- **Public API:** `SDL_Libretro_*` prefix, `bool` returns with `SDL_SetError()`.
- **Memory:** `SDL_malloc` / `SDL_free` / `SDL_calloc` / `SDL_realloc`.
- **Strings:** `SDL_strlcpy`, `SDL_strcmp`, `SDL_snprintf`.
- **Logging:** `SDL_Log*` family.

## Dependencies

- **SDL3** (required) — video, audio, input, threads, atomics, file I/O, dynamic loading
- **libretro.h** (required) — API header from libretro-common submodule
- **SDL_PhysFS** (optional) — zip support via `SDL_Libretro_SetVFS()`
