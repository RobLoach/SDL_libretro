#include "libretro.h"
#include <string.h>
#include <stdlib.h>

#define WIDTH  320
#define HEIGHT 240

static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static uint16_t framebuffer[WIDTH * HEIGHT];
static uint8_t save_ram[64];
static uint8_t state_data[128];
static bool game_loaded;

RETRO_API void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;
    bool no_game = false;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb) { (void)cb; }
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
RETRO_API void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
RETRO_API void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

RETRO_API void retro_init(void) {
    memset(framebuffer, 0, sizeof(framebuffer));
    memset(save_ram, 0, sizeof(save_ram));
    memset(state_data, 0, sizeof(state_data));
}

RETRO_API void retro_deinit(void) {}

RETRO_API unsigned retro_api_version(void) { return RETRO_API_VERSION; }

RETRO_API void retro_get_system_info(struct retro_system_info *info) {
    memset(info, 0, sizeof(*info));
    info->library_name = "test_core";
    info->library_version = "1.0";
    info->valid_extensions = "txt";
    info->need_fullpath = false;
    info->block_extract = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info) {
    info->geometry.base_width = WIDTH;
    info->geometry.base_height = HEIGHT;
    info->geometry.max_width = WIDTH;
    info->geometry.max_height = HEIGHT;
    info->geometry.aspect_ratio = (float)WIDTH / (float)HEIGHT;
    info->timing.fps = 60.0;
    info->timing.sample_rate = 44100.0;
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device) {
    (void)port; (void)device;
}

RETRO_API void retro_reset(void) {
    memset(framebuffer, 0, sizeof(framebuffer));
}

RETRO_API void retro_run(void) {
    if (input_poll_cb) input_poll_cb();

    static uint16_t color = 0;
    color++;
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        framebuffer[i] = color;
    }

    if (video_cb) video_cb(framebuffer, WIDTH, HEIGHT, WIDTH * sizeof(uint16_t));

    int16_t audio[882 * 2];
    memset(audio, 0, sizeof(audio));
    if (audio_batch_cb) audio_batch_cb(audio, 882);
}

RETRO_API size_t retro_serialize_size(void) { return sizeof(state_data); }

RETRO_API bool retro_serialize(void *data, size_t len) {
    if (len < sizeof(state_data)) return false;
    memcpy(data, state_data, sizeof(state_data));
    return true;
}

RETRO_API bool retro_unserialize(const void *data, size_t len) {
    if (len < sizeof(state_data)) return false;
    memcpy(state_data, data, sizeof(state_data));
    return true;
}

RETRO_API void retro_cheat_reset(void) {}
RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code) {
    (void)index; (void)enabled; (void)code;
}

RETRO_API bool retro_load_game(const struct retro_game_info *game) {
    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (environ_cb) environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);
    game_loaded = true;

    if (game && game->data && game->size > 0) {
        size_t n = game->size < sizeof(save_ram) ? game->size : sizeof(save_ram);
        memcpy(save_ram, game->data, n);
    }
    return true;
}

RETRO_API bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num) {
    (void)type; (void)info; (void)num;
    return false;
}

RETRO_API void retro_unload_game(void) { game_loaded = false; }

RETRO_API unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }

RETRO_API void *retro_get_memory_data(unsigned id) {
    if (id == RETRO_MEMORY_SAVE_RAM) return save_ram;
    return NULL;
}

RETRO_API size_t retro_get_memory_size(unsigned id) {
    if (id == RETRO_MEMORY_SAVE_RAM) return sizeof(save_ram);
    return 0;
}
