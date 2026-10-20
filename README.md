# SDL_libretro

A [libretro](https://www.libretro.com/) frontend library for [SDL3](https://libsdl.org/).

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

All other files include `SDL_libretro.h` normally without the define.

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
    SDL_Libretro_Render(renderer, lr, NULL);
    SDL_RenderPresent(renderer);
}

SDL_Libretro_Destroy(lr);
```

- [API Documentation](https://robloach.github.io/SDL_libretro/)
- [SDL_libretro_basic Example](example/SDL_libretro_basic.c)
- [SDL_libretro](example/SDL_libretro.c): The full-featured app, with the menu ([try it online](https://robloach.github.io/SDL_libretro/demo/))

### Cores and Content

`SDL_Libretro_SetCoreDirectory()` scans a directory for cores and their `.info` files, and returns `false` when the directory doesn't exist. The core library is sorted by core name, so when `SDL_Libretro_LoadGame()` is called with no core loaded, the first core in that order that supports the game's extension wins.

Cores that support running without content (see `SDL_Libretro_IsGameRequired()`) start with a `NULL` game:

```c
SDL_Libretro_LoadCore(lr, "core.so");
SDL_Libretro_LoadGame(lr, NULL);
```

### Input and Language

- `SDL_Libretro_SetMousePort()` / `SDL_Libretro_GetMousePort()`: The one controller port the mouse drives (default `0`). Only that port answers the mouse, pointer and lightgun devices, so one mouse doesn't act as every player.
- `SDL_Libretro_SetLanguage()` / `SDL_Libretro_GetLanguage()`: The `RETRO_LANGUAGE_*` reported to cores, detected from the system locale by default; pass `-1` to go back to detecting it. Cores usually ask while loading, so a change applies on the next core load.

### Disk Control

Multi-disc games swap disks with `SDL_Libretro_EjectDisk()`, `SDL_Libretro_SetDiskIndex()` and `SDL_Libretro_InsertDisk()`. With the tray ejected, `SDL_Libretro_AddDiskImage(lr, path)` or `SDL_Libretro_AddDiskImage_IO(lr, src, path, closeio)` appends an image; `path` is the name the image is known by. Depending on the core, it gets the image's bytes, or, when it needs a full path, a temp file the stream was written to.

### Zip Loading

To enable Zip Loading, link PhysFS by enabling the `SDL_LIBRETRO_PHYSFS` CMake option, and then let SDL_libretro know it's available with `SDL_LIBRETRO_ENABLE_PHYSFS`.

```c
#define SDL_LIBRETRO_IMPLEMENTATION
#define SDL_LIBRETRO_ENABLE_PHYSFS
#include "SDL_libretro.h"

// Enables loading games directly from .zip files.
SDL_Libretro_LoadGame(lr, "game.zip");
```

Cores that set `block_extract` in their `.info` file, or that list `zip` as a supported extension, get the raw `.zip` instead of the content inside it.

### Config

`SDL_Libretro_InitConfig(lr, org, app)` or `SDL_Libretro_InitConfigFile(lr, file)` loads settings from an `.ini` file and saves them back when the context is destroyed. Besides each core's options and the keyboard and gamepad bindings, it stores `volume`, `username`, `audiolatency`, `fitmode`, `scalemode`, `savedirectory`, `systemdirectory`, `coredirectory`, `coreassetsdirectory`, `rewindenabled`, `rewindframes`, `rewindinterval`, `rewindmemorylimit`, `mouseport`, `language` and `filebrowserdirectory`.

### Menu

To enable the in-app menu, enable the `SDL_LIBRETRO_MENU` CMake option (linking the `SDL_libretro_menu` target), and let SDL_libretro know it's available with `SDL_LIBRETRO_ENABLE_MENU`.

The menu reports what it does through SDL events (see the Events section below). Applications can also add their own entries with `SDL_Libretro_AddMenuButton()` and `SDL_Libretro_AddMenuCheckbox()`, and jump straight to a page with `SDL_Libretro_OpenMenuPath(menu, "Settings/Audio & Video")`.

Besides loading games and core options, the menu rebinds Player 1's keyboard (Settings > Keyboard) and each player's gamepad buttons and analog-to-digital D-Pad (Settings > Gamepad, or `SDL_Libretro_SetGamepadMapping()` and `SDL_Libretro_SetAnalogToDigital()` from code), swaps disks for multi-disk games (Disks), and shows core-reported progress messages.

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

Every SDL_libretro event falls between `SDL_EVENT_LIBRETRO` and `SDL_EVENT_LIBRETRO_LAST`, a fixed range in the SDL user events. The first `SDL_Libretro_Create()` reserves it through [`SDL_RegisterEvents()`](https://wiki.libsdl.org/SDL3/SDL_RegisterEvents), so events the application registers afterwards never collide with it; events registered before then should stay below `SDL_EVENT_LIBRETRO`, or a warning is logged.

- Environment commands the core calls that SDL_libretro doesn't handle itself arrive as `SDL_EVENT_LIBRETRO | RETRO_ENVIRONMENT_*` — for example `SDL_EVENT_LIBRETRO | RETRO_ENVIRONMENT_GET_CAN_DUPE`. Commands carrying the `RETRO_ENVIRONMENT_EXPERIMENTAL` (or `RETRO_ENVIRONMENT_PRIVATE`) flag need it masked off, since it doesn't fit the SDL event range: `SDL_EVENT_LIBRETRO | (RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE & ~RETRO_ENVIRONMENT_EXPERIMENTAL)`. The data pointer the core passed rides along in `event->user.data2`.
- `SDL_EVENT_LIBRETRO_CORE_LOADED` / `SDL_EVENT_LIBRETRO_GAME_LOADED`: A core or game finished loading, whether directly or through the menu. `data2` is the core or game name.
- `SDL_EVENT_LIBRETRO_CORE_UNLOADED` / `SDL_EVENT_LIBRETRO_GAME_UNLOADED`: The core or game was unloaded; unloading a core reports the game first.
- `SDL_EVENT_LIBRETRO_SHUTDOWN`: The core requested shutdown, mirroring `SDL_Libretro_ShouldQuit()`.
- `SDL_EVENT_LIBRETRO_GEOMETRY_CHANGED`: The video size, aspect ratio, or timing changed mid-game; query `SDL_Libretro_GetSize()` and friends for the new values.
- `SDL_EVENT_LIBRETRO_MENU_OPENED` / `SDL_EVENT_LIBRETRO_MENU_CLOSED`: The menu became visible (the game pauses) or was dismissed (the game resumes). `data2` is the `SDL_LibretroMenu*`.

```c
while (SDL_PollEvent(&event)) {
    switch (event.type) {
        case SDL_EVENT_LIBRETRO_GAME_LOADED:
            SDL_Log("Loaded: %s", (const char*)event.user.data2);
            break;
        case SDL_EVENT_LIBRETRO | RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE:
            SDL_Log("Core asked for location services");
            break;
    }
}
```

An environment command's `data2` pointer is only valid while the core waits inside the environment call, which has already returned by the time the event is polled. To *implement* an environment command, handle the event from an [`SDL_AddEventWatch()`](https://wiki.libsdl.org/SDL3/SDL_AddEventWatch) callback instead — watches run synchronously while the core waits. Set `event->user.code` to a non-zero value there to tell the core the command succeeded; a claimed command is consumed by the watch and doesn't also reach the event queue, while earlier unclaimed copies of the same command stay queued. An [`SDL_SetEventFilter()`](https://wiki.libsdl.org/SDL3/SDL_SetEventFilter) filter can claim a command the same way, and may return `false` to drop the event.

```c
static bool SDLCALL MyEventWatch(void* userdata, SDL_Event* event) {
    if (event->type == (SDL_EVENT_LIBRETRO | (RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE & ~RETRO_ENVIRONMENT_EXPERIMENTAL))) {
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

To build a release instead, download its `SDL_libretro-<version>.tar.gz`, which includes the git submodules. GitHub's generated "Source code" archives leave them out.

An SDL3 already installed on the system is used when found (point `CMAKE_PREFIX_PATH` at it if it's somewhere CMake doesn't look). Otherwise, SDL3 is fetched and built via CMake FetchContent.

### Emscripten

With the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) activated, configure through `emcmake`:

```sh
emcmake cmake -B build-web
cmake --build build-web
```

## Embedding on the Web

The Emscripten build embeds into any webpage with [`SDL_libretro_embed.js`](example/SDL_libretro_embed.js). Serve the build output (`SDL_libretro.js`, `SDL_libretro.wasm` and `SDL_libretro.data`) together with `SDL_libretro_embed.js`, include the script, and point it at an element on the page:

```html
<div id="player" style="width: 640px; height: 480px"></div>
<script src="SDL_libretro_embed.js"></script>
<script>
    SDL_libretro.embed('#player');
</script>
```

`SDL_libretro.embed(target, options)` accepts a CSS selector or an element, creates the canvas inside it, and starts the emulator. Options:

- `script`: URL of the Emscripten-generated script (default `SDL_libretro.js`)
- `arguments`: Argument list for SDL_libretro, e.g. `['/cores/core.wasm', '/game.rom']`
- `pixelated`: Keep upscaled pixels crisp instead of smoothed (default `true`)
- `onReady`: Called with the embed handle once the runtime is initialized
- `onError`: Called with an `Error` when the script fails to load

See [`example/SDL_libretro_embed.html`](example/SDL_libretro_embed.html) for a complete page. The Emscripten build copies both files next to its output, so the build directory is directly servable. One embed can run per page.

## Configuration

Use macros before `SDL_LIBRETRO_IMPLEMENTATION` to change how SDL_Libretro behaves.

- `SDL_LIBRETRO_MAX_USERS`: The number of controller ports (default `4`)
- `SDL_LIBRETRO_GET_TICKS_NS()`: The nanosecond clock that paces `SDL_Libretro_Update()` and the frame-time callback (default `SDL_GetTicksNS()`), e.g. to substitute a fake clock in tests
- `SDL_LIBRETRO_AUDIO_DEFAULT_LATENCY_MS`: The default audio latency threshold (default `100`)
- `SDL_LIBRETRO_AUDIO_DEFAULT_SAMPLE_RATE`: The rate used when a core reports no sample rate (default `48000`)
- `SDL_LIBRETRO_AUDIO_DRC_MAX_DELTA`: How hard dynamic rate control may bend the audio pitch (default `0.005`)
- `SDL_LIBRETRO_MIC_DEFAULT_RATE`: The default microphone sample rate (default `44100`)
- `SDL_LIBRETRO_REWIND_DEFAULT_MAX_BYTES`: The default rewind memory limit (default 256 MiB)
- `SDL_LIBRETRO_ENABLE_REWIND_DELTA`: Enable the XOR delta between rewind frames to reduce memory at the expense of performance
- `SDL_LIBRETRO_ENABLE_PHYSFS`: Enable .zip loading with PhysFS
- `SDL_LIBRETRO_PHYSFS_MOUNT_POINT`: The PhysFS mount point archives are mounted at (default `"game"`)
- `SDL_LIBRETRO_ENABLE_MENU`: Enable the nuklear_console menu
- `SDL_LIBRETRO_MENU_TOGGLE_KEY`: The key that toggles the menu (default `SDLK_F1`)
- `SDL_LIBRETRO_MENU_FONT_HEIGHT`: Base menu font height in pixels (default `13`)
- `SDL_LIBRETRO_MENU_DEFAULT_STYLE`: The initial menu theme (default `SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_MOCHA`)
- `SDL_LIBRETRO_DISABLE_PHYSFS` / `SDL_LIBRETRO_DISABLE_MENU`: Turn PhysFS or the menu back off, even when enabled
- `SDL_LIBRETRO_DISABLE_VFS`: Don't provide the SDL-backed libretro VFS, e.g. when implementing your own (PhysFS requires it)
- `SDL_LIBRETRO_NO_INI_IMPLEMENTATION`, `SDL_LIBRETRO_PHYSFS_NO_SDL_PHYSFS_IMPLEMENTATION`, `SDL_LIBRETRO_MENU_NO_NK_IMPLEMENTATION`: Skip compiling the bundled SDL_ini, SDL_PhysFS or Nuklear implementations when they're already compiled elsewhere
- `SDL_LIBRETRO_MENU_NUKLEAR_H`, `SDL_LIBRETRO_MENU_NUKLEAR_SDL3_RENDERER_H`, `SDL_LIBRETRO_MENU_NUKLEAR_GAMEPAD_H`, `SDL_LIBRETRO_MENU_NUKLEAR_CONSOLE_H`: Include paths for the menu's Nuklear headers

## Limitations

- Only one `SDL_Libretro` context can have a core loaded at a time, as libretro callbacks reach it through a single global. The API isn't thread-safe; use it from the main thread.
- SRAM isn't loaded or saved automatically; call `SDL_Libretro_LoadSRAM()` and `SDL_Libretro_SaveSRAM()` from the application ([#182](https://github.com/RobLoach/SDL_libretro/issues/182)).
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
