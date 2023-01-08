vendor/libretro-fceumm/fceumm_libretro_emscripten.bc:
	$(MAKE) -C vendor/libretro-fceumm platform=emscripten

build:
	mkdir -p build

build-emscripten:
	mkdir -p build-emscripten

build-emscripten/out/index.html: build-emscripten
	
