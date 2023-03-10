# CMAKE Modules
#set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

# SDL_libretro
add_executable(SDL_libretro SDL_libretro.c)

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(SHELL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/SDL_libretro.html")
    set(PRELOAD_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../wasm@/")

    set(USE_FLAGS "-s USE_SDL=2 -sMAIN_MODULE=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS} --shell-file ${SHELL_FILE} --preload-file ${PRELOAD_FILE}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS}")
    set(CMAKE_EXECUTABLE_SUFFIX .html)
    set_target_properties(SDL_libretro
        PROPERTIES
        OUTPUT_NAME "index"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../out"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../out"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../out"
    )
else()
    find_package(SDL2 REQUIRED)
    #set(SDL2_LIBRARIES
    #    SDL2::SDL2
    #    SDL2::SDL2main
    #)
endif()

# Resources
file(GLOB resources resources/*)
set(examples_resources)
list(APPEND examples_resources ${resources})
file(COPY ${examples_resources} DESTINATION "resources/")

target_link_libraries(SDL_libretro PRIVATE
    ${SDL2_LIBRARIES}
    SDL_libretro_h
)
