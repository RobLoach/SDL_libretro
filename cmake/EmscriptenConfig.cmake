# Emscripten Configuration
set(CMAKE_EXECUTABLE_SUFFIX ".html")

# Ship the embed loader and its example page next to the build output, so the
# build directory is directly servable.
configure_file("${PROJECT_SOURCE_DIR}/example/SDL_libretro_embed.js" "${PROJECT_BINARY_DIR}/SDL_libretro_embed.js" COPYONLY)
configure_file("${PROJECT_SOURCE_DIR}/example/SDL_libretro_embed.html" "${PROJECT_BINARY_DIR}/SDL_libretro_embed.html" COPYONLY)

# Everything below configures the SDL_libretro app, which needs the menu and
# examples. It builds SDL_libretro.html, SDL_libretro.js, SDL_libretro.wasm
# and SDL_libretro.data.
if(NOT TARGET SDL_libretro_app)
    return()
endif()

# Preload the Core Files
set(SDL_LIBRETRO_PRELOAD_FLAGS "")
foreach(CORE IN LISTS SDL_LIBRETRO_CORES)
    list(APPEND SDL_LIBRETRO_PRELOAD_FLAGS
        "--preload-file \"${SDL_LIBRETRO_CORES_DIR}/${CORE}_libretro.info@/cores/\""
        "--preload-file \"${SDL_LIBRETRO_CORES_DIR}/${CORE}_libretro.wasm@/cores/\"")
endforeach()
target_link_libraries(SDL_libretro_app PRIVATE ${SDL_LIBRETRO_PRELOAD_FLAGS})

# The prebuilt cores are SIDE_MODULEs that import libretro-common
# helpers (file streams, VFS, string/path, etc) from the frontend
# rather than bundling them themselves. Considering this, we will
# compile those in and force-export them (--export): the frontend never
# calls them, so the linker would otherwise garbage-collect the objects.
set(SDL_LIBRETRO_COMMON_DIR "${PROJECT_SOURCE_DIR}/vendor/libretro-common")
target_sources(SDL_libretro_app PRIVATE
    "${SDL_LIBRETRO_COMMON_DIR}/compat/compat_strl.c"
    "${SDL_LIBRETRO_COMMON_DIR}/compat/compat_posix_string.c"
    "${SDL_LIBRETRO_COMMON_DIR}/string/stdstring.c"
    "${SDL_LIBRETRO_COMMON_DIR}/encodings/encoding_utf.c"
    "${SDL_LIBRETRO_COMMON_DIR}/file/file_path.c"
    "${SDL_LIBRETRO_COMMON_DIR}/file/file_path_io.c"
    "${SDL_LIBRETRO_COMMON_DIR}/streams/file_stream.c"
    "${SDL_LIBRETRO_COMMON_DIR}/streams/file_stream_transforms.c"
    "${SDL_LIBRETRO_COMMON_DIR}/vfs/vfs_implementation.c"
)

# Explicitly call out the exports.
set(SDL_LIBRETRO_CORE_EXPORTS
    filestream_open
    filestream_close
    filestream_read
    filestream_write
    filestream_seek
    filestream_tell
    filestream_gets
    filestream_vfs_init
    rfopen
    rfclose
    rfread
    rfgets
    rfseek
    rftell
    fill_pathname_join
    strlcpy_retro__
    strlcat_retro__
)
list(TRANSFORM SDL_LIBRETRO_CORE_EXPORTS PREPEND "-Wl,--export=")

# Confiugure the Emscripten options
target_link_options(SDL_libretro_app PRIVATE
    -sMAIN_MODULE=1 # Enables dynamic linking with dlopen()
    -sGL_ENABLE_GET_PROC_ADDRESS=1 # Allows cores to resolve GL at runtime

    # Memory
    #-sALLOW_MEMORY_GROWTH

    # Use a fixed 256MB heap instead of -sALLOW_MEMORY_GROWTH. A fixed
    # heap keeps the buffer non-resizable and is plenty for these cores.
    -sINITIAL_MEMORY=268435456

    # Avoid using ASYNCIFY since it breaks SDL's render loop

    # zlib
    -sUSE_ZLIB=1

     # Used for the Drag & Drop exposure in the JS handler
    -sEXPORTED_RUNTIME_METHODS=ccall,FS
    "--post-js=${PROJECT_SOURCE_DIR}/example/SDL_libretro_web.js"
    "--shell-file=${PROJECT_SOURCE_DIR}/example/SDL_libretro.html" # Minimal canvas-only page
    ${SDL_LIBRETRO_CORE_EXPORTS})
