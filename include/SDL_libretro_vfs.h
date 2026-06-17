#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_VFS_IMPL_ONCE)
#define SDL_LIBRETRO_VFS_IMPL_ONCE

/*
 * SDL_libretro - libretro VFS interface backed by SDL_IOStream
 *
 * Provides GET_VFS_INTERFACE v1–v3. Truncate is best-effort (no SDL3 native).
 */

#define SDL_LIBRETRO_VFS_SUPPORTED_VERSION 3

struct retro_vfs_file_handle {
    SDL_IOStream* io;
    char* path;
    unsigned mode;
};

struct retro_vfs_dir_handle {
    char* dir;
    char** names;
    bool* isDir;
    size_t count;
    size_t index;
};

/* ---- file helpers ---- */

static const char* SDL_Libretro_VFS_GetPath(struct retro_vfs_file_handle* stream) {
    return stream ? stream->path : NULL;
}

static struct retro_vfs_file_handle* SDL_Libretro_VFS_Open(const char* path, unsigned mode, unsigned hints) {
    (void)hints;
    if (!path) return NULL;

    const char* sdlMode;
    bool update = (mode & RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING) != 0;
    bool read   = (mode & RETRO_VFS_FILE_ACCESS_READ)  != 0;
    bool write  = (mode & RETRO_VFS_FILE_ACCESS_WRITE) != 0;

    if (read && write) {
        sdlMode = update ? "r+b" : "w+b";
    } else if (write) {
        sdlMode = update ? "r+b" : "wb";
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

static int SDL_Libretro_VFS_Close(struct retro_vfs_file_handle* stream) {
    if (!stream) return -1;
    int ret = SDL_CloseIO(stream->io) ? 0 : -1;
    SDL_free(stream->path);
    SDL_free(stream);
    return ret;
}

static int64_t SDL_Libretro_VFS_Size(struct retro_vfs_file_handle* stream) {
    if (!stream) return -1;
    int64_t sz = SDL_GetIOSize(stream->io);
    return sz < 0 ? -1 : sz;
}

static int64_t SDL_Libretro_VFS_Tell(struct retro_vfs_file_handle* stream) {
    if (!stream) return -1;
    return SDL_TellIO(stream->io);
}

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

static int64_t SDL_Libretro_VFS_Read(struct retro_vfs_file_handle* stream, void* s, uint64_t len) {
    if (!stream || !s) return -1;
    size_t n = SDL_ReadIO(stream->io, s, (size_t)len);
    if (n == 0 && SDL_GetIOStatus(stream->io) == SDL_IO_STATUS_ERROR) return -1;
    return (int64_t)n;
}

static int64_t SDL_Libretro_VFS_Write(struct retro_vfs_file_handle* stream, const void* s, uint64_t len) {
    if (!stream || !s) return -1;
    size_t n = SDL_WriteIO(stream->io, s, (size_t)len);
    if (n == 0 && SDL_GetIOStatus(stream->io) == SDL_IO_STATUS_ERROR) return -1;
    return (int64_t)n;
}

static int SDL_Libretro_VFS_Flush(struct retro_vfs_file_handle* stream) {
    if (!stream) return -1;
    return SDL_FlushIO(stream->io) ? 0 : -1;
}

static int SDL_Libretro_VFS_Remove(const char* path) {
    return (path && SDL_RemovePath(path)) ? 0 : -1;
}

static int SDL_Libretro_VFS_Rename(const char* old_path, const char* new_path) {
    return (old_path && new_path && SDL_RenamePath(old_path, new_path)) ? 0 : -1;
}

/* Best-effort truncate: grow via zero-pad, shrink via rewrite. */
static int64_t SDL_Libretro_VFS_Truncate(struct retro_vfs_file_handle* stream, int64_t length) {
    if (!stream || length < 0) return -1;

    int64_t cur_size = SDL_GetIOSize(stream->io);
    if (cur_size < 0) return -1;

    if (length >= cur_size) {
        /* Grow: seek to end and write zeros. */
        if (SDL_SeekIO(stream->io, 0, SDL_IO_SEEK_END) < 0) return -1;
        int64_t to_pad = length - cur_size;
        if (to_pad > 0) {
            Uint8 zero = 0;
            for (int64_t i = 0; i < to_pad; i++) {
                if (SDL_WriteIO(stream->io, &zero, 1) != 1) return -1;
            }
        }
        return 0;
    }

    /* Shrink: read first `length` bytes, rewrite the file. */
    Uint8* buf = (Uint8*)SDL_malloc((size_t)length);
    if (!buf) return -1;

    if (SDL_SeekIO(stream->io, 0, SDL_IO_SEEK_SET) < 0) { SDL_free(buf); return -1; }
    size_t got = SDL_ReadIO(stream->io, buf, (size_t)length);
    if ((int64_t)got != length) { SDL_free(buf); return -1; }

    SDL_CloseIO(stream->io);
    stream->io = SDL_IOFromFile(stream->path, "wb");
    if (!stream->io) { SDL_free(buf); stream->io = SDL_IOFromFile(stream->path, "rb"); return -1; }
    SDL_WriteIO(stream->io, buf, (size_t)length);
    SDL_free(buf);

    /* Reopen in original mode for continued use. */
    SDL_CloseIO(stream->io);
    bool rw = (stream->mode & RETRO_VFS_FILE_ACCESS_READ_WRITE) == RETRO_VFS_FILE_ACCESS_READ_WRITE;
    stream->io = SDL_IOFromFile(stream->path, rw ? "r+b" : "rb");
    return stream->io ? 0 : -1;
}

/* ---- stat / mkdir ---- */

static int SDL_Libretro_VFS_Stat(const char* path, int32_t* size) {
    if (!path) return 0;
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path, &info)) return 0;
    int flags = RETRO_VFS_STAT_IS_VALID;
    if (info.type == SDL_PATHTYPE_DIRECTORY) flags |= RETRO_VFS_STAT_IS_DIRECTORY;
    if (size) *size = (int32_t)info.size;
    return flags;
}

static int SDL_Libretro_VFS_Mkdir(const char* dir) {
    if (!dir) return -1;
    SDL_PathInfo info;
    if (SDL_GetPathInfo(dir, &info)) return -2; /* already exists */
    return SDL_CreateDirectory(dir) ? 0 : -1;
}

/* ---- directory enumeration ---- */

typedef struct {
    char** names;
    bool* isDir;
    size_t count;
    size_t capacity;
    const char* dirPath;
    bool includeHidden;
} SDL_Libretro_DirCollect;

static SDL_EnumerationResult SDL_Libretro_DirCallback(void* userdata, const char* dirname, const char* fname) {
    (void)dirname;
    SDL_Libretro_DirCollect* ctx = (SDL_Libretro_DirCollect*)userdata;
    if (!ctx->includeHidden && fname[0] == '.') return SDL_ENUM_CONTINUE;

    if (ctx->count >= ctx->capacity) {
        size_t newCap = ctx->capacity ? ctx->capacity * 2 : 32;
        char** newNames = (char**)SDL_realloc(ctx->names, newCap * sizeof(char*));
        bool* newIsDir  = (bool*)SDL_realloc(ctx->isDir,  newCap * sizeof(bool));
        if (!newNames || !newIsDir) return SDL_ENUM_FAILURE;
        ctx->names = newNames;
        ctx->isDir = newIsDir;
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

static struct retro_vfs_dir_handle* SDL_Libretro_VFS_Opendir(const char* dir, bool include_hidden) {
    if (!dir) return NULL;

    SDL_Libretro_DirCollect ctx;
    SDL_memset(&ctx, 0, sizeof(ctx));
    ctx.dirPath = dir;
    ctx.includeHidden = include_hidden;

    if (!SDL_EnumerateDirectory(dir, SDL_Libretro_DirCallback, &ctx)) {
        /* Free partial results on failure. */
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
    h->index = (size_t)-1; /* readdir advances before returning */
    return h;
}

static bool SDL_Libretro_VFS_Readdir(struct retro_vfs_dir_handle* dirstream) {
    if (!dirstream) return false;
    size_t next = (dirstream->index == (size_t)-1) ? 0 : dirstream->index + 1;
    if (next >= dirstream->count) return false;
    dirstream->index = next;
    return true;
}

static const char* SDL_Libretro_VFS_DirentGetName(struct retro_vfs_dir_handle* dirstream) {
    if (!dirstream || dirstream->index >= dirstream->count) return NULL;
    return dirstream->names[dirstream->index];
}

static bool SDL_Libretro_VFS_DirentIsDir(struct retro_vfs_dir_handle* dirstream) {
    if (!dirstream || dirstream->index >= dirstream->count) return false;
    return dirstream->isDir[dirstream->index];
}

static int SDL_Libretro_VFS_Closedir(struct retro_vfs_dir_handle* dirstream) {
    if (!dirstream) return -1;
    for (size_t i = 0; i < dirstream->count; i++) SDL_free(dirstream->names[i]);
    SDL_free(dirstream->names);
    SDL_free(dirstream->isDir);
    SDL_free(dirstream->dir);
    SDL_free(dirstream);
    return 0;
}

static struct retro_vfs_interface SDL_Libretro_vfs_interface = {
    SDL_Libretro_VFS_GetPath,
    SDL_Libretro_VFS_Open,
    SDL_Libretro_VFS_Close,
    SDL_Libretro_VFS_Size,
    SDL_Libretro_VFS_Tell,
    SDL_Libretro_VFS_Seek,
    SDL_Libretro_VFS_Read,
    SDL_Libretro_VFS_Write,
    SDL_Libretro_VFS_Flush,
    SDL_Libretro_VFS_Remove,
    SDL_Libretro_VFS_Rename,
    /* v2 */
    SDL_Libretro_VFS_Truncate,
    /* v3 */
    SDL_Libretro_VFS_Stat,
    SDL_Libretro_VFS_Mkdir,
    SDL_Libretro_VFS_Opendir,
    SDL_Libretro_VFS_Readdir,
    SDL_Libretro_VFS_DirentGetName,
    SDL_Libretro_VFS_DirentIsDir,
    SDL_Libretro_VFS_Closedir,
};

#endif /* SDL_LIBRETRO_VFS_IMPL_ONCE */
