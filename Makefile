vendor/libretro-fceumm/fceumm_libretro_emscripten.bc:
	CFLAGS="-s SIDE_MODULE=1" SHARED="-s SIDE_MODULE=1" emmake make -C vendor/libretro-fceumm platform=emscripten

build-emscripten/assets:
	mkdir -p build-emscripten/assets

build-emscripten/assets/fceumm_libretro_emscripten.bc: build-emscripten/assets vendor/libretro-fceumm/fceumm_libretro_emscripten.bc
	cp -rf vendor/libretro-fceumm/fceumm_libretro_emscripten.bc build-emscripten/assets/fceumm_libretro_emscripten.bc

emscripten-assets: build-emscripten/assets/fceumm_libretro_emscripten.bc

build-emscripten/Makefile:
	emcmake cmake -S . -B build-emscripten

build-emscripten/out/index.html: emscripten-assets build-emscripten/Makefile
	emmake make -C build-emscripten

emscripten: build-emscripten/out/index.html

start-web:
	php -S localhost:9999 -t build-emscripten/out
