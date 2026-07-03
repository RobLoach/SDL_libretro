# Emscripten Configuration
set(CMAKE_EXECUTABLE_SUFFIX ".html")

# Preload the Core Files
set(PRELOAD_FLAGS "")
foreach(CORE IN LISTS CORES)
    list(APPEND PRELOAD_FLAGS
        "--preload-file \"${CORES_DIR}/${CORE}_libretro.info@/cores/\""
        "--preload-file \"${CORES_DIR}/${CORE}_libretro.wasm@/cores/\"")
endforeach()
target_link_libraries(SDL_libretro_demo PRIVATE ${PRELOAD_FLAGS})

# The prebuilt cores are SIDE_MODULEs that import libretro-common
# helpers (file streams, VFS, string/path, etc) from the frontend
# rather than bundling them themselves. Considering this, we will
# compile those in and force-export them (--export): the frontend never
# calls them, so the linker would otherwise garbage-collect the objects.
set(LIBRETRO_COMMON "${CMAKE_SOURCE_DIR}/vendor/libretro-common")
target_sources(SDL_libretro_demo PRIVATE
    "${LIBRETRO_COMMON}/compat/compat_strl.c"
    "${LIBRETRO_COMMON}/compat/compat_posix_string.c"
    "${LIBRETRO_COMMON}/string/stdstring.c"
    "${LIBRETRO_COMMON}/encodings/encoding_utf.c"
    "${LIBRETRO_COMMON}/file/file_path.c"
    "${LIBRETRO_COMMON}/file/file_path_io.c"
    "${LIBRETRO_COMMON}/streams/file_stream.c"
    "${LIBRETRO_COMMON}/streams/file_stream_transforms.c"
    "${LIBRETRO_COMMON}/vfs/vfs_implementation.c"
)

# Explicitly call out the exports.
set(CORE_EXPORTS
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
list(TRANSFORM CORE_EXPORTS PREPEND "-Wl,--export=")

# Confiugure the Emscripten options
target_link_options(SDL_libretro_demo PRIVATE
    -sMAIN_MODULE=1 # Enables dynamic linking with dlopen()
    -sGL_ENABLE_GET_PROC_ADDRESS=1 # Allows cores to resolve GL at runtime
    # Use a fixed 256MB heap instead of -sALLOW_MEMORY_GROWTH. A fixed
    # heap keeps the buffer non-resizable and is plenty for these cores.
    -sINITIAL_MEMORY=268435456
    # Avoid using ASYNCIFY since it breaks SDL's render loop
    -sUSE_ZLIB=1
    -sEXPORTED_RUNTIME_METHODS=ccall,FS # Used for the Drag & Drop exposure in the JS handler
    "--post-js=${CMAKE_SOURCE_DIR}/example/SDL_libretro_demo_web.js"
    "--shell-file=${CMAKE_SOURCE_DIR}/example/SDL_libretro_demo.html" # Minimal canvas-only page
    ${CORE_EXPORTS})
