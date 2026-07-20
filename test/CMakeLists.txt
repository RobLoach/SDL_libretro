# SDL_libretro unit tests. Added from the top-level CMakeLists.txt via
# add_subdirectory() when SDL_LIBRETRO_BUILD_TESTS is on.

set(LIBRETRO_COMMON_INCLUDE ${PROJECT_SOURCE_DIR}/vendor/libretro-common/include)

add_library(test_core SHARED test_core.c)
target_include_directories(test_core PRIVATE ${LIBRETRO_COMMON_INCLUDE})
set_target_properties(test_core PROPERTIES PREFIX "" SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})

add_executable(SDL_libretro_test SDL_libretro_test.c)
target_link_libraries(SDL_libretro_test PRIVATE SDL_libretro SDL3::SDL3_test)
target_compile_definitions(SDL_libretro_test PRIVATE
    TEST_CORE_PATH="$<TARGET_FILE:test_core>"
    TEST_CONTENT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test_content.txt")

if(SDL_LIBRETRO_MENU)
    target_link_libraries(SDL_libretro_test PRIVATE SDL_libretro_menu)
endif()

if(SDL_LIBRETRO_PHYSFS)
    target_link_libraries(SDL_libretro_test PRIVATE SDL_libretro_physfs)

    # A test-core build that skips the early GET_VFS_INTERFACE probe, for
    # testing the extract-to-disk fallback taken for full-path cores that
    # read files directly from the OS.
    add_library(test_core_novfs SHARED test_core.c)
    target_include_directories(test_core_novfs PRIVATE ${LIBRETRO_COMMON_INCLUDE})
    target_compile_definitions(test_core_novfs PRIVATE TEST_CORE_NO_EARLY_VFS)
    set_target_properties(test_core_novfs PROPERTIES PREFIX "" SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})

    # Zip fixtures for the PhysFS loader tests:
    #   test_content.zip  - base name matches the .txt inside
    #   test_fullpath.zip - holds a need_fullpath (".vfs") file
    #   test_priority.zip - base-name match must beat another candidate
    #   test_disc.zip     - a .cue sheet must beat a plain candidate
    #   test_nested.zip   - content in a subdirectory is still found
    set(TEST_ZIP_STAGING "${CMAKE_CURRENT_BINARY_DIR}/test_zip_staging")
    file(MAKE_DIRECTORY ${TEST_ZIP_STAGING})
    file(MAKE_DIRECTORY ${TEST_ZIP_STAGING}/sub/dir)
    configure_file(test_content.txt ${TEST_ZIP_STAGING}/test_content.txt COPYONLY)
    configure_file(test_content.txt ${TEST_ZIP_STAGING}/game.vfs COPYONLY)
    file(WRITE ${TEST_ZIP_STAGING}/aaa.txt "WRONG PICK")
    file(WRITE ${TEST_ZIP_STAGING}/test_priority.txt "PRIORITY PICK")
    file(WRITE ${TEST_ZIP_STAGING}/data.txt "WRONG DISC PICK")
    file(WRITE ${TEST_ZIP_STAGING}/disc.cue "CUE SHEET PICK")
    file(WRITE ${TEST_ZIP_STAGING}/sub/dir/nested.txt "NESTED PICK")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar cf
        ${CMAKE_CURRENT_BINARY_DIR}/test_content.zip --format=zip test_content.txt
        WORKING_DIRECTORY ${TEST_ZIP_STAGING})
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar cf
        ${CMAKE_CURRENT_BINARY_DIR}/test_fullpath.zip --format=zip game.vfs
        WORKING_DIRECTORY ${TEST_ZIP_STAGING})
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar cf
        ${CMAKE_CURRENT_BINARY_DIR}/test_priority.zip --format=zip aaa.txt test_priority.txt
        WORKING_DIRECTORY ${TEST_ZIP_STAGING})
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar cf
        ${CMAKE_CURRENT_BINARY_DIR}/test_disc.zip --format=zip data.txt disc.cue
        WORKING_DIRECTORY ${TEST_ZIP_STAGING})
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar cf
        ${CMAKE_CURRENT_BINARY_DIR}/test_nested.zip --format=zip sub/dir/nested.txt
        WORKING_DIRECTORY ${TEST_ZIP_STAGING})
    target_compile_definitions(SDL_libretro_test PRIVATE
        TEST_CORE_NOVFS_PATH="$<TARGET_FILE:test_core_novfs>"
        TEST_CONTENT_ZIP_PATH="${CMAKE_CURRENT_BINARY_DIR}/test_content.zip"
        TEST_FULLPATH_ZIP_PATH="${CMAKE_CURRENT_BINARY_DIR}/test_fullpath.zip"
        TEST_PRIORITY_ZIP_PATH="${CMAKE_CURRENT_BINARY_DIR}/test_priority.zip"
        TEST_DISC_ZIP_PATH="${CMAKE_CURRENT_BINARY_DIR}/test_disc.zip"
        TEST_NESTED_ZIP_PATH="${CMAKE_CURRENT_BINARY_DIR}/test_nested.zip")
endif()

add_test(NAME SDL_libretro_test COMMAND SDL_libretro_test)
