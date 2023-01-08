vendor/libretro-fceumm/fceumm_libretro_emscripten.bc:
	CFLAGS="-sSIDE_MODULE=1 -sEXPORTED_FUNCTIONS='[\"_retro_init\", \"_retro_deinit\", \"_retro_api_version\", \"_retro_get_system_info\", \"_retro_get_system_av_info\", \"_retro_set_controller_port_device\", \"_retro_reset\", \"_retro_run\", \"_retro_serialize_size\", \"_retro_serialize\", \"_retro_unserialize\", \"_retro_cheat_reset\", \"_retro_cheat_set\", \"_retro_load_game\", \"_retro_load_game_special\", \"_retro_unload_game\", \"_retro_get_region\", \"_retro_get_memory_data\", \"_retro_get_memory_size\"]'" SHARED="-sSIDE_MODULE=1" emmake make -C vendor/libretro-fceumm platform=emscripten

clean: clean-emscripten

clean-emscripten:
	make -C vendor/libretro-fceumm clean

build-emscripten/assets:
	mkdir -p build-emscripten/assets

build-emscripten/assets/fceumm_libretro_emscripten.bc: build-emscripten/assets vendor/libretro-fceumm/fceumm_libretro_emscripten.bc
	cp -rf vendor/libretro-fceumm/fceumm_libretro_emscripten.bc build-emscripten/assets/fceumm_libretro_emscripten.bc

emscripten-assets: build-emscripten/assets/fceumm_libretro_emscripten.bc

build-emscripten/Makefile:
	emcmake cmake -S . -B build-emscripten

build-emscripten/out/index.html: build-emscripten/Makefile
	emmake make -C build-emscripten

emscripten: build-emscripten/out/index.html

start-web:
	php -S localhost:9999 -t build-emscripten/out
