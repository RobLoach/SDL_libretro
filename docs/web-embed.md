# Web Embed

The Emscripten build embeds into any webpage with [`SDL_libretro_embed.js`](../example/SDL_libretro_embed.js). Serve the build output (`SDL_libretro.js`, `SDL_libretro.wasm` and `SDL_libretro.data`) together with `SDL_libretro_embed.js`, include the script, and point it at an element on the page:

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
- `canvasId`: The `id` of the created canvas (default `canvas`)
- `onReady`: Called with the embed handle once the runtime is initialized
- `onError`: Called with an `Error` when the script fails to load

The emulator takes keyboard input while its canvas has focus, which clicking it gives, so the rest of the page keeps its keyboard.

See [`example/SDL_libretro_embed.html`](../example/SDL_libretro_embed.html) for a complete page. The Emscripten build copies both files next to its output, so the build directory is directly servable. One embed can run per page.
