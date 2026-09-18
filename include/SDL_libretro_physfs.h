/**
 * SDL_libretro - zip loading with PhysFS
 *
 * Enabled by defining SDL_LIBRETRO_ENABLE_PHYSFS alongside
 * SDL_LIBRETRO_IMPLEMENTATION; requires the SDL VFS. Without it, the
 * SDL_Libretro_PhysFS_* functions are stubs that fail with an error.
 *
 * @file SDL_libretro_physfs.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_PHYSFS_IMPL_ONCE) && defined(SDL_LIBRETRO_ENABLE_PHYSFS) && !defined(SDL_LIBRETRO_DISABLE_PHYSFS)
#define SDL_LIBRETRO_PHYSFS_IMPL_ONCE

#ifdef SDL_LIBRETRO_DISABLE_VFS
#error "SDL_libretro_physfs requires the SDL VFS; do not define SDL_LIBRETRO_DISABLE_VFS"
#endif

#ifndef SDL_LIBRETRO_PHYSFS_NO_SDL_PHYSFS_IMPLEMENTATION
#define SDL_PHYSFS_IMPLEMENTATION
#endif
#include "SDL_PhysFS.h"

#ifndef SDL_PHYSFS_PHYSFS_H
#define SDL_PHYSFS_PHYSFS_H "physfs.h"
#endif
#include SDL_PHYSFS_PHYSFS_H

#ifndef SDL_LIBRETRO_PHYSFS_MOUNT_POINT
/**
 * PhysFS mount point for archives.
 *
 * Will be used to prefix the virtual content paths handed to cores.
 */
#define SDL_LIBRETRO_PHYSFS_MOUNT_POINT "game"
#endif

/**
 * @internal
 * PhysFS VFS Open override: Loads a file that exist in the PhysFS search path.
 *
 * When note found, will fall through to the SDL VFS.
 *
 * @see SDL_Libretro_VFS_Open()
 */
static struct retro_vfs_file_handle* SDL_Libretro_PhysFS_VFS_Open(const char* path, unsigned mode, unsigned hints) {
    PHYSFS_Stat st;
    if (path && (mode & RETRO_VFS_FILE_ACCESS_WRITE) == 0 &&
        PHYSFS_stat(path, &st) && st.filetype == PHYSFS_FILETYPE_REGULAR) {
        SDL_IOStream* io = SDL_PhysFS_IOFromFile(path);
        if (io) {
            return SDL_Libretro_VFS_WrapIO(io, path, mode);
        }
    }
    return SDL_Libretro_VFS_Open(path, mode, hints);
}

/**
 * @internal
 * PhysFS VFS Stat64 override to fall through to the SDL VFS.
 *
 * @see SDL_Libretro_VFS_Stat64()
 */
static int SDL_Libretro_PhysFS_VFS_Stat64(const char* path, int64_t* size) {
    PHYSFS_Stat st;
    if (path && PHYSFS_stat(path, &st)) {
        int flags = RETRO_VFS_STAT_IS_VALID;
        if (st.filetype == PHYSFS_FILETYPE_DIRECTORY) {
            flags |= RETRO_VFS_STAT_IS_DIRECTORY;
        }
        if (size != NULL) {
            *size = (int64_t)st.filesize;
        }
        return flags;
    }
    return SDL_Libretro_VFS_Stat64(path, size);
}

/**
 * @internal
 * PhysFS VFS Stat override, falling through to the SDL VFS.
 * @see SDL_Libretro_PhysFS_VFS_Stat64
 */
static int SDL_Libretro_PhysFS_VFS_Stat(const char* path, int32_t* size) {
    int64_t outSize = 0;
    int out = SDL_Libretro_PhysFS_VFS_Stat64(path, &outSize);
    if (size != NULL) {
        *size = SDL_Libretro_VFS_ClampStatSize(path, outSize);
    }
    return out;
}

/**
 * @internal
 * PhysFS VFS Opendir override, falling through to the SDL VFS.
 *
 * @see SDL_Libretro_VFS_Opendir
 */
static struct retro_vfs_dir_handle* SDL_Libretro_PhysFS_VFS_Opendir(const char* dir, bool include_hidden) {
    PHYSFS_Stat st;
    if (!dir || !PHYSFS_stat(dir, &st) || st.filetype != PHYSFS_FILETYPE_DIRECTORY) {
        return SDL_Libretro_VFS_Opendir(dir, include_hidden);
    }

    char** entries = PHYSFS_enumerateFiles(dir);
    if (!entries) return NULL;

    size_t count = 0;
    while (entries[count]) count++;

    struct retro_vfs_dir_handle* h = (struct retro_vfs_dir_handle*)SDL_calloc(1, sizeof(*h));
    char** names = count > 0 ? (char**)SDL_calloc(count, sizeof(char*)) : NULL;
    bool* isDir = count > 0 ? (bool*)SDL_calloc(count, sizeof(bool)) : NULL;
    if (!h || (count > 0 && (!names || !isDir))) {
        SDL_free(h);
        SDL_free(names);
        SDL_free(isDir);
        PHYSFS_freeList(entries);
        return NULL;
    }

    size_t stored = 0;
    for (size_t i = 0; i < count; i++) {
        if (!include_hidden && entries[i][0] == '.') continue;
        names[stored] = SDL_strdup(entries[i]);
        if (!names[stored]) continue;
        char full[SDL_LIBRETRO_MAX_PATH];
        SDL_snprintf(full, sizeof(full), "%s/%s", dir, entries[i]);
        PHYSFS_Stat entryStat;
        isDir[stored] = PHYSFS_stat(full, &entryStat) && entryStat.filetype == PHYSFS_FILETYPE_DIRECTORY;
        stored++;
    }
    PHYSFS_freeList(entries);

    h->dir = SDL_strdup(dir);
    if (!h->dir) {
        for (size_t i = 0; i < stored; i++) SDL_free(names[i]);
        SDL_free(names);
        SDL_free(isDir);
        SDL_free(h);
        return NULL;
    }
    h->names = names;
    h->isDir = isDir;
    h->count = stored;
    h->index = (size_t)-1; // readdir advances before returning
    return h;
}

/**
 * @internal
 * Unmount the PhysFS archive currently mounted at the mount point, if any.
 */
static void SDL_Libretro_PhysFS_ClearMount(SDL_Libretro* lr) {
    if (!lr) return;
    if (lr->physfsMountSource[0] != '\0') {
        SDL_PhysFS_Unmount(lr->physfsMountSource);
        lr->physfsMountSource[0] = '\0';
    }
}

/**
 * Initialize PhysFS and route the libretro VFS through it.
 *
 * This will be called automatically in SDL_Libretro_PhysFS_LoadGame()
 * if PhysFS was not initialized. Cores can then read content directly
 * from the mounted archive through the VFS, with anything outside PhysFS
 * falling through to the SDL VFS. Safe to call repeatedly.
 *
 * Requires the implementation to be compiled with
 * SDL_LIBRETRO_ENABLE_PHYSFS; otherwise it always fails.
 *
 * @param lr the libretro context to install the VFS overrides on.
 * @returns true on success, false if lr is NULL or PhysFS failed to
 *          initialize; call SDL_GetError() for more information.
 *
 * @see SDL_Libretro_PhysFS_Quit()
 */
bool SDL_Libretro_PhysFS_Init(SDL_Libretro* lr) {
    if (!lr) return false;

    if (!lr->physfsReady) {
        if (!SDL_PhysFS_Init(NULL)) {
            return false;
        }
        lr->physfsReady = true;
    }

    struct retro_vfs_interface vfs = {0};
    vfs.open = SDL_Libretro_PhysFS_VFS_Open;
    vfs.stat = SDL_Libretro_PhysFS_VFS_Stat;
    vfs.stat_64 = SDL_Libretro_PhysFS_VFS_Stat64;
    vfs.opendir = SDL_Libretro_PhysFS_VFS_Opendir;
    SDL_Libretro_SetVFS(lr, &vfs);
    return true;
}

/**
 * Tear down PhysFS, unmounting any active archive and restoring the plain SDL
 * VFS on the given context.
 *
 * Unload the game first, since a game loaded from an archive still reads
 * from the mount. Safe to call when PhysFS was never initialized.
 *
 * @param lr the libretro context. NULL is ignored.
 *
 * @see SDL_Libretro_PhysFS_Init()
 */
void SDL_Libretro_PhysFS_Quit(SDL_Libretro* lr) {
    if (!lr) return;
    SDL_Libretro_PhysFS_ClearMount(lr);
    SDL_Libretro_SetVFS(lr, NULL);
    if (lr->physfsReady) {
        SDL_PhysFS_Quit();
        lr->physfsReady = false;
    }
}

/**
 * @internal
 * A list of files within a directory.
 * @see SDL_Libretro_PhysFS_PickContent()
 */
typedef struct SDL_Libretro_PhysFS_FileList {
    char** paths;
    int count;
    int capacity;
} SDL_Libretro_PhysFS_FileList;

/**
 * @internal
 * Recursively collect the regular files under `dir` in the PhysFS search path.
 */
static void SDL_Libretro_PhysFS_CollectFiles(const char* dir, SDL_Libretro_PhysFS_FileList* list) {
    char** entries = PHYSFS_enumerateFiles(dir);
    if (!entries) return;

    for (char** entry = entries; *entry; entry++) {
        char full[SDL_LIBRETRO_MAX_PATH];
        SDL_snprintf(full, sizeof(full), "%s/%s", dir, *entry);
        PHYSFS_Stat st;
        if (!PHYSFS_stat(full, &st)) continue;
        if (st.filetype == PHYSFS_FILETYPE_DIRECTORY) {
            SDL_Libretro_PhysFS_CollectFiles(full, list);
        }
        else if (st.filetype == PHYSFS_FILETYPE_REGULAR) {
            if (list->count >= list->capacity) {
                int newCapacity = list->capacity > 0 ? list->capacity * 2 : 16;
                char** grown = (char**)SDL_realloc(list->paths, (size_t)newCapacity * sizeof(char*));
                if (!grown) continue;
                list->paths = grown;
                list->capacity = newCapacity;
            }
            list->paths[list->count] = SDL_strdup(full);
            if (list->paths[list->count]) list->count++;
        }
    }
    PHYSFS_freeList(entries);
}

/**
 * @internal
 * Whether content with the extension `ext` can be loaded: by the loaded core
 * when its valid extensions are known (`validExts`), otherwise by any core in
 * the library. With neither to go by, anything goes.
 */
static bool SDL_Libretro_PhysFS_IsLoadableExtension(const SDL_Libretro* lr, const char* ext, const char* validExts) {
    if (validExts[0] != '\0') {
        return SDL_Libretro_ExtensionInList(ext, validExts);
    }
    for (unsigned c = 0; c < lr->coreLibraryCount; c++) {
        if (SDL_Libretro_ExtensionInList(ext, lr->coreLibrary[c].supported_extensions)) {
            return true;
        }
    }
    return lr->coreLibraryCount == 0;
}

/**
 * @internal
 * Pick the content file within the mounted archive.
 *
 * Tries each strategy in turn, stopping at the first match:
 * 1. Disc metadata: First .m3u playlist, then the first .cue sheet, so
 *    CD-based cores receive the descriptor rather than a raw track. Only
 *    considered when the loaded core advertises that extension.
 * 2. A loadable entry whose base name matches the archive's ("mario.zip" >
 *    "mario.nes", but not "mario.nfo").
 * 3. The first entry whose extension is in the loaded core's valid
 *    extensions. Without a loaded core, it'll pick the first entry that
 *    any core recognizes.
 * 4. Without a loaded core, grab the first file.
 *
 * Subdirectories are searched recursively.
 *
 * @param lr the libretro context, for the loaded core and core library.
 * @param mountPoint the PhysFS mount point the archive is mounted at.
 * @param archivePath the OS path of the archive (used for the base name).
 * @param validExts the core's valid extensions, or "" to accept any
 *                  recognized content.
 * @param dst receives the virtual path of the pick.
 * @param dstSize the size of dst in bytes.
 * @return true if a candidate was found and copied into `dst`.
 */
static bool SDL_Libretro_PhysFS_PickContent(SDL_Libretro* lr, const char* mountPoint, const char* archivePath, const char* validExts, char* dst, size_t dstSize) {
    SDL_Libretro_PhysFS_FileList list = {0};
    SDL_Libretro_PhysFS_CollectFiles(mountPoint, &list);

    bool hasExts = validExts != NULL && validExts[0] != '\0';

    // Get the archive's filename.
    char archiveName[SDL_LIBRETRO_MAX_PATH];
    SDL_Libretro_GetFileName(archiveName, sizeof(archiveName), archivePath, false);

    // Search for the file to load within the .zip.
    const char* found = NULL;

    // Pass 1: Disc metadata, gated on the core actually supporting it.
    static const char* discExts[] = {"m3u", "cue"};
    for (size_t d = 0; !found && d < SDL_arraysize(discExts); d++) {
        if (hasExts && !SDL_Libretro_ExtensionInList(discExts[d], validExts)) continue;
        for (int i = 0; !found && i < list.count; i++) {
            if (SDL_strcasecmp(SDL_Libretro_GetExtension(list.paths[i]), discExts[d]) == 0) {
                found = list.paths[i];
            }
        }
    }

    // Pass 2: Base name matching the archive's, among the loadable entries,
    // so a "mario.nfo" next to "mario.nes" doesn't win just by sorting first.
    for (int i = 0; !found && i < list.count; i++) {
        char entryName[SDL_LIBRETRO_MAX_PATH];
        SDL_Libretro_GetFileName(entryName, sizeof(entryName), list.paths[i], false);
        if (SDL_strcasecmp(entryName, archiveName) == 0 &&
            SDL_Libretro_PhysFS_IsLoadableExtension(lr, SDL_Libretro_GetExtension(list.paths[i]), validExts)) {
            found = list.paths[i];
        }
    }

    // Pass 3: extension known to the loaded core, or to any core in the library.
    for (int i = 0; !found && i < list.count; i++) {
        if (SDL_Libretro_PhysFS_IsLoadableExtension(lr, SDL_Libretro_GetExtension(list.paths[i]), validExts)) {
            found = list.paths[i];
        }
    }

    // Pass 4: No file found, so assume it's the first file.
    if (!found && !hasExts && list.count > 0) {
        found = list.paths[0];
    }

    if (found) {
        SDL_strlcpy(dst, found, dstSize);
    }

    // Clear the file list.
    for (int i = 0; i < list.count; i++) {
        SDL_free(list.paths[i]);
    }
    SDL_free(list.paths);

    return found != NULL;
}

/**
 * @internal
 * Extract every file in the mounted archive into a new temp directory.
 *
 * For need_fullpath cores that read files from the OS rather than the VFS:
 * the picked content may refer to other files in the archive, like a .cue
 * sheet's tracks or an .m3u's discs, so they're all extracted, keeping the
 * archive's layout. The directory is named after the archive, uniquified
 * with a "-<n>" suffix when taken.
 *
 * @param lr the libretro context.
 * @param archivePath the OS path of the archive (used for the name).
 * @param dir receives the new directory, ending in a slash.
 * @param dirSize the size of `dir` in bytes.
 * @returns true when every file was extracted; on failure nothing is left behind.
 */
static bool SDL_Libretro_PhysFS_ExtractArchive(SDL_Libretro* lr, const char* archivePath, char* dir, size_t dirSize) {
    char tempDir[SDL_LIBRETRO_MAX_PATH];
    if (!SDL_Libretro_GetTempDirectory(lr, tempDir, sizeof(tempDir))) {
        return false;
    }
    char name[SDL_LIBRETRO_MAX_PATH];
    if (SDL_Libretro_GetFileName(name, sizeof(name), archivePath, false) == 0) {
        SDL_strlcpy(name, "content", sizeof(name));
    }

    // Find a directory name that doesn't exist yet.
    SDL_snprintf(dir, dirSize, "%s%s/", tempDir, name);
    for (int attempt = 1; SDL_GetPathInfo(dir, NULL); attempt++) {
        if (attempt > 99) {
            return SDL_SetError("[SDL_Libretro] Could not find a free directory name for '%s' in '%s'", name, tempDir);
        }
        SDL_snprintf(dir, dirSize, "%s%s-%d/", tempDir, name, attempt);
    }

    SDL_Libretro_PhysFS_FileList list = {0};
    SDL_Libretro_PhysFS_CollectFiles(SDL_LIBRETRO_PHYSFS_MOUNT_POINT, &list);
    size_t prefixLen = SDL_strlen(SDL_LIBRETRO_PHYSFS_MOUNT_POINT "/");
    bool ok = SDL_CreateDirectory(dir);
    for (int i = 0; ok && i < list.count; i++) {
        // PhysFS rejects ".." and absolute paths, so entries stay inside `dir`.
        char path[SDL_LIBRETRO_MAX_PATH];
        SDL_snprintf(path, sizeof(path), "%s%s", dir, list.paths[i] + prefixLen);

        // Make sure the entry's subdirectory exists.
        char* slash = SDL_strrchr(path, '/');
        if (slash && slash > path + SDL_strlen(dir) - 1) {
            *slash = '\0';
            ok = SDL_CreateDirectory(path);
            *slash = '/';
        }

        size_t size = 0;
        void* data = ok ? SDL_LoadFile_IO(SDL_PhysFS_IOFromFile(list.paths[i]), &size, true) : NULL;
        ok = data != NULL && SDL_SaveFile(path, data, size);
        SDL_free(data);
        if (!ok) {
            SDL_SetError("[SDL_Libretro] Failed to extract '%s' to '%s': %s", list.paths[i], path, SDL_GetError());
        }
    }

    for (int i = 0; i < list.count; i++) {
        SDL_free(list.paths[i]);
    }
    SDL_free(list.paths);

    if (!ok) {
        SDL_Libretro_RemoveTree(dir);
    }
    return ok;
}

/**
 * Resolve the content path a load would choose from `path`, without loading it.
 *
 * A non-archive path passes through unchanged; for a .zip the chosen entry's
 * virtual path is returned.
 *
 * @param lr the libretro context.
 * @param path the OS path of the content or archive.
 * @param dst receives the resolved content path.
 * @param dstSize the size of `dst`.
 * @return true if a content path was resolved into `dst`.
 */
bool SDL_Libretro_PhysFS_PeekContent(SDL_Libretro* lr, const char* path, char* dst, size_t dstSize) {
    if (!lr || !path || !dst || dstSize == 0) {
        return false;
    }

    // Not an archive: the path is the content.
    if (SDL_strcasecmp(SDL_Libretro_GetExtension(path), "zip") != 0) {
        SDL_strlcpy(dst, path, dstSize);
        return true;
    }

    if (!SDL_Libretro_PhysFS_Init(lr)) {
        return false;
    }

    // Already mounted for the running game: inspect it in place rather than
    // mounting a second copy of the same archive.
    if (SDL_strcmp(path, lr->physfsMountSource) == 0) {
        return SDL_Libretro_PhysFS_PickContent(lr, SDL_LIBRETRO_PHYSFS_MOUNT_POINT, path, "", dst, dstSize);
    }

    // A private mount point keeps any running game's mount ("game") intact.
    static const char* peekMount = SDL_LIBRETRO_PHYSFS_MOUNT_POINT "_peek";
    if (!SDL_PhysFS_Mount(path, peekMount)) {
        return false;
    }
    bool found = SDL_Libretro_PhysFS_PickContent(lr, peekMount, path, "", dst, dstSize);
    SDL_PhysFS_Unmount(path);
    return found;
}

/**
 * Archive-aware replacement for SDL_Libretro_LoadGame().
 *
 * Content that isn't a .zip loads exactly like SDL_Libretro_LoadGame(). For a
 * .zip, the archive is mounted with PhysFS and the content inside is handed to
 * the core: as a data buffer for byte-oriented cores, or by its virtual path in
 * a PhysFS-backed VFS for full-path cores that use the libretro VFS. Full-path
 * cores that read files from the OS get the whole archive extracted to a temp
 * directory instead, so content like a .cue sheet finds its tracks; it's
 * removed when the game unloads.
 *
 * The raw .zip is handed over unextracted instead when the loaded core reports
 * block_extract or lists "zip" in its valid extensions, or, with no core
 * loaded, when the first core in the library that lists "zip" sets
 * block_extract in its .info file.
 *
 * Any running game is unloaded and any previous archive unmounted first.
 * PhysFS is initialized on demand; if that or mounting the archive fails, the
 * .zip falls back to a plain SDL_Libretro_LoadGame()-style load. Without a
 * loaded core, the core is picked from the library based on the chosen
 * content file.
 *
 * SDL_Libretro_LoadGame() will call this automatically when loading an archive,
 * so there is no need to call it directly. Requires the implementation to be
 * compiled with SDL_LIBRETRO_ENABLE_PHYSFS; otherwise it always fails.
 *
 * @param lr the libretro context.
 * @param gamePath the OS path of the content or archive. May be NULL for a
 *                 content-less core load.
 * @returns true when the game loaded, false on failure, including when lr is
 *          NULL, no suitable content is found inside the archive, or no core
 *          can run it; call SDL_GetError() for more information.
 *
 * @see SDL_Libretro_LoadGame()
 */
bool SDL_Libretro_PhysFS_LoadGame(SDL_Libretro* lr, const char* gamePath) {
    if (!lr) return false;

    // Unload any running game before touching the mount. Otherwise the old
    // archive is pulled out from under it before retro_unload_game(), and the
    // unload inside SDL_Libretro_LoadGameFile() would unmount the new one.
    SDL_Libretro_UnloadGame(lr);
    SDL_Libretro_PhysFS_ClearMount(lr);

    // Load the core without content.
    if (!gamePath) {
        return SDL_Libretro_LoadGameFile(lr, NULL);
    }

    // If it's not a .zip file, load it as normal.
    if (SDL_strcasecmp(SDL_Libretro_GetExtension(gamePath), "zip") != 0) {
        return SDL_Libretro_LoadGameFile(lr, gamePath);
    }

    // Initialize PhysFS on demand. Fall back to the plain loader on failure.
    if (!SDL_Libretro_PhysFS_Init(lr)) {
        return SDL_Libretro_LoadGameFile(lr, gamePath);
    }

    // Cores that parse archives themselves get the raw .zip.
    if (SDL_Libretro_IsCoreReady(lr)) {
        if (lr->core.blockExtract || SDL_Libretro_ExtensionInList("zip", lr->core.validExtensions)) {
            return SDL_Libretro_LoadGameFile(lr, gamePath);
        }
    }
    else {
        // With no core loaded, look at the core that would be picked for a
        // .zip (the first in sorted library order). Only when its .info sets
        // block_extract does it get the raw archive; a core that merely lists
        // "zip" would otherwise claim every archive, whatever is inside.
        for (unsigned i = 0; i < lr->coreLibraryCount; i++) {
            const SDL_Libretro_CoreInfo* info = &lr->coreLibrary[i];
            if (!SDL_Libretro_ExtensionInList("zip", info->supported_extensions)) continue;
            if (info->block_extract && SDL_Libretro_LoadCore(lr, info->path)) {
                return SDL_Libretro_LoadGameFile(lr, gamePath);
            }
            break;
        }
    }

    // Mount the .zip file.
    if (!SDL_PhysFS_Mount(gamePath, SDL_LIBRETRO_PHYSFS_MOUNT_POINT)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[SDL_Libretro] Failed to mount '%s': %s", gamePath, SDL_GetError());
        return SDL_Libretro_LoadGameFile(lr, gamePath);
    }

    // Remember which .zip file was mounted.
    SDL_strlcpy(lr->physfsMountSource, gamePath, sizeof(lr->physfsMountSource));

    // Find the content to load within the mount.
    char virtualPath[SDL_LIBRETRO_MAX_PATH];
    if (!SDL_Libretro_PhysFS_PickContent(lr, SDL_LIBRETRO_PHYSFS_MOUNT_POINT, gamePath,
                                         SDL_Libretro_IsCoreReady(lr) ? lr->core.validExtensions : "",
                                         virtualPath, sizeof(virtualPath))) {
        SDL_Libretro_PhysFS_ClearMount(lr);
        SDL_SetError("[SDL_Libretro] No suitable content found inside '%s'", gamePath);
        return false;
    }

    // The core must be loaded to resolve need_fullpath for the picked file.
    if (!SDL_Libretro_IsCoreReady(lr) && !SDL_Libretro_LoadCoreForGame(lr, virtualPath)) {
        SDL_Libretro_PhysFS_ClearMount(lr);
        return false;
    }

    // Attempt to load the content.
    bool result;
    if (SDL_Libretro_ContentNeedsFullpath(lr, SDL_Libretro_GetExtension(virtualPath)) && lr->core.usedVFS) {
        // VFS-aware full-path core: pass the virtual path; the PhysFS-backed
        // VFS serves it straight out of the mount.
        result = SDL_Libretro_LoadGameFile(lr, virtualPath);
    }
    else if (SDL_Libretro_ContentNeedsFullpath(lr, SDL_Libretro_GetExtension(virtualPath))) {
        // Full-path core that reads from the OS: extract the archive, so the
        // content finds any files it refers to, and load from there.
        char dir[SDL_LIBRETRO_MAX_PATH];
        result = SDL_Libretro_PhysFS_ExtractArchive(lr, gamePath, dir, sizeof(dir));
        if (result) {
            char path[SDL_LIBRETRO_MAX_PATH];
            SDL_snprintf(path, sizeof(path), "%s%s", dir, virtualPath + SDL_strlen(SDL_LIBRETRO_PHYSFS_MOUNT_POINT "/"));
            result = SDL_Libretro_LoadGameFile(lr, path);
            if (result) {
                SDL_strlcpy(lr->core.contentTempDir, dir, sizeof(lr->core.contentTempDir));
            }
            else {
                SDL_Libretro_RemoveTree(dir);
            }
        }
    }
    else {
        // Byte-oriented cores get the content bytes.
        SDL_IOStream* io = SDL_PhysFS_IOFromFile(virtualPath);
        result = io != NULL && SDL_Libretro_LoadGame_IO(lr, io, virtualPath, true);
    }

    if (!result) {
        SDL_Libretro_PhysFS_ClearMount(lr);
    }
    return result;
}

#else

bool SDL_Libretro_PhysFS_Init(SDL_Libretro* lr) {
    (void)lr;
    return SDL_SetError("SDL_Libretro_PhysFS not enabled");
}

void SDL_Libretro_PhysFS_Quit(SDL_Libretro* lr) {
    (void)lr;
}

bool SDL_Libretro_PhysFS_LoadGame(SDL_Libretro* lr, const char* gamePath) {
    (void)lr;
    (void)gamePath;
    return SDL_SetError("SDL_Libretro_PhysFS not enabled");
}

bool SDL_Libretro_PhysFS_PeekContent(SDL_Libretro* lr, const char* path, char* dst, size_t dstSize) {
    (void)lr;
    (void)path;
    (void)dst;
    (void)dstSize;
    return false;
}

static void SDL_Libretro_PhysFS_ClearMount(SDL_Libretro* lr) {
    (void)lr;
}

#endif
