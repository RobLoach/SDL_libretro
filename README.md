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
- Zip loading with [PhysicsFS](https://icculus.org/physfs/) (optional)
- Menu with [nuklear_console](https://github.com/RobLoach/nuklear_console) (optional)

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

To enable the in-app menu, enable the `SDL_LIBRETRO_MENU` CMake option (linking the `SDL_libretro_menu` target), and let SDL_libretro know it's available with `SDL_LIBRETRO_ENABLE_MENU`.

The menu reports what it does through SDL events (see [Events](#events)). Applications can also add their own entries with `SDL_Libretro_AddMenuButton()` and `SDL_Libretro_AddMenuCheckbox()`.

```c
#define SDL_LIBRETRO_IMPLEMENTATION
#define SDL_LIBRETRO_ENABLE_MENU
#include "SDL_libretro.h"

SDL_Libretro_SetRenderer(lr, renderer);
SDL_LibretroMenu* menu = SDL_Libretro_CreateMenu(lr);

// For each event...
if (!SDL_Libretro_HandleMenuEvent(menu, &event)) {
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

### Events

SDL_libretro reports what happens through the SDL event queue, as `SDL_UserEvent`s based at `SDL_EVENT_LIBRETRO`. `event->user.data1` is always the `SDL_Libretro*` that sent the event.

- Environment commands the core calls that SDL_libretro doesn't handle itself arrive as `SDL_EVENT_LIBRETRO | RETRO_ENVIRONMENT_*` — for example `SDL_EVENT_LIBRETRO | RETRO_ENVIRONMENT_GET_CAN_DUPE`. Commands carrying the `RETRO_ENVIRONMENT_EXPERIMENTAL` flag need it stripped, which the `SDL_EVENT_LIBRETRO_ENV(cmd)` macro does for any command. The data pointer the core passed rides along in `event->user.data2`.
- `SDL_EVENT_LIBRETRO_CORE_LOADED` / `SDL_EVENT_LIBRETRO_GAME_LOADED`: A core or game finished loading, whether directly or through the menu.
- `SDL_EVENT_LIBRETRO_MENU_OPENED` / `SDL_EVENT_LIBRETRO_MENU_CLOSED`: The menu became visible (the game pauses) or was dismissed (the game resumes).

```c
while (SDL_PollEvent(&event)) {
    switch (event.type) {
        case SDL_EVENT_LIBRETRO_GAME_LOADED:
            SDL_Log("Loaded: %s", SDL_Libretro_GetGameName(event.user.data1));
            break;
        case SDL_EVENT_LIBRETRO | RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE:
            SDL_Log("Core asked for location services");
            break;
    }
}
```

An environment command's `data2` pointer is only valid while the core waits inside the environment call, which has already returned by the time the event is polled. To *implement* an environment command, handle the event from an [`SDL_AddEventWatch()`](https://wiki.libsdl.org/SDL3/SDL_AddEventWatch) callback instead — watches run synchronously while the core waits. Set `event->user.code` to a non-zero value there to tell the core the command succeeded.

```c
static bool SDLCALL MyEventWatch(void* userdata, SDL_Event* event) {
    if (event->type == SDL_EVENT_LIBRETRO_ENV(RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE)) {
        struct retro_camera_callback* camera = event->user.data2;
        // ... fill in the camera interface ...
        event->user.code = 1; // Tell the core the command succeeded.
    }
    return true;
}

SDL_AddEventWatch(MyEventWatch, NULL);
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

## Embedding on the Web

The Emscripten build embeds into any webpage with [`SDL_libretro.js`](example/SDL_libretro.js). Serve the build output (`SDL_libretro_demo.js`, `SDL_libretro_demo.wasm` and `SDL_libretro_demo.data`) together with `SDL_libretro.js`, include the script, and point it at an element on the page:

```html
<div id="player" style="width: 640px; height: 480px"></div>
<script src="SDL_libretro.js"></script>
<script>
    SDL_libretro.embed('#player');
</script>
```

`SDL_libretro.embed(target, options)` accepts a CSS selector or an element, creates the canvas inside it, and starts the emulator. Options:

- `script`: URL of the Emscripten-generated script (default `SDL_libretro_demo.js`)
- `arguments`: Argument list for the demo, e.g. `['/cores/core.wasm', '/game.rom']`
- `pixelated`: Keep upscaled pixels crisp instead of smoothed (default `true`)
- `onReady`: Called with the embed handle once the runtime is initialized
- `onError`: Called with an `Error` when the script fails to load

See [`example/SDL_libretro_embed.html`](example/SDL_libretro_embed.html) for a complete page. The Emscripten build copies both files next to its output, so the build directory is directly servable. One embed can run per page.

## Configuration

Use macros before `SDL_LIBRETRO_IMPLEMENTATION` to change how SDL_Libretro behaves.

- `SDL_LIBRETRO_ENABLE_REWIND_DELTA`: Enable the XOR delta between rewind frames to reduce memory at the expense of performance
- `SDL_LIBRETRO_ENABLE_PHYSFS`: Enable .zip loading with PhysFS
- `SDL_LIBRETRO_PHYSFS_MOUNT_POINT`: The PhysFS mount point archives are mounted at (default `"game"`)
- `SDL_LIBRETRO_ENABLE_MENU`: Enable the nuklear_console menu
- `SDL_LIBRETRO_MENU_TOGGLE_KEY`: The key that toggles the menu (default `SDLK_F1`)
- `SDL_LIBRETRO_MENU_FONT_HEIGHT`: Base menu font height in pixels (default `16`)
- `SDL_LIBRETRO_MENU_DEFAULT_STYLE`: The initial menu theme (default `SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_MOCHA`)

## Dependencies

- [SDL3](https://github.com/libsdl-org/SDL) (fetched automatically if not installed)
- [libretro.h](https://github.com/libretro/libretro-common) (git submodule)
- [SDL_ini.h](https://github.com/RobLoach/SDL_ini) (included)
- [PhysicsFS](https://github.com/icculus/physfs) and [SDL_PhysFS](https://github.com/RobLoach/SDL_PhysFS) (optional)
- [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear), [nuklear_console](https://github.com/RobLoach/nuklear_console), [nuklear_gamepad](https://github.com/RobLoach/nuklear_gamepad) and [c-vector](https://github.com/eteran/c-vector) (git submodules, optional, for the menu)

## Development

Use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to apply coding standards.
```sh
clang-format -i include/SDL_libre*
```

## License

[zlib/libpng](LICENSE)
