# PhysFS
# https://github.com/icculus/physfs
if(TARGET physfs-static)
    set(PhysFS_FOUND TRUE)
    return()
endif()

include(FetchContent)

# FetchContent_Populate without MakeAvailable is intentional: the target is
# defined below from the fetched sources. Silence the CMake 3.30+ deprecation.
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

FetchContent_Declare(
    physfs
    GIT_REPOSITORY https://github.com/icculus/physfs.git
    GIT_TAG 23d0b5696445d86262221519c3db3ae351376da0
)
FetchContent_GetProperties(physfs)
if(NOT physfs_POPULATED)
    FetchContent_Populate(physfs)
endif()

add_library(physfs-static STATIC
    ${physfs_SOURCE_DIR}/src/physfs.c
    ${physfs_SOURCE_DIR}/src/physfs_byteorder.c
    ${physfs_SOURCE_DIR}/src/physfs_unicode.c
    ${physfs_SOURCE_DIR}/src/physfs_archiver_dir.c
    ${physfs_SOURCE_DIR}/src/physfs_archiver_zip.c
    ${physfs_SOURCE_DIR}/src/physfs_platform_sdl3.c
)
target_include_directories(physfs-static PUBLIC ${physfs_SOURCE_DIR}/src)
target_compile_definitions(physfs-static
    PUBLIC PHYSFS_STATIC
    PRIVATE
        PHYSFS_PLATFORM_SDL3=1
        PHYSFS_SUPPORTS_7Z=0
        PHYSFS_SUPPORTS_GRP=0
        PHYSFS_SUPPORTS_WAD=0
        PHYSFS_SUPPORTS_CSM=0
        PHYSFS_SUPPORTS_HOG=0
        PHYSFS_SUPPORTS_MVL=0
        PHYSFS_SUPPORTS_QPAK=0
        PHYSFS_SUPPORTS_ROFS=0
        PHYSFS_SUPPORTS_SLB=0
        PHYSFS_SUPPORTS_ISO9660=0
        PHYSFS_SUPPORTS_VDF=0
        PHYSFS_SUPPORTS_LECARCHIVES=0
        PHYSFS_SUPPORTS_POD=0
)
target_link_libraries(physfs-static PRIVATE SDL3::SDL3)

set(PhysFS_FOUND TRUE)
