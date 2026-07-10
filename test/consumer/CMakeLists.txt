# Minimal downstream consumer used to verify that an installed SDL_libretro
# is usable via find_package(). Not part of the main build.
cmake_minimum_required(VERSION 3.16)
project(SDL_libretro_consumer C)

find_package(SDL_libretro REQUIRED)

add_executable(app main.c)
target_link_libraries(app SDL_libretro::SDL_libretro)
