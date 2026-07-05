# SDL_libretro

A [libretro](https://www.libretro.com/) frontend library for [SDL3](https://libsdl.org/).

## Features

- Header-only C99 library
- Audio via `SDL_AudioStream` with dynamic rate control
- Input with gamepad, keyboard, mouse, lightgun, or pointer
- Save states and SRAM
- Memory access and memory-map descriptors (cheats, debuggers, RAM watching)
- Core options
- Fast-forward and slow-motion
- Rewind with delta compression
- Rumble
- On-screen display messages

## Usage

Define `SDL_LIBRETRO_IMPLEMENTATION` in exactly one `.c` file before including the header:

```c
#define SDL_LIBRETRO_IMPLEMENTATION
#include "SDL_libretro.h"
```

All other files include `SDL_libretro.h` normally without the define.

### Quickstart

```c
SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);
SDL_Window* window = SDL_CreateWindow("SDL_libretro", 800, 600, SDL_WINDOW_RESIZABLE);
SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

SDL_Libretro* lr = SDL_Libretro_Create();
SDL_Libretro_LoadCore(lr, "core.so");
SDL_Libretro_LoadGame(lr, "game.rom");

while (!SDL_Libretro_ShouldQuit(lr)) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        SDL_Libretro_HandleEvent(lr, &event);
    }

    SDL_Libretro_Update(lr);

    SDL_RenderClear(renderer);
    SDL_Libretro_Render(renderer, lr, NULL);
    SDL_RenderPresent(renderer);
}

SDL_Libretro_Destroy(lr);
```

- [API Documentation](https://robloach.github.io/SDL_libretro/)
- [SDL_libretro_basic Example](example/SDL_libretro_basic.c)
- [Demo](https://robloach.github.io/SDL_libretro/demo/)

### Loading from `.zip` archives

Optional zip support is provided by `SDL_libretro_minizip.h` (built on
[SDL_minizip](https://github.com/RobLoach/SDL_minizip) and
[minizip-ng](https://github.com/zlib-ng/minizip-ng)). Include it after
`SDL_libretro.h` in your implementation file, and enable
`SDL_LIBRETRO_ENABLE_MINIZIP` when configuring so the build links minizip-ng:

```c
#define SDL_LIBRETRO_IMPLEMENTATION
#include "SDL_libretro.h"
#include "SDL_libretro_minizip.h"

// Auto-detects the content file inside the archive, or name it explicitly.
SDL_Libretro_LoadGame_Zip(lr, "game.zip");
SDL_Libretro_LoadGame_ZipEntry(lr, "game.zip", "roms/game.sfc");
```

The archive stays mounted as a read-only VFS layer for the life of the game, so
the core can read the ROM and companion files (BIOS, multi-disk content, …)
directly from the `.zip` through the libretro VFS, including opening files,
querying sizes, and listing directories. Cores that bypass the libretro VFS and
open files with raw stdio are not supported.

See [SDL_libretro_zip.c](example/SDL_libretro_zip.c) for a complete example.

## Build

```sh
git clone --recurse-submodules https://github.com/RobLoach/SDL_libretro.git
cd SDL_libretro
mkdir build && cd build
cmake ..
cmake --build .
```

SDL3 is fetched automatically via CMake FetchContent if not already installed on the system.

### Emscripten

With the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) activated, configure through `emcmake`:

```sh
emcmake cmake -B build-web
cmake --build build-web
```

## Configuration

Use macros before `SDL_LIBRETRO_IMPLEMENTATION` to change how SDL_Libretro behaves.

- `SDL_LIBRETRO_ENABLE_REWIND_DELTA`: Enable the XOR delta between rewind frames to reduce memory at the expense of performance
- `SDL_LIBRETRO_NO_MINIZIP_IMPLEMENTATION`: When using `SDL_libretro_minizip.h`, skip emitting the bundled SDL_minizip implementation (compile it in a separate translation unit instead)

The `SDL_LIBRETRO_ENABLE_MINIZIP` CMake option (default `OFF`) enables `.zip`
content loading; when on, minizip-ng is fetched and linked. The base library has
no minizip dependency.

## Dependencies

- [SDL3](https://github.com/libsdl-org/SDL) (fetched automatically if not installed)
- [libretro.h](https://github.com/libretro/libretro-common) (git submodule)
- [SDL_ini.h](https://github.com/RobLoach/SDL_ini) (included)
- [SDL_minizip.h](https://github.com/RobLoach/SDL_minizip) (included; optional, for `.zip` loading)
- [minizip-ng](https://github.com/zlib-ng/minizip-ng) (fetched only when `SDL_LIBRETRO_ENABLE_MINIZIP` is on)

## License

[zlib/libpng](LICENSE)
