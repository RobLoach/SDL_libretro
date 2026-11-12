# Configuration

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
- `SDL_LIBRETRO_MENU_TOGGLE_KEY`: The default key of the Menu hotkey, which toggles the menu (default `SDLK_F1`)
- `SDL_LIBRETRO_MENU_FONT_HEIGHT`: Base menu font height in pixels (default `13`)
- `SDL_LIBRETRO_MENU_DEFAULT_STYLE`: The initial menu theme (default `SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_MOCHA`)
- `NK_CONSOLE_FILE_NO_SDL_NATIVE_DIALOG`: Load Game with the menu's own file browser instead of the OS file dialog. Gamepads, and systems where the OS dialog can't open, use the file browser either way
- `SDL_LIBRETRO_DISABLE_PHYSFS` / `SDL_LIBRETRO_DISABLE_MENU`: Turn PhysFS or the menu back off, even when enabled
- `SDL_LIBRETRO_DISABLE_VFS`: Don't provide the SDL-backed libretro VFS, e.g. when implementing your own (PhysFS requires it)
- `SDL_LIBRETRO_NO_INI_IMPLEMENTATION`, `SDL_LIBRETRO_PHYSFS_NO_SDL_PHYSFS_IMPLEMENTATION`, `SDL_LIBRETRO_MENU_NO_NK_IMPLEMENTATION`: Skip compiling the bundled SDL_ini, SDL_PhysFS or Nuklear implementations when they're already compiled elsewhere
- `SDL_LIBRETRO_MENU_NUKLEAR_H`, `SDL_LIBRETRO_MENU_NUKLEAR_SDL3_RENDERER_H`, `SDL_LIBRETRO_MENU_NUKLEAR_GAMEPAD_H`, `SDL_LIBRETRO_MENU_NUKLEAR_CONSOLE_H`: Include paths for the menu's Nuklear headers

## Zip Loading

To enable Zip Loading, link PhysFS by enabling the `SDL_LIBRETRO_PHYSFS` CMake option, and then let SDL_libretro know it's available with `SDL_LIBRETRO_ENABLE_PHYSFS`.

```c
#define SDL_LIBRETRO_IMPLEMENTATION
#define SDL_LIBRETRO_ENABLE_PHYSFS
#include "SDL_libretro.h"

// Enables loading games directly from .zip files.
SDL_Libretro_LoadGame(lr, "game.zip");
```

## Menu

To enable the in-app menu, enable the `SDL_LIBRETRO_MENU` CMake option (linking the `SDL_libretro_menu` target), and let SDL_libretro know it's available with `SDL_LIBRETRO_ENABLE_MENU`.

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
SDL_Libretro_Render(lr, renderer, NULL);
SDL_Libretro_UpdateMenu(menu);
SDL_Libretro_RenderMenu(menu);
SDL_RenderPresent(renderer);
```

`SDL_Libretro_HandleMenuEvent()` also runs the hotkeys, which can be rebound in Settings > Hot Keys, or with `SDL_Libretro_SetMenuHotkey()` and `SDL_Libretro_SetMenuHotkeyButton()`. Rebinding a key or button unbinds any other hotkey on it, and the bindings are saved to the config (`hotkey_savestate`, `hotkey_savestate_gamepad`, and so on).

| Hotkey | Default |
| --- | --- |
| Menu | `F1`, gamepad Guide |
| Fast Forward (hold) | `Space` |
| Slow Motion (hold) | `Tab` |
| Rewind (hold) | `Backspace` |
| Save State / Load State | `F2` / `F4` |
| Screenshot | `F12` |
| Volume Up / Down | `=` / `-` |
| Fullscreen | `F11` |
| Mute, Reset, Quit | Unbound |

Disable Hotkeys (or `SDL_Libretro_SetMenuHotkeysEnabled(menu, false)`) passes every key through to the core, for cores that use them; only the Menu hotkey stays live.
