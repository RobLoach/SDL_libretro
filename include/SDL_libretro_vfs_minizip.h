/**
 * SDL_libretro - Zip-backed libretro Virtual File System (via SDL_minizip)
 *
 * Layers a read-only, archive-aware VFS over the default SDL-backed one. When a
 * .zip is mounted (see SDL_Libretro_LoadGame_Zip), file opens/stats that resolve
 * to an entry inside the archive are served straight from the zip; everything
 * else falls through to the on-disk SDL VFS. Because a served entry is exposed
 * as an SDL_IOStream stored in the shared `retro_vfs_file_handle`, the existing
 * read/seek/tell/size/close callbacks operate on it unchanged.
 *
 * @file SDL_libretro_vfs_minizip.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_VFS_MINIZIP_IMPL_ONCE)
#define SDL_LIBRETRO_VFS_MINIZIP_IMPL_ONCE

#ifdef SDL_LIBRETRO_DISABLE_VFS
#error "SDL_libretro_minizip requires the SDL libretro VFS; do not define SDL_LIBRETRO_DISABLE_VFS."
#endif

/**
 * A mounted archive. Backs SDL_Libretro's opaque `zipMount`. The path/file
 * buffers give stable storage for gameInfoExt.archive_path / archive_file.
 *
 * @internal
 */
typedef struct {
    SDL_Storage* storage;                     /** The open zip, as an SDL_Storage. */
    char archivePath[SDL_LIBRETRO_MAX_PATH];  /** Full path to the .zip file. */
    char archiveFile[SDL_LIBRETRO_MAX_PATH];  /** The inner entry loaded as content. */
} SDL_Libretro_ZipMount;

/**
 * Close the archive and free the mount. Installed as SDL_Libretro.zipMountFree.
 *
 * @internal
 */
static void SDL_Libretro_ZipMount_Free(void* mount) {
    SDL_Libretro_ZipMount* m = (SDL_Libretro_ZipMount*)mount;
    if (!m) return;
    if (m->storage) SDL_CloseStorage(m->storage);
    SDL_free(m);
}

/* --- Owning in-memory SDL_IOStream over a decompressed entry ------------- */

/**
 * @internal
 */
typedef struct {
    Uint8* data; /** The decompressed entry bytes; owned (freed on close). */
    Sint64 size;
    Sint64 pos;
} SDL_Libretro_ZipMemIO;

/**
 * @internal
 */
static Sint64 SDLCALL SDL_Libretro_ZipMemIO_Size(void* userdata) {
    return ((SDL_Libretro_ZipMemIO*)userdata)->size;
}

/**
 * @internal
 */
static Sint64 SDLCALL SDL_Libretro_ZipMemIO_Seek(void* userdata, Sint64 offset, SDL_IOWhence whence) {
    SDL_Libretro_ZipMemIO* m = (SDL_Libretro_ZipMemIO*)userdata;
    Sint64 base;
    switch (whence) {
        case SDL_IO_SEEK_SET: base = 0;      break;
        case SDL_IO_SEEK_CUR: base = m->pos;  break;
        case SDL_IO_SEEK_END: base = m->size; break;
        default: return SDL_SetError("Invalid seek whence"), -1;
    }
    Sint64 newpos = base + offset;
    if (newpos < 0) return SDL_SetError("Seek before start of stream"), -1;
    m->pos = newpos;
    return newpos;
}

/**
 * @internal
 */
static size_t SDLCALL SDL_Libretro_ZipMemIO_Read(void* userdata, void* ptr, size_t size, SDL_IOStatus* status) {
    SDL_Libretro_ZipMemIO* m = (SDL_Libretro_ZipMemIO*)userdata;
    if (m->pos >= m->size) {
        *status = SDL_IO_STATUS_EOF;
        return 0;
    }
    Sint64 remaining = m->size - m->pos;
    size_t n = (size_t)((Sint64)size < remaining ? (Sint64)size : remaining);
    SDL_memcpy(ptr, m->data + m->pos, n);
    m->pos += (Sint64)n;
    return n;
}

/**
 * @internal
 */
static bool SDLCALL SDL_Libretro_ZipMemIO_Close(void* userdata) {
    SDL_Libretro_ZipMemIO* m = (SDL_Libretro_ZipMemIO*)userdata;
    if (m) {
        SDL_free(m->data);
        SDL_free(m);
    }
    return true;
}

/**
 * Wrap `data` (size bytes) in a read-only SDL_IOStream that frees `data` when
 * the stream is closed. On success the stream takes ownership of `data`; on
 * failure NULL is returned and `data` is left untouched (the caller frees it).
 *
 * @internal
 */
static SDL_IOStream* SDL_Libretro_ZipOpenMemIO(void* data, size_t size) {
    SDL_Libretro_ZipMemIO* m = (SDL_Libretro_ZipMemIO*)SDL_malloc(sizeof(*m));
    if (!m) return NULL;
    m->data = (Uint8*)data;
    m->size = (Sint64)size;
    m->pos = 0;

    SDL_IOStreamInterface iface;
    SDL_zero(iface);
    iface.version = sizeof(iface);
    iface.size = SDL_Libretro_ZipMemIO_Size;
    iface.seek = SDL_Libretro_ZipMemIO_Seek;
    iface.read = SDL_Libretro_ZipMemIO_Read;
    iface.close = SDL_Libretro_ZipMemIO_Close;

    SDL_IOStream* io = SDL_OpenIO(&iface, m);
    if (!io) {
        SDL_free(m); // leave `data` to the caller
        return NULL;
    }
    return io;
}

/* --- Zip-aware VFS callbacks --------------------------------------------- */

/**
 * Resolve `path` to an entry key in the mounted archive. Tries an exact match
 * first, then the basename (cores often prepend a working directory to the
 * virtual path). Returns true and fills *info / keyOut on success.
 *
 * @internal
 */
static bool SDL_Libretro_ZipResolve(SDL_Storage* storage, const char* path,
                                    char* keyOut, size_t keyLen, SDL_PathInfo* info) {
    if (!storage || !path || !path[0]) return false;

    if (SDL_GetStoragePathInfo(storage, path, info)) {
        SDL_strlcpy(keyOut, path, keyLen);
        return true;
    }

    const char* base = SDL_strrchr(path, '/');
    const char* bslash = SDL_strrchr(path, '\\');
    if (bslash && (!base || bslash > base)) base = bslash;
    base = base ? base + 1 : NULL;
    if (base && base[0] && SDL_GetStoragePathInfo(storage, base, info)) {
        SDL_strlcpy(keyOut, base, keyLen);
        return true;
    }
    return false;
}

/**
 * @internal
 */
static struct retro_vfs_file_handle* SDL_Libretro_ZipVFS_Open(const char* path, unsigned mode, unsigned hints) {
    SDL_Libretro* lr = SDL_Libretro_active;
    SDL_Libretro_ZipMount* m = lr ? (SDL_Libretro_ZipMount*)lr->zipMount : NULL;

    // Only intercept read-only opens that resolve to a file in the archive.
    // Writes and non-archive paths fall through to the on-disk SDL VFS.
    if (m && m->storage && path && !(mode & RETRO_VFS_FILE_ACCESS_WRITE)) {
        char key[SDL_LIBRETRO_MAX_PATH];
        SDL_PathInfo info;
        if (SDL_Libretro_ZipResolve(m->storage, path, key, sizeof(key), &info) &&
            info.type == SDL_PATHTYPE_FILE) {
            size_t size = (size_t)info.size;
            void* buf = SDL_malloc(size ? size : 1);
            if (buf && (size == 0 || SDL_ReadStorageFile(m->storage, key, buf, (Uint64)size))) {
                SDL_IOStream* io = SDL_Libretro_ZipOpenMemIO(buf, size); // takes ownership on success
                if (io) {
                    struct retro_vfs_file_handle* h =
                        (struct retro_vfs_file_handle*)SDL_malloc(sizeof(*h));
                    if (h) {
                        h->io = io;
                        h->path = SDL_strdup(path);
                        h->mode = mode;
                        return h;
                    }
                    SDL_CloseIO(io); // frees buf + the memio context
                    return NULL;
                }
            }
            SDL_free(buf); // alloc/read/memio failure; the entry exists but we could not serve it
            return NULL;
        }
    }

    return SDL_Libretro_VFS_Open(path, mode, hints);
}

/**
 * @internal
 */
static int SDL_Libretro_ZipVFS_Stat64(const char* path, int64_t* size) {
    SDL_Libretro* lr = SDL_Libretro_active;
    SDL_Libretro_ZipMount* m = lr ? (SDL_Libretro_ZipMount*)lr->zipMount : NULL;

    if (m && m->storage && path) {
        char key[SDL_LIBRETRO_MAX_PATH];
        SDL_PathInfo info;
        if (SDL_Libretro_ZipResolve(m->storage, path, key, sizeof(key), &info)) {
            int flags = RETRO_VFS_STAT_IS_VALID;
            if (info.type == SDL_PATHTYPE_DIRECTORY) flags |= RETRO_VFS_STAT_IS_DIRECTORY;
            if (size) *size = (int64_t)info.size;
            return flags;
        }
    }

    return SDL_Libretro_VFS_Stat64(path, size);
}

/**
 * @internal
 */
static int SDL_Libretro_ZipVFS_Stat(const char* path, int32_t* size) {
    int64_t out64 = 0;
    int flags = SDL_Libretro_ZipVFS_Stat64(path, &out64);
    if (size) {
        *size = (out64 > SDL_MAX_SINT32) ? SDL_MAX_SINT32 : (int32_t)out64;
    }
    return flags;
}

/* --- Directory enumeration ------------------------------------------------
 * Only opendir is overridden: it fills the shared retro_vfs_dir_handle with the
 * archive's entries, so the default readdir / dirent_get_name / dirent_is_dir /
 * closedir callbacks iterate it unchanged (same allocation contract). */

/**
 * @internal
 */
typedef struct {
    SDL_Storage* storage;
    char** names;
    bool* isDir;
    size_t count;
    size_t capacity;
    bool includeHidden;
} SDL_Libretro_ZipDirCollect;

/**
 * @internal
 */
static SDL_EnumerationResult SDLCALL SDL_Libretro_ZipDirCB(void* userdata, const char* dirname, const char* fname) {
    SDL_Libretro_ZipDirCollect* ctx = (SDL_Libretro_ZipDirCollect*)userdata;
    if (!ctx->includeHidden && fname[0] == '.') return SDL_ENUM_CONTINUE;

    if (ctx->count >= ctx->capacity) {
        size_t newCap = ctx->capacity ? ctx->capacity * 2 : 16;
        // Store each successful realloc back immediately so a partial failure leaves
        // no dangling pointer for the cleanup path (matches the SDL VFS collector).
        char** newNames = (char**)SDL_realloc(ctx->names, newCap * sizeof(char*));
        if (newNames) ctx->names = newNames;
        bool* newIsDir = (bool*)SDL_realloc(ctx->isDir, newCap * sizeof(bool));
        if (newIsDir) ctx->isDir = newIsDir;
        if (!newNames || !newIsDir) return SDL_ENUM_FAILURE;
        ctx->capacity = newCap;
    }

    ctx->names[ctx->count] = SDL_strdup(fname);
    if (!ctx->names[ctx->count]) return SDL_ENUM_FAILURE;

    char full[SDL_LIBRETRO_MAX_PATH];
    SDL_snprintf(full, sizeof(full), "%s%s", dirname ? dirname : "", fname);
    SDL_PathInfo info;
    ctx->isDir[ctx->count] = SDL_GetStoragePathInfo(ctx->storage, full, &info) &&
                             info.type == SDL_PATHTYPE_DIRECTORY;
    ctx->count++;
    return SDL_ENUM_CONTINUE;
}

/**
 * @internal
 */
static struct retro_vfs_dir_handle* SDL_Libretro_ZipVFS_Opendir(const char* dir, bool include_hidden) {
    SDL_Libretro* lr = SDL_Libretro_active;
    SDL_Libretro_ZipMount* m = lr ? (SDL_Libretro_ZipMount*)lr->zipMount : NULL;

    if (m && m->storage && dir) {
        SDL_Libretro_ZipDirCollect ctx;
        SDL_zero(ctx);
        ctx.storage = m->storage;
        ctx.includeHidden = include_hidden;

        // Enumerating the archive at this prefix yields entries only when `dir` is a
        // directory inside the .zip; an empty result means it is not (fall through to
        // disk). This works whether or not the archive stores explicit dir entries.
        if (SDL_EnumerateStorageDirectory(m->storage, dir, SDL_Libretro_ZipDirCB, &ctx) && ctx.count > 0) {
            struct retro_vfs_dir_handle* h =
                (struct retro_vfs_dir_handle*)SDL_malloc(sizeof(*h));
            if (h) {
                h->dir = SDL_strdup(dir);
                h->names = ctx.names;
                h->isDir = ctx.isDir;
                h->count = ctx.count;
                h->index = (size_t)-1; // readdir advances before returning
                return h;
            }
        }

        // Not an archive directory, empty, or allocation failed: free partial results.
        for (size_t i = 0; i < ctx.count; i++) SDL_free(ctx.names[i]);
        SDL_free(ctx.names);
        SDL_free(ctx.isDir);
    }

    return SDL_Libretro_VFS_Opendir(dir, include_hidden);
}

/**
 * Install the zip-aware VFS and record the mount on the context. The archive is
 * released (and the default VFS restored) by SDL_Libretro_UnloadGame.
 *
 * @internal
 */
static void SDL_Libretro_MountZipVFS(SDL_Libretro* lr, SDL_Libretro_ZipMount* mount) {
    if (!lr || !mount) return;
    lr->zipMount = mount;
    lr->zipMountFree = SDL_Libretro_ZipMount_Free;

    // Only the archive-aware members are set; SDL_Libretro_SetVFS fills the rest
    // with the SDL defaults, which operate correctly on our in-memory handles.
    struct retro_vfs_interface zipvfs;
    SDL_zero(zipvfs);
    zipvfs.open = SDL_Libretro_ZipVFS_Open;
    zipvfs.stat = SDL_Libretro_ZipVFS_Stat;
    zipvfs.stat_64 = SDL_Libretro_ZipVFS_Stat64;
    zipvfs.opendir = SDL_Libretro_ZipVFS_Opendir;
    SDL_Libretro_SetVFS(lr, &zipvfs);
}

/**
 * Release the mounted archive (if any) and restore the default SDL-backed VFS.
 * Used by the zip loader's failure paths; the base SDL_Libretro_UnloadGame does
 * the equivalent teardown through the generic zipMountFree pointer.
 *
 * @internal
 */
static void SDL_Libretro_ZipUnmount(SDL_Libretro* lr) {
    if (lr && lr->zipMount && lr->zipMountFree) {
        lr->zipMountFree(lr->zipMount);
        lr->zipMount = NULL;
        lr->zipMountFree = NULL;
        SDL_Libretro_SetVFS(lr, NULL);
    }
}

#endif /* SDL_LIBRETRO_VFS_MINIZIP_IMPL_ONCE */
