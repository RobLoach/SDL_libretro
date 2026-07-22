/**
 * SDL_libretro.js: embeds the SDL_libretro web (Emscripten) build into any
 * element on a webpage.
 *
 * Serve the files produced by the Emscripten build (SDL_libretro_demo.js,
 * SDL_libretro_demo.wasm and SDL_libretro_demo.data) next to this script,
 * include it, and point it at an element:
 *
 *     <div id="player" style="width: 640px; height: 480px"></div>
 *     <script src="SDL_libretro.js"></script>
 *     <script>
 *         SDL_libretro.embed('#player');
 *     </script>
 *
 * Options for SDL_libretro.embed(target, options):
 *
 *     script     URL of the Emscripten-generated script ('SDL_libretro_demo.js').
 *     arguments  Argument list for the demo, e.g. ['/cores/core.wasm', '/game.rom'].
 *     pixelated  Keep upscaled pixels crisp instead of smoothed (true).
 *     canvasId   id for the created canvas, matching SDL's selector ('canvas').
 *     onReady    Called with the handle once the runtime is initialized.
 *     onError    Called with an Error when the script fails to load.
 *
 * Returns a handle with { element, canvas, module }. Emscripten builds without
 * MODULARIZE share one global Module, so only one embed can run per page.
 */
(function (global) {
    'use strict';

    var current = null;

    function resolveElement(target) {
        if (typeof target === 'string') {
            return document.querySelector(target);
        }
        return target || null;
    }

    function embed(target, options) {
        options = options || {};
        var element = resolveElement(target);
        if (element == null) {
            throw new Error('SDL_libretro.embed: target element not found');
        }
        if (current != null) {
            throw new Error('SDL_libretro.embed: only one instance can run per page');
        }

        // The canvas SDL attaches to, filling the target element. SDL's
        // Emscripten backend finds it with document.querySelector() using a CSS
        // selector (default "#canvas"), so the id must match that selector.
        var canvas = document.createElement('canvas');
        canvas.id = options.canvasId || 'canvas';
        canvas.tabIndex = -1;
        canvas.style.display = 'block';
        canvas.style.width = '100%';
        canvas.style.height = '100%';
        if (options.pixelated !== false) {
            canvas.style.imageRendering = 'pixelated';
        }
        canvas.addEventListener('contextmenu', function (event) {
            event.preventDefault();
        });
        element.appendChild(canvas);

        var handle = {
            element: element,
            canvas: canvas,
            module: {
                canvas: canvas,
                arguments: options.arguments || [],
                onRuntimeInitialized: function () {
                    if (options.onReady) {
                        options.onReady(handle);
                    }
                }
            }
        };

        // The Emscripten script reads the global Module as its configuration.
        global.Module = handle.module;

        var script = document.createElement('script');
        script.src = options.script || 'SDL_libretro_demo.js';
        script.async = true;
        script.onerror = function () {
            var error = new Error('SDL_libretro.embed: failed to load ' + script.src);
            if (options.onError) {
                options.onError(error);
            } else {
                console.error(error);
            }
        };
        document.body.appendChild(script);

        current = handle;
        return handle;
    }

    global.SDL_libretro = {
        embed: embed
    };
})(typeof window !== 'undefined' ? window : this);
