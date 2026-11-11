# Example App

[SDL_libretro](../example/SDL_libretro.c) is the full-featured example app, with the menu ([try it online](https://robloach.github.io/SDL_libretro/demo/)).

## Paths

The example app looks for cores in the `cores` directory next to its executable, so it finds them however it's launched. Saves, system files (BIOS) and `SDL_libretro.cfg` go in the app's pref path (`SDL_GetPrefPath()`, e.g. `~/.local/share/SDL_libretro/SDL_libretro/` on Linux); `SDL_libretro.cfg` can point any of these directories elsewhere.
