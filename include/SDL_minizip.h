#ifndef SDL_MINIZIP_H
#define SDL_MINIZIP_H

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SDL_MINIZIP_DECLSPEC
#define SDL_MINIZIP_DECLSPEC
#endif

/**
 * \brief Opens a zip archive as an SDL_Storage instance using an SDL_IOStream.
 *
 * This function allows reading files contained within a zip archive transparently
 * through the SDL_Storage API. The provided SDL_IOStream is used as the backing
 * storage for the zip archive.
 *
 * \param src The SDL_IOStream containing the zip archive data.
 * \param closeio true to close the SDL_IOStream whether this function succeeds or fails.
 * \return A new SDL_Storage instance on success, or NULL on failure. Use SDL_GetError() for more information.
 */
SDL_MINIZIP_DECLSPEC SDL_Storage *SDL_OpenMinizipStorage_IO(SDL_IOStream *src, bool closeio);

/**
 * \brief Opens a zip archive as an SDL_Storage instance from a file path.
 *
 * This is a convenience wrapper around SDL_OpenMinizipStorage_IO that
 * automatically opens an SDL_IOStream for the given file path.
 *
 * \param file_path The path to the zip archive file.
 * \return A new SDL_Storage instance on success, or NULL on failure. Use SDL_GetError() for more information.
 */
SDL_MINIZIP_DECLSPEC SDL_Storage *SDL_OpenMinizipStorage(const char *file_path);

/**
 * \brief Opens a zip archive as an SDL_Storage instance from a block of memory.
 *
 * This is a convenience wrapper around SDL_OpenMinizipStorage_IO that
 * automatically opens an SDL_IOStream for the given memory block.
 *
 * \param mem A pointer to a read-only buffer containing the zip archive.
 * \param size The size of the memory buffer in bytes.
 * \return A new SDL_Storage instance on success, or NULL on failure. Use SDL_GetError() for more information.
 */
SDL_MINIZIP_DECLSPEC SDL_Storage *SDL_OpenMinizipStorage_Mem(const void *mem, size_t size);

#ifdef __cplusplus
}
#endif

#endif // SDL_MINIZIP_H

#ifdef SDL_MINIZIP_IMPLEMENTATION
#ifndef SDL_MINIZIP_IMPLEMENTATION_ONCE
#define SDL_MINIZIP_IMPLEMENTATION_ONCE

#ifdef __cplusplus
extern "C" {
#endif

#include <mz.h>
#include <mz_strm.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>

/**
 * Our Minizip interface.
 *
 * \internal
 */
typedef struct {
    mz_stream stream; /** The base minizip class (must be first). */
    void *reader; /** The minizip reader. */
    SDL_IOStream *src; /** The IOStream for the .zip file we are reading. */
    SDL_Mutex *lock; /** Thread-safety. */
    bool closeio; /** Whether or not we are owning closing the IOStream when complete. */
} SDL_Minizip;

static void SDL_Minizip__SetError(int32_t err, const char *msg) {
    if (err == MZ_OK) return;
    switch (err) {
        case MZ_PASSWORD_ERROR: SDL_SetError("%s: Invalid password", msg); break;
        case MZ_CRC_ERROR: SDL_SetError("%s: CRC mismatch", msg); break;
        case MZ_EXIST_ERROR: SDL_SetError("%s: File not found", msg); break;
        case MZ_FORMAT_ERROR: SDL_SetError("%s: Invalid format", msg); break;
        case MZ_MEM_ERROR: SDL_OutOfMemory(); break;
        default: SDL_SetError("%s (Error %d)", msg, (int)err); break;
    }
}

static int32_t SDL_Minizip__open(void *stream, const char *path, int32_t mode) {
    if (!stream || ((SDL_Minizip *)stream)->src == NULL) {
        return MZ_OPEN_ERROR;
    }
    return MZ_OK;
}

static int32_t SDL_Minizip__is_open(void *stream) {
    if (!stream) return MZ_EXIST_ERROR;
    SDL_Minizip *sdl_stream = (SDL_Minizip *)stream;
    if (!stream) {
        return MZ_PARAM_ERROR;
    }
    return (sdl_stream->src != NULL) ? MZ_OK : MZ_EXIST_ERROR;
}

static int32_t SDL_Minizip__read(void *stream, void *buf, int32_t size) {
    if (!stream) return MZ_PARAM_ERROR;
    SDL_Minizip *sdl_stream = (SDL_Minizip *)stream;
    size_t bytes_read = SDL_ReadIO(sdl_stream->src, buf, size);
    if (bytes_read == 0) {
        if (SDL_GetIOStatus(sdl_stream->src) == SDL_IO_STATUS_EOF) {
            return 0; // EOF
        }
        return MZ_READ_ERROR;
    }
    return (int32_t)bytes_read;
}

static int32_t SDL_Minizip__write(void *stream, const void *buf, int32_t size) {
    if (!stream) return MZ_PARAM_ERROR;
    SDL_Minizip *sdl_stream = (SDL_Minizip *)stream;
    size_t bytes_written = SDL_WriteIO(sdl_stream->src, buf, size);
    if (bytes_written != (size_t)size) {
        return MZ_WRITE_ERROR;
    }
    return (int32_t)bytes_written;
}

static int64_t SDL_Minizip__tell(void *stream) {
    if (!stream) return MZ_TELL_ERROR;
    SDL_Minizip *sdl_stream = (SDL_Minizip *)stream;
    Sint64 pos = SDL_TellIO(sdl_stream->src);
    if (pos < 0) return MZ_TELL_ERROR;
    return pos;
}

static int32_t SDL_Minizip__seek(void *stream, int64_t offset, int32_t origin) {
    if (!stream) return MZ_SEEK_ERROR;
    SDL_Minizip *sdl_stream = (SDL_Minizip *)stream;
    int sdl_whence;
    switch (origin) {
        case MZ_SEEK_SET: sdl_whence = SDL_IO_SEEK_SET; break;
        case MZ_SEEK_CUR: sdl_whence = SDL_IO_SEEK_CUR; break;
        case MZ_SEEK_END: sdl_whence = SDL_IO_SEEK_END; break;
        default: return MZ_SEEK_ERROR;
    }
    Sint64 result = SDL_SeekIO(sdl_stream->src, offset, sdl_whence);
    if (result < 0) return MZ_SEEK_ERROR;
    return MZ_OK;
}

static int32_t SDL_Minizip__error(void *stream) {
    return MZ_OK;
}

static mz_stream_vtbl SDL_Minizip__vtbl = {
    SDL_Minizip__open,
    SDL_Minizip__is_open,
    SDL_Minizip__read,
    SDL_Minizip__write,
    SDL_Minizip__tell,
    SDL_Minizip__seek,
    NULL, // close is handled natively in SDL_Minizip__close
    SDL_Minizip__error,
    NULL, // create
    NULL, // destroy
    NULL, // get_prop_int64
    NULL  // set_prop_int64
};

static bool SDLCALL SDL_Minizip__close(void *userdata) {
    SDL_Minizip *ctx = (SDL_Minizip *)userdata;
    if (ctx) {
        if (ctx->reader) {
            mz_zip_reader_close(ctx->reader);
            mz_zip_reader_delete(&(ctx->reader));
        }
        if (ctx->closeio && ctx->src) {
            SDL_CloseIO(ctx->src);
        }
        if (ctx->lock) {
            SDL_DestroyMutex(ctx->lock);
        }
        SDL_free(ctx);
    }
    return true;
}

static bool SDLCALL SDL_Minizip__ready(void *userdata) {
    if (!userdata) return false;
    return true;
}

static bool SDLCALL SDL_Minizip__enumerate(void *userdata, const char *path, SDL_EnumerateDirectoryCallback callback, void *callback_userdata) {
    if (!userdata || !path || !callback) return false;
    SDL_Minizip *ctx = (SDL_Minizip *)userdata;
    SDL_LockMutex(ctx->lock);

    int32_t err = mz_zip_reader_goto_first_entry(ctx->reader);
    size_t path_len = SDL_strlen(path);
    char *search_path = (char *)SDL_malloc(path_len + 2);
    if (!search_path) {
        SDL_UnlockMutex(ctx->lock);
        return false;
    }
    SDL_snprintf(search_path, path_len + 2, "%s%s", path, (path_len > 0 && path[path_len - 1] != '/') ? "/" : "");
    if (path_len == 0) search_path[0] = '\0';
    size_t search_path_len = SDL_strlen(search_path);

    while (err == MZ_OK) {
        mz_zip_file *file_info = NULL;
        err = mz_zip_reader_entry_get_info(ctx->reader, &file_info);
        if (err == MZ_OK && file_info && file_info->filename) {
            if (search_path_len == 0 || SDL_strncmp(file_info->filename, search_path, search_path_len) == 0) {
                const char *rel_name = file_info->filename + search_path_len;
                if (rel_name[0] != '\0') {
                    const char *slash = SDL_strchr(rel_name, '/');
                    if (slash == NULL || (slash[1] == '\0' && (size_t)(slash - rel_name) == SDL_strlen(rel_name) - 1)) {
                        size_t name_len = SDL_strlen(rel_name);
                        char *name_buf = (char *)SDL_malloc(name_len + 1);
                        if (name_buf) {
                            SDL_strlcpy(name_buf, rel_name, name_len + 1);
                            if (name_buf[0] != '\0' && name_buf[name_len - 1] == '/') {
                                name_buf[name_len - 1] = '\0';
                            }
                            callback(callback_userdata, search_path, name_buf);
                            SDL_free(name_buf);
                        }
                    }
                }
            }
        }
        err = mz_zip_reader_goto_next_entry(ctx->reader);
    }

    SDL_free(search_path);
    SDL_UnlockMutex(ctx->lock);
    return true;
}

static bool SDLCALL SDL_Minizip__info(void *userdata, const char *path, SDL_PathInfo *info) {
    if (!userdata || !path || !info) return false;
    SDL_Minizip *ctx = (SDL_Minizip *)userdata;
    SDL_LockMutex(ctx->lock);

    if (mz_zip_reader_locate_entry(ctx->reader, path, 0) == MZ_OK) {
        mz_zip_file *file_info = NULL;
        if (mz_zip_reader_entry_get_info(ctx->reader, &file_info) == MZ_OK) {
            info->size = file_info->uncompressed_size;
            info->type = (mz_zip_attrib_is_dir(file_info->external_fa, file_info->version_madeby) == MZ_OK)
                            ? SDL_PATHTYPE_DIRECTORY : SDL_PATHTYPE_FILE;
            info->create_time = file_info->creation_date;
            info->modify_time = file_info->modified_date;
            info->access_time = file_info->accessed_date;
            SDL_UnlockMutex(ctx->lock);
            return true;
        }
    }

    size_t path_len = SDL_strlen(path);
    char *dir_path = (char *)SDL_malloc(path_len + 2);
    if (!dir_path) {
        SDL_UnlockMutex(ctx->lock);
        return false;
    }
    SDL_snprintf(dir_path, path_len + 2, "%s/", path);
    if (mz_zip_reader_locate_entry(ctx->reader, dir_path, 0) == MZ_OK) {
        mz_zip_file *file_info = NULL;
        if (mz_zip_reader_entry_get_info(ctx->reader, &file_info) == MZ_OK) {
            info->size = 0;
            info->type = SDL_PATHTYPE_DIRECTORY;
            info->create_time = file_info->creation_date;
            info->modify_time = file_info->modified_date;
            info->access_time = file_info->accessed_date;
            SDL_free(dir_path);
            SDL_UnlockMutex(ctx->lock);
            return true;
        }
    }
    SDL_free(dir_path);
    SDL_SetError("File not found in zip");
    SDL_UnlockMutex(ctx->lock);
    return false;
}

static bool SDLCALL SDL_Minizip__read_file(void *userdata, const char *path, void *destination, Uint64 length) {
    if (!userdata || !path || !destination) return false;
    SDL_Minizip *ctx = (SDL_Minizip *)userdata;
    SDL_LockMutex(ctx->lock);

    int32_t err = mz_zip_reader_locate_entry(ctx->reader, path, 0);
    if (err != MZ_OK) {
        SDL_Minizip__SetError(err, "File not found in zip");
        SDL_UnlockMutex(ctx->lock);
        return false;
    }
    err = mz_zip_reader_entry_open(ctx->reader);
    if (err != MZ_OK) {
        SDL_Minizip__SetError(err, "Could not open zip entry for reading");
        SDL_UnlockMutex(ctx->lock);
        return false;
    }
    int32_t bytes_read = mz_zip_reader_entry_read(ctx->reader, destination, (int32_t)length);
    mz_zip_reader_entry_close(ctx->reader);
    if (bytes_read < 0) {
        SDL_Minizip__SetError(bytes_read, "Failed to read from zip entry");
        SDL_UnlockMutex(ctx->lock);
        return false;
    }

    SDL_UnlockMutex(ctx->lock);
    return true;
}

SDL_MINIZIP_DECLSPEC SDL_Storage *SDL_OpenMinizipStorage_IO(SDL_IOStream *src, bool closeio) {
    if (!src) {
        SDL_SetError("Invalid IOStream");
        return NULL;
    }

    SDL_Minizip *ctx = (SDL_Minizip *)SDL_malloc(sizeof(SDL_Minizip));
    if (!ctx) {
        SDL_OutOfMemory();
        if (closeio) SDL_CloseIO(src);
        return NULL;
    }
    SDL_zero(*ctx);
    ctx->src = src;
    ctx->closeio = closeio;
    ctx->lock = SDL_CreateMutex();

    if (!ctx->lock) {
        SDL_free(ctx);
        if (closeio) SDL_CloseIO(src);
        return NULL;
    }

    ctx->stream.vtbl = &SDL_Minizip__vtbl;

    mz_zip_reader_create(&ctx->reader);
    if (!ctx->reader) {
        SDL_SetError("Failed to create zip reader");
        SDL_Minizip__close(ctx);
        return NULL;
    }

    int32_t err = mz_zip_reader_open(ctx->reader, &ctx->stream);
    if (err != MZ_OK) {
        SDL_Minizip__SetError(err, "Failed to open zip from stream");
        SDL_Minizip__close(ctx);
        return NULL;
    }

    SDL_StorageInterface iface;
    SDL_zero(iface);
    iface.version = sizeof(iface);
    iface.close = SDL_Minizip__close;
    iface.ready = SDL_Minizip__ready;
    iface.enumerate = SDL_Minizip__enumerate;
    iface.info = SDL_Minizip__info;
    iface.read_file = SDL_Minizip__read_file;

    SDL_Storage *storage = SDL_OpenStorage(&iface, ctx);
    if (!storage) {
        SDL_Minizip__close(ctx);
        return NULL;
    }

    return storage;
}

SDL_MINIZIP_DECLSPEC SDL_Storage *SDL_OpenMinizipStorage(const char *file_path) {
    if (!file_path) {
        SDL_SetError("Invalid file path");
        return NULL;
    }
    SDL_IOStream *src = SDL_IOFromFile(file_path, "r");
    if (!src) {
        return NULL;
    }
    return SDL_OpenMinizipStorage_IO(src, true);
}

SDL_MINIZIP_DECLSPEC SDL_Storage *SDL_OpenMinizipStorage_Mem(const void *mem, size_t size) {
    if (!mem || size == 0) {
        SDL_SetError("Invalid memory pointer or size");
        return NULL;
    }
    SDL_IOStream *src = SDL_IOFromConstMem(mem, size);
    if (!src) {
        return NULL;
    }
    return SDL_OpenMinizipStorage_IO(src, true);
}

#ifdef __cplusplus
}
#endif

#endif
#endif // SDL_MINIZIP_IMPLEMENTATION
