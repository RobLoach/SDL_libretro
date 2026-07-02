/**
 * Drag & drop support for the Emscripten (web) build of SDL_libretro_demo.
 */
Module.installDemoDrop = function (appPtr) {
    function loadDroppedFile(file) {
        var reader = new FileReader();
        reader.onload = function () {
            var path = '/' + file.name;
            try {
                Module.FS.writeFile(path, new Uint8Array(reader.result));
            } catch (err) {
                console.error('SDL_libretro: failed to write dropped file to FS:', err);
                return;
            }
            Module.ccall('SDL_Libretro_DemoDropFile', null, ['number', 'string'], [appPtr, path]);
        };
        reader.onerror = function () {
            console.error('SDL_libretro: failed to read dropped file:', reader.error);
        };
        reader.readAsArrayBuffer(file);
    }

    function onDrop(e) {
        e.preventDefault();
        e.stopPropagation();
        var files = e.dataTransfer && e.dataTransfer.files;
        if (files && files.length > 0) {
            loadDroppedFile(files[0]);
        }
    }

    function onDragOver(e) {
        e.preventDefault();
        e.stopPropagation();
        e.dataTransfer.dropEffect = 'copy';
    }

    window.addEventListener('dragover', onDragOver, false);
    window.addEventListener('drop', onDrop, false);
};
