# SDL_libretro

A [libretro](https://www.libretro.com/) frontend library for [SDL3](https://libsdl.org/).

## Features

- Header-only C99 library
- Audio via `SDL_AudioStream` with dynamic rate control
- Input with gamepad, keyboard, mouse, lightgun, or pointer
- Save states and SRAM
- Core options
- Fast-forward, slow-motion, rewind
- Rumble, accelerometer, gyroscope
- Microphone input
- Disk control for multi-disc games
- Virtual file system
- Config system for saving and loading settings
- On-screen display messages
- Zip loading with [PhysicsFS](https://icculus.org/physfs/) (optionally)
- In-app menu with [nuklear_console](https://github.com/RobLoach/nuklear_console) (optionally)

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

To enable Zip Loading, link PhysFS by enabling the `SDL_LIBRETRO_PHYSFS` CMake option, and then let SDL_libretro know it's available with `SDL_LIBRETRO_ENABLE_PHYSFS`.

```c
#define SDL_LIBRETRO_IMPLEMENTATION
#define SDL_LIBRETRO_ENABLE_PHYSFS
#include "SDL_libretro.h"

// Enables loading games directly from .zip files.
SDL_Libretro_LoadGame(lr, "game.zip");
```

### Menu

To enable the in-app menu, enable the `SDL_LIBRETRO_MENU` CMake option (linking the `SDL_libretro_menu` target), and let SDL_libretro know it's available with `SDL_LIBRETRO_ENABLE_MENU`. It brings Load Game, save states, core options and settings, navigable with keyboard, mouse or gamepad. Toggle it with `F1` or the gamepad Guide button.

```c
#define SDL_LIBRETRO_IMPLEMENTATION
#define SDL_LIBRETRO_ENABLE_MENU
#include "SDL_libretro.h"

SDL_Libretro_SetRenderer(lr, renderer);
SDL_LibretroMenu* menu = SDL_Libretro_CreateMenu(lr);

// For each event...
if (!SDL_Libretro_MenuHandleEvent(menu, &event)) {
    SDL_Libretro_HandleEvent(lr, &event);
}

// Each frame...
if (!SDL_Libretro_IsMenuOpen(menu)) {
    SDL_Libretro_Update(lr);
}
SDL_Libretro_Render(renderer, lr, NULL);
SDL_Libretro_UpdateMenu(menu);
SDL_Libretro_RenderMenu(menu);
SDL_RenderPresent(renderer);
```

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
- `SDL_LIBRETRO_ENABLE_PHYSFS`: Enable .zip loading with PhysFS
- `SDL_LIBRETRO_PHYSFS_MOUNT_POINT`: The PhysFS mount point archives are mounted at (default `"game"`)
- `SDL_LIBRETRO_ENABLE_MENU`: Enable the nuklear_console menu
- `SDL_LIBRETRO_MENU_TOGGLE_KEY`: The key that toggles the menu (default `SDLK_F1`)
- `SDL_LIBRETRO_MENU_FONT_HEIGHT`: Base menu font height in pixels (default `16`)

## Dependencies

- [SDL3](https://github.com/libsdl-org/SDL) (fetched automatically if not installed)
- [libretro.h](https://github.com/libretro/libretro-common) (git submodule)
- [SDL_ini.h](https://github.com/RobLoach/SDL_ini) (included)
- [PhysicsFS](https://github.com/icculus/physfs) and [SDL_PhysFS](https://github.com/RobLoach/SDL_PhysFS) (optional)
- [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear), [nuklear_console](https://github.com/RobLoach/nuklear_console), [nuklear_gamepad](https://github.com/RobLoach/nuklear_gamepad), [c-vector](https://github.com/eteran/c-vector) and [tinydir](https://github.com/cxong/tinydir) (git submodules, optional, for the menu)

## Development

Use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to apply coding standards.
```sh
clang-format -i include/SDL_libre*
```

## License

[zlib/libpng](LICENSE)
