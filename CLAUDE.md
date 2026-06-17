# CLAUDE.md — SDL_libretro

An SDL3-based [libretro](https://www.libretro.com/) frontend library. Spiritual successor to [raylib-libretro](https://github.com/RobLoach/raylib-libretro), redesigned with SDL3-idiomatic conventions. C99, zlib/libpng license.

## Architecture

### Context-based design

All state lives in an opaque `SDL_Libretro*` context, unlike raylib-libretro's global static. Internally split into:
- `struct SDL_Libretro` — persistent settings (directories, volume, speed, keyboard map, OSD)
- `SDL_LibretroCoreData` — per-core state, zeroed on unload (symbols, texture, audio, options)

Since libretro C callbacks have no userdata parameter, a file-static `SDL_Libretro_active` pointer is set when a core loads. **Only one active context per process.**

### Header-only library

Consumers define `SDL_LIBRETRO_IMPLEMENTATION` in **exactly one** translation unit before `#include "SDL_libretro.h"`; everything else includes it normally (the example is that one TU). **Linkage split:** the public `SDL_Libretro_*` API has **external** linkage so it's callable from any TU, while internal helpers and cross-file functions are `static`. `include/SDL_libretro.h` is the umbrella: public declarations on top, then an `#ifdef SDL_LIBRETRO_IMPLEMENTATION` region holding the private structs/`#define`s, the `static SDL_Libretro_active` global, internal (`static`) forward declarations, and `#include`s of each `include/*.h` implementation fragment (lifecycle last). There is no compiled library — the CMake `SDL_libretro` target is `INTERFACE` (include paths + SDL3 link only).

### File layout

- `include/SDL_libretro.h` — umbrella: public API + private types + implementation fan-out
- `include/SDL_libretro_core.h` — lifecycle (create, destroy, load core/game)
- `include/SDL_libretro_video.h` — texture creation, pixel format conversion, video refresh callback
- `include/SDL_libretro_audio.h` — ring buffer with atomics, SDL_AudioStream, sample callbacks
- `include/SDL_libretro_input.h` — gamepad/keyboard/mouse polling, RETROK→SDL_Scancode table
- `include/SDL_libretro_env.h` — environment callback dispatch (the big switch)
- `include/SDL_libretro_options.h` — core variables/options (dynamic arrays, not fixed-size)
- `include/SDL_libretro_serialize.h` — save state, SRAM, cheats

Each `include/*.h` fragment is guarded by `#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(<FILE>_IMPL_ONCE)` and is meant to be pulled in only by the umbrella header.

**Adding a function:** a *public* one is declared non-`static` in the umbrella's public section and defined non-`static` in a fragment; an *internal* one is forward-declared `static` in the umbrella's implementation region and defined `static` in a fragment (declaration and definition linkage must match). Fragment-local helpers are plain `static` with no umbrella declaration. A new subsystem fragment must be added to the umbrella's `#include` fan-out (before `SDL_libretro_env.h` if the env switch references it).

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
