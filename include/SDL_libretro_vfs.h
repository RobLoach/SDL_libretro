/**
 * SDL_libretro - SDL3 Virtual File System interface for libretro
 *
 * @file SDL_libretro_vfs.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_VFS_IMPL_ONCE)
#define SDL_LIBRETRO_VFS_IMPL_ONCE


#ifdef __DOXYGEN
/**
 * Allows disabling the SDL3 libretro Virtual File System.
 *
 * Use this if you're implementing your own VFS.
 *
 * @see SDL_Libretro_SetVFS
 */
#define SDL_LIBRETRO_DISABLE_VFS
#endif

#ifndef SDL_LIBRETRO_DISABLE_VFS

/*
 * SDL_libretro - libretro VFS interface backed by SDL_IOStream
 *
 * Provides GET_VFS_INTERFACE v1–v4.
 */

/**
 * @internal
 */
struct retro_vfs_file_handle {
    SDL_IOStream* io;
    char* path;
    unsigned mode;
};

/**
 * @internal
 */
struct retro_vfs_dir_handle {
    char* dir;
    char** names;
    bool* isDir;
    size_t count;
    size_t index;
};

/**
 * @internal
 */
static const char* SDL_Libretro_VFS_GetPath(struct retro_vfs_file_handle* stream) {
    return stream ? stream->path : NULL;
}

/**
 * @internal
 */
static struct retro_vfs_file_handle* SDL_Libretro_VFS_Open(const char* path, unsigned mode, unsigned hints) {
    (void)hints;
    if (!path) return NULL;

    // The VFS contract requires open() to fail on a directory; SDL_IOFromFile may otherwise succeed in opening one for read on some platforms.
    SDL_PathInfo pathInfo;
    if (SDL_GetPathInfo(path, &pathInfo) && pathInfo.type == SDL_PATHTYPE_DIRECTORY) return NULL;

    const char* sdlMode;
    bool update = (mode & RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING) != 0;
    bool read   = (mode & RETRO_VFS_FILE_ACCESS_READ)  != 0;
    bool write  = (mode & RETRO_VFS_FILE_ACCESS_WRITE) != 0;

    if (read && write) {
        sdlMode = update ? "r+b" : "w+b";
    } else if (write) {
        sdlMode = "wb";
    } else {
        sdlMode = "rb";
    }

    SDL_IOStream* io = SDL_IOFromFile(path, sdlMode);
    if (!io) return NULL;

    struct retro_vfs_file_handle* h = (struct retro_vfs_file_handle*)SDL_malloc(sizeof(*h));
    if (!h) { SDL_CloseIO(io); return NULL; }
    h->io   = io;
    h->path = SDL_strdup(path);
    h->mode = mode;
    return h;
}

/**
 * @internal
 */
static int SDL_Libretro_VFS_Close(struct retro_vfs_file_handle* stream) {
    if (!stream) return -1;
    int ret = SDL_CloseIO(stream->io) ? 0 : -1;
    SDL_free(stream->path);
    SDL_free(stream);
    return ret;
}

/**
 * @internal
 */
static int64_t SDL_Libretro_VFS_Size(struct retro_vfs_file_handle* stream) {
    if (!stream) return -1;
    int64_t sz = SDL_GetIOSize(stream->io);
    return sz < 0 ? -1 : sz;
}

/**
 * @internal
 */
static int64_t SDL_Libretro_VFS_Tell(struct retro_vfs_file_handle* stream) {
    if (!stream) return -1;
    return SDL_TellIO(stream->io);
}

/**
 * @internal
 */
static int64_t SDL_Libretro_VFS_Seek(struct retro_vfs_file_handle* stream, int64_t offset, int seek_position) {
    if (!stream) return -1;
    SDL_IOWhence whence;
    switch (seek_position) {
        case RETRO_VFS_SEEK_POSITION_START:   whence = SDL_IO_SEEK_SET; break;
        case RETRO_VFS_SEEK_POSITION_CURRENT: whence = SDL_IO_SEEK_CUR; break;
        case RETRO_VFS_SEEK_POSITION_END:     whence = SDL_IO_SEEK_END; break;
        default: return -1;
    }
    return SDL_SeekIO(stream->io, offset, whence);
}

/**
 * @internal
 */
static int64_t SDL_Libretro_VFS_Read(struct retro_vfs_file_handle* stream, void* s, uint64_t len) {
    if (!stream || !s) return -1;
    size_t n = SDL_ReadIO(stream->io, s, (size_t)len);
    if (n == 0 && SDL_GetIOStatus(stream->io) == SDL_IO_STATUS_ERROR) return -1;
    return (int64_t)n;
}

/**
 * @internal
 */
static int64_t SDL_Libretro_VFS_Write(struct retro_vfs_file_handle* stream, const void* s, uint64_t len) {
    if (!stream || !s) return -1;
    size_t n = SDL_WriteIO(stream->io, s, (size_t)len);
    if (n == 0 && SDL_GetIOStatus(stream->io) == SDL_IO_STATUS_ERROR) return -1;
    return (int64_t)n;
}

/**
 * @internal
 */
static int SDL_Libretro_VFS_Flush(struct retro_vfs_file_handle* stream) {
    if (!stream) return -1;
    return SDL_FlushIO(stream->io) ? 0 : -1;
}

/**
 * @internal
 */
static int SDL_Libretro_VFS_Remove(const char* path) {
    return (path && SDL_RemovePath(path)) ? 0 : -1;
}

/**
 * @internal
 */
static int SDL_Libretro_VFS_Rename(const char* old_path, const char* new_path) {
    return (old_path && new_path && SDL_RenamePath(old_path, new_path)) ? 0 : -1;
}

/**
 * Truncate a file to a specified size.
 *
 * @param path A null-terminated string specifying the path of the file to be truncated. Must refer to a file that the caller is permitted to modify.
 * @param length The target size of the file in bytes. Must be non-negative.
 *
 * @return 0 on success. On error, -1 is returned.
 * @internal
 */
static int64_t SDL_Libretro_VFS_Truncate(struct retro_vfs_file_handle* stream, int64_t length) {
    if (!stream || length < 0) return -1;

    int64_t cur_size = SDL_GetIOSize(stream->io);
    if (cur_size < 0) return -1;

    // Preserve the caller's read/write position across the operation.
    int64_t orig_pos = SDL_TellIO(stream->io);

    if (length >= cur_size) {
        // Grow: seek to end and pad with zeros in chunks.
        if (SDL_SeekIO(stream->io, 0, SDL_IO_SEEK_END) < 0) return -1;
        int64_t to_pad = length - cur_size;
        if (to_pad > 0) {
            Uint8 zeros[4096];
            SDL_memset(zeros, 0, sizeof(zeros));
            while (to_pad > 0) {
                size_t chunk = (to_pad < (int64_t)sizeof(zeros)) ? (size_t)to_pad : sizeof(zeros);
                if (SDL_WriteIO(stream->io, zeros, chunk) != chunk) return -1;
                to_pad -= (int64_t)chunk;
            }
        }
        if (orig_pos >= 0) SDL_SeekIO(stream->io, orig_pos, SDL_IO_SEEK_SET);
        return 0;
    }

    // Read the first `length` bytes, then rewrite the file. length == 0 (truncate to empty) needs no buffer.
    Uint8* buf = NULL;
    if (length > 0) {
        buf = (Uint8*)SDL_malloc((size_t)length);
        if (!buf) return -1;

        if (SDL_SeekIO(stream->io, 0, SDL_IO_SEEK_SET) < 0) { SDL_free(buf); return -1; }
        size_t got = SDL_ReadIO(stream->io, buf, (size_t)length);
        if ((int64_t)got != length) { SDL_free(buf); return -1; }
    }

    // Reopen with write access matching the original handle so subsequent reads/writes keep working.
    bool canWrite = (stream->mode & RETRO_VFS_FILE_ACCESS_WRITE) != 0;

    SDL_CloseIO(stream->io);
    stream->io = SDL_IOFromFile(stream->path, "wb");
    if (!stream->io) { SDL_free(buf); stream->io = SDL_IOFromFile(stream->path, canWrite ? "r+b" : "rb"); return -1; }
    // A short write would leave the file the wrong size; report that as failure.
    if (length > 0 && (int64_t)SDL_WriteIO(stream->io, buf, (size_t)length) != length) {
        SDL_free(buf);
        SDL_CloseIO(stream->io);
        stream->io = SDL_IOFromFile(stream->path, canWrite ? "r+b" : "rb");
        return -1;
    }
    SDL_free(buf);

    // Reopen in original mode for continued use.
    SDL_CloseIO(stream->io);
    stream->io = SDL_IOFromFile(stream->path, canWrite ? "r+b" : "rb");
    if (!stream->io) return -1;

    // Restore the position, clamped to the new (smaller) length.
    if (orig_pos >= 0) {
        SDL_SeekIO(stream->io, orig_pos < length ? orig_pos : length, SDL_IO_SEEK_SET);
    }
    return 0;
}

/**
 * @internal
 */
static int SDL_Libretro_VFS_Stat64(const char* path, int64_t* size) {
    if (!path) return 0;
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path, &info)) {
        return 0;
    }

    int flags = RETRO_VFS_STAT_IS_VALID;
    if (info.type == SDL_PATHTYPE_DIRECTORY) {
        flags |= RETRO_VFS_STAT_IS_DIRECTORY;
    }

    if (size != NULL) {
        *size = info.size;
    }
    return flags;
}

/**
 * @internal
 */
static int SDL_Libretro_VFS_Stat(const char* path, int32_t* size) {
    if (!path) return 0;
    int64_t outSize = 0;
    int out = SDL_Libretro_VFS_Stat64(path, &outSize);
    if (size != NULL) {
        if (outSize > SDL_MAX_SINT32) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "[SDL_Libretro] VFS stat size %" SDL_PRIs64 " for '%s' exceeds int32. Clamping to SDL_MAX_SINT32.",
                outSize, path);
            *size = SDL_MAX_SINT32;
        }
        else {
            *size = (int32_t)outSize;
        }
    }
    return out;
}

/**
 * @internal
 */
static int SDL_Libretro_VFS_Mkdir(const char* dir) {
    if (!dir) return -1;
    SDL_PathInfo info;
    if (SDL_GetPathInfo(dir, &info)) return -2; /* already exists */
    return SDL_CreateDirectory(dir) ? 0 : -1;
}

/**
 * @internal
 */
typedef struct {
    char** names;
    bool* isDir;
    size_t count;
    size_t capacity;
    const char* dirPath;
    bool includeHidden;
} SDL_Libretro_DirCollect;

/**
 * @internal
 */
static SDL_EnumerationResult SDL_Libretro_DirCallback(void* userdata, const char* dirname, const char* fname) {
    (void)dirname;
    SDL_Libretro_DirCollect* ctx = (SDL_Libretro_DirCollect*)userdata;
    if (!ctx->includeHidden && fname[0] == '.') return SDL_ENUM_CONTINUE;

    if (ctx->count >= ctx->capacity) {
        size_t newCap = ctx->capacity ? ctx->capacity * 2 : 32;
        // Store each successful realloc back immediately: realloc frees the old block, so on partial failure the cleanup path must not be left holding a dangling pointer (use-after-free / double-free).
        char** newNames = (char**)SDL_realloc(ctx->names, newCap * sizeof(char*));
        if (newNames) ctx->names = newNames;
        bool* newIsDir  = (bool*)SDL_realloc(ctx->isDir,  newCap * sizeof(bool));
        if (newIsDir) ctx->isDir = newIsDir;
        if (!newNames || !newIsDir) return SDL_ENUM_FAILURE;
        ctx->capacity = newCap;
    }

    ctx->names[ctx->count] = SDL_strdup(fname);
    if (!ctx->names[ctx->count]) return SDL_ENUM_FAILURE;

    /* Determine if entry is a directory. */
    char fullpath[SDL_LIBRETRO_MAX_PATH];
    SDL_snprintf(fullpath, sizeof(fullpath), "%s/%s", ctx->dirPath, fname);
    SDL_PathInfo info;
    ctx->isDir[ctx->count] = SDL_GetPathInfo(fullpath, &info) && info.type == SDL_PATHTYPE_DIRECTORY;
    ctx->count++;
    return SDL_ENUM_CONTINUE;
}

/**
 * @internal
 */
static struct retro_vfs_dir_handle* SDL_Libretro_VFS_Opendir(const char* dir, bool include_hidden) {
    if (!dir) return NULL;

    SDL_Libretro_DirCollect ctx;
    SDL_memset(&ctx, 0, sizeof(ctx));
    ctx.dirPath = dir;
    ctx.includeHidden = include_hidden;

    if (!SDL_EnumerateDirectory(dir, SDL_Libretro_DirCallback, &ctx)) {
        // Free partial results on failure.
        for (size_t i = 0; i < ctx.count; i++) SDL_free(ctx.names[i]);
        SDL_free(ctx.names);
        SDL_free(ctx.isDir);
        return NULL;
    }

    struct retro_vfs_dir_handle* h = (struct retro_vfs_dir_handle*)SDL_malloc(sizeof(*h));
    if (!h) {
        for (size_t i = 0; i < ctx.count; i++) SDL_free(ctx.names[i]);
        SDL_free(ctx.names);
        SDL_free(ctx.isDir);
        return NULL;
    }
    h->dir   = SDL_strdup(dir);
    h->names = ctx.names;
    h->isDir = ctx.isDir;
    h->count = ctx.count;
    h->index = (size_t)-1; // readdir advances before returning
    return h;
}

/**
 * @internal
 */
static bool SDL_Libretro_VFS_Readdir(struct retro_vfs_dir_handle* dirstream) {
    if (!dirstream) return false;
    size_t next = (dirstream->index == (size_t)-1) ? 0 : dirstream->index + 1;
    if (next >= dirstream->count) return false;
    dirstream->index = next;
    return true;
}

/**
 * @internal
 */
static const char* SDL_Libretro_VFS_DirentGetName(struct retro_vfs_dir_handle* dirstream) {
    if (!dirstream || dirstream->index >= dirstream->count) return NULL;
    return dirstream->names[dirstream->index];
}

/**
 * @internal
 */
static bool SDL_Libretro_VFS_DirentIsDir(struct retro_vfs_dir_handle* dirstream) {
    if (!dirstream || dirstream->index >= dirstream->count) return false;
    return dirstream->isDir[dirstream->index];
}

/**
 * @internal
 */
static int SDL_Libretro_VFS_Closedir(struct retro_vfs_dir_handle* dirstream) {
    if (!dirstream) return -1;
    for (size_t i = 0; i < dirstream->count; i++) SDL_free(dirstream->names[i]);
    SDL_free(dirstream->names);
    SDL_free(dirstream->isDir);
    SDL_free(dirstream->dir);
    SDL_free(dirstream);
    return 0;
}

/**
 * Set the Virtual File System for libretro.
 *
 * @param lr the libretro context.
 * @param vfs_interface A void* pointing to `struct retro_vfs_interface`. When set to NULL, will set the SDL3's File System.
 *
 * @see retro_vfs_interface
 */
void SDL_Libretro_SetVFS(SDL_Libretro* lr, void* vfs_interface) {
    if (!lr) return;
    struct retro_vfs_interface temp = {0};
    struct retro_vfs_interface* vfs = (struct retro_vfs_interface*)vfs_interface;
    if (vfs == NULL) {
        vfs = &temp;
    }

    lr->vfs_interface.get_path = vfs->get_path != NULL ? vfs->get_path : &SDL_Libretro_VFS_GetPath;
    lr->vfs_interface.open = vfs->open != NULL ? vfs->open : &SDL_Libretro_VFS_Open;
    lr->vfs_interface.close = vfs->close != NULL ? vfs->close : &SDL_Libretro_VFS_Close;
    lr->vfs_interface.size = vfs->size != NULL ? vfs->size : &SDL_Libretro_VFS_Size;
    lr->vfs_interface.tell = vfs->tell != NULL ? vfs->tell : &SDL_Libretro_VFS_Tell;
    lr->vfs_interface.seek = vfs->seek != NULL ? vfs->seek : &SDL_Libretro_VFS_Seek;
    lr->vfs_interface.read = vfs->read != NULL ? vfs->read : &SDL_Libretro_VFS_Read;
    lr->vfs_interface.write = vfs->write != NULL ? vfs->write : &SDL_Libretro_VFS_Write;
    lr->vfs_interface.flush = vfs->flush != NULL ? vfs->flush : &SDL_Libretro_VFS_Flush;
    lr->vfs_interface.remove = vfs->remove != NULL ? vfs->remove : &SDL_Libretro_VFS_Remove;
    lr->vfs_interface.rename = vfs->rename != NULL ? vfs->rename : &SDL_Libretro_VFS_Rename;
    lr->vfs_interface.truncate = vfs->truncate != NULL ? vfs->truncate : &SDL_Libretro_VFS_Truncate;
    lr->vfs_interface.stat = vfs->stat != NULL ? vfs->stat : &SDL_Libretro_VFS_Stat;
    lr->vfs_interface.mkdir = vfs->mkdir != NULL ? vfs->mkdir : &SDL_Libretro_VFS_Mkdir;
    lr->vfs_interface.opendir = vfs->opendir != NULL ? vfs->opendir : &SDL_Libretro_VFS_Opendir;
    lr->vfs_interface.readdir = vfs->readdir != NULL ? vfs->readdir : &SDL_Libretro_VFS_Readdir;
    lr->vfs_interface.dirent_get_name = vfs->dirent_get_name != NULL ? vfs->dirent_get_name : &SDL_Libretro_VFS_DirentGetName;
    lr->vfs_interface.dirent_is_dir = vfs->dirent_is_dir != NULL ? vfs->dirent_is_dir : &SDL_Libretro_VFS_DirentIsDir;
    lr->vfs_interface.closedir = vfs->closedir != NULL ? vfs->closedir : &SDL_Libretro_VFS_Closedir;
    lr->vfs_interface.stat_64 = vfs->stat_64 != NULL ? vfs->stat_64 : &SDL_Libretro_VFS_Stat64;
}

#endif /* !SDL_LIBRETRO_DISABLE_VFS */

#endif /* SDL_LIBRETRO_VFS_IMPL_ONCE */
