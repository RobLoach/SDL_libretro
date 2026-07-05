/**
 * SDL_libretro - Optional .zip content loading (via SDL_minizip / minizip-ng)
 *
 * Optional add-on to SDL_libretro. Include it AFTER SDL_libretro.h. In the one
 * translation unit that defines SDL_LIBRETRO_IMPLEMENTATION, this header also
 * emits its implementation (and, unless SDL_LIBRETRO_NO_MINIZIP_IMPLEMENTATION
 * is defined, the bundled SDL_minizip implementation):
 *
 *     #define SDL_LIBRETRO_IMPLEMENTATION
 *     #include "SDL_libretro.h"
 *     #include "SDL_libretro_minizip.h"
 *
 * That TU must be able to find and link minizip-ng (MINIZIP::minizip); see the
 * SDL_LIBRETRO_ENABLE_MINIZIP CMake option. The base SDL_libretro library does
 * not depend on minizip.
 *
 * A loaded archive stays mounted as a read-only VFS layer until the next
 * unload, so the core can read the ROM and any companion files (BIOS, multi-disk
 * content, overlays) straight from the .zip through the libretro VFS. Cores that
 * bypass the libretro VFS with raw stdio are not supported.
 *
 * Copyright (c) 2026 Rob Loach
 *
 * This software is provided "as-is", without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software
 *      in a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *      be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any source distribution.
 *
 * @file SDL_libretro_minizip.h
 */

#ifndef SDL_LIBRETRO_MINIZIP_H
#define SDL_LIBRETRO_MINIZIP_H

#include "SDL_libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup SDL_Libretro
 * @{
 */

#ifdef __DOXYGEN
/**
 * Opt out of emitting the bundled SDL_minizip implementation.
 *
 * By default, the translation unit that defines SDL_LIBRETRO_IMPLEMENTATION and
 * includes SDL_libretro_minizip.h also compiles the SDL_minizip implementation.
 * Define this if you compile SDL_minizip in a separate translation unit.
 */
#define SDL_LIBRETRO_NO_MINIZIP_IMPLEMENTATION
#endif

/**
 * Load libretro content from a .zip archive.
 *
 * Opens the archive, auto-detects the content file inside it (the first entry
 * whose extension is claimed by an available core, otherwise the first file),
 * loads a matching core if one is not already loaded, and loads the content.
 *
 * The archive remains mounted as a read-only VFS layer for the duration of the
 * game, so the core can read the ROM and its companion files directly from the
 * .zip. It is released by SDL_Libretro_UnloadGame() (called implicitly when the
 * next game is loaded).
 *
 * @param lr the libretro context.
 * @param zipPath path to the .zip archive.
 * @returns true on success; false if the archive cannot be opened, no loadable
 *          content is found, no core matches, or the core rejects the content.
 *
 * @see SDL_Libretro_LoadGame_ZipEntry()
 * @see SDL_Libretro_LoadGame()
 * @see SDL_Libretro_UnloadGame()
 */
bool SDL_Libretro_LoadGame_Zip(SDL_Libretro* lr, const char* zipPath);

/**
 * Load a specific entry from a .zip archive as libretro content.
 *
 * Like SDL_Libretro_LoadGame_Zip(), but loads the named entry rather than
 * auto-detecting it. Passing NULL for @p entry is equivalent to
 * SDL_Libretro_LoadGame_Zip().
 *
 * @param lr the libretro context.
 * @param zipPath path to the .zip archive.
 * @param entry the archive-relative path of the content file, or NULL to
 *              auto-detect.
 * @returns true on success; false otherwise (see SDL_Libretro_LoadGame_Zip()).
 *
 * @see SDL_Libretro_LoadGame_Zip()
 */
bool SDL_Libretro_LoadGame_ZipEntry(SDL_Libretro* lr, const char* zipPath, const char* entry);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SDL_LIBRETRO_MINIZIP_H */

/* ===================================================================== */
/*  Implementation                                                       */
/* ===================================================================== */
#ifdef SDL_LIBRETRO_IMPLEMENTATION
#ifndef SDL_LIBRETRO_MINIZIP_IMPL_ONCE
#define SDL_LIBRETRO_MINIZIP_IMPL_ONCE

#ifndef SDL_LIBRETRO_NO_MINIZIP_IMPLEMENTATION
#define SDL_MINIZIP_IMPLEMENTATION
#endif
#include "SDL_minizip.h"

#include "SDL_libretro_vfs_minizip.h"

/**
 * Whether any available core claims the given (raw-case) extension: the loaded
 * core's extensions if a core is ready, otherwise any discovered core's.
 *
 * @internal
 */
static bool SDL_Libretro_ZipExtSupported(SDL_Libretro* lr, const char* ext) {
    if (!ext || !ext[0]) return false;
    if (SDL_Libretro_IsCoreReady(lr)) {
        return SDL_Libretro_ExtensionInList(ext, lr->core.validExtensions);
    }
    for (unsigned i = 0; i < lr->coreLibraryCount; i++) {
        if (SDL_Libretro_ExtensionInList(ext, lr->coreLibrary[i].supported_extensions)) {
            return true;
        }
    }
    return false;
}

/**
 * @internal
 */
typedef struct {
    SDL_Storage* storage;
    SDL_Libretro* lr;
    char** dirs;                        /* subdirectories queued for a later pass */
    size_t dirCount, dirCap;
    char best[SDL_LIBRETRO_MAX_PATH];   /* first extension match (preferred) */
    char first[SDL_LIBRETRO_MAX_PATH];  /* first file of any kind (fallback) */
    bool haveBest, haveFirst;
    size_t visited;
} SDL_Libretro_ZipScan;

/**
 * @internal
 */
static SDL_EnumerationResult SDLCALL SDL_Libretro_ZipScanCB(void* userdata, const char* dirname, const char* fname) {
    SDL_Libretro_ZipScan* s = (SDL_Libretro_ZipScan*)userdata;
    if (s->visited++ > 8192) return SDL_ENUM_SUCCESS; // guard against pathological archives

    char full[SDL_LIBRETRO_MAX_PATH];
    SDL_snprintf(full, sizeof(full), "%s%s", dirname ? dirname : "", fname);

    SDL_PathInfo info;
    if (!SDL_GetStoragePathInfo(s->storage, full, &info)) return SDL_ENUM_CONTINUE;

    if (info.type == SDL_PATHTYPE_DIRECTORY) {
        // Queue for a later, non-nested pass (the minizip storage keeps a single
        // shared read cursor, so it must not be enumerated re-entrantly).
        if (s->dirCount >= s->dirCap) {
            size_t cap = s->dirCap ? s->dirCap * 2 : 8;
            char** grown = (char**)SDL_realloc(s->dirs, cap * sizeof(char*));
            if (!grown) return SDL_ENUM_CONTINUE;
            s->dirs = grown;
            s->dirCap = cap;
        }
        s->dirs[s->dirCount] = SDL_strdup(full);
        if (s->dirs[s->dirCount]) s->dirCount++;
        return SDL_ENUM_CONTINUE;
    }

    if (!s->haveFirst) {
        SDL_strlcpy(s->first, full, sizeof(s->first));
        s->haveFirst = true;
    }
    if (!s->haveBest) {
        const char* dot = SDL_strrchr(fname, '.');
        if (dot && SDL_Libretro_ZipExtSupported(s->lr, dot + 1)) {
            SDL_strlcpy(s->best, full, sizeof(s->best));
            s->haveBest = true;
        }
    }
    return SDL_ENUM_CONTINUE;
}

/**
 * Auto-detect the content entry inside a mounted archive: prefer the first file
 * whose extension a core supports; otherwise the first file found. Walks
 * subdirectories breadth-first.
 *
 * @internal
 */
static bool SDL_Libretro_ZipPickEntry(SDL_Storage* storage, SDL_Libretro* lr, char* out, size_t outLen) {
    SDL_Libretro_ZipScan s;
    SDL_zero(s);
    s.storage = storage;
    s.lr = lr;

    bool ok = SDL_EnumerateStorageDirectory(storage, "", SDL_Libretro_ZipScanCB, &s);
    // Newly discovered directories append to s.dirs during iteration, extending the walk.
    for (size_t i = 0; ok && !s.haveBest && i < s.dirCount; i++) {
        SDL_EnumerateStorageDirectory(storage, s.dirs[i], SDL_Libretro_ZipScanCB, &s);
    }

    for (size_t i = 0; i < s.dirCount; i++) SDL_free(s.dirs[i]);
    SDL_free(s.dirs);

    if (s.haveBest)  { SDL_strlcpy(out, s.best, outLen);  return true; }
    if (s.haveFirst) { SDL_strlcpy(out, s.first, outLen); return true; }
    return false;
}

bool SDL_Libretro_LoadGame_ZipEntry(SDL_Libretro* lr, const char* zipPath, const char* entry) {
    if (!lr || !zipPath) return false;

    // Switching content: unload the current game (also unmounts any prior archive).
    SDL_Libretro_UnloadGame(lr);

    SDL_Storage* storage = SDL_OpenMinizipStorage(zipPath);
    if (!storage) {
        SDL_SetError("[SDL_Libretro] Failed to open zip '%s'", zipPath);
        return false;
    }

    // The minizip storage is ready synchronously, but honor the SDL_Storage contract.
    for (int i = 0; i < 100 && !SDL_StorageReady(storage); i++) SDL_Delay(1);
    if (!SDL_StorageReady(storage)) {
        SDL_CloseStorage(storage);
        SDL_SetError("[SDL_Libretro] Zip storage not ready: '%s'", zipPath);
        return false;
    }

    // Resolve the inner entry (explicit or auto-detected).
    char inner[SDL_LIBRETRO_MAX_PATH];
    if (entry && entry[0]) {
        SDL_PathInfo probe;
        if (!SDL_GetStoragePathInfo(storage, entry, &probe) || probe.type != SDL_PATHTYPE_FILE) {
            SDL_CloseStorage(storage);
            SDL_SetError("[SDL_Libretro] Entry '%s' not found in zip '%s'", entry, zipPath);
            return false;
        }
        SDL_strlcpy(inner, entry, sizeof(inner));
    } else if (!SDL_Libretro_ZipPickEntry(storage, lr, inner, sizeof(inner))) {
        SDL_CloseStorage(storage);
        SDL_SetError("[SDL_Libretro] No loadable content found in zip '%s'", zipPath);
        return false;
    }

    // Ensure a core is loaded, selected by the inner file's extension.
    if (!SDL_Libretro_IsCoreReady(lr) && !SDL_Libretro_LoadCoreForGame(lr, inner)) {
        SDL_CloseStorage(storage);
        return false; // SDL_Libretro_LoadCoreForGame set the error
    }

    // Mount the archive so the core can read from it during and after load. From
    // here, `storage` is owned by the mount and released via SDL_Libretro_ZipUnmount.
    SDL_Libretro_ZipMount* mount = (SDL_Libretro_ZipMount*)SDL_calloc(1, sizeof(*mount));
    if (!mount) {
        SDL_CloseStorage(storage);
        return false;
    }
    mount->storage = storage;
    SDL_strlcpy(mount->archivePath, zipPath, sizeof(mount->archivePath));
    SDL_strlcpy(mount->archiveFile, inner, sizeof(mount->archiveFile));
    SDL_Libretro_MountZipVFS(lr, mount);

    // Content identity from the inner entry (need_fullpath, save paths, gameInfoExt).
    SDL_memset(&lr->core.gameInfoExt, 0, sizeof(lr->core.gameInfoExt));
    const char* ext = SDL_Libretro_SetContentIdentity(lr, inner);

    // Advertise the archive origin for cores that read GET_GAME_INFO_EXT.
    lr->core.gameInfoExt.file_in_archive = true;
    lr->core.gameInfoExt.archive_path    = mount->archivePath;
    lr->core.gameInfoExt.archive_file    = mount->archiveFile;

    bool needFullpath = SDL_Libretro_ContentNeedsFullpath(lr, ext);
    bool persistData = false;

    struct retro_game_info gameInfo = {0};
    gameInfo.path = lr->core.contentPath; // == inner entry; resolved by the zip VFS
    void* fileData = NULL;

    if (!needFullpath) {
        // Extract the entry into memory for cores that consume a data buffer.
        persistData = SDL_Libretro_ContentPersistData(lr, ext);

        Uint64 size = 0;
        if (!SDL_GetStorageFileSize(storage, inner, &size)) {
            SDL_SetError("[SDL_Libretro] Failed to size zip entry '%s'", inner);
            SDL_Libretro_ZipUnmount(lr);
            SDL_Libretro_ResetContentState(lr);
            return false;
        }
        fileData = SDL_malloc(size ? (size_t)size : 1);
        if (!fileData) {
            SDL_Libretro_ZipUnmount(lr);
            SDL_Libretro_ResetContentState(lr);
            return false;
        }
        if (size && !SDL_ReadStorageFile(storage, inner, fileData, size)) {
            SDL_SetError("[SDL_Libretro] Failed to read zip entry '%s'", inner);
            SDL_free(fileData);
            SDL_Libretro_ZipUnmount(lr);
            SDL_Libretro_ResetContentState(lr);
            return false;
        }
        gameInfo.data = fileData;
        gameInfo.size = (size_t)size;
        lr->core.gameInfoExt.data            = fileData;
        lr->core.gameInfoExt.size            = (size_t)size;
        lr->core.gameInfoExt.persistent_data = persistData;
    }

    SDL_Libretro_SetCoreCallbacks(lr);

    bool result = lr->core.symbols.retro_load_game(&gameInfo);

    // Mirror SDL_Libretro_LoadGame: persistent content keeps its buffer (freed by
    // SDL_Libretro_ResetContentState on unload); otherwise it is only valid for the
    // duration of the load, so free it now and clear the dangling pointer.
    if (fileData && !(persistData && result)) {
        SDL_free(fileData);
        lr->core.gameInfoExt.data = NULL;
        lr->core.gameInfoExt.size = 0;
    }

    if (!result) {
        SDL_SetError("[SDL_Libretro] Core failed to load zip content '%s'", inner);
        SDL_Libretro_ZipUnmount(lr);
        SDL_Libretro_ResetContentState(lr);
        return false;
    }

    SDL_Libretro_FinishGameLoad(lr);
    return true; // archive stays mounted until the next unload
}

bool SDL_Libretro_LoadGame_Zip(SDL_Libretro* lr, const char* zipPath) {
    return SDL_Libretro_LoadGame_ZipEntry(lr, zipPath, NULL);
}

#endif /* SDL_LIBRETRO_MINIZIP_IMPL_ONCE */
#endif /* SDL_LIBRETRO_IMPLEMENTATION */
