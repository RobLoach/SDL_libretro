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

/* Captures what GET_GAME_INFO_EXT returns during retro_load_game so the
 * frontend test can verify the extended-game-info wiring. Exposed as
 * SYSTEM_RAM: [0..15] lower-case ext, [16] persistent_data, [17] data != NULL. */
static uint8_t game_info_probe[32];

static bool RETRO_CALLCONV test_update_display_callback(void) {
    if (!environ_cb) return false;
    struct retro_variable var = { "test_option_b", NULL };
    environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var);
    bool hide_a = (var.value && strcmp(var.value, "no") == 0);
    struct retro_core_option_display disp = { "test_option_a", !hide_a };
    environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &disp);
    return true;
}

RETRO_API void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;
    bool no_game = true; // retro_load_game(NULL) is handled, so advertise it
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);

    unsigned perf_level = 2;
    cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &perf_level);

    // Mark "txt" content as loaded into a persistent buffer; the frontend test
    // checks this round-trips through GET_GAME_INFO_EXT.
    static const struct retro_system_content_info_override overrides[] = {
        { "txt", false, true },
        { NULL, false, false },
    };
    cb(RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE, (void *)overrides);

    // Register a dummy subsystem for testing SET_SUBSYSTEM_INFO.
    static const struct retro_subsystem_memory_info sgb_mem[] = {
        { "srm", 0x100 },
    };
    static const struct retro_subsystem_rom_info sgb_roms[] = {
        { "Game Boy ROM", "gb|gbc", false, false, true, sgb_mem, 1 },
        { "Super Game Boy BIOS", "sfc|smc", true, false, true, NULL, 0 },
    };
    static const struct retro_subsystem_info subsystems[] = {
        { "Super Game Boy", "sgb", sgb_roms, 2, 1 },
        { NULL, NULL, NULL, 0, 0 },
    };
    cb(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, (void *)subsystems);

    // Register two options under one category, then hide the second one to test
    // SET_CORE_OPTIONS_DISPLAY.
    static const struct retro_core_option_v2_category opt_cats[] = {
        { "test_category", "Test Category", "Category for test options" },
        { NULL, NULL, NULL },
    };
    static const struct retro_core_option_v2_definition opt_defs[] = {
        { "test_option_a", "Option A", "Option A Category",
          "First test option", NULL, "test_category",
          { {"on", NULL}, {"off", NULL}, {NULL, NULL} }, "on" },
        { "test_option_b", "Option B", "Option B Category",
          "Second test option", NULL, "test_category",
          { {"yes", NULL}, {"no", NULL}, {NULL, NULL} }, "yes" },
        { NULL, NULL, NULL, NULL, NULL, NULL, {{NULL, NULL}}, NULL },
    };
    static const struct retro_core_options_v2 opts = {
        (struct retro_core_option_v2_category *)opt_cats,
        (struct retro_core_option_v2_definition *)opt_defs };
    cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, (void *)&opts);

    // Hide option B; frontend can verify via SDL_Libretro_GetOption(...)->visible.
    static const struct retro_core_option_display hide_b = { "test_option_b", false };
    cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, (void *)&hide_b);

    // Register an update-display callback that hides option A when option B is "no".
    static const struct retro_core_options_update_display_callback update_cb = {
        test_update_display_callback
    };
    cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK, (void *)&update_cb);
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

    struct retro_led_interface led_iface = {0};
    if (environ_cb && environ_cb(RETRO_ENVIRONMENT_GET_LED_INTERFACE, &led_iface) && led_iface.set_led_state) {
        led_iface.set_led_state(0, 1);
        led_iface.set_led_state(0, 0);
    }
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

    // Publish a one-entry memory map covering save_ram so the frontend's
    // memory-map accessors have something to read.
    static const struct retro_memory_descriptor mmap_desc[] = {
        { RETRO_MEMDESC_SYSTEM_RAM, save_ram, 0, 0x0000, 0, 0, sizeof(save_ram), "WRAM" },
    };
    static const struct retro_memory_map mmap = { mmap_desc, 1 };
    if (environ_cb) environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, (void *)&mmap);

    if (game && game->data && game->size > 0) {
        size_t n = game->size < sizeof(save_ram) ? game->size : sizeof(save_ram);
        memcpy(save_ram, game->data, n);
    }

    // Probe the extended game info the frontend published for this content.
    memset(game_info_probe, 0, sizeof(game_info_probe));
    const struct retro_game_info_ext *ext = NULL;
    if (environ_cb && environ_cb(RETRO_ENVIRONMENT_GET_GAME_INFO_EXT, &ext) && ext) {
        if (ext->ext) strncpy((char *)game_info_probe, ext->ext, 15);
        game_info_probe[16] = ext->persistent_data ? 1 : 0;
        game_info_probe[17] = ext->data != NULL ? 1 : 0;
    }
    return true;
}

RETRO_API bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num) {
    // Only the registered "sgb" subsystem (id 1) with its two ROMs is valid.
    if (type != 1 || num != 2 || !info) return false;

    // ROM 0 declares need_fullpath=false, so the frontend loads it into memory;
    // ROM 1 declares need_fullpath=true, so only its path is passed through.
    if (info[0].data == NULL || info[1].data != NULL) return false;

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (environ_cb) environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);
    game_loaded = true;

    if (info[0].size > 0) {
        size_t n = info[0].size < sizeof(save_ram) ? info[0].size : sizeof(save_ram);
        memcpy(save_ram, info[0].data, n);
    }

    // Probe the extended game info the frontend published for the primary ROM.
    memset(game_info_probe, 0, sizeof(game_info_probe));
    const struct retro_game_info_ext *ext = NULL;
    if (environ_cb && environ_cb(RETRO_ENVIRONMENT_GET_GAME_INFO_EXT, &ext) && ext) {
        if (ext->ext) strncpy((char *)game_info_probe, ext->ext, 15);
        game_info_probe[16] = ext->persistent_data ? 1 : 0;
        game_info_probe[17] = ext->data != NULL ? 1 : 0;
    }
    return true;
}

RETRO_API void retro_unload_game(void) { game_loaded = false; }

RETRO_API unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }

RETRO_API void *retro_get_memory_data(unsigned id) {
    if (id == RETRO_MEMORY_SAVE_RAM) return save_ram;
    if (id == RETRO_MEMORY_SYSTEM_RAM) return game_info_probe;
    return NULL;
}

RETRO_API size_t retro_get_memory_size(unsigned id) {
    if (id == RETRO_MEMORY_SAVE_RAM) return sizeof(save_ram);
    if (id == RETRO_MEMORY_SYSTEM_RAM) return sizeof(game_info_probe);
    return 0;
}
