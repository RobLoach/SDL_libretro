/**
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
 * Copyright (c) 2026 Rob Loach
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
 *
 * @file SDL_libretro.h
 */

#ifndef SDL_LIBRETRO_H
#define SDL_LIBRETRO_H

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_Libretro SDL_Libretro;

typedef enum SDL_LibretroScaleMode {
    SDL_LIBRETRO_SCALE_ASPECT = 0,
    SDL_LIBRETRO_SCALE_INTEGER = 1
} SDL_LibretroScaleMode;

// Libretro Instance

SDL_Libretro* SDL_Libretro_Create(void);
void SDL_Libretro_Destroy(SDL_Libretro* lr);

// Directories

bool SDL_Libretro_SetCoreDirectory(SDL_Libretro* lr, const char* path);
bool SDL_Libretro_SetSaveDirectory(SDL_Libretro* lr, const char* path);
bool SDL_Libretro_SetSystemDirectory(SDL_Libretro* lr, const char* path);
bool SDL_Libretro_SetCoreAssetsDirectory(SDL_Libretro* lr, const char* path);
bool SDL_Libretro_SetUsername(SDL_Libretro* lr, const char* username);

// Core

bool SDL_Libretro_LoadCore(SDL_Libretro* lr, const char* corePath);
void SDL_Libretro_UnloadCore(SDL_Libretro* lr);
bool SDL_Libretro_IsCoreReady(const SDL_Libretro* lr);
bool SDL_Libretro_IsShutdown(const SDL_Libretro* lr);

// Game

bool SDL_Libretro_LoadGame(SDL_Libretro* lr, const char* gamePath, SDL_Renderer* renderer);
void SDL_Libretro_UnloadGame(SDL_Libretro* lr);
bool SDL_Libretro_IsGameReady(const SDL_Libretro* lr);
bool SDL_Libretro_IsGameRequired(const SDL_Libretro* lr);
bool SDL_Libretro_Reset(SDL_Libretro* lr);
void SDL_Libretro_RunFrame(SDL_Libretro* lr);

// Video

SDL_Texture* SDL_Libretro_GetTexture(const SDL_Libretro* lr);
SDL_Surface* SDL_Libretro_CreateSurface(const SDL_Libretro* lr);
bool SDL_Libretro_Render(SDL_Libretro* lr, const SDL_FRect* dstRect);
void SDL_Libretro_GetSize(const SDL_Libretro* lr, int* w, int* h);
float SDL_Libretro_GetAspectRatio(const SDL_Libretro* lr);
double SDL_Libretro_GetFPS(const SDL_Libretro* lr);
int SDL_Libretro_GetRotation(const SDL_Libretro* lr);
void SDL_Libretro_SetScaleMode(SDL_Libretro* lr, SDL_LibretroScaleMode mode);
SDL_LibretroScaleMode SDL_Libretro_GetScaleMode(const SDL_Libretro* lr);

// Audio

bool SDL_Libretro_InitAudio(SDL_Libretro* lr);
void SDL_Libretro_CloseAudio(SDL_Libretro* lr);
void SDL_Libretro_SetVolume(SDL_Libretro* lr, float volume);
float SDL_Libretro_GetVolume(const SDL_Libretro* lr);
void SDL_Libretro_SetSpeed(SDL_Libretro* lr, float speed);
float SDL_Libretro_GetSpeed(const SDL_Libretro* lr);
void SDL_Libretro_SetAudioLatency(SDL_Libretro* lr, unsigned latencyMs);
unsigned SDL_Libretro_GetAudioLatency(const SDL_Libretro* lr);
double SDL_Libretro_GetSampleRate(const SDL_Libretro* lr);

// Input

void SDL_Libretro_HandleEvent(SDL_Libretro* lr, const SDL_Event* event);
bool SDL_Libretro_SetPortDevice(SDL_Libretro* lr, unsigned port, unsigned device);
unsigned SDL_Libretro_GetPortDevice(const SDL_Libretro* lr, unsigned port);
void SDL_Libretro_SetKeyboardMapping(SDL_Libretro* lr, int retroButton, SDL_Scancode scancode);
void SDL_Libretro_SetVirtualButton(SDL_Libretro* lr, unsigned port, int button, bool pressed);
unsigned SDL_Libretro_GetInputDescriptorCount(const SDL_Libretro* lr);
bool SDL_Libretro_GetInputDescriptor(const SDL_Libretro* lr, unsigned index,
    unsigned* port, unsigned* device, unsigned* id, const char** description);

// Save States

size_t SDL_Libretro_GetStateSize(const SDL_Libretro* lr);
bool SDL_Libretro_SaveState(SDL_Libretro* lr, const char* file);
bool SDL_Libretro_SaveState_IO(SDL_Libretro* lr, SDL_IOStream* dst, bool closeio);
bool SDL_Libretro_LoadState(SDL_Libretro* lr, const char* file);
bool SDL_Libretro_LoadState_IO(SDL_Libretro* lr, SDL_IOStream* src, bool closeio);

// Memory

bool SDL_Libretro_SaveMemory(SDL_Libretro* lr, unsigned memoryType, const char* file);
bool SDL_Libretro_SaveMemory_IO(SDL_Libretro* lr, unsigned memoryType, SDL_IOStream* dst, bool closeio);
bool SDL_Libretro_LoadMemory(SDL_Libretro* lr, unsigned memoryType, const char* file);
bool SDL_Libretro_LoadMemory_IO(SDL_Libretro* lr, unsigned memoryType, SDL_IOStream* src, bool closeio);
void* SDL_Libretro_GetMemoryData(const SDL_Libretro* lr, unsigned memoryType, size_t* size);
bool SDL_Libretro_SetMemoryData(SDL_Libretro* lr, unsigned memoryType, const void* data, size_t size);
bool SDL_Libretro_SaveSRAM(SDL_Libretro* lr, const char* file);
bool SDL_Libretro_SaveSRAM_IO(SDL_Libretro* lr, SDL_IOStream* dst, bool closeio);
bool SDL_Libretro_LoadSRAM(SDL_Libretro* lr, const char* file);
bool SDL_Libretro_LoadSRAM_IO(SDL_Libretro* lr, SDL_IOStream* src, bool closeio);
unsigned SDL_Libretro_GetMemoryMapCount(const SDL_Libretro* lr);
bool SDL_Libretro_GetMemoryMapDescriptor(const SDL_Libretro* lr, unsigned index,
    Uint64* flags, void** ptr, size_t* offset, size_t* start,
    size_t* select, size_t* disconnect, size_t* len, const char** addrspace);
void* SDL_Libretro_GetMapAddress(const SDL_Libretro* lr, size_t address, size_t* regionRemaining);

// Core Options

unsigned SDL_Libretro_GetOptionCount(const SDL_Libretro* lr);
const char* SDL_Libretro_GetOptionKey(const SDL_Libretro* lr, unsigned index);
const char* SDL_Libretro_GetOptionValue(const SDL_Libretro* lr, const char* key);
bool SDL_Libretro_SetOptionValue(SDL_Libretro* lr, const char* key, const char* value);
bool SDL_Libretro_ResetOption(SDL_Libretro* lr, const char* key);
void SDL_Libretro_ResetAllOptions(SDL_Libretro* lr);
bool SDL_Libretro_AreOptionsDirty(SDL_Libretro* lr);

// Cheats

bool SDL_Libretro_SetCheat(SDL_Libretro* lr, unsigned index, bool enabled, const char* code);
void SDL_Libretro_ResetCheats(SDL_Libretro* lr);

// Meta Data

const char* SDL_Libretro_GetCoreName(const SDL_Libretro* lr);
const char* SDL_Libretro_GetCoreVersion(const SDL_Libretro* lr);
const char* SDL_Libretro_GetValidExtensions(const SDL_Libretro* lr);
const char* SDL_Libretro_GetContentExtension(const SDL_Libretro* lr);
unsigned SDL_Libretro_GetPerformanceLevel(const SDL_Libretro* lr);

// Utilities

size_t SDL_Libretro_GetFileName(char* dst, size_t dstSize, const char* path, bool withExtension);
size_t SDL_Libretro_GetSavePath(const SDL_Libretro* lr, const char* extension, char* dst, size_t dstSize);

// Rewind

bool SDL_Libretro_SetRewindEnabled(SDL_Libretro* lr, bool enabled, unsigned bufferFrames, unsigned captureInterval);
bool SDL_Libretro_GetRewindEnabled(const SDL_Libretro* lr);
bool SDL_Libretro_RewindStep(SDL_Libretro* lr);
double SDL_Libretro_GetRewindRemaining(const SDL_Libretro* lr);
size_t SDL_Libretro_GetRewindMemoryUsage(const SDL_Libretro* lr);
void SDL_Libretro_SetRewindMemoryLimit(SDL_Libretro* lr, size_t maxBytes);
size_t SDL_Libretro_GetRewindMemoryLimit(const SDL_Libretro* lr);

// VFS

void SDL_Libretro_SetVFS(SDL_Libretro* lr, void* vfs);

// Logging

void SDL_Libretro_SetLogLevel(SDL_Libretro* lr, int level);
int SDL_Libretro_GetLogLevel(const SDL_Libretro* lr);

// On-Screen Display

void SDL_Libretro_SetMessage(SDL_Libretro* lr, const char* msg, double duration);
const char* SDL_Libretro_GetMessage(SDL_Libretro* lr);

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
#define SDL_LIBRETRO_AUDIO_SINGLE_SAMPLE_BUFFER_SIZE 512
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

    bool loaded;     /** A core is loaded. */
    bool gameLoaded; /** A game is loaded into the core (content or no-content). */
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
    bool supportNoGame;
    unsigned apiVersion;
    enum retro_pixel_format pixelFormat;
    unsigned performanceLevel; /** @see SDL_Libretro_GetPerformanceLevel() */
    uint64_t serializationQuirks;
    int rotation;

    // Video
    SDL_Texture* texture;
    SDL_Renderer* renderer;
    SDL_FRect renderDstRect;
    bool videoReinitPending;

    // Audio
    SDL_AudioStream* audioStream;
    int audioQueueThresholdBytes;
    unsigned minimumAudioLatencyMs;
    int16_t singleSampleBuffer[SDL_LIBRETRO_AUDIO_SINGLE_SAMPLE_BUFFER_SIZE * 2];
    size_t singleSampleCount;
    int audioDropWarnCount;
    bool audioReinitPending;
    struct retro_audio_callback audio_callback;
    struct retro_audio_buffer_status_callback audio_buffer_status; /** @see SDL_Libretro_ReportAudioBufferStatus() */

    // Input
    SDL_Window* window;
    float inputLastMouseX, inputLastMouseY;
    float inputMouseX, inputMouseY;
    unsigned portDeviceMap[16];
    bool virtualJoypadState[16];
    retro_keyboard_event_t keyboard_event;

    // Timing
    struct retro_frame_time_callback runloop_frame_time;
    retro_usec_t runloop_frame_time_last;

    // Core Options
    SDL_LibretroCoreOption* options;
    unsigned optionCount;
    unsigned optionCapacity;
    bool optionsDirty;

    // Input Descriptors
    struct retro_input_descriptor* inputDescriptors;
    unsigned inputDescriptorCount;
    struct retro_controller_info* controllerInfo;
    unsigned controllerPortCount;

    // Game Content
    char contentPath[SDL_LIBRETRO_MAX_PATH]; /** The path to the content that's being loaded. */
    char contentName[SDL_LIBRETRO_MAX_PATH]; /** The human-readable content name. */
    char contentDir[SDL_LIBRETRO_MAX_PATH]; /** Directory of the content file; backs gameInfoExt.dir. */
    char contentExt[16]; /** Lower-case content extension; backs gameInfoExt.ext. */

    struct retro_game_info_ext gameInfoExt; /** Extended game info handed to cores via GET_GAME_INFO_EXT. A non-NULL full_path marks it valid; .data owns the content buffer when persistent. */

    // Content Info Overrides
    struct retro_system_content_info_override* contentInfoOverrides; /** Deep-copied; owned by the context. */
    unsigned contentInfoOverrideCount; /** The number of content info overrides. @see contentInfoOverrides */

    // Rumble
    float rumbleStrong[SDL_LIBRETRO_RUMBLE_PORTS];
    float rumbleWeak[SDL_LIBRETRO_RUMBLE_PORTS];

    // Disk control
    struct retro_disk_control_ext_callback disk_control;
    bool diskControlActive;

    // Memory Maps
    struct retro_memory_descriptor* memoryMapDescriptors;
    unsigned memoryMapDescriptorCount;

    // Dynamic Rate Control
    bool drcEnabled; /** Dynamic Rate Control. @see SDL_Libretro_UpdateDRC() */
    float drcAdjustment; /** How much the dynamic rate control should be adjusted. */
    double drcDriftAvg;

    // Performance Counter
    struct retro_perf_counter* perfCounters[64];
    unsigned perfCounterCount;
    retro_perf_tick_t gameTimeNSEC;
} SDL_LibretroCoreData;

typedef struct SDL_LibretroRewindDelta {
    unsigned char* data;
    size_t length;   /* used bytes of the encoded delta */
    size_t capacity; /* allocated bytes of data (>= length); enables in-place reuse */
} SDL_LibretroRewindDelta;

struct SDL_Libretro {
    // Persistent Settings Across Cores
    float volume; /** The audio volume. */
    float speed;
    SDL_LibretroScaleMode scaleMode;
    double speedAccumulator;
    Uint64 lastTickNS; /* Wall-clock of the previous RunFrame (SDL_GetTicksNS); 0 until first call. */
    SDL_Scancode keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_R3 + 1];
    char coreDirectory[SDL_LIBRETRO_MAX_PATH];
    char saveDirectory[SDL_LIBRETRO_MAX_PATH];
    char systemDirectory[SDL_LIBRETRO_MAX_PATH];
    char coreAssetsDirectory[SDL_LIBRETRO_MAX_PATH];
    char playlistDirectory[SDL_LIBRETRO_MAX_PATH];
    char fileBrowserStartDirectory[SDL_LIBRETRO_MAX_PATH];
    char username[128];

    // Logging
    int logLevel;

    // On-Screen Display Message
    char osdMessage[256]; /** The current On-Screen Display message. */
    Uint64 osdEndTimeMs; /** The time at which the OSD should finish. */

    // Virtual File System
    struct retro_vfs_interface vfs_interface;

    // Rewind (delta-compressed circular buffer)
    unsigned char* rewindReference;
    unsigned char* rewindScratch;
    unsigned char* rewindEncodeScratch; /* reusable worst-case-sized buffer for one-pass delta encoding */
    SDL_LibretroRewindDelta* rewindEntries;
    size_t rewindSlotSize;
    size_t rewindBytes;    /* live encoded delta bytes currently stored */
    size_t rewindMaxBytes; /* memory budget for delta data; 0 = unbounded */
    unsigned rewindCapacity;
    unsigned rewindHead;
    unsigned rewindCount;
    unsigned rewindCaptureInterval;
    unsigned rewindFrameCounter;
    bool rewindEnabled;
    bool rewindHasReference;
    bool rewindActive; /* true only during a backward step's re-run (mutes audio, neutralizes input) */

    /* SDL gamepads (opened handles, indexed by port) */
    SDL_Gamepad* gamepads[16];
    unsigned gamepadCount;

    /* Per-core state */
    SDL_LibretroCoreData core;
};

/**
 * Active context for libretro C callbacks (one per process)
 */
static SDL_Libretro* SDL_Libretro_active = NULL;

static bool SDL_Libretro_InitVideo(SDL_Libretro* lr);
static void SDL_Libretro_CloseVideo(SDL_Libretro* lr);
static void SDL_Libretro_VideoRefresh(const void* data, unsigned width, unsigned height, size_t pitch);

static void SDL_Libretro_AudioSample(int16_t left, int16_t right);
static size_t SDL_Libretro_AudioSampleBatch(const int16_t* data, size_t frames);
static void SDL_Libretro_FlushSingleSamples(SDL_Libretro* lr);
static void SDL_Libretro_UpdateDRC(SDL_Libretro* lr, float speed);
static unsigned SDL_Libretro_UpdateAudioThreshold(SDL_Libretro* lr);
static void SDL_Libretro_ReportAudioBufferStatus(SDL_Libretro* lr);

static void SDL_Libretro_InputPoll(void);
static int16_t SDL_Libretro_InputState(unsigned port, unsigned device, unsigned index, unsigned id);

static bool SDL_Libretro_EnvironmentCallback(unsigned cmd, void* data);
static void SDL_Libretro_ClearRewind(SDL_Libretro* lr);

static SDL_Scancode SDL_Libretro_RetroKeyToScancode(unsigned key);
static unsigned SDL_Libretro_ScancodeToRetroKey(SDL_Scancode scancode);
static uint16_t SDL_Libretro_KeymodToRetroMod(SDL_Keymod mod);
static SDL_GamepadButton SDL_Libretro_RetroJoypadToGamepadButton(unsigned button);

#ifdef SDL_LIBRETRO_ENABLE_REWIND_DELTA
static size_t SDL_Libretro_RewindEncodeDelta(const unsigned char* cur, const unsigned char* ref, size_t len, unsigned char* out, size_t outCap);
static bool SDL_Libretro_RewindDecodeDelta(const unsigned char* delta, size_t deltaLen, unsigned char* state, size_t stateLen);
#endif
static void SDL_Libretro_RewindCapture(SDL_Libretro* lr);
static bool SDL_Libretro_RewindStepState(SDL_Libretro* lr);
static void SDL_Libretro_RewindFreeEntry(SDL_Libretro* lr, SDL_LibretroRewindDelta* entry);
static void SDL_Libretro_RewindEvictToBudget(SDL_Libretro* lr);
static void SDL_Libretro_RewindFree(SDL_Libretro* lr);

static bool SDL_Libretro_ExtensionInList(const char* ext, const char* pipeList);

static void SDL_Libretro_InitCoreOption(SDL_Libretro* lr, const char* key, const char* defaultValue,
    const char* label, const char* valuesList, const char* displayList,
    const char* tooltip, const char* categoryKey);
static void SDL_Libretro_FreeCoreOptions(SDL_Libretro* lr);

static void SDL_Libretro_FreeMemoryMap(SDL_Libretro* lr);
static void SDL_Libretro_FreeContentInfoOverrides(SDL_Libretro* lr);

#include "SDL_libretro_video.h"
#include "SDL_libretro_audio.h"
#include "SDL_libretro_input.h"
#include "SDL_libretro_options.h"
#include "SDL_libretro_serialize.h"
#include "SDL_libretro_vfs.h"
#include "SDL_libretro_env.h"
#include "SDL_libretro_core.h"

#endif /* SDL_LIBRETRO_IMPLEMENTATION_ONCE */
#endif /* SDL_LIBRETRO_IMPLEMENTATION */

#endif /* SDL_LIBRETRO_H */
