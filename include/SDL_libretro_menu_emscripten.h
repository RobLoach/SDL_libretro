/**
 * SDL_libretro - menu behavior for the Emscripten web build
 *
 * The browser has no native file system to browse, so Load Game opens a
 * browser file picker that copies the chosen files into the in-memory file
 * system, which is pruned of content that is no longer in use. There is also
 * nothing to quit to, so the menu has no Quit entry.
 *
 * Included by SDL_libretro_menu.h when compiled with Emscripten; it provides
 * the web variants of the menu's platform-specific functions.
 *
 * @file SDL_libretro_menu_emscripten.h
 */

#if defined(SDL_LIBRETRO_MENU_IMPL_ONCE) && defined(__EMSCRIPTEN__) && !defined(SDL_LIBRETRO_MENU_EMSCRIPTEN_IMPL_ONCE)
#define SDL_LIBRETRO_MENU_EMSCRIPTEN_IMPL_ONCE

#include <emscripten.h>

/**
 * @internal
 * Free the browser files written into the in-memory file system that no
 * longer back any content: the Load Game picks (/picked/<n>/) and SDL's
 * drag & drop copies (/tmp/filedrop/<n>/). The directories holding
 * loadedPath and pendingPath are kept, since a core may still read from, or
 * reopen, its content (e.g. the tracks of a .cue) while it runs.
 *
 * SDL writes each dropped file to its own directory and queues its drop event,
 * so drops newer than the newest one in use are left for their events.
 */
EM_JS(void, SDL_Libretro_MenuWebPruneFiles, (const char* loadedPath, const char* pendingPath), {
    var FS = Module.FS;
    var keep = [UTF8ToString(loadedPath), UTF8ToString(pendingPath)];
    ['/picked', '/tmp/filedrop'].forEach(function(root) {
        var prefix = root + '/';
        var kept = [];
        var newest = -1;
        keep.forEach(function(path) {
            if (path.indexOf(prefix) !== 0) return;
            var rest = path.substring(prefix.length);
            var slash = rest.indexOf('/');
            if (slash > 0) {
                kept.push(rest.substring(0, slash));
                newest = Math.max(newest, Number(rest.substring(0, slash)));
            }
        });
        var entries;
        try {
            entries = FS.readdir(root);
        } catch (err) {
            return;
        }
        entries.forEach(function(name) {
            if (name === '.' || name === '..' || kept.indexOf(name) >= 0) return;
            if (root === '/tmp/filedrop' && newest >= 0 && Number(name) > newest) return;
            var dir = prefix + name;
            try {
                FS.readdir(dir).forEach(function(file) {
                    if (file !== '.' && file !== '..') FS.unlink(dir + '/' + file);
                });
                FS.rmdir(dir);
            } catch (err) {
                console.warn('SDL_libretro: failed to remove ' + dir + ':', err);
            }
        });
    });
});
/**
 * @internal
 * Called once a menu-triggered load finishes. The previous game is unloaded
 * by then, so free the files it was read from.
 */
static void SDL_Libretro_MenuGameLoadAttempted(SDL_LibretroMenu* menu, const char* path, bool loaded) {
    if (loaded) {
        SDL_strlcpy(menu->webGamePath, path, sizeof(menu->webGamePath));
    }
    else if (!SDL_Libretro_IsGameReady(menu->lr)) {
        menu->webGamePath[0] = '\0';
    }
    SDL_Libretro_MenuWebPruneFiles(menu->webGamePath, menu->pendingGamePath);
}

/**
 * @internal
 * The browser file picker filters by itself when it opens.
 */
static void SDL_Libretro_MenuUpdateLoadGameFilter(SDL_LibretroMenu* menu) {
    (void)menu;
}

/**
 * @internal
 * There is no file browser to point at the Content directory.
 */
static void SDL_Libretro_MenuSetBrowseDirectory(SDL_LibretroMenu* menu) {
    (void)menu;
}

/**
 * @internal
 * Create a directory setting under @p parent as a textedit, since the web
 * build's virtual file system isn't worth browsing.
 */
static nk_console* SDL_Libretro_MenuAddSettingDir(nk_console* parent, const char* label, SDL_LibretroMenu* menu, char* buffer, size_t bufferSize) {
    return SDL_Libretro_MenuAddSettingTextedit(parent, label, menu, buffer, bufferSize);
}

/**
 * @internal
 * Opens a browser file picker; the chosen files are written into a fresh
 * /picked/<n>/ directory in the virtual file system, and one of them is
 * handed back through SDL_Libretro_MenuLoadGameFromJS().
 *
 * Several files can be picked for content that spans them (.cue + .bin,
 * .m3u + discs, .gdi + tracks). The one loaded is the first playlist or
 * descriptor the cores take, else the first file they take, else the first.
 *
 * @param filter the ";"-separated extensions the cores take (".gb;.GB"),
 *               used for the picker's accept attribute; empty for any file.
 *
 * Requires ccall and FS in -sEXPORTED_RUNTIME_METHODS.
 */
EM_JS(void, SDL_Libretro_MenuOpenWebFilePicker, (SDL_LibretroMenu * menu, const char* filter), {
    var exts = [];
    UTF8ToString(filter).split(';').forEach(function(ext) {
        ext = ext.toLowerCase();
        if (ext.length > 0 && exts.indexOf(ext) < 0) exts.push(ext);
    });
    var extOf = function(name) {
        var dot = name.lastIndexOf('.');
        return dot < 0 ? "" : name.substring(dot).toLowerCase();
    };
    var supported = function(name) {
        return exts.length === 0 || exts.indexOf(extOf(name)) >= 0;
    };

    var input = document.createElement('input');
    input.type = 'file';
    input.multiple = true;
    if (exts.length > 0) {
        input.accept = exts.join(',');
    }
    input.onchange = function() {
        var files = Array.prototype.slice.call(input.files || []);
        if (files.length === 0) return;

        var FS = Module.FS;
        Module.SDL_libretroPickCount = (Module.SDL_libretroPickCount || 0) + 1;
        var dir = '/picked/' + Module.SDL_libretroPickCount;
        var removeDir = function() {
            try {
                FS.readdir(dir).forEach(function(file) {
                    if (file !== '.' && file !== '..') FS.unlink(dir + '/' + file);
                });
                FS.rmdir(dir);
            } catch (err) {
            }
        };

        // Write one file at a time, so only one is held in JS memory at once.
        var index = 0;
        var writeNext = function() {
            if (index >= files.length) {
                var chosen = null;
                ['.m3u', '.cue', '.gdi', '.ccd', '.toc', '.mds'].forEach(function(ext) {
                    if (chosen === null && supported(ext)) {
                        chosen = files.find(function(file) { return extOf(file.name) === ext; }) || null;
                    }
                });
                if (chosen === null) chosen = files.find(function(file) { return supported(file.name); }) || files[0];
                Module.ccall('SDL_Libretro_MenuLoadGameFromJS', null, ['number', 'string'], [menu, dir + '/' + chosen.name]);
                return;
            }
            var file = files[index++];
            file.arrayBuffer().then(function(buffer) {
                try {
                    FS.writeFile(dir + '/' + file.name, new Uint8Array(buffer));
                } catch (err) {
                    console.error('SDL_libretro: failed to write picked file to FS:', err);
                    removeDir();
                    return;
                }
                writeNext();
            }, function(err) {
                console.error('SDL_libretro: failed to read picked file:', err);
                removeDir();
            });
        };
        try {
            try {
                FS.mkdir('/picked');
            } catch (err) {
            }
            FS.mkdir(dir);
        } catch (err) {
            console.error('SDL_libretro: failed to create ' + dir + ':', err);
            return;
        }
        writeNext();
    };
    input.click();
});

/**
 * @internal
 * Called from SDL_Libretro_MenuOpenWebFilePicker()'s JS once the picked files
 * land in the virtual file system.
 */
EMSCRIPTEN_KEEPALIVE void SDL_Libretro_MenuLoadGameFromJS(SDL_LibretroMenu* menu, const char* path) {
    if (menu == NULL || path == NULL) {
        return;
    }
    SDL_strlcpy(menu->loadGamePath, path, sizeof(menu->loadGamePath));
    SDL_Libretro_MenuLoadPendingGame(menu);

    // A load already freed the earlier picks. Otherwise this pick waits on
    // the core picker, or no core takes it, so drop the picks left unused.
    SDL_Libretro_MenuWebPruneFiles(menu->webGamePath, menu->pendingGamePath);
}

/**
 * @internal
 */
static void SDL_Libretro_MenuWebLoadGameClicked(SDL_LibretroMenu* menu, void* userdata) {
    (void)userdata;
    char* filter = SDL_Libretro_MenuBuildLoadGameFilter(menu);
    // The picker reads the filter before returning, so it can go right away.
    SDL_Libretro_MenuOpenWebFilePicker(menu, filter != NULL ? filter : "");
    SDL_free(filter);
}
/**
 * @internal
 * Add the top-level Load Game entry. Browsing the virtual file system isn't
 * useful on the web; a JS file picker brings the user's own files in instead.
 */
static void SDL_Libretro_MenuAddLoadGame(SDL_LibretroMenu* menu) {
    menu->loadGameButton = SDL_Libretro_MenuAddButton(menu, menu->console, "Load Game", NK_SYMBOL_NONE, &SDL_Libretro_MenuWebLoadGameClicked, NULL);
}

/**
 * @internal
 * No Quit entry on the web, where there is nothing to quit to.
 */
static void SDL_Libretro_MenuAddQuit(SDL_LibretroMenu* menu) {
    (void)menu;
}

#endif /* SDL_LIBRETRO_MENU_EMSCRIPTEN_IMPL_ONCE */
