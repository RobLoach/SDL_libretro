# Building

```sh
git clone --recurse-submodules https://github.com/RobLoach/SDL_libretro.git
cd SDL_libretro
mkdir build && cd build
cmake ..
cmake --build .
```

To build a release instead, download its `SDL_libretro-<version>.tar.gz`, which includes the git submodules. GitHub's generated "Source code" archives leave them out.

An SDL3 already installed on the system is used when found (point `CMAKE_PREFIX_PATH` at it if it's somewhere CMake doesn't look). Otherwise, SDL3 is fetched and built via CMake FetchContent.

With `SDL_LIBRETRO_DOWNLOAD_CORES` enabled (default for native builds), a few libretro cores are downloaded into the build directory for testing:

- [snes9x](https://github.com/libretro/snes9x) (Super Nintendo)
- [fceumm](https://github.com/libretro/libretro-fceumm) (Nintendo Entertainment System)
- [genesis_plus_gx](https://github.com/libretro/Genesis-Plus-GX) (Sega Genesis / Master System / Game Gear)

See [the example app](example.md) for where it looks for those cores.

## Emscripten

With the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) activated, configure through `emcmake`:

```sh
emcmake cmake -B build-web
cmake --build build-web
```

To embed the web build in a page, see [Web Embed](web-embed.md).
