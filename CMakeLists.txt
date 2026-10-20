cmake_minimum_required(VERSION 3.21)
project(SDL_libretro
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "SDL3-based libretro frontend library"
)

list(PREPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# The library needs libretro.h and SDL_ini.h from the git submodules, which
# GitHub's generated "Source code" archives leave out.
foreach(SDL_LIBRETRO_VENDOR_FILE
        vendor/libretro-common/include/libretro.h
        vendor/SDL_ini/SDL_ini.h)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/${SDL_LIBRETRO_VENDOR_FILE}")
        message(FATAL_ERROR "${SDL_LIBRETRO_VENDOR_FILE} is missing. Run: git submodule update --init, "
            "or use the SDL_libretro-<version>.tar.gz release archive, which includes the submodules.")
    endif()
endforeach()

find_package(SDL3 REQUIRED)

# Development conveniences and the optional components default to on only
# when SDL_libretro is the top-level project, not when it's added with
# add_subdirectory() or FetchContent.
option(SDL_LIBRETRO_DOWNLOAD_CORES "Download libretro cores for testing" ${PROJECT_IS_TOP_LEVEL})
option(SDL_LIBRETRO_PHYSFS "Enable PhysFS-backed zip loading (SDL_libretro_physfs.h)" ${PROJECT_IS_TOP_LEVEL})
option(SDL_LIBRETRO_MENU "Enable the nuklear_console menu (SDL_libretro_menu.h)" ${PROJECT_IS_TOP_LEVEL})
option(SDL_LIBRETRO_BUILD_EXAMPLES "Build example applications" ${PROJECT_IS_TOP_LEVEL})
option(SDL_LIBRETRO_BUILD_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})

set(SDL_LIBRETRO_CORES_DIR "${PROJECT_BINARY_DIR}/cores" CACHE PATH
    "Directory the downloaded libretro cores are placed in")

if(SDL_LIBRETRO_DOWNLOAD_CORES)
    include(DownloadCores)
endif()

include(GNUInstallDirs)

# Header-only (INTERFACE) library. Consumers define SDL_LIBRETRO_IMPLEMENTATION
# in exactly one translation unit before including <SDL_libretro.h>.
add_library(SDL_libretro INTERFACE)

target_include_directories(SDL_libretro INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vendor/libretro-common/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vendor/SDL_ini>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SDL_libretro>
)

# SDL3::SDL3 resolves to the shared or static SDL3, whichever is available,
# so consumers aren't pinned to one flavor. It is linked only in the build
# interface: the default build fetches SDL3 in-tree (cmake/FindSDL3.cmake)
# and in-tree targets can't be exported, so the installed package config
# attaches the SDL3 dependency instead (cmake/SDL_libretroConfig.cmake.in).
target_link_libraries(SDL_libretro INTERFACE
    $<BUILD_INTERFACE:SDL3::SDL3>
    ${CMAKE_DL_LIBS})
target_compile_features(SDL_libretro INTERFACE c_std_99)

# PhysFS-backed archive loading (SDL_libretro_physfs.h). Kept out of the base
# SDL_libretro target so consumers that don't want it pull in no PhysFS.
if(SDL_LIBRETRO_PHYSFS)
    find_package(PhysFS REQUIRED)     # physfs-static, built on the SDL3 platform backend
    find_package(SDL_PhysFS REQUIRED) # SDL_PhysFS header-only interface

    add_library(SDL_libretro_physfs INTERFACE)
    target_link_libraries(SDL_libretro_physfs INTERFACE
        SDL_libretro
        SDL_PhysFS
        physfs-static)
endif()

# Menu system (SDL_libretro_menu.h) built on nuklear_console. Kept out of the
# base SDL_libretro target so consumers that don't want it pull in no Nuklear.
if(SDL_LIBRETRO_MENU)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/vendor/Nuklear/nuklear.h")
        message(FATAL_ERROR "SDL_LIBRETRO_MENU requires the vendored submodules: git submodule update --init")
    endif()

    add_library(SDL_libretro_menu INTERFACE)
    # The repository root is on the path so nuklear_console's default
    # "vendor/c-vector/cvector.h" and "vendor/tinydir/tinydir.h" includes
    # resolve against the sibling submodules in vendor/.
    target_include_directories(SDL_libretro_menu INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vendor/Nuklear>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vendor/nuklear_console>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vendor/nuklear_gamepad>)
    target_compile_definitions(SDL_libretro_menu INTERFACE SDL_LIBRETRO_ENABLE_MENU)
    target_link_libraries(SDL_libretro_menu INTERFACE SDL_libretro)
    if(UNIX)
        # Nuklear's font baking uses libm.
        target_link_libraries(SDL_libretro_menu INTERFACE m)
    endif()
endif()

# Example
if(SDL_LIBRETRO_BUILD_EXAMPLES)
    add_executable(SDL_libretro_basic example/SDL_libretro_basic.c)
    target_link_libraries(SDL_libretro_basic PRIVATE SDL_libretro)

    if(SDL_LIBRETRO_MENU)
        # The SDL_libretro app assumes the menu; SDL_libretro_basic is the
        # menu-less example. The target is SDL_libretro_app because the
        # library already owns the SDL_libretro target name, but the built
        # binary is named SDL_libretro.
        add_executable(SDL_libretro_app example/SDL_libretro.c)
        set_target_properties(SDL_libretro_app PROPERTIES OUTPUT_NAME SDL_libretro)
        target_link_libraries(SDL_libretro_app PRIVATE SDL_libretro SDL_libretro_menu)
        if(SDL_LIBRETRO_PHYSFS)
            target_link_libraries(SDL_libretro_app PRIVATE SDL_libretro_physfs)
        else()
            # The app opts into PhysFS in source; turn it back off.
            target_compile_definitions(SDL_libretro_app PRIVATE SDL_LIBRETRO_DISABLE_PHYSFS)
        endif()
    endif()
endif()

# Tests
if(SDL_LIBRETRO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Install (only when this is the top-level project)
if(PROJECT_IS_TOP_LEVEL)
    include(CMakePackageConfigHelpers)

    install(TARGETS SDL_libretro EXPORT SDL_libretroTargets)

    # Implementation fragments + the libretro API header are part of the
    # shipped header set (needed by the TU that defines the macro). They go
    # in their own directory so libretro.h and SDL_ini.h don't collide with
    # other copies in the include root; consumers still #include
    # <SDL_libretro.h>.
    install(
        DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SDL_libretro
        FILES_MATCHING PATTERN "SDL_*.h"
    )
    install(FILES
        vendor/libretro-common/include/libretro.h
        vendor/SDL_ini/SDL_ini.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SDL_libretro)

    install(EXPORT SDL_libretroTargets
        NAMESPACE SDL_libretro::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SDL_libretro)

    configure_package_config_file(
        cmake/SDL_libretroConfig.cmake.in
        ${PROJECT_BINARY_DIR}/SDL_libretroConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SDL_libretro)

    write_basic_package_version_file(
        ${PROJECT_BINARY_DIR}/SDL_libretroConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(FILES
        ${PROJECT_BINARY_DIR}/SDL_libretroConfig.cmake
        ${PROJECT_BINARY_DIR}/SDL_libretroConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SDL_libretro)
endif()

if(EMSCRIPTEN)
    if(TARGET SDL_libretro_app AND NOT SDL_LIBRETRO_DOWNLOAD_CORES)
        message(FATAL_ERROR "The Emscripten SDL_libretro app requires SDL_LIBRETRO_DOWNLOAD_CORES=ON: "
            "cores must be downloaded so they can be preloaded into the virtual filesystem.")
    endif()
    include(EmscriptenConfig)
endif()
