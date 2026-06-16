/*
 * SDL_libretro - internal header
 */

#ifndef SDL_LIBRETRO_INTERNAL_H
#define SDL_LIBRETRO_INTERNAL_H

#include <SDL3/SDL.h>
#include "libretro.h"
#include "../include/SDL_libretro.h"

#define SDL_LIBRETRO_MAX_PATH 4096
#define SDL_LIBRETRO_AUDIO_RING_BUFFER_SIZE 8192
#define SDL_LIBRETRO_AUDIO_SINGLE_SAMPLE_BUFFER_SIZE 512
#define SDL_LIBRETRO_MAX_CONTENT_INFO_OVERRIDES 16
#define SDL_LIBRETRO_CONTENT_INFO_OVERRIDE_EXTS_LEN 256
#define SDL_LIBRETRO_RUMBLE_PORTS 4

typedef struct SDL_LibretroCoreSymbols {
    SDL_SharedObject* handle;
    void (*retro_init)(void);
    void (*retro_deinit)(void);
    unsigned (*retro_api_version)(void);
    void (*retro_set_environment)(retro_environment_t);
    void (*retro_set_video_refresh)(retro_video_refresh_t);
    void (*retro_set_audio_sample)(retro_audio_sample_t);
    void (*retro_set_audio_sample_batch)(retro_audio_sample_batch_t);
    void (*retro_set_input_poll)(retro_input_poll_t);
    void (*retro_set_input_state)(retro_input_state_t);
    void (*retro_get_system_info)(struct retro_system_info*);
    void (*retro_get_system_av_info)(struct retro_system_av_info*);
    void (*retro_set_controller_port_device)(unsigned port, unsigned device);
    void (*retro_reset)(void);
    void (*retro_run)(void);
    size_t (*retro_serialize_size)(void);
    bool (*retro_serialize)(void*, size_t);
    bool (*retro_unserialize)(const void*, size_t);
    void (*retro_cheat_reset)(void);
    void (*retro_cheat_set)(unsigned index, bool enabled, const char* code);
    bool (*retro_load_game)(const struct retro_game_info*);
    bool (*retro_load_game_special)(unsigned, const struct retro_game_info*, size_t);
    void (*retro_unload_game)(void);
    unsigned (*retro_get_region)(void);
    void* (*retro_get_memory_data)(unsigned);
    size_t (*retro_get_memory_size)(unsigned);
} SDL_LibretroCoreSymbols;

typedef struct SDL_LibretroCoreOption {
    char* key;
    char* value;
    char* defaultValue;
    char* label;
    char* valuesList;
    char* displayList;
    char* tooltip;
    char* categoryKey;
    bool visible;
} SDL_LibretroCoreOption;

typedef struct SDL_LibretroCoreData {
    SDL_LibretroCoreSymbols symbols;

    bool loaded;
    bool shutdown;
    unsigned width, height;
    double fps;
    double sampleRate;
    float aspectRatio;
    char corePath[SDL_LIBRETRO_MAX_PATH];
    char libraryName[200];
    char libraryVersion[200];
    char validExtensions[200];
    bool needFullpath;
    bool blockExtract;
    bool supportNoGame;
    unsigned apiVersion;
    enum retro_pixel_format pixelFormat;
    unsigned performanceLevel;
    uint64_t serializationQuirks;
    int rotation;

    /* Video */
    SDL_Texture* texture;
    SDL_Renderer* renderer;
    void* frameBuffer;
    size_t frameBufferSize;
    bool textureRebuild;

    /* Audio */
    SDL_AudioStream* audioStream;
    float* audioRingBuffer;
    size_t audioRingBufferSize;
    SDL_AtomicInt audioRingWritePos;
    SDL_AtomicInt audioRingAvailable;
    unsigned minimumAudioLatencyMs;
    int16_t singleSampleBuffer[SDL_LIBRETRO_AUDIO_SINGLE_SAMPLE_BUFFER_SIZE * 2];
    size_t singleSampleCount;
    int audioDropWarnCount;

    /* Input */
    SDL_Window* window;
    float inputLastMouseX, inputLastMouseY;
    float inputMouseX, inputMouseY;
    unsigned portDeviceMap[16];
    bool virtualJoypadState[16];

    /* Callbacks from core */
    retro_keyboard_event_t keyboard_event;
    struct retro_frame_time_callback runloop_frame_time;
    retro_usec_t runloop_frame_time_last;
    struct retro_audio_callback audio_callback;

    /* Core options (dynamic) */
    SDL_LibretroCoreOption* options;
    unsigned optionCount;
    unsigned optionCapacity;
    bool optionsDirty;
    bool optionsVisibilityDirty;

    /* Input descriptors */
    struct retro_input_descriptor* inputDescriptors;
    unsigned inputDescriptorCount;
    struct retro_controller_info* controllerInfo;
    unsigned controllerPortCount;

    /* Content */
    char contentPath[SDL_LIBRETRO_MAX_PATH];
    char contentDir[SDL_LIBRETRO_MAX_PATH];
    char contentName[SDL_LIBRETRO_MAX_PATH];
    char contentExt[16];
    struct retro_game_info_ext gameInfoExt;
    bool gameInfoExtValid;
    unsigned char* persistentGameData;
    size_t persistentGameDataSize;

    /* Content info overrides */
    char contentInfoOverrideExts[SDL_LIBRETRO_MAX_CONTENT_INFO_OVERRIDES][SDL_LIBRETRO_CONTENT_INFO_OVERRIDE_EXTS_LEN];
    bool contentInfoOverrideNeedFullpath[SDL_LIBRETRO_MAX_CONTENT_INFO_OVERRIDES];
    bool contentInfoOverridePersistent[SDL_LIBRETRO_MAX_CONTENT_INFO_OVERRIDES];
    unsigned contentInfoOverrideCount;

    /* Rumble */
    float rumbleStrong[SDL_LIBRETRO_RUMBLE_PORTS];
    float rumbleWeak[SDL_LIBRETRO_RUMBLE_PORTS];

    /* Disk control */
    struct retro_disk_control_ext_callback disk_control;
    bool diskControlActive;

    /* Memory maps */
    struct retro_memory_descriptor* memoryMapDescriptors;
    unsigned memoryMapDescriptorCount;

    /* DRC */
    float drcAdjustment;
    bool drcEnabled;

    /* Perf */
    struct retro_perf_counter* perf_counter_last;
    retro_perf_tick_t gameTimeNSEC;
} SDL_LibretroCoreData;

struct SDL_Libretro {
    /* Persistent settings */
    float volume;
    float speed;
    double speedAccumulator;
    SDL_Scancode keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_R3 + 1];
    char coreDirectory[SDL_LIBRETRO_MAX_PATH];
    char saveDirectory[SDL_LIBRETRO_MAX_PATH];
    char systemDirectory[SDL_LIBRETRO_MAX_PATH];
    char coreAssetsDirectory[SDL_LIBRETRO_MAX_PATH];
    char username[128];

    /* OSD message */
    char osdMessage[256];
    Uint64 osdEndTimeMs;

    /* VFS callbacks (optional) */
    SDL_Libretro_VFSCallbacks vfs;
    bool vfsActive;

    /* SDL gamepads (opened handles, indexed by port) */
    SDL_Gamepad* gamepads[16];
    unsigned gamepadCount;

    /* Per-core state */
    SDL_LibretroCoreData core;
};

/* File-static active context for libretro C callbacks */
extern SDL_Libretro* SDL_Libretro_active;

/* Internal subsystem functions */
bool SDL_Libretro_InitVideo(SDL_Libretro* lr);
void SDL_Libretro_CloseVideo(SDL_Libretro* lr);
void SDL_Libretro_VideoRefresh(const void* data, unsigned width, unsigned height, size_t pitch);

void SDL_Libretro_AudioSample(int16_t left, int16_t right);
size_t SDL_Libretro_AudioSampleBatch(const int16_t* data, size_t frames);
void SDL_Libretro_FlushSingleSamples(SDL_Libretro* lr);

void SDL_Libretro_InputPoll(void);
int16_t SDL_Libretro_InputState(unsigned port, unsigned device, unsigned index, unsigned id);

bool SDL_Libretro_EnvironmentCallback(unsigned cmd, void* data);

void SDL_Libretro_PixelFormatARGB1555ToRGB565(void* output, const void* input,
    int width, int height, int out_stride, int in_stride);

SDL_Scancode SDL_Libretro_RetroKeyToScancode(unsigned key);
SDL_GamepadButton SDL_Libretro_RetroJoypadToGamepadButton(unsigned button);

void SDL_Libretro_InitCoreOption(SDL_Libretro* lr, const char* key, const char* defaultValue,
    const char* label, const char* valuesList, const char* displayList,
    const char* tooltip, const char* categoryKey);
void SDL_Libretro_FreeCoreOptions(SDL_Libretro* lr);

#endif /* SDL_LIBRETRO_INTERNAL_H */
