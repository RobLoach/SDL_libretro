/*
 * SDL_libretro - SDL3 libretro frontend library
 *
 * Single-header library. To compile the implementation, define
 * SDL_LIBRETRO_IMPLEMENTATION in exactly ONE translation unit before
 * including this header:
 *
 *     #define SDL_LIBRETRO_IMPLEMENTATION
 *     #include "SDL_libretro.h"
 *
 * Every other translation unit includes "SDL_libretro.h" normally.
 *
 * Copyright (c) 2026 Rob Loach (@RobLoach)
 *
 * This software is provided "as-is", without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software
 *      in a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *      be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any source distribution.
 */

#ifndef SDL_LIBRETRO_H
#define SDL_LIBRETRO_H

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_Libretro SDL_Libretro;

typedef struct SDL_Libretro_VFSCallbacks {
    void* (*load_file)(const char* path, size_t* size);
    int (*stat)(const char* path, Sint64* size);
    void (*free_file)(void* data);
} SDL_Libretro_VFSCallbacks;

/* Lifecycle */
SDL_Libretro* SDL_Libretro_Create(void);
void SDL_Libretro_Destroy(SDL_Libretro* lr);

/* Directories */
bool SDL_Libretro_SetCoreDirectory(SDL_Libretro* lr, const char* path);
bool SDL_Libretro_SetSaveDirectory(SDL_Libretro* lr, const char* path);
bool SDL_Libretro_SetSystemDirectory(SDL_Libretro* lr, const char* path);
bool SDL_Libretro_SetCoreAssetsDirectory(SDL_Libretro* lr, const char* path);
bool SDL_Libretro_SetUsername(SDL_Libretro* lr, const char* username);

/* Core loading */
bool SDL_Libretro_LoadCore(SDL_Libretro* lr, const char* corePath);
void SDL_Libretro_UnloadCore(SDL_Libretro* lr);
bool SDL_Libretro_IsCoreReady(const SDL_Libretro* lr);

/* Game loading */
bool SDL_Libretro_LoadGame(SDL_Libretro* lr, const char* gamePath, SDL_Renderer* renderer);
bool SDL_Libretro_LoadGameFromMemory(SDL_Libretro* lr, const void* data, size_t size,
                                            const char* contentPath, SDL_Renderer* renderer);
void SDL_Libretro_UnloadGame(SDL_Libretro* lr);
bool SDL_Libretro_IsGameReady(const SDL_Libretro* lr);
bool SDL_Libretro_IsGameRequired(const SDL_Libretro* lr);
bool SDL_Libretro_Reset(SDL_Libretro* lr);

/* Frame */
void SDL_Libretro_RunFrame(SDL_Libretro* lr);
bool SDL_Libretro_ShouldClose(const SDL_Libretro* lr);

/* Video */
SDL_Texture* SDL_Libretro_GetTexture(const SDL_Libretro* lr);
bool SDL_Libretro_Render(SDL_Libretro* lr, const SDL_FRect* dstRect);
void SDL_Libretro_GetSize(const SDL_Libretro* lr, int* w, int* h);
float SDL_Libretro_GetAspectRatio(const SDL_Libretro* lr);
double SDL_Libretro_GetFPS(const SDL_Libretro* lr);
int SDL_Libretro_GetRotation(const SDL_Libretro* lr);

/* Audio */
bool SDL_Libretro_InitAudio(SDL_Libretro* lr);
void SDL_Libretro_CloseAudio(SDL_Libretro* lr);
void SDL_Libretro_SetVolume(SDL_Libretro* lr, float volume);
float SDL_Libretro_GetVolume(const SDL_Libretro* lr);
void SDL_Libretro_SetSpeed(SDL_Libretro* lr, float speed);
float SDL_Libretro_GetSpeed(const SDL_Libretro* lr);

/* Input */
void SDL_Libretro_HandleEvent(SDL_Libretro* lr, const SDL_Event* event);
bool SDL_Libretro_SetPortDevice(SDL_Libretro* lr, unsigned port, unsigned device);
void SDL_Libretro_SetKeyboardMapping(SDL_Libretro* lr, int retroButton, SDL_Scancode scancode);
void SDL_Libretro_SetVirtualButton(SDL_Libretro* lr, unsigned port, int button, bool pressed);

/* Save states */
size_t SDL_Libretro_GetSerializeSize(const SDL_Libretro* lr);
bool SDL_Libretro_Serialize(SDL_Libretro* lr, void* data, size_t size);
bool SDL_Libretro_Unserialize(SDL_Libretro* lr, const void* data, size_t size);

/* SRAM */
void* SDL_Libretro_GetSRAMData(const SDL_Libretro* lr, size_t* size);
bool SDL_Libretro_SetSRAMData(SDL_Libretro* lr, const void* data, size_t size);

/* Core options */
unsigned SDL_Libretro_GetOptionCount(const SDL_Libretro* lr);
const char* SDL_Libretro_GetOptionKey(const SDL_Libretro* lr, unsigned index);
const char* SDL_Libretro_GetOptionValue(const SDL_Libretro* lr, const char* key);
bool SDL_Libretro_SetOptionValue(SDL_Libretro* lr, const char* key, const char* value);
bool SDL_Libretro_ResetOption(SDL_Libretro* lr, const char* key);
void SDL_Libretro_ResetAllOptions(SDL_Libretro* lr);
bool SDL_Libretro_AreOptionsDirty(SDL_Libretro* lr);

/* Cheats */
bool SDL_Libretro_SetCheat(SDL_Libretro* lr, unsigned index, bool enabled, const char* code);
void SDL_Libretro_ResetCheats(SDL_Libretro* lr);

/* Core metadata */
const char* SDL_Libretro_GetCoreName(const SDL_Libretro* lr);
const char* SDL_Libretro_GetCoreVersion(const SDL_Libretro* lr);
const char* SDL_Libretro_GetValidExtensions(const SDL_Libretro* lr);

/* VFS */
bool SDL_Libretro_SetVFS(SDL_Libretro* lr, const SDL_Libretro_VFSCallbacks* vfs);

/* OSD messages */
void SDL_Libretro_SetMessage(SDL_Libretro* lr, const char* msg, double duration);

#ifdef __cplusplus
}
#endif

/* ===================================================================== */
/*  Implementation                                                       */
/* ===================================================================== */
#ifdef SDL_LIBRETRO_IMPLEMENTATION
#ifndef SDL_LIBRETRO_IMPLEMENTATION_ONCE
#define SDL_LIBRETRO_IMPLEMENTATION_ONCE

#include "libretro.h"

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
    Uint64 lastTickNS; /* Wall-clock of the previous RunFrame (SDL_GetTicksNS); 0 until first call. */
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

/* File-static active context for libretro C callbacks (one per process) */
static SDL_Libretro* SDL_Libretro_active = NULL;

/* Internal subsystem functions (defined across the implementation fragments) */
static bool SDL_Libretro_InitVideo(SDL_Libretro* lr);
static void SDL_Libretro_CloseVideo(SDL_Libretro* lr);
static void SDL_Libretro_VideoRefresh(const void* data, unsigned width, unsigned height, size_t pitch);

static void SDL_Libretro_AudioSample(int16_t left, int16_t right);
static size_t SDL_Libretro_AudioSampleBatch(const int16_t* data, size_t frames);
static void SDL_Libretro_FlushSingleSamples(SDL_Libretro* lr);

static void SDL_Libretro_InputPoll(void);
static int16_t SDL_Libretro_InputState(unsigned port, unsigned device, unsigned index, unsigned id);

static bool SDL_Libretro_EnvironmentCallback(unsigned cmd, void* data);

static void SDL_Libretro_PixelFormatARGB1555ToRGB565(void* output, const void* input,
    int width, int height, int out_stride, int in_stride);

static SDL_Scancode SDL_Libretro_RetroKeyToScancode(unsigned key);
static SDL_GamepadButton SDL_Libretro_RetroJoypadToGamepadButton(unsigned button);

static void SDL_Libretro_InitCoreOption(SDL_Libretro* lr, const char* key, const char* defaultValue,
    const char* label, const char* valuesList, const char* displayList,
    const char* tooltip, const char* categoryKey);
static void SDL_Libretro_FreeCoreOptions(SDL_Libretro* lr);

/* Subsystem implementation fragments (lifecycle last; it references the rest) */
#include "SDL_libretro_video.h"
#include "SDL_libretro_audio.h"
#include "SDL_libretro_input.h"
#include "SDL_libretro_options.h"
#include "SDL_libretro_serialize.h"
#include "SDL_libretro_env.h"
#include "SDL_libretro_core.h"

#endif /* SDL_LIBRETRO_IMPLEMENTATION_ONCE */
#endif /* SDL_LIBRETRO_IMPLEMENTATION */

#endif /* SDL_LIBRETRO_H */
