# SDL_libretro

A [libretro](https://www.libretro.com/) frontend library for [SDL3](https://libsdl.org/).

![SDL_libretro logo](example/resources/SDL_libretro_icon_thumb.png)

## Features

- Header-only C99 library
- Audio input and dynamic rate control output
- Input with gamepad, keyboard, mouse, lightgun, or pointer
- Rumble, accelerometer, gyroscope
- Save states and SRAM
- Fast-forward, slow-motion, rewind
- Disk control for multi-disc games
- Virtual file system
- Core options
- Config system for saving and loading settings
- On-screen display messages
- Zip loading with [PhysicsFS](https://icculus.org/physfs/) (optional)
- Menu with [nuklear_console](https://github.com/RobLoach/nuklear_console) (optional)

## Usage

Define `SDL_LIBRETRO_IMPLEMENTATION` in exactly one `.c` file before including the header:

```c
#define SDL_LIBRETRO_IMPLEMENTATION
#include "SDL_libretro.h"
```

### Quickstart

```c
SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);
SDL_Window* window = SDL_CreateWindow("SDL_libretro", 800, 600, SDL_WINDOW_RESIZABLE);
SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

SDL_Libretro* lr = SDL_Libretro_Create();
SDL_Libretro_LoadCore(lr, "core.so");
SDL_Libretro_LoadGame(lr, "game.rom");

bool running = true;
while (running && !SDL_Libretro_ShouldQuit(lr)) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }
        SDL_Libretro_HandleEvent(lr, &event);
    }

    SDL_Libretro_Update(lr);

    SDL_RenderClear(renderer);
    SDL_Libretro_Render(lr, renderer, NULL);
    SDL_RenderPresent(renderer);
}

SDL_Libretro_Destroy(lr);
```

- [API Documentation](https://robloach.github.io/SDL_libretro/)
- [SDL_libretro_basic Example](example/SDL_libretro_basic.c)
- [SDL_libretro](example/SDL_libretro.c): The full-featured app, with the menu ([try it online](https://robloach.github.io/SDL_libretro/demo/))

### Documentation

- [Building](docs/building.md): Build options, test cores and Emscripten
- [Configuration](docs/configuration.md): Compile-time macros, Zip loading and the menu
- [Events](docs/events.md): Events SDL_libretro reports, and implementing environment commands
- [Example App](docs/example.md): Where the example app finds cores, saves and its config
- [Web Embed](docs/web-embed.md): Embedding the Emscripten build in a webpage

## Build

```sh
git clone --recurse-submodules https://github.com/RobLoach/SDL_libretro.git
cd SDL_libretro
mkdir build && cd build
cmake ..
cmake --build .
```

See [Building](docs/building.md) for more, including Emscripten.

## Limitations

- Only one `SDL_Libretro` context can have a core loaded at a time, as libretro callbacks reach it through a single global. The API isn't thread-safe; use it from the main thread.
- Web builds don't persist saves between visits ([#222](https://github.com/RobLoach/SDL_libretro/issues/222)).
- Hardware rendering (`RETRO_ENVIRONMENT_SET_HW_RENDER`) is unsupported, so cores need software rendering ([#14](https://github.com/RobLoach/SDL_libretro/issues/14)).

## Dependencies

- [SDL3](https://github.com/libsdl-org/SDL) (fetched automatically if not installed)
- [libretro.h](https://github.com/libretro/libretro-common) (git submodule)
- [SDL_ini.h](https://github.com/RobLoach/SDL_ini) (git submodule)
- [PhysicsFS](https://github.com/icculus/physfs) and [SDL_PhysFS](https://github.com/RobLoach/SDL_PhysFS) (optional)
- [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear), [nuklear_console](https://github.com/RobLoach/nuklear_console), [nuklear_gamepad](https://github.com/RobLoach/nuklear_gamepad) and [c-vector](https://github.com/eteran/c-vector) (git submodules, optional, for the menu)

## Development

Use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to apply coding standards.
```sh
clang-format -i include/SDL_libre*
```

## License

[zlib/libpng](LICENSE)
