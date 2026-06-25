#define SDL_LIBRETRO_IMPLEMENTATION
#include "SDL_libretro.h"

#include <SDL3/SDL_main.h>
#include <SDL3/SDL_test.h>

static int SDLCALL test_CreateDestroy(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDLTest_AssertCheck(lr != NULL, "SDL_Libretro_Create returned non-NULL");

    // Defaults
    SDLTest_AssertCheck(lr->volume == 1.0f, "Default volume is 1.0");
    SDLTest_AssertCheck(lr->speed == 1.0f, "Default speed is 1.0");
    SDLTest_AssertCheck(SDL_strcmp(lr->username, "SDL_libretro") == 0, "Default username is SDL_libretro");

    // Default Keyboard Mappings
    SDLTest_AssertCheck(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_B] == SDL_SCANCODE_Z, "B mapped to Z");
    SDLTest_AssertCheck(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_X, "A mapped to X");
    SDLTest_AssertCheck(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_START] == SDL_SCANCODE_RETURN, "Start mapped to Return");
    SDLTest_AssertCheck(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_UP] == SDL_SCANCODE_UP, "Up mapped to Up");

    SDL_Libretro_Destroy(lr);
    SDL_Libretro_Destroy(NULL);

    return TEST_COMPLETED;
}

static int SDLCALL test_StateQueries(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();
    int w = -1, h = -1;

    SDLTest_AssertCheck(SDL_Libretro_IsCoreReady(lr) == false, "IsCoreReady false on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == false, "IsGameReady false on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_IsShutdown(lr) == false, "ShouldClose false on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_GetTexture(lr) == NULL, "GetTexture NULL on fresh context");
    SDL_Libretro_GetSize(lr, &w, &h);
    SDLTest_AssertCheck(w == 0 && h == 0, "GetSize returns 0x0, got %dx%d", w, h);
    SDLTest_AssertCheck(SDL_Libretro_GetAspectRatio(lr) == 0.0f, "GetAspectRatio 0.0 on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_GetFPS(lr) == 0.0, "GetFPS 0.0 on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_GetRotation(lr) == 0, "GetRotation 0 on fresh context");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetCoreName(lr), "") == 0, "GetCoreName empty on fresh context");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetCoreVersion(lr), "") == 0, "GetCoreVersion empty on fresh context");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetValidExtensions(lr), "") == 0, "GetValidExtensions empty on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_GetStateSize(lr) == 0, "GetStateSize 0 on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionCount(lr) == 0, "GetOptionCount 0 on fresh context");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

static int SDLCALL test_NullSafety(void *arg) {
    int w = -1, h = -1;

    SDLTest_AssertCheck(SDL_Libretro_IsCoreReady(NULL) == false, "IsCoreReady(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(NULL) == false, "IsGameReady(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_IsShutdown(NULL) == false, "ShouldClose(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_GetTexture(NULL) == NULL, "GetTexture(NULL) NULL");
    SDL_Libretro_GetSize(NULL, &w, &h);
    SDLTest_AssertCheck(w == 0 && h == 0, "GetSize(NULL) returns 0x0, got %dx%d", w, h);
    SDLTest_AssertCheck(SDL_Libretro_GetAspectRatio(NULL) == 0.0f, "GetAspectRatio(NULL) 0.0");
    SDLTest_AssertCheck(SDL_Libretro_GetFPS(NULL) == 0.0, "GetFPS(NULL) 0.0");
    SDLTest_AssertCheck(SDL_Libretro_GetRotation(NULL) == 0, "GetRotation(NULL) 0");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetCoreName(NULL), "") == 0, "GetCoreName(NULL) empty");
    SDLTest_AssertCheck(SDL_Libretro_GetVolume(NULL) == 0.0f, "GetVolume(NULL) 0.0");
    SDLTest_AssertCheck(SDL_Libretro_GetSpeed(NULL) == 1.0f, "GetSpeed(NULL) 1.0");
    SDLTest_AssertCheck(SDL_Libretro_GetStateSize(NULL) == 0, "GetStateSize(NULL) 0");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionCount(NULL) == 0, "GetOptionCount(NULL) 0");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionKey(NULL, 0) == NULL, "GetOptionKey(NULL) NULL");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionValue(NULL, "foo") == NULL, "GetOptionValue(NULL) NULL");

    return TEST_COMPLETED;
}

static int SDLCALL test_DirectorySetters(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    SDLTest_AssertCheck(SDL_Libretro_SetCoreDirectory(lr, "/tmp/cores") == true, "SetCoreDirectory returns true");
    SDLTest_AssertCheck(SDL_strcmp(lr->coreDirectory, "/tmp/cores") == 0, "CoreDirectory stored correctly");
    SDLTest_AssertCheck(SDL_Libretro_SetSaveDirectory(lr, "/tmp/saves") == true, "SetSaveDirectory returns true");
    SDLTest_AssertCheck(SDL_strcmp(lr->saveDirectory, "/tmp/saves") == 0, "SaveDirectory stored correctly");
    SDLTest_AssertCheck(SDL_Libretro_SetSystemDirectory(lr, "/tmp/system") == true, "SetSystemDirectory returns true");
    SDLTest_AssertCheck(SDL_strcmp(lr->systemDirectory, "/tmp/system") == 0, "SystemDirectory stored correctly");
    SDLTest_AssertCheck(SDL_Libretro_SetCoreAssetsDirectory(lr, "/tmp/assets") == true, "SetCoreAssetsDirectory returns true");
    SDLTest_AssertCheck(SDL_strcmp(lr->coreAssetsDirectory, "/tmp/assets") == 0, "CoreAssetsDirectory stored correctly");
    SDLTest_AssertCheck(SDL_Libretro_SetUsername(lr, "TestUser") == true, "SetUsername returns true");
    SDLTest_AssertCheck(SDL_strcmp(lr->username, "TestUser") == 0, "Username stored correctly");

    SDLTest_AssertCheck(SDL_Libretro_SetCoreDirectory(NULL, "/tmp") == false, "SetCoreDirectory(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_SetSaveDirectory(NULL, "/tmp") == false, "SetSaveDirectory(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_SetSystemDirectory(NULL, "/tmp") == false, "SetSystemDirectory(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_SetCoreAssetsDirectory(NULL, "/tmp") == false, "SetCoreAssetsDirectory(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_SetUsername(NULL, "x") == false, "SetUsername(NULL) false");

    SDLTest_AssertCheck(SDL_Libretro_SetCoreDirectory(lr, NULL) == true, "SetCoreDirectory(NULL path) true");
    SDLTest_AssertCheck(SDL_strcmp(lr->coreDirectory, "") == 0, "NULL path stores empty string");
    SDLTest_AssertCheck(SDL_Libretro_SetUsername(lr, NULL) == true, "SetUsername(NULL path) true");
    SDLTest_AssertCheck(SDL_strcmp(lr->username, "") == 0, "NULL username stores empty string");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

static int SDLCALL test_VolumeSpeed(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    SDLTest_AssertCheck(SDL_Libretro_GetVolume(lr) == 1.0f, "Default volume is 1.0");
    SDL_Libretro_SetVolume(lr, 0.5f);
    SDLTest_AssertCheck(SDL_Libretro_GetVolume(lr) == 0.5f, "Volume set to 0.5");
    SDL_Libretro_SetVolume(lr, -1.0f);
    SDLTest_AssertCheck(SDL_Libretro_GetVolume(lr) == 0.0f, "Volume clamped to 0.0 from -1.0");
    SDL_Libretro_SetVolume(lr, 2.0f);
    SDLTest_AssertCheck(SDL_Libretro_GetVolume(lr) == 1.0f, "Volume clamped to 1.0 from 2.0");
    SDL_Libretro_SetVolume(NULL, 0.5f);

    SDLTest_AssertCheck(SDL_Libretro_GetSpeed(lr) == 1.0f, "Default speed is 1.0");
    SDL_Libretro_SetSpeed(lr, 2.0f);
    SDLTest_AssertCheck(SDL_Libretro_GetSpeed(lr) == 2.0f, "Speed set to 2.0");
    SDL_Libretro_SetSpeed(lr, 0.0f);
    SDLTest_AssertCheck(SDL_Libretro_GetSpeed(lr) == 0.0f, "Speed 0.0 accepted (paused)");
    SDL_Libretro_SetSpeed(lr, -5.0f);
    SDLTest_AssertCheck(SDL_Libretro_GetSpeed(lr) == 0.0f, "Speed clamped to 0.0 from -5.0");
    SDL_Libretro_SetSpeed(lr, 10.0f);
    SDLTest_AssertCheck(SDL_Libretro_GetSpeed(lr) == 10.0f, "Speed set to 10.0 (no upper clamp)");
    SDL_Libretro_SetSpeed(NULL, 2.0f);

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

static int SDLCALL test_Input(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    SDLTest_AssertCheck(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_X, "Default A is X");
    SDL_Libretro_SetKeyboardMapping(lr, RETRO_DEVICE_ID_JOYPAD_A, SDL_SCANCODE_SPACE);
    SDLTest_AssertCheck(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_SPACE, "A remapped to Space");
    SDL_Libretro_SetKeyboardMapping(lr, -1, SDL_SCANCODE_Z);
    SDLTest_AssertCheck(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_SPACE, "Negative button is no-op");
    SDL_Libretro_SetKeyboardMapping(NULL, 0, SDL_SCANCODE_Z);

    SDL_Libretro_SetVirtualButton(lr, 0, RETRO_DEVICE_ID_JOYPAD_A, true);
    SDLTest_AssertCheck(lr->core.virtualJoypadState[RETRO_DEVICE_ID_JOYPAD_A] == true, "Virtual button A pressed");
    SDL_Libretro_SetVirtualButton(lr, 0, RETRO_DEVICE_ID_JOYPAD_A, false);
    SDLTest_AssertCheck(lr->core.virtualJoypadState[RETRO_DEVICE_ID_JOYPAD_A] == false, "Virtual button A released");
    SDL_Libretro_SetVirtualButton(lr, 16, 0, true);
    SDL_Libretro_SetVirtualButton(lr, 0, 16, true);
    SDL_Libretro_SetVirtualButton(NULL, 0, 0, true);

    SDLTest_AssertCheck(SDL_Libretro_GetPortDevice(lr, 0) == RETRO_DEVICE_NONE, "Port 0 device defaults to NONE");
    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(lr, 0, RETRO_DEVICE_JOYPAD) == true, "SetPortDevice port 0 true");
    SDLTest_AssertCheck(SDL_Libretro_GetPortDevice(lr, 0) == RETRO_DEVICE_JOYPAD, "GetPortDevice returns stored device");
    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(lr, 15, RETRO_DEVICE_JOYPAD) == true, "SetPortDevice port 15 true");
    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(lr, 16, RETRO_DEVICE_JOYPAD) == false, "SetPortDevice port 16 false");
    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(NULL, 0, RETRO_DEVICE_JOYPAD) == false, "SetPortDevice(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_GetPortDevice(lr, 16) == RETRO_DEVICE_NONE, "GetPortDevice out-of-range NONE");
    SDLTest_AssertCheck(SDL_Libretro_GetPortDevice(NULL, 0) == RETRO_DEVICE_NONE, "GetPortDevice(NULL) NONE");

    // Input descriptors (empty without a loaded core)
    SDLTest_AssertCheck(SDL_Libretro_GetInputDescriptorCount(lr) == 0, "Descriptor count 0 without core");
    SDLTest_AssertCheck(SDL_Libretro_GetInputDescriptor(lr, 0, NULL, NULL, NULL, NULL) == false, "GetInputDescriptor false without core");
    SDLTest_AssertCheck(SDL_Libretro_GetInputDescriptorCount(NULL) == 0, "GetInputDescriptorCount(NULL) 0");
    SDLTest_AssertCheck(SDL_Libretro_GetInputDescriptor(NULL, 0, NULL, NULL, NULL, NULL) == false, "GetInputDescriptor(NULL) false");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

static int SDLCALL test_Options(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    SDLTest_AssertCheck(SDL_Libretro_GetOptionCount(lr) == 0, "Option count 0 on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionKey(lr, 0) == NULL, "GetOptionKey(0) NULL on empty");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionValue(lr, "foo") == NULL, "GetOptionValue NULL on empty");
    SDLTest_AssertCheck(SDL_Libretro_AreOptionsDirty(lr) == false, "Options not dirty on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "key", "val") == false, "SetOptionValue false on empty");
    SDLTest_AssertCheck(SDL_Libretro_ResetOption(lr, "key") == false, "ResetOption false on empty");
    SDL_Libretro_ResetAllOptions(lr);

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

static int SDLCALL test_Rewind(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    // Memory budget: a fresh context carries the default, and an explicit 0
    // (unbounded) must survive enabling rather than being reset to the default.
    SDLTest_AssertCheck(SDL_Libretro_GetRewindMemoryLimit(lr) > 0, "Fresh context has a default rewind budget");
    SDL_Libretro_SetRewindMemoryLimit(lr, 0);
    SDLTest_AssertCheck(SDL_Libretro_SetRewindEnabled(lr, true, 600, 2) == true, "Enable rewind (unbounded budget)");
    SDLTest_AssertCheck(SDL_Libretro_GetRewindMemoryLimit(lr) == 0, "Explicit unbounded budget survives enabling");
    SDL_Libretro_SetRewindEnabled(lr, false, 0, 0);

    SDLTest_AssertCheck(SDL_Libretro_SetRewindEnabled(lr, true, 600, 2) == true, "Enable rewind before core load");
    SDLTest_AssertCheck(lr->rewindEnabled == true, "Rewind enabled flag set");
    SDLTest_AssertCheck(lr->rewindCapacity == 600, "Rewind capacity stored");
    SDLTest_AssertCheck(lr->rewindCaptureInterval == 2, "Rewind capture interval stored");
    SDLTest_AssertCheck(lr->rewindReference == NULL, "Buffer not allocated without core");

    SDLTest_AssertCheck(SDL_Libretro_SetRewindEnabled(lr, false, 0, 0) == true, "Disable rewind");
    SDLTest_AssertCheck(lr->rewindEnabled == false, "Rewind disabled");

    SDLTest_AssertCheck(SDL_Libretro_SetRewindEnabled(NULL, true, 100, 1) == false, "SetRewindEnabled(NULL) false");

    // Negative speed only accepted when rewind is enabled.
    SDL_Libretro_SetSpeed(lr, -1.0f);
    SDLTest_AssertCheck(lr->speed == 0.0f, "Negative speed clamped without rewind");
    SDL_Libretro_SetRewindEnabled(lr, true, 100, 1);
    SDL_Libretro_SetSpeed(lr, -1.0f);
    SDLTest_AssertCheck(lr->speed == -1.0f, "Negative speed accepted with rewind");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

// Rewind

/**
 * Minimal stub core: serialize/unserialize just copy a small "RAM"
 * buffer, so the rewind capture/step path can run without loading
 * a real core.
 */
static unsigned char g_rewindState[32];

static void rewind_stub_run(void) {
    // Nothing.
}

static size_t rewind_stub_size(void) {
    return sizeof(g_rewindState);
}

static bool rewind_stub_serialize(void* data, size_t size) {
    if (size < sizeof(g_rewindState)) return false;
    SDL_memcpy(data, g_rewindState, sizeof(g_rewindState));
    return true;
}

static bool rewind_stub_unserialize(const void* data, size_t size) {
    if (size < sizeof(g_rewindState)) return false;
    SDL_memcpy(g_rewindState, data, sizeof(g_rewindState));
    return true;
}

static int SDLCALL test_RewindBuffer(void *arg) {
#ifdef SDL_LIBRETRO_ENABLE_REWIND_DELTA
    // Codec: encoding then decoding a delta reconstructs the older state.
    // (Full-state mode has no codec; the end-to-end checks below cover it.)
    unsigned char older[32], newer[32], work[32], enc[64];
    SDL_memset(older, 0xA5, sizeof(older));
    SDL_memcpy(newer, older, sizeof(newer));
    newer[0] = 0x00; newer[31] = 0xFF; // differ at both ends
    size_t encSize = SDL_Libretro_RewindEncodeDelta(newer, older, sizeof(newer), enc, sizeof(enc));
    SDL_memcpy(work, newer, sizeof(work));
    bool decoded = SDL_Libretro_RewindDecodeDelta(enc, encSize, work, sizeof(work));
    SDLTest_AssertCheck(decoded && SDL_memcmp(work, older, sizeof(work)) == 0,
        "Codec delta round-trip reconstructs the previous state");

    // Worst case: every byte differs over a buffer that spans several 128-byte
    // literal chunks. SDL_Libretro_RewindMaxEncodedSize must bound the output so
    // the capture path's single-pass encode never overflows its scratch (a
    // too-small buffer makes the encoder return 0 and silently drop the frame).
    {
        unsigned char wOld[300], wNew[300], wWork[300];
        SDL_memset(wOld, 0x00, sizeof(wOld));
        SDL_memset(wNew, 0xFF, sizeof(wNew)); // all bytes differ
        size_t cap = SDL_Libretro_RewindMaxEncodedSize(sizeof(wNew));
        unsigned char* wEnc = (unsigned char*)SDL_malloc(cap);
        size_t wEncSize = SDL_Libretro_RewindEncodeDelta(wNew, wOld, sizeof(wNew), wEnc, cap);
        SDLTest_AssertCheck(wEncSize > 0, "Worst-case delta fits the bounded scratch (got %zu of %zu)", wEncSize, cap);
        SDL_memcpy(wWork, wNew, sizeof(wWork));
        bool wDecoded = SDL_Libretro_RewindDecodeDelta(wEnc, wEncSize, wWork, sizeof(wWork));
        SDLTest_AssertCheck(wDecoded && SDL_memcmp(wWork, wOld, sizeof(wWork)) == 0,
            "Worst-case delta round-trips after a single-pass encode");
        SDL_free(wEnc);
    }
#endif /* SDL_LIBRETRO_ENABLE_REWIND_DELTA */

    // End-to-end: capture a few states through a stub core, then rewind.
    SDL_Libretro* lr = SDL_Libretro_Create();
    lr->core.loaded = true;
    lr->core.gameLoaded = true;
    lr->core.fps = 60.0;
    lr->core.symbols.retro_run = rewind_stub_run;
    lr->core.symbols.retro_serialize_size = rewind_stub_size;
    lr->core.symbols.retro_serialize = rewind_stub_serialize;
    lr->core.symbols.retro_unserialize = rewind_stub_unserialize;

    SDLTest_AssertCheck(SDL_Libretro_SetRewindEnabled(lr, true, 16, 1) == true, "Enable rewind with stub core");

    // State k is all-bytes==k. First capture only seeds the reference.
    for (int k = 0; k < 4; k++) {
        SDL_memset(g_rewindState, (unsigned char)k, sizeof(g_rewindState));
        SDL_Libretro_RewindCapture(lr);
    }
    SDLTest_AssertCheck(lr->rewindCount == 3, "Three deltas captured");

    SDL_Libretro_RewindStep(lr);
    SDLTest_AssertCheck(g_rewindState[0] == 2, "Step back restores the previous state");
    SDL_Libretro_RewindStep(lr);
    SDLTest_AssertCheck(g_rewindState[0] == 1, "Step back again restores the earlier state");

    SDL_Libretro_ClearRewind(lr);
    SDLTest_AssertCheck(lr->rewindCount == 0, "ClearRewind empties the buffer");

    SDL_Libretro_SetRewindEnabled(lr, false, 0, 0);
    lr->core.loaded = false; /* let Destroy skip the unset core teardown symbols */
    lr->core.gameLoaded = false;
    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

static int SDLCALL test_OptionVisibility(void *arg) {
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // test_core registers test_option_a (visible) and test_option_b (hidden)
    SDLTest_AssertCheck(SDL_Libretro_GetOptionCount(lr) == 2, "Two options registered by test_core");
    SDLTest_AssertCheck(SDL_Libretro_IsOptionVisible(lr, "test_option_a") == true, "Option A visible");
    SDLTest_AssertCheck(SDL_Libretro_IsOptionVisible(lr, "test_option_b") == false, "Option B hidden via SET_CORE_OPTIONS_DISPLAY");
    SDLTest_AssertCheck(SDL_Libretro_AreOptionsDirty(lr) == true, "optionsDirty set after display change");
    SDLTest_AssertCheck(SDL_Libretro_IsOptionVisible(lr, NULL) == false, "IsOptionVisible(NULL key) false");
    SDLTest_AssertCheck(SDL_Libretro_IsOptionVisible(NULL, "test_option_a") == false, "IsOptionVisible(NULL lr) false");

    // test_core registers one category (test_category) with both options attached.
    SDLTest_AssertCheck(SDL_Libretro_GetCategoryCount(lr) == 1, "One category registered by test_core");
    const char* catKey = SDL_Libretro_GetCategoryKey(lr, 0);
    SDLTest_AssertCheck(catKey && SDL_strcmp(catKey, "test_category") == 0, "Category key is test_category");
    const char* catLabel = SDL_Libretro_GetCategoryLabel(lr, "test_category");
    SDLTest_AssertCheck(catLabel && SDL_strcmp(catLabel, "Test Category") == 0, "Category label is Test Category");
    const char* catInfo = SDL_Libretro_GetCategoryInfo(lr, "test_category");
    SDLTest_AssertCheck(catInfo && SDL_strcmp(catInfo, "Category for test options") == 0, "Category info matches");
    SDLTest_AssertCheck(SDL_Libretro_GetCategoryCount(NULL) == 0, "GetOptionCategoryCount(NULL) 0");
    SDLTest_AssertCheck(SDL_Libretro_GetCategoryKey(lr, 5) == NULL, "GetOptionCategoryKey out-of-range NULL");
    SDLTest_AssertCheck(SDL_Libretro_GetCategoryLabel(lr, "nope") == NULL, "Unknown category label NULL");
    SDLTest_AssertCheck(SDL_Libretro_GetCategoryInfo(lr, NULL) == NULL, "GetOptionCategoryInfo(NULL key) NULL");

    // Option metadata accessors (test_option_a: label "Option A", info "First test option").
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetOptionLabel(lr, "test_option_a"), "Option A") == 0, "Option A label");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetOptionInfo(lr, "test_option_a"), "First test option") == 0, "Option A info");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetOptionCategory(lr, "test_option_a"), "test_category") == 0, "Option A category");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionLabel(lr, "nope") == NULL, "Unknown option label NULL");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionCategory(NULL, "test_option_a") == NULL, "GetOptionCategory(NULL lr) NULL");

    // Value enumeration (test_option_a has values on|off, no per-value labels).
    SDLTest_AssertCheck(SDL_Libretro_GetOptionValueCount(lr, "test_option_a") == 2, "Option A has 2 values");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetOptionValueByIndex(lr, "test_option_a", 0), "on") == 0, "Value 0 is on");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetOptionValueByIndex(lr, "test_option_a", 1), "off") == 0, "Value 1 is off");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionValueByIndex(lr, "test_option_a", 2) == NULL, "Value index out of range NULL");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetOptionValueLabelByIndex(lr, "test_option_a", 0), "on") == 0, "Value 0 label falls back to value");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionValueCount(lr, "nope") == 0, "Unknown option value count 0");

    // SetOptionValue validation against declared values.
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "test_option_a", "off") == true, "Set valid value succeeds");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetOptionValue(lr, "test_option_a"), "off") == 0, "Value updated to off");
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "test_option_a", "bogus") == false, "Set invalid value rejected");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetOptionValue(lr, "test_option_a"), "off") == 0, "Value unchanged after rejected set");

    // ResetAllOptions restores defaults and marks options dirty.
    SDL_Libretro_AreOptionsDirty(lr); // clear the dirty flag set by SetOptionValue
    SDL_Libretro_ResetAllOptions(lr);
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetOptionValue(lr, "test_option_a"), "on") == 0, "ResetAllOptions restores default");
    SDLTest_AssertCheck(SDL_Libretro_AreOptionsDirty(lr) == true, "ResetAllOptions marks options dirty");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_LoadCore(void *arg) {
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_Libretro* lr = SDL_Libretro_Create();

    SDLTest_AssertCheck(SDL_Libretro_LoadCore(lr, "nonexistent.so") == false, "LoadCore fails for missing file");
    SDLTest_AssertCheck(SDL_Libretro_IsCoreReady(lr) == false, "Not ready after failed load");

    SDLTest_AssertCheck(SDL_Libretro_LoadCore(lr, TEST_CORE_PATH) == true, "LoadCore succeeds for test_core");
    SDLTest_AssertCheck(SDL_Libretro_IsCoreReady(lr) == true, "Core is ready after load");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetCoreName(lr), "test_core") == 0, "Core name is test_core");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetCoreVersion(lr), "1.0") == 0, "Core version is 1.0");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetValidExtensions(lr), "txt") == 0, "Valid extensions is txt");
    SDLTest_AssertCheck(SDL_Libretro_GetPerformanceLevel(lr) == 2, "Performance level reported by core is 2");
    SDLTest_AssertCheck(SDL_Libretro_GetPerformanceLevel(NULL) == 0, "GetPerformanceLevel(NULL) 0");

    // With a core loaded but no game, game-state operations no-op safely.
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == false, "Game not ready with core but no game");
    SDLTest_AssertCheck(SDL_Libretro_GetStateSize(lr) == 0, "GetStateSize 0 without a game");
    SDLTest_AssertCheck(SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, NULL) == NULL, "GetMemoryData NULL without a game");
    SDLTest_AssertCheck(SDL_Libretro_GetMemoryMapCount(lr) == 0, "No memory map without a game");
    SDLTest_AssertCheck(SDL_Libretro_Reset(lr) == false, "Reset fails without a game");
    SDLTest_AssertCheck(SDL_Libretro_SaveState(lr, "should_not_exist.sav") == false, "SaveState fails without a game");

    // The LoadCore content-name default makes GetSavePath usable before a game loads.
    char buf[256];
    SDLTest_AssertCheck(SDL_Libretro_GetSavePath(lr, ".srm", buf, sizeof(buf)) > 0, "GetSavePath uses core name before a game loads");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_LoadGame(void *arg) {
#if !defined(TEST_CORE_PATH) || !defined(TEST_CONTENT_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH or TEST_CONTENT_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH, renderer) == true, "LoadGame succeeds with test content");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game is ready after load");

    int w = 0, h = 0;
    SDL_Libretro_GetSize(lr, &w, &h);
    SDLTest_AssertCheck(w == 320 && h == 240, "Core reports 320x240, got %dx%d", w, h);
    SDLTest_AssertCheck(SDL_Libretro_GetFPS(lr) == 60.0, "FPS is 60.0");
    SDLTest_AssertCheck(SDL_Libretro_GetTexture(lr) != NULL, "Texture created");
    SDLTest_AssertCheck(SDL_Libretro_GetStateSize(lr) == 128, "Serialize size is 128");

    SDL_Libretro_RunFrame(lr);
    SDL_Libretro_RunFrame(lr);

    SDLTest_AssertCheck(SDL_Libretro_SaveState(lr, "test_state.sav") == true, "SaveState succeeds");
    SDLTest_AssertCheck(SDL_Libretro_LoadState(lr, "test_state.sav") == true, "LoadState succeeds");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_GameInfoExt(void *arg) {
#if !defined(TEST_CORE_PATH) || !defined(TEST_CONTENT_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH or TEST_CONTENT_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH, renderer);

    // The test core probes GET_GAME_INFO_EXT during load and exposes the result
    // via SYSTEM_RAM, verifying the frontend populated the extended game info.
    size_t sz = 0;
    const Uint8* probe = (const Uint8*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SYSTEM_RAM, &sz);
    SDLTest_AssertCheck(probe != NULL && sz >= 18, "GET_GAME_INFO_EXT probe region available (size %zu)", sz);
    if (probe) {
        SDLTest_AssertCheck(SDL_strcmp((const char*)probe, "txt") == 0,
            "GET_GAME_INFO_EXT reports lower-case ext 'txt', got '%s'", (const char*)probe);
        SDLTest_AssertCheck(probe[16] == 1, "content-info override set persistent_data");
        SDLTest_AssertCheck(probe[17] == 1, "content data buffer present (need_fullpath false)");
    }

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_LoadGameNoContent(void *arg) {
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // The test core opts into no-content, so a NULL load runs.
    SDLTest_AssertCheck(SDL_Libretro_IsGameRequired(lr) == false, "Core does not require content");
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, NULL, renderer) == true, "LoadGame succeeds with NULL content");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready with no content");

    SDL_Libretro_RunFrame(lr);
    SDL_Libretro_UnloadGame(lr);

    // A core that requires content must reject a NULL load (and not disturb state).
    lr->core.supportNoGame = false;
    SDLTest_AssertCheck(SDL_Libretro_IsGameRequired(lr) == true, "Core now reports content required");
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, NULL, renderer) == false, "LoadGame(NULL) rejected when content is required");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == false, "Game not ready after a rejected NULL load");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_LoadGameFailure(void *arg) {
#if !defined(TEST_CORE_PATH) || !defined(TEST_CONTENT_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH or TEST_CONTENT_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // A missing ".txt" file fails the load (the core wants the data buffer).
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, "definitely_missing.txt", renderer) == false,
        "LoadGame fails for a missing content file");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == false, "Game not ready after a failed load");

    // The partial content identity is reset: extension cleared, and the content
    // name falls back to the core-name default so GetSavePath still works.
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetContentExtension(lr), "") == 0,
        "Content extension cleared after a failed load");
    char path[256];
    SDLTest_AssertCheck(SDL_Libretro_GetSavePath(lr, ".srm", path, sizeof(path)) > 0,
        "GetSavePath still works (core-name default) after a failed load");
    SDLTest_AssertCheck(SDL_strstr(path, "test_core") != NULL,
        "Save path falls back to the core name, not the failed content: %s", path);

    // The context still works: a real load afterward succeeds.
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH, renderer) == true,
        "LoadGame succeeds after a prior failure");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_Memory(void *arg) {
#if !defined(TEST_CORE_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    // NULL / no-core safety.
    SDLTest_AssertCheck(SDL_Libretro_GetMemoryData(NULL, RETRO_MEMORY_SAVE_RAM, NULL) == NULL, "GetMemoryData(NULL) NULL");
    SDLTest_AssertCheck(SDL_Libretro_SetMemoryData(NULL, RETRO_MEMORY_SAVE_RAM, "x", 1) == false, "SetMemoryData(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_GetMemoryMapCount(NULL) == 0, "GetMemoryMapCount(NULL) 0");
    SDLTest_AssertCheck(SDL_Libretro_GetMemoryMapDescriptor(NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == false, "GetMemoryMapDescriptor(NULL) false");

    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDL_Libretro_LoadGame(lr, NULL, renderer);

    // GetMemoryData: SAVE_RAM is exposed at 64 bytes; an unsupported type is not.
    size_t sz = 123;
    void* mem = SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, &sz);
    SDLTest_AssertCheck(mem != NULL, "GetMemoryData(SAVE_RAM) non-NULL");
    SDLTest_AssertCheck(sz == 64, "GetMemoryData(SAVE_RAM) size 64, got %zu", sz);
    sz = 123;
    SDLTest_AssertCheck(SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_VIDEO_RAM, &sz) == NULL, "GetMemoryData(VIDEO_RAM) NULL");
    SDLTest_AssertCheck(sz == 0, "GetMemoryData(VIDEO_RAM) size 0");

    // SetMemoryData writes through to the live buffer; oversized input clamps.
    unsigned char pattern[64];
    for (int i = 0; i < 64; i++) pattern[i] = (unsigned char)(i + 1);
    SDLTest_AssertCheck(SDL_Libretro_SetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, pattern, 64) == true, "SetMemoryData(SAVE_RAM) true");
    SDLTest_AssertCheck(SDL_memcmp(mem, pattern, 64) == 0, "SetMemoryData wrote through to live buffer");
    unsigned char big[100];
    SDL_memset(big, 0x55, sizeof(big));
    SDLTest_AssertCheck(SDL_Libretro_SetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, big, sizeof(big)) == true, "SetMemoryData clamps oversized input");
    SDLTest_AssertCheck(((unsigned char*)mem)[63] == 0x55, "SetMemoryData wrote up to capacity");
    SDLTest_AssertCheck(SDL_Libretro_SetMemoryData(lr, RETRO_MEMORY_VIDEO_RAM, pattern, 64) == false, "SetMemoryData(VIDEO_RAM) false");
    SDLTest_AssertCheck(SDL_Libretro_SetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, NULL, 1) == false, "SetMemoryData(NULL data) false");

    // Save/Load round-trip: persist pattern, scribble over it, restore.
    SDL_Libretro_SetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, pattern, 64);
    SDLTest_AssertCheck(SDL_Libretro_SaveMemory(lr, RETRO_MEMORY_SAVE_RAM, "test_mem.sav") == true, "SaveMemory(SAVE_RAM) true");
    SDL_memset(mem, 0, 64);
    SDLTest_AssertCheck(SDL_Libretro_LoadMemory(lr, RETRO_MEMORY_SAVE_RAM, "test_mem.sav") == true, "LoadMemory(SAVE_RAM) true");
    SDLTest_AssertCheck(SDL_memcmp(mem, pattern, 64) == 0, "LoadMemory restores saved bytes");

    // Unavailable type: save is a no-op success, load fails.
    SDLTest_AssertCheck(SDL_Libretro_SaveMemory(lr, RETRO_MEMORY_VIDEO_RAM, "test_vram.sav") == true, "SaveMemory(VIDEO_RAM) true (nothing to save)");
    SDLTest_AssertCheck(SDL_Libretro_LoadMemory(lr, RETRO_MEMORY_VIDEO_RAM, "test_mem.sav") == false, "LoadMemory(VIDEO_RAM) false");

    // SRAM wrappers route to the same path.
    SDLTest_AssertCheck(SDL_Libretro_SaveSRAM(lr, "test_sram.sav") == true, "SaveSRAM true");
    SDL_memset(mem, 0, 64);
    SDLTest_AssertCheck(SDL_Libretro_LoadSRAM(lr, "test_sram.sav") == true, "LoadSRAM true");
    SDLTest_AssertCheck(SDL_memcmp(mem, pattern, 64) == 0, "LoadSRAM restores saved bytes");

    // Memory map descriptors.
    SDLTest_AssertCheck(SDL_Libretro_GetMemoryMapCount(lr) == 1, "GetMemoryMapCount 1");
    Uint64 flags = 0; void* ptr = NULL; size_t off = 9, start = 9, sel = 9, dis = 9, len = 0; const char* as = NULL;
    SDLTest_AssertCheck(SDL_Libretro_GetMemoryMapDescriptor(lr, 0, &flags, &ptr, &off, &start, &sel, &dis, &len, &as) == true, "GetMemoryMapDescriptor(0) true");
    SDLTest_AssertCheck(ptr == mem, "Descriptor ptr matches SAVE_RAM region");
    SDLTest_AssertCheck(len == 64, "Descriptor len 64, got %zu", len);
    SDLTest_AssertCheck(as != NULL && SDL_strcmp(as, "WRAM") == 0, "Descriptor addrspace is WRAM");
    SDLTest_AssertCheck(SDL_Libretro_GetMemoryMapDescriptor(lr, 1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == false, "GetMemoryMapDescriptor out-of-range false");

    // Guest -> host address translation through the map.
    size_t rem = 0;
    SDLTest_AssertCheck(SDL_Libretro_GetMapAddress(lr, 0, &rem) == mem, "MapAddress(0) resolves to the SAVE_RAM base");
    SDLTest_AssertCheck(rem == 64, "MapAddress remaining 64 at base, got %zu", rem);
    SDLTest_AssertCheck(SDL_Libretro_GetMapAddress(lr, 10, &rem) == (void*)((Uint8*)mem + 10), "MapAddress(10) offsets into the region");
    SDLTest_AssertCheck(rem == 54, "MapAddress remaining 54 at offset 10, got %zu", rem);
    SDLTest_AssertCheck(SDL_Libretro_GetMapAddress(lr, 64, NULL) == NULL, "MapAddress past the region is NULL");
    SDLTest_AssertCheck(SDL_Libretro_GetMapAddress(NULL, 0, NULL) == NULL, "MapAddress(NULL) NULL");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    SDL_RemovePath("test_mem.sav");
    SDL_RemovePath("test_sram.sav");
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_SavePath(void *arg) {
#if !defined(TEST_CORE_PATH) || !defined(TEST_CONTENT_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH or TEST_CONTENT_PATH not defined");
    return TEST_COMPLETED;
#else
    char path[256];

    // GetSavePath edge cases (no content, no context).
    SDLTest_AssertCheck(SDL_Libretro_GetSavePath(NULL, ".srm", path, sizeof(path)) == 0, "GetSavePath(NULL) 0");
    SDL_Libretro* fresh = SDL_Libretro_Create();
    SDLTest_AssertCheck(SDL_Libretro_GetSavePath(fresh, ".srm", path, sizeof(path)) == 0, "GetSavePath with no content 0");
    SDL_Libretro_Destroy(fresh);

    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetSaveDirectory(lr, "test_saves");
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH, renderer);

    SDLTest_AssertCheck(SDL_Libretro_GetSavePath(lr, ".srm", path, sizeof(path)) > 0, "GetSavePath derives a path with content");
    SDLTest_AssertCheck(SDL_strstr(path, "test_saves") != NULL && SDL_strstr(path, ".srm") != NULL, "Derived path uses save dir and extension: %s", path);

    // LoadGame again should unload the existing game and reload cleanly.
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH, renderer) == true, "LoadGame again succeeds (reloads over existing game)");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready after reload");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_LogLevel(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    SDLTest_AssertCheck(SDL_Libretro_GetLogLevel(lr) == RETRO_LOG_DEBUG, "Default log level is DEBUG");
    SDL_Libretro_SetLogLevel(lr, RETRO_LOG_WARN);
    SDLTest_AssertCheck(SDL_Libretro_GetLogLevel(lr) == RETRO_LOG_WARN, "Log level set to WARN");
    SDL_Libretro_SetLogLevel(lr, RETRO_LOG_ERROR);
    SDLTest_AssertCheck(SDL_Libretro_GetLogLevel(lr) == RETRO_LOG_ERROR, "Log level set to ERROR");
    SDL_Libretro_SetLogLevel(lr, -1);
    SDLTest_AssertCheck(SDL_Libretro_GetLogLevel(lr) == RETRO_LOG_DEBUG, "Negative level clamped to DEBUG");
    SDL_Libretro_SetLogLevel(lr, 99);
    SDLTest_AssertCheck(SDL_Libretro_GetLogLevel(lr) == RETRO_LOG_ERROR, "Overflow level clamped to ERROR");
    SDLTest_AssertCheck(SDL_Libretro_GetLogLevel(NULL) == RETRO_LOG_DEBUG, "GetLogLevel(NULL) returns DEBUG");
    SDL_Libretro_SetLogLevel(NULL, RETRO_LOG_WARN);

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

static int SDLCALL test_ContentExtension(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetContentExtension(lr), "") == 0,
        "GetContentExtension empty without content");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetContentExtension(NULL), "") == 0,
        "GetContentExtension(NULL) empty");

    SDL_strlcpy(lr->core.contentPath, "/path/to/game.sfc", sizeof(lr->core.contentPath));
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetContentExtension(lr), "sfc") == 0,
        "GetContentExtension returns 'sfc' for game.sfc");

    SDL_strlcpy(lr->core.contentPath, "/path/to/rom.nes", sizeof(lr->core.contentPath));
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetContentExtension(lr), "nes") == 0,
        "GetContentExtension returns 'nes' for rom.nes");

    SDL_strlcpy(lr->core.contentPath, "/path/to/noext", sizeof(lr->core.contentPath));
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetContentExtension(lr), "") == 0,
        "GetContentExtension empty for file without extension");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

static int SDLCALL test_ExtensionInList(void *arg) {
    SDLTest_AssertCheck(SDL_Libretro_ExtensionInList("sfc", "sfc|smc|bs") == true,
        "sfc found at start of list");
    SDLTest_AssertCheck(SDL_Libretro_ExtensionInList("smc", "sfc|smc|bs") == true,
        "smc found in middle of list");
    SDLTest_AssertCheck(SDL_Libretro_ExtensionInList("bs", "sfc|smc|bs") == true,
        "bs found at end of list");
    SDLTest_AssertCheck(SDL_Libretro_ExtensionInList("gb", "sfc|smc|bs") == false,
        "gb not in list");
    SDLTest_AssertCheck(SDL_Libretro_ExtensionInList("sfc", "sfc") == true,
        "sfc found in single-entry list");
    SDLTest_AssertCheck(SDL_Libretro_ExtensionInList("SFC", "sfc|smc") == true,
        "Case-insensitive match");
    SDLTest_AssertCheck(SDL_Libretro_ExtensionInList(NULL, "sfc") == false,
        "NULL ext returns false");
    SDLTest_AssertCheck(SDL_Libretro_ExtensionInList("sfc", NULL) == false,
        "NULL list returns false");

    return TEST_COMPLETED;
}

/* Test case references. The function name doubles as the test name via #fn,
   and file-scope compound literals let us list the cases inline. */
#define LIBRETRO_TEST_CASE(fn, desc) \
    &(const SDLTest_TestCaseReference){ fn, #fn, desc, TEST_ENABLED }

static const SDLTest_TestCaseReference *testCases[] = {
    LIBRETRO_TEST_CASE(test_CreateDestroy,    "Create context, verify defaults, destroy"),
    LIBRETRO_TEST_CASE(test_StateQueries,     "Query getters on fresh context"),
    LIBRETRO_TEST_CASE(test_NullSafety,       "All getters handle NULL without crashing"),
    LIBRETRO_TEST_CASE(test_DirectorySetters, "Directory and username setters"),
    LIBRETRO_TEST_CASE(test_VolumeSpeed,      "Volume and speed with clamping"),
    LIBRETRO_TEST_CASE(test_Input,            "Keyboard mapping, virtual buttons, port device"),
    LIBRETRO_TEST_CASE(test_Options,          "Core options on empty list"),
    LIBRETRO_TEST_CASE(test_Rewind,           "Rewind buffer setup and speed"),
    LIBRETRO_TEST_CASE(test_RewindBuffer,     "Rewind codec round-trip and capture/step"),
    LIBRETRO_TEST_CASE(test_Memory,           "Memory get/set, save/load, and memory map"),
    LIBRETRO_TEST_CASE(test_SavePath,         "Derived save path and game reload"),
    LIBRETRO_TEST_CASE(test_LogLevel,         "Log level filtering"),
    LIBRETRO_TEST_CASE(test_OptionVisibility, "SET_CORE_OPTIONS_DISPLAY and IsOptionVisible"),
    LIBRETRO_TEST_CASE(test_LoadCore,         "Load test core and verify metadata"),
    LIBRETRO_TEST_CASE(test_LoadGame,         "Load game, run frames, save/load state"),
    LIBRETRO_TEST_CASE(test_GameInfoExt,       "Extended game info via GET_GAME_INFO_EXT"),
    LIBRETRO_TEST_CASE(test_LoadGameNoContent, "Load game with no content file"),
    LIBRETRO_TEST_CASE(test_LoadGameFailure,   "Failed load resets content state cleanly"),
    LIBRETRO_TEST_CASE(test_ContentExtension,  "Content extension extraction"),
    LIBRETRO_TEST_CASE(test_ExtensionInList,   "Extension-in-pipe-list matching"),
    NULL
};

static const SDLTest_TestSuiteReference testSuite = {
    "SDL_libretro",
    NULL, // Setup
    testCases,
    NULL // Teardown
};

int main(int argc, char *argv[]) {
    SDLTest_TestSuiteReference *suites[] = {
        (SDLTest_TestSuiteReference *)&testSuite,
        NULL
    };

    SDLTest_CommonState *state = SDLTest_CommonCreateState(argv, 0);
    SDLTest_TestSuiteRunner *runner = SDLTest_CreateTestSuiteRunner(state, suites);
    int result = SDLTest_ExecuteTestSuiteRunner(runner);
    SDLTest_DestroyTestSuiteRunner(runner);
    SDLTest_CommonDestroyState(state);

    return result;
}
