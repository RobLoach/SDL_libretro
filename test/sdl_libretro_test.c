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
    SDLTest_AssertCheck(SDL_Libretro_ShouldClose(lr) == false, "ShouldClose false on fresh context");
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
    SDLTest_AssertCheck(SDL_Libretro_ShouldClose(NULL) == false, "ShouldClose(NULL) false");
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
    SDLTest_AssertCheck(SDL_Libretro_GetSpeed(lr) == 0.1f, "Speed clamped to 0.1 from 0.0");
    SDL_Libretro_SetSpeed(lr, -5.0f);
    SDLTest_AssertCheck(SDL_Libretro_GetSpeed(lr) == 0.1f, "Speed clamped to 0.1 from -5.0");
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

    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(lr, 0, RETRO_DEVICE_JOYPAD) == true, "SetPortDevice port 0 true");
    SDLTest_AssertCheck(lr->core.portDeviceMap[0] == RETRO_DEVICE_JOYPAD, "Port 0 device stored");
    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(lr, 15, RETRO_DEVICE_JOYPAD) == true, "SetPortDevice port 15 true");
    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(lr, 16, RETRO_DEVICE_JOYPAD) == false, "SetPortDevice port 16 false");
    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(NULL, 0, RETRO_DEVICE_JOYPAD) == false, "SetPortDevice(NULL) false");

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

    SDLTest_AssertCheck(SDL_Libretro_IsRewinding(lr) == false, "Not rewinding on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_SetRewindEnabled(lr, true, 600, 2) == true, "Enable rewind before core load");
    SDLTest_AssertCheck(lr->rewindEnabled == true, "Rewind enabled flag set");
    SDLTest_AssertCheck(lr->rewindCapacity == 600, "Rewind capacity stored");
    SDLTest_AssertCheck(lr->rewindCaptureInterval == 2, "Rewind capture interval stored");
    SDLTest_AssertCheck(lr->rewindBuffer == NULL, "Buffer not allocated without core");

    SDLTest_AssertCheck(SDL_Libretro_SetRewindEnabled(lr, false, 0, 0) == true, "Disable rewind");
    SDLTest_AssertCheck(lr->rewindEnabled == false, "Rewind disabled");

    SDLTest_AssertCheck(SDL_Libretro_SetRewindEnabled(NULL, true, 100, 1) == false, "SetRewindEnabled(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_IsRewinding(NULL) == false, "IsRewinding(NULL) false");

    // Negative speed only accepted when rewind is enabled.
    SDL_Libretro_SetSpeed(lr, -1.0f);
    SDLTest_AssertCheck(lr->speed == 0.1f, "Negative speed clamped without rewind");
    SDL_Libretro_SetRewindEnabled(lr, true, 100, 1);
    SDL_Libretro_SetSpeed(lr, -1.0f);
    SDLTest_AssertCheck(lr->speed == -1.0f, "Negative speed accepted with rewind");
  
    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
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
    LIBRETRO_TEST_CASE(test_LogLevel,         "Log level filtering"),
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
