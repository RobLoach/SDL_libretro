set(CORES
    snes9x
    fceumm
    genesis_plus_gx
)

file(MAKE_DIRECTORY "${CORES_DIR}")

function(download_cores)
    file(MAKE_DIRECTORY "${CORES_DIR}")
    foreach(URL IN LISTS ARGN)
        get_filename_component(ARCHIVE_NAME "${URL}" NAME)
        set(ARCHIVE_PATH "${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}")

        if(NOT EXISTS "${ARCHIVE_PATH}")
            message(STATUS "Downloading ${URL}")
            file(DOWNLOAD "${URL}" "${ARCHIVE_PATH}"
                STATUS DOWNLOAD_STATUS
                SHOW_PROGRESS
            )
            list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
            list(GET DOWNLOAD_STATUS 1 STATUS_MSG)
            if(NOT STATUS_CODE EQUAL 0)
                message(WARNING "Failed to download ${URL}: ${STATUS_MSG}")
                file(REMOVE "${ARCHIVE_PATH}")
                continue()
            endif()
        endif()

        message(STATUS "Extracting ${ARCHIVE_NAME} to ${CORES_DIR}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf "${ARCHIVE_PATH}"
            WORKING_DIRECTORY "${CORES_DIR}"
            RESULT_VARIABLE EXTRACT_RESULT
        )
        if(NOT EXTRACT_RESULT EQUAL 0)
            message(WARNING "Failed to extract ${ARCHIVE_NAME}")
        endif()
    endforeach()
endfunction()

function(download_nightly_cores BASE_URL EXT)
    set(URLS "")
    foreach(CORE IN LISTS CORES)
        list(APPEND URLS "${BASE_URL}/${CORE}_libretro${EXT}.zip")
    endforeach()
    download_cores(${URLS})
endfunction()

# Downloads pre-built wasm SIDE_MODULE cores.
# https://github.com/konsumer/libretro-wasm-cores
function(download_wasm_cores BASE_URL)
    file(MAKE_DIRECTORY "${CORES_DIR}")
    foreach(CORE IN LISTS CORES)
        set(WASM_URL "${BASE_URL}/${CORE}_libretro.wasm")
        set(WASM_DST "${CORES_DIR}/${CORE}_libretro.wasm")
        if(NOT EXISTS "${WASM_DST}")
            message(STATUS "Downloading wasm core: ${CORE}")
            file(DOWNLOAD "${WASM_URL}" "${WASM_DST}"
                STATUS DOWNLOAD_STATUS
                SHOW_PROGRESS
            )
            list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
            list(GET DOWNLOAD_STATUS 1 STATUS_MSG)
            if(NOT STATUS_CODE EQUAL 0)
                message(WARNING "Failed to download ${CORE}: ${STATUS_MSG}")
                file(REMOVE "${WASM_DST}")
            endif()
        endif()
    endforeach()
endfunction()

set(CORE_INFO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/libretro-core-info")
function(copy_core_info)
    file(MAKE_DIRECTORY "${CORES_DIR}")
    foreach(CORE IN LISTS ARGN)
        set(INFO_SRC "${CORE_INFO_DIR}/${CORE}_libretro.info")
        set(INFO_DST "${CORES_DIR}/${CORE}_libretro.info")
        if(EXISTS "${INFO_SRC}")
            file(COPY_FILE "${INFO_SRC}" "${INFO_DST}" ONLY_IF_DIFFERENT)
        else()
            message(FATAL_ERROR "${CORE}_libretro.info not found in ${CORE_INFO_DIR}. "
                "Run: git submodule update --init -- vendor/libretro-core-info")
        endif()
    endforeach()
endfunction()

if(EMSCRIPTEN)
    download_wasm_cores("https://github.com/konsumer/libretro-wasm-cores/releases/download/nightly")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        download_nightly_cores("https://buildbot.libretro.com/nightly/linux/aarch64/latest" ".so")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        download_nightly_cores("https://buildbot.libretro.com/nightly/linux/x86_64/latest" ".so")
    else()
        message(WARNING "Core download: unsupported Linux architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        download_nightly_cores("https://buildbot.libretro.com/nightly/windows/x86_64/latest" ".dll")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|i[3-6]86)$")
        download_nightly_cores("https://buildbot.libretro.com/nightly/windows/x86/latest" ".dll")
    else()
        message(WARNING "Core download: unsupported Windows architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        download_nightly_cores("https://buildbot.libretro.com/nightly/apple/osx/arm64/latest" ".dylib")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        download_nightly_cores("https://buildbot.libretro.com/nightly/apple/osx/x86_64/latest" ".dylib")
    else()
        message(WARNING "Core download: unsupported macOS architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
else()
    message(WARNING "Core download: unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

copy_core_info(${CORES})
