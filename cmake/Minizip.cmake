# Fetches minizip-ng and exposes the MINIZIP::minizip target for the optional
# .zip content-loading feature (SDL_libretro_minizip.h / SDL_minizip.h).
#
# Only included when SDL_LIBRETRO_ENABLE_MINIZIP is ON. The base SDL_libretro
# INTERFACE library does not depend on this; only translation units that define
# SDL_MINIZIP_IMPLEMENTATION need it. Flags mirror upstream SDL_minizip.

include(FetchContent)

# Build only the read-side, zlib-backed codec; disable everything else.
set(MZ_COMPAT OFF CACHE BOOL "" FORCE)
set(MZ_ZLIB ON CACHE BOOL "" FORCE)
set(MZ_BZIP2 OFF CACHE BOOL "" FORCE)
set(MZ_LZMA OFF CACHE BOOL "" FORCE)
set(MZ_PPMD OFF CACHE BOOL "" FORCE)
set(MZ_ZSTD OFF CACHE BOOL "" FORCE)
set(MZ_PKCRYPT OFF CACHE BOOL "" FORCE)
set(MZ_WZAES OFF CACHE BOOL "" FORCE)
set(MZ_OPENSSL OFF CACHE BOOL "" FORCE)
set(MZ_LIBBSD OFF CACHE BOOL "" FORCE)
set(MZ_ICONV OFF CACHE BOOL "" FORCE)
set(MZ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MZ_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
# Fetch zlib automatically when it is not already available on the system so the
# opt-in feature builds without requiring the user to install zlib themselves.
set(MZ_FETCH_LIBS ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    minizip_ng
    GIT_REPOSITORY https://github.com/zlib-ng/minizip-ng.git
    GIT_TAG 3.0.4
)
FetchContent_MakeAvailable(minizip_ng)
