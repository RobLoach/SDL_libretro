cmake_minimum_required(VERSION 3.16)
project(SDL_libretro
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "SDL3-based libretro frontend library"
)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(SDL3 REQUIRED)

# Core downloads for development/testing
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(SDL_LIBRETRO_DOWNLOAD_CORES "Download libretro cores for testing" ON)
else()
    option(SDL_LIBRETRO_DOWNLOAD_CORES "Download libretro cores for testing" OFF)
endif()

if(NOT DEFINED CORES_DIR)
    set(CORES_DIR "${CMAKE_BINARY_DIR}/cores")
endif()

if(SDL_LIBRETRO_DOWNLOAD_CORES)
    include(DownloadCores)
endif()

# Header-only (INTERFACE) library. Consumers define SDL_LIBRETRO_IMPLEMENTATION
# in exactly one translation unit before including <SDL_libretro.h>.
add_library(SDL_libretro INTERFACE)

target_include_directories(SDL_libretro INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/libretro-common/include>
    $<INSTALL_INTERFACE:include>
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
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(SDL_LIBRETRO_PHYSFS "Enable PhysFS-backed zip loading (SDL_libretro_physfs.h)" ON)
else()
    option(SDL_LIBRETRO_PHYSFS "Enable PhysFS-backed zip loading (SDL_libretro_physfs.h)" OFF)
endif()

if(SDL_LIBRETRO_PHYSFS)
    find_package(PhysFS REQUIRED)     # physfs-static, built on the SDL3 platform backend
    find_package(SDL_PhysFS REQUIRED) # SDL_PhysFS header-only interface

    add_library(SDL_libretro_physfs INTERFACE)
    target_link_libraries(SDL_libretro_physfs INTERFACE
        SDL_libretro
        SDL_PhysFS
        physfs-static)
endif()

# Example
option(SDL_LIBRETRO_BUILD_EXAMPLES "Build example applications" ON)
if(SDL_LIBRETRO_BUILD_EXAMPLES)
    add_executable(SDL_libretro_basic example/SDL_libretro_basic.c)
    target_link_libraries(SDL_libretro_basic PRIVATE SDL_libretro)

    add_executable(SDL_libretro_demo example/SDL_libretro_demo.c)
    target_link_libraries(SDL_libretro_demo PRIVATE SDL_libretro)
endif()

# Tests
option(SDL_LIBRETRO_BUILD_TESTS "Build unit tests" ON)
if(SDL_LIBRETRO_BUILD_TESTS)
    enable_testing()

    add_library(test_core SHARED test/test_core.c)
    target_include_directories(test_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libretro-common/include)
    set_target_properties(test_core PROPERTIES PREFIX "" SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})

    add_executable(SDL_libretro_test test/SDL_libretro_test.c)
    target_link_libraries(SDL_libretro_test PRIVATE SDL_libretro SDL3::SDL3_test)
    target_compile_definitions(SDL_libretro_test PRIVATE
        TEST_CORE_PATH="$<TARGET_FILE:test_core>"
        TEST_CONTENT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test/test_content.txt")

    if(SDL_LIBRETRO_PHYSFS)
        target_link_libraries(SDL_libretro_test PRIVATE SDL_libretro_physfs)

        # Zip fixtures for the PhysFS loader tests: one whose base name matches
        # the file inside, and one holding a need_fullpath (".vfs") file.
        set(TEST_ZIP_STAGING "${CMAKE_CURRENT_BINARY_DIR}/test_zip_staging")
        file(MAKE_DIRECTORY ${TEST_ZIP_STAGING})
        configure_file(test/test_content.txt ${TEST_ZIP_STAGING}/test_content.txt COPYONLY)
        configure_file(test/test_content.txt ${TEST_ZIP_STAGING}/game.vfs COPYONLY)
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar cf
            ${CMAKE_CURRENT_BINARY_DIR}/test_content.zip --format=zip test_content.txt
            WORKING_DIRECTORY ${TEST_ZIP_STAGING})
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar cf
            ${CMAKE_CURRENT_BINARY_DIR}/test_fullpath.zip --format=zip game.vfs
            WORKING_DIRECTORY ${TEST_ZIP_STAGING})
        target_compile_definitions(SDL_libretro_test PRIVATE
            TEST_CONTENT_ZIP_PATH="${CMAKE_CURRENT_BINARY_DIR}/test_content.zip"
            TEST_FULLPATH_ZIP_PATH="${CMAKE_CURRENT_BINARY_DIR}/test_fullpath.zip")
    endif()

    add_test(NAME SDL_libretro_test COMMAND SDL_libretro_test)
endif()

# Install (only when this is the top-level project)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(CMakePackageConfigHelpers)
    include(GNUInstallDirs)

    install(TARGETS SDL_libretro EXPORT SDL_libretroTargets)

    # Implementation fragments + the libretro API header are part of the
    # shipped header set (needed by the TU that defines the macro).
    install(
        DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "SDL_*.h"
    )
    install(FILES vendor/libretro-common/include/libretro.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT SDL_libretroTargets
        NAMESPACE SDL_libretro::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SDL_libretro)

    configure_package_config_file(
        cmake/SDL_libretroConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/SDL_libretroConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SDL_libretro)

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/SDL_libretroConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/SDL_libretroConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/SDL_libretroConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SDL_libretro)
endif()

if(EMSCRIPTEN)
    if(NOT SDL_LIBRETRO_DOWNLOAD_CORES)
        message(FATAL_ERROR "Emscripten builds require SDL_LIBRETRO_DOWNLOAD_CORES=ON: "
            "cores must be downloaded so they can be preloaded into the virtual filesystem.")
    endif()
    include(EmscriptenConfig)
endif()
