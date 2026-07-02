# Emscripten Configuration
set(CMAKE_EXECUTABLE_SUFFIX ".html")

# Assets: preload each downloaded core (.wasm) and its metadata (.info)
set(PRELOAD_FLAGS "")
foreach(CORE IN LISTS CORES)
    list(APPEND PRELOAD_FLAGS
        "--preload-file \"${CORES_DIR}/${CORE}_libretro.info@/\""
        "--preload-file \"${CORES_DIR}/${CORE}_libretro.wasm@/\""
    )
endforeach()
target_link_libraries(SDL_libretro_basic PRIVATE ${PRELOAD_FLAGS})
