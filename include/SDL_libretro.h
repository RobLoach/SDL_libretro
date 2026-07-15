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

/**
 * \defgroup SDL_Libretro SDL_Libretro
 * @{
 */

/**
 * The major version of SDL_Libretro.
 *
 * \see SDL_LIBRETRO_VERSION
 * \see SDL_LIBRETRO_VERSION_ATLEAST
 */
#define SDL_LIBRETRO_MAJOR_VERSION 0

/**
 * The minor version of SDL_Libretro.
 *
 * \see SDL_LIBRETRO_VERSION
 * \see SDL_LIBRETRO_VERSION_ATLEAST
 */
#define SDL_LIBRETRO_MINOR_VERSION 1

/**
 * The micro/patch version of SDL_Libretro.
 *
 * \see SDL_LIBRETRO_VERSION
 * \see SDL_LIBRETRO_VERSION_ATLEAST
 */
#define SDL_LIBRETRO_MICRO_VERSION 0

/**
 * Retrieves an integer representation of the of the SDL_Libretro version.
 */
#define SDL_LIBRETRO_VERSION SDL_VERSIONNUM(SDL_LIBRETRO_MAJOR_VERSION, SDL_LIBRETRO_MINOR_VERSION, SDL_LIBRETRO_MICRO_VERSION)

/**
 * Checks if SDL_Libretro is at least the given version.
 */
#define SDL_LIBRETRO_VERSION_ATLEAST(X, Y, Z) (SDL_LIBRETRO_VERSION >= SDL_VERSIONNUM(X, Y, Z))

typedef struct SDL_Libretro SDL_Libretro;

/**
 * When rendering the libretro context, determine how to display within the destination.
 */
typedef enum SDL_LibretroFitMode {
    SDL_LIBRETRO_FIT_ASPECT = 0, /** Keep the same aspect ratio, and fit within the confines of the destination. */
    SDL_LIBRETRO_FIT_INTEGER = 1, /** Keep the same aspect ratio, while keeping integer scaling within the confines of the destination. */
    SDL_LIBRETRO_FIT_STRETCH = 2 /** Ignore the aspect ratio, and stretch the image to match the destination rectangle. */
} SDL_LibretroFitMode;

#ifdef __cplusplus
extern "C" {
#endif

// Libretro Instance

SDL_Libretro* SDL_Libretro_Create(void);
void SDL_Libretro_Destroy(SDL_Libretro* lr);
int SDL_Libretro_Version(void);

// Directories

bool SDL_Libretro_SetCoreDirectory(SDL_Libretro* lr, const char* path);
bool SDL_Libretro_SetSaveDirectory(SDL_Libretro* lr, const char* path);
bool SDL_Libretro_SetSystemDirectory(SDL_Libretro* lr, const char* path);
bool SDL_Libretro_SetCoreAssetsDirectory(SDL_Libretro* lr, const char* path);
const char* SDL_Libretro_GetCoreDirectory(SDL_Libretro* lr);
const char* SDL_Libretro_GetSaveDirectory(SDL_Libretro* lr);
const char* SDL_Libretro_GetSystemDirectory(SDL_Libretro* lr);
const char* SDL_Libretro_GetCoreAssetsDirectory(SDL_Libretro* lr);
bool SDL_Libretro_SetUsername(SDL_Libretro* lr, const char* username);
const char* SDL_Libretro_GetUsername(SDL_Libretro* lr);

// Config

bool SDL_Libretro_InitConfig(SDL_Libretro* lr, const char* org, const char* app);
bool SDL_Libretro_InitConfigFile(SDL_Libretro* lr, const char* file);

// Core

bool SDL_Libretro_LoadCore(SDL_Libretro* lr, const char* core);
void SDL_Libretro_UnloadCore(SDL_Libretro* lr);
bool SDL_Libretro_IsCoreReady(const SDL_Libretro* lr);
bool SDL_Libretro_ShouldQuit(const SDL_Libretro* lr);

// Game

bool SDL_Libretro_LoadGame(SDL_Libretro* lr, const char* gamePath);
bool SDL_Libretro_LoadGame_IO(SDL_Libretro* lr, SDL_IOStream* src, const char* path, bool closeio);
void SDL_Libretro_UnloadGame(SDL_Libretro* lr);
bool SDL_Libretro_IsGameReady(const SDL_Libretro* lr);
bool SDL_Libretro_IsGameRequired(const SDL_Libretro* lr);
bool SDL_Libretro_Reset(SDL_Libretro* lr);
void SDL_Libretro_Update(SDL_Libretro* lr);

// Video

bool SDL_Libretro_SetRenderer(SDL_Libretro* lr, SDL_Renderer* renderer);
SDL_Renderer* SDL_Libretro_GetRenderer(const SDL_Libretro* lr);
SDL_Texture* SDL_Libretro_GetTexture(const SDL_Libretro* lr);
SDL_Surface* SDL_Libretro_CreateSurface(const SDL_Libretro* lr);
bool SDL_Libretro_Render(SDL_Renderer* renderer, SDL_Libretro* lr, const SDL_FRect* dstRect);
void SDL_Libretro_GetSize(const SDL_Libretro* lr, int* w, int* h);
float SDL_Libretro_GetAspectRatio(const SDL_Libretro* lr);
double SDL_Libretro_GetFPS(const SDL_Libretro* lr);
int SDL_Libretro_GetRotation(const SDL_Libretro* lr);
void SDL_Libretro_SetFitMode(SDL_Libretro* lr, SDL_LibretroFitMode mode);
SDL_LibretroFitMode SDL_Libretro_GetFitMode(const SDL_Libretro* lr);

// Audio

void SDL_Libretro_SetVolume(SDL_Libretro* lr, float volume);
float SDL_Libretro_GetVolume(const SDL_Libretro* lr);
void SDL_Libretro_SetSpeed(SDL_Libretro* lr, float speed);
float SDL_Libretro_GetSpeed(const SDL_Libretro* lr);
bool SDL_Libretro_IsFastforwardOverrideActive(const SDL_Libretro* lr);
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
bool SDL_Libretro_GetInputDescriptor(const SDL_Libretro* lr, unsigned index, unsigned* port, unsigned* device, unsigned* id, const char** description);

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

/**
 * One selectable value for a core option.
 */
typedef struct SDL_LibretroOptionValue {
    const char* value; /** The value the core stores. */
    const char* label; /** Human-readable name; may be NULL, in which case display value itself. */
} SDL_LibretroOptionValue;

/**
 * A core option and its current state.
 */
typedef struct SDL_LibretroOption {
    const char* key;          /** Unique identifier the core queries. */
    const char* desc;         /** Human-readable name. */
    const char* info;         /** Description / help text; may be empty. */
    const char* value;        /** The current value. */
    const char* defaultValue; /** The default value. */
    const char* category;     /** Key of the category it belongs to; empty if none. */
    bool visible;             /** Whether the frontend should display it. */
    unsigned valuesCount;     /** The number of populated entries in values. */
    SDL_LibretroOptionValue* values; /** The selectable values; dynamically allocated. */
    unsigned valuesCapacity;  /** Allocated capacity of the values array. */
} SDL_LibretroOption;

/**
 * A group of related core options.
 */
typedef struct SDL_LibretroCategory {
    const char* key;  /** Unique identifier referenced by SDL_LibretroOption::category. */
    const char* desc; /** Human-readable name. */
    const char* info; /** Description / help text; may be empty. */
} SDL_LibretroCategory;

unsigned SDL_Libretro_GetOptionCount(const SDL_Libretro* lr);
const SDL_LibretroOption* SDL_Libretro_GetOption(const SDL_Libretro* lr, const char* key);
const SDL_LibretroOption* SDL_Libretro_GetOptionByIndex(const SDL_Libretro* lr, unsigned index);
bool SDL_Libretro_SetOptionValue(SDL_Libretro* lr, const char* key, const char* value);
const char* SDL_Libretro_GetOptionValue(SDL_Libretro* lr, const char* key);
const char* SDL_Libretro_GetOptionValueLabel(SDL_Libretro* lr, const char* key);
bool SDL_Libretro_CycleOptionValue(SDL_Libretro* lr, const char* key, int direction);
bool SDL_Libretro_ResetOption(SDL_Libretro* lr, const char* key);
void SDL_Libretro_ResetAllOptions(SDL_Libretro* lr);
bool SDL_Libretro_AreOptionsDirty(SDL_Libretro* lr);
bool SDL_Libretro_UpdateOptionVisibility(SDL_Libretro* lr);
unsigned SDL_Libretro_GetCategoryCount(const SDL_Libretro* lr);
const SDL_LibretroCategory* SDL_Libretro_GetCategory(const SDL_Libretro* lr, const char* key);
const SDL_LibretroCategory* SDL_Libretro_GetCategoryByIndex(const SDL_Libretro* lr, unsigned index);

// Subsystems

bool SDL_Libretro_LoadGameSpecial(SDL_Libretro* lr, const char* subsystem, const char** paths, unsigned numPaths);
bool SDL_Libretro_LoadGameSpecialById(SDL_Libretro* lr, unsigned subsystemId, const char** paths, unsigned numPaths);

// Disk Control

unsigned SDL_Libretro_GetDiskCount(const SDL_Libretro* lr);
unsigned SDL_Libretro_GetDiskIndex(const SDL_Libretro* lr);
bool SDL_Libretro_SetDiskIndex(SDL_Libretro* lr, unsigned index);
bool SDL_Libretro_EjectDisk(SDL_Libretro* lr);
bool SDL_Libretro_InsertDisk(SDL_Libretro* lr);
bool SDL_Libretro_AddDiskImage(SDL_Libretro* lr, const char* path);
bool SDL_Libretro_AddDiskImage_IO(SDL_Libretro* lr, SDL_IOStream* src, bool closeio);
bool SDL_Libretro_GetDiskLabel(const SDL_Libretro* lr, unsigned index, char* label, size_t len);
bool SDL_Libretro_SetInitialDisk(SDL_Libretro* lr, unsigned index, const char* path);

// Cheats

bool SDL_Libretro_SetCheat(SDL_Libretro* lr, unsigned index, bool enabled, const char* code);
void SDL_Libretro_ResetCheats(SDL_Libretro* lr);

// Meta Data

const char* SDL_Libretro_GetCoreName(const SDL_Libretro* lr);
const char* SDL_Libretro_GetCoreVersion(const SDL_Libretro* lr);
const char* SDL_Libretro_GetValidExtensions(const SDL_Libretro* lr);
const char* SDL_Libretro_GetContentExtension(const SDL_Libretro* lr);
bool SDL_Libretro_GetBlockExtract(const SDL_Libretro* lr);
unsigned SDL_Libretro_GetPerformanceLevel(const SDL_Libretro* lr);
enum retro_savestate_context SDL_Libretro_GetSavestateContext(const SDL_Libretro* lr);
void SDL_Libretro_SetSavestateContext(SDL_Libretro* lr, enum retro_savestate_context context);

// Utilities

size_t SDL_Libretro_GetFileName(char* dst, size_t dstSize, const char* path, bool withExtension);
size_t SDL_Libretro_GetSavePath(const SDL_Libretro* lr, const char* extension, char* dst, size_t dstSize);

// Rewind

bool SDL_Libretro_SetRewindEnabled(SDL_Libretro* lr, bool enabled, unsigned bufferFrames, unsigned captureInterval);
bool SDL_Libretro_GetRewindEnabled(const SDL_Libretro* lr);
double SDL_Libretro_GetRewindRemaining(const SDL_Libretro* lr);
size_t SDL_Libretro_GetRewindMemoryUsage(const SDL_Libretro* lr);
void SDL_Libretro_SetRewindMemoryLimit(SDL_Libretro* lr, size_t maxBytes);
size_t SDL_Libretro_GetRewindMemoryLimit(const SDL_Libretro* lr);
bool SDL_Libretro_SetRewindMemoryDuration(SDL_Libretro* lr, double seconds);

// VFS

void SDL_Libretro_SetVFS(SDL_Libretro* lr, void* vfs);

// Logging

void SDL_Libretro_SetLogLevel(SDL_Libretro* lr, SDL_LogPriority level);
SDL_LogPriority SDL_Libretro_GetLogLevel(const SDL_Libretro* lr);

// On-Screen Display

void SDL_Libretro_SetMessage(SDL_Libretro* lr, const char* msg, double duration);
const char* SDL_Libretro_GetMessage(SDL_Libretro* lr);
int SDL_Libretro_GetMessageProgress(SDL_Libretro* lr);
int SDL_Libretro_GetMessageType(SDL_Libretro* lr);
unsigned SDL_Libretro_GetMessageCount(SDL_Libretro* lr);
bool SDL_Libretro_GetMessageByIndex(SDL_Libretro* lr, int index, const char** msg, int* progress, int* type);

// PhysFS

bool SDL_Libretro_PhysFS_Init(SDL_Libretro* lr);
void SDL_Libretro_PhysFS_Quit(SDL_Libretro* lr);
bool SDL_Libretro_PhysFS_LoadGame(SDL_Libretro* lr, const char* gamePath);

/**
 * @}
 */

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

#ifndef SDL_LIBRETRO_NO_INI_IMPLEMENTATION
#define SDL_INI_IMPLEMENTATION
#include "SDL_ini.h"
#endif

#ifndef SDL_LIBRETRO_MAX_PATH
/**
 * The maximum length for file system paths.
 *
 * @internal
 */
#define SDL_LIBRETRO_MAX_PATH 4096
#endif
#define SDL_LIBRETRO_AUDIO_SINGLE_SAMPLE_BUFFER_SIZE 512
#define SDL_LIBRETRO_MAX_RUMBLE_PORTS 4
#define SDL_LIBRETRO_MAX_SENSOR_PORTS 4
#define SDL_LIBRETRO_OSD_INITIAL_CAPACITY 4

#ifndef SDL_LIBRETRO_MAX_GAMEPADS
/**
 * The number of controller ports (gamepads) the frontend tracks.
 */
#define SDL_LIBRETRO_MAX_GAMEPADS 8
#endif

/**
 * The number of RETRO_DEVICE_ID_JOYPAD_* button ids (0..R3 inclusive).
 *
 * RETRO_DEVICE_ID_JOYPAD_R3 + 1
 *
 * @see RETRO_DEVICE_ID_JOYPAD_R3
 * @internal
 */
#define SDL_LIBRETRO_MAX_JOYPAD_BUTTONS 16

typedef struct SDL_Libretro_CoreInfo {
    char* corename;
    char* supported_extensions;
    char* path;
    bool needs_fullpath;
    bool supports_no_game;
    bool block_extract;
} SDL_Libretro_CoreInfo;

typedef struct SDL_LibretroOsdEntry {
    char* msg;
    Uint64 endTimeMs;
    unsigned priority;
    enum retro_message_type type;
    int8_t progress;
} SDL_LibretroOsdEntry;

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

/**
 * @see SDL_Libretro_MicOpen()
 */
typedef struct SDL_LibretroMicrophone {
    SDL_AudioStream* stream;
    unsigned rate; /** The sample rate. */
    bool active; /** Whether or not the micropohne device is active. */
    SDL_Libretro* lr; /** A pointer back to the libretro data, used to dereference itself in case the core doesn't close for us. */
} SDL_LibretroMicrophone;

typedef struct SDL_LibretroCoreData {
    SDL_LibretroCoreSymbols symbols;

    bool gameLoaded; /** A game is currently loaded into the core (content or no-content). */
    bool shutdown; /** Whether or not the core has requested to shutdown. */
    float speed; /** The speed the core is running. 1.0f is normal, 0.5f slow motion, 1.5f fast forward, -1.0f rewind. Reset to 1.0f each time a core is loaded. */
    double speedAccumulator; /** Fractional frames accumulated for run-loop pacing; reset with the core. */
    Uint64 lastTickNS; /** Wall-clock of the previous RunFrame (SDL_GetTicksNS); 0 until first call, reset with the core. */
    unsigned width, height;
    double fps;
    double sampleRate;
    float aspectRatio;
    char corePath[SDL_LIBRETRO_MAX_PATH];
    char libraryName[128];
    char libraryVersion[128];
    char validExtensions[128];
    bool needFullpath;
    bool blockExtract;
    bool usedVFS; /** The core requested RETRO_ENVIRONMENT_GET_VFS_INTERFACE. */
    bool supportNoGame;
    unsigned apiVersion;
    enum retro_pixel_format pixelFormat;
    unsigned performanceLevel; /** @see SDL_Libretro_GetPerformanceLevel() */
    uint64_t serializationQuirks;
    enum retro_savestate_context savestateContext;
    int rotation;

    // Video
    SDL_Texture* texture;
    SDL_ScaleMode textureScaleMode;
    SDL_FRect renderDstRect; /** The desired destination rendering rectangle. */
    bool videoReinitPending; /** True when the video requires a re-initialization. */

    /**
     * Non-NULL while a software buffer lock is active.
     * @see RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER
     */
    void* softwareFramebufferPixels;

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
    float inputLastMouseX, inputLastMouseY;
    float inputMouseX, inputMouseY;
    float inputWheelAccumX, inputWheelAccumY; /** Wheel delta accumulated between frames via SDL_Libretro_HandleEvent(). */
    float inputWheelX, inputWheelY; /** Wheel deltas for the current frame, snapshotted in SDL_Libretro_InputPoll(). */
    unsigned portDeviceMap[SDL_LIBRETRO_MAX_GAMEPADS];
    bool virtualJoypadState[SDL_LIBRETRO_MAX_GAMEPADS][SDL_LIBRETRO_MAX_JOYPAD_BUTTONS];
    retro_keyboard_event_t keyboard_event;

    // Timing
    struct retro_frame_time_callback runloop_frame_time;
    retro_usec_t runloop_frame_time_last;

    // Fast-forward override
    struct retro_fastforwarding_override fastforwardOverride;
    bool fastforwardOverrideActive;

    // Core Options
    SDL_LibretroOption* options; /** The options that have been set by the core. The strings are owned by the context. */
    unsigned optionCount; /** The number of options. */
    unsigned optionCapacity; /** The capacity of the options array. */
    bool optionsDirtyCore; /** An option changed since the core last polled GET_VARIABLE_UPDATE. */
    bool optionsDirtyApp; /** An option changed since the app last called SDL_Libretro_AreOptionsDirty(). */

    retro_core_options_update_display_callback_t optionsUpdateDisplayCallback; /** A callback used to determine the visibility of options. */
    SDL_LibretroCategory* optionCategories; /** Categories registered by the core. The strings are owned by the context. */
    unsigned optionCategoriesCount;
    unsigned optionCategoriesCapacity;

    // Input Descriptors
    struct retro_input_descriptor* inputDescriptors;
    unsigned inputDescriptorsCount;

    // Controller Information
    struct retro_controller_info* controllerInfo;
    unsigned controllerInfoCount;

    // Game Content
    char contentPath[SDL_LIBRETRO_MAX_PATH]; /** The path to the content that's being loaded. */
    char contentName[SDL_LIBRETRO_MAX_PATH]; /** The human-readable content name. */
    char contentDir[SDL_LIBRETRO_MAX_PATH]; /** Directory of the content file; backs gameInfoExt.dir. */
    char contentExt[8]; /** Lower-case content extension; backs gameInfoExt.ext. */
    char contentTempPath[SDL_LIBRETRO_MAX_PATH]; /** File spilled to disk by SDL_Libretro_LoadGame_IO() for a need_fullpath core; removed when the content unloads. */

    struct retro_game_info_ext gameInfoExt; /** Extended game info handed to cores via GET_GAME_INFO_EXT. A non-NULL full_path marks it valid; .data owns the content buffer when persistent. */

    // Content Info Overrides
    struct retro_system_content_info_override* contentInfoOverrides; /** Deep-copied; owned by the context. */
    unsigned contentInfoOverrideCount; /** The number of content info overrides. @see contentInfoOverrides */

    // Subsystems
    struct retro_subsystem_info* subsystems; /** Deep-copied from RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO; owned by the context. */
    unsigned subsystemCount;

    // Rumble
    float rumbleStrong[SDL_LIBRETRO_MAX_RUMBLE_PORTS];
    float rumbleWeak[SDL_LIBRETRO_MAX_RUMBLE_PORTS];

    // Sensors
    SDL_Sensor* sensorAccel[SDL_LIBRETRO_MAX_SENSOR_PORTS];
    SDL_Sensor* sensorGyro[SDL_LIBRETRO_MAX_SENSOR_PORTS];
    float sensorAccelData[SDL_LIBRETRO_MAX_SENSOR_PORTS][3];
    float sensorGyroData[SDL_LIBRETRO_MAX_SENSOR_PORTS][3];

    // Microphone
    SDL_LibretroMicrophone* microphone;

    // Disk control
    struct retro_disk_control_ext_callback disk_control;

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
    SDL_LibretroFitMode fitMode; /** How the libretro context should fit into its destination when rendering. */
    SDL_Scancode keyboardPlayer1[SDL_LIBRETRO_MAX_JOYPAD_BUTTONS];
    char coreDirectory[SDL_LIBRETRO_MAX_PATH];
    char saveDirectory[SDL_LIBRETRO_MAX_PATH];
    char systemDirectory[SDL_LIBRETRO_MAX_PATH];
    char coreAssetsDirectory[SDL_LIBRETRO_MAX_PATH];
    char playlistDirectory[SDL_LIBRETRO_MAX_PATH];
    char fileBrowserStartDirectory[SDL_LIBRETRO_MAX_PATH];
    char username[64];

    // Video
    SDL_Renderer* renderer; /** The render target. @see SDL_Libretro_SetRenderer() */
    SDL_Window* window; /** Derived from the renderer via SDL_GetRenderWindow(). */

    // Logging
    enum retro_log_level logLevel;

    // On-Screen Display Message Queue
    SDL_LibretroOsdEntry* osdQueue;
    int osdQueueCount;
    int osdQueueCapacity;

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
    bool rewindEnabled; /** When enabled, will capture serialized frames to allow rewinding. */
    bool rewindHasReference;
    bool rewindActive; /** true only during a backward step's re-run (mutes audio, neutralizes input) */

    // Input
    SDL_Gamepad* gamepads[SDL_LIBRETRO_MAX_GAMEPADS];
    unsigned gamepadCount;

    SDL_LibretroCoreData core; /** The loaded core state. */

    // Configuration
    char* iniFile;
    SDL_ini* ini;

    // Core Library
    SDL_Libretro_CoreInfo* coreLibrary; /** The list of cores that were discovered in the "cores" directory. */
    unsigned coreLibraryCount; /** The number of core libraries represented in coreLibrary. */

    void* userData; /** Generic data available to the implementation. */

    #if defined(SDL_LIBRETRO_ENABLE_PHYSFS) && !defined(SDL_LIBRETRO_DISABLE_PHYSFS)
    bool physfsReady; /** PhysFS is initialized and the VFS overrides are installed. */
    char physfsMountSource[SDL_LIBRETRO_MAX_PATH]; /** The archive currently mounted at the mount point. */
    #endif
};

/**
 * Active context for libretro C callbacks (one per process).
 *
 * libretro currently only allows running one core per thread, so that's what we'll target.
 */
static SDL_Libretro* SDL_Libretro_active = NULL;

static bool SDL_Libretro_InitVideo(SDL_Libretro* lr);
static void SDL_Libretro_CloseVideo(SDL_Libretro* lr);
static void SDL_Libretro_VideoRefresh(const void* data, unsigned width, unsigned height, size_t pitch);
static void SDL_Libretro_ReleaseSoftwareFramebuffer(SDL_Libretro* lr);

static void SDL_Libretro_AudioSample(int16_t left, int16_t right);
static size_t SDL_Libretro_AudioSampleBatch(const int16_t* data, size_t frames);
static void SDL_Libretro_FlushSingleSamples(SDL_Libretro* lr);
static void SDL_Libretro_UpdateDRC(SDL_Libretro* lr, float speed);
static unsigned SDL_Libretro_UpdateAudioThreshold(SDL_Libretro* lr);
static void SDL_Libretro_ReportAudioBufferStatus(SDL_Libretro* lr);

static bool SDL_Libretro_InitAudio(SDL_Libretro* lr);
static void SDL_Libretro_CloseAudio(SDL_Libretro* lr);
static void SDL_Libretro_InputPoll(void);
static int16_t SDL_Libretro_InputState(unsigned port, unsigned device, unsigned index, unsigned id);
static bool SDL_Libretro_RewindStep(SDL_Libretro* lr);
static void SDL_Libretro_OsdPush(SDL_Libretro* lr, const char* msg, double durationSec, unsigned priority, enum retro_message_type type, int8_t progress);
static void SDL_Libretro_FreeMessages(SDL_Libretro* lr);
static bool SDL_Libretro_EnvironmentCallback(unsigned cmd, void* data);
static void SDL_Libretro_ClearRewind(SDL_Libretro* lr);

static SDL_Scancode SDL_Libretro_RetroKeyToScancode(unsigned key);
static unsigned SDL_Libretro_ScancodeToRetroKey(SDL_Scancode scancode);
static uint16_t SDL_Libretro_KeymodToRetroMod(SDL_Keymod mod);
static SDL_GamepadButton SDL_Libretro_RetroJoypadToGamepadButton(unsigned button);

// Sensors

static bool SDL_Libretro_SetSensorState(unsigned port, enum retro_sensor_action action, unsigned rate);
static float SDL_Libretro_GetSensorInput(unsigned port, unsigned id);
static void SDL_Libretro_CloseSensors(SDL_Libretro* lr);

// Microphone

static retro_microphone_t* SDL_Libretro_MicOpen(const retro_microphone_params_t* params);
static void SDL_Libretro_MicClose(retro_microphone_t* microphone);
static bool SDL_Libretro_MicGetParams(const retro_microphone_t* microphone, retro_microphone_params_t* params);
static bool SDL_Libretro_MicSetState(retro_microphone_t* microphone, bool state);
static bool SDL_Libretro_MicGetState(const retro_microphone_t* microphone);
static int SDL_Libretro_MicRead(retro_microphone_t* microphone, int16_t* samples, size_t num_samples);
static void SDL_Libretro_CloseMicrophone(SDL_Libretro* lr);

// Rewind

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
    const char* desc, const struct retro_core_option_value* values,
    const char* info, const char* categoryKey);
static void SDL_Libretro_InitCoreOptionCategory(SDL_Libretro* lr, const char* key,
    const char* desc, const char* info);
static void SDL_Libretro_FreeCoreOptions(SDL_Libretro* lr);

static void SDL_Libretro_FreeMemoryMap(SDL_Libretro* lr);
static void SDL_Libretro_FreeContentInfoOverrides(SDL_Libretro* lr);
static void SDL_Libretro_FreeSubsystems(SDL_Libretro* lr);
static void SDL_Libretro_FreeInputDescriptors(SDL_Libretro* lr);
static void SDL_Libretro_FreeControllerInfo(SDL_Libretro* lr);
static const struct retro_subsystem_info* SDL_Libretro_GetSubsystemById(const SDL_Libretro* lr, unsigned subsystemId);
static const struct retro_subsystem_info* SDL_Libretro_GetSubsystemByName(const SDL_Libretro* lr, const char* name);

// Directory

static void SDL_Libretro_FreeCoreLibrary(SDL_Libretro* lr);

// Config

static bool SDL_Libretro_LoadCoreConfig(SDL_Libretro* lr);
static bool SDL_Libretro_SaveCoreConfig(SDL_Libretro* lr);
static bool SDL_Libretro_CloseConfig(SDL_Libretro* lr);

// PhysFS

static void SDL_Libretro_PhysFS_ClearMount(SDL_Libretro* lr);

#include "SDL_libretro_video.h"
#include "SDL_libretro_audio.h"
#include "SDL_libretro_input.h"
#include "SDL_libretro_options.h"
#include "SDL_libretro_serialize.h"
#include "SDL_libretro_vfs.h"
#include "SDL_libretro_messages.h"
#include "SDL_libretro_env.h"
#include "SDL_libretro_core.h"
#include "SDL_libretro_config.h"
#include "SDL_libretro_physfs.h"

#endif /* SDL_LIBRETRO_IMPLEMENTATION_ONCE */
#endif /* SDL_LIBRETRO_IMPLEMENTATION */

#endif /* SDL_LIBRETRO_H */
