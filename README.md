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
- Zip content loading via [PhysicsFS](https://icculus.org/physfs/) (optional)

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

### Zip Loading

Link the `SDL_libretro_physfs` CMake target (enabled with `SDL_LIBRETRO_PHYSFS`), define the implementation next to the core one, and load games through the PhysFS-aware loader:

```c
#define SDL_LIBRETRO_IMPLEMENTATION
#define SDL_LIBRETRO_PHYSFS_IMPLEMENTATION
#include "SDL_libretro.h"
#include "SDL_libretro_physfs.h"

SDL_Libretro_PhysFS_LoadGame(lr, "game.zip"); // instead of SDL_Libretro_LoadGame()
// ...
SDL_Libretro_PhysFS_Quit(lr); // before SDL_Libretro_Destroy()
```

The archive is mounted with [PhysicsFS](https://icculus.org/physfs/) (built on its SDL3 platform backend) and the game inside is picked automatically: disc metadata (`.m3u`/`.cue`) first, then a file matching the archive's name, then the first file the core's extensions accept. Byte-oriented cores receive the extracted data; `need_fullpath` cores read the virtual path through the libretro VFS, straight out of the archive. Cores flagged `block_extract` (or listing `zip` as a valid extension) receive the raw archive instead. Non-zip content passes through unchanged.

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
- `SDL_LIBRETRO_PHYSFS_MOUNT_POINT`: The PhysFS mount point archives are mounted at (default `"game"`)

## Dependencies

- [SDL3](https://github.com/libsdl-org/SDL) (fetched automatically if not installed)
- [libretro.h](https://github.com/libretro/libretro-common) (git submodule)
- [SDL_ini.h](https://github.com/RobLoach/SDL_ini) (included)
- [PhysicsFS](https://github.com/icculus/physfs) and [SDL_PhysFS](https://github.com/RobLoach/SDL_PhysFS) (fetched automatically, only with `SDL_LIBRETRO_PHYSFS`)

## License

[zlib/libpng](LICENSE)
