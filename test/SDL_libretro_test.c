#define SDL_LIBRETRO_IMPLEMENTATION
#define SDL_LIBRETRO_ENABLE_PHYSFS
#include "SDL_libretro.h"

#include <SDL3/SDL_main.h>
#include <SDL3/SDL_test.h>

static int SDLCALL test_CreateDestroy(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDLTest_AssertCheck(lr != NULL, "SDL_Libretro_Create returned non-NULL");

    // Defaults
    SDLTest_AssertCheck(lr->volume == 1.0f, "Default volume is 1.0");
    SDLTest_AssertCheck(lr->core.speed == 1.0f, "Default speed is 1.0");
    SDLTest_AssertCheck(SDL_Libretro_GetSaveDirectory(lr) == NULL, "No save directory on a fresh context");
    SDLTest_AssertCheck(SDL_Libretro_GetSystemDirectory(lr) == NULL, "No system directory on a fresh context");

    // Default Keyboard Mappings
    SDLTest_AssertCheck(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_B] == SDL_SCANCODE_Z, "B mapped to Z");
    SDLTest_AssertCheck(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_X, "A mapped to X");
    SDLTest_AssertCheck(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_START] == SDL_SCANCODE_RETURN, "Start mapped to Return");
    SDLTest_AssertCheck(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_UP] == SDL_SCANCODE_UP, "Up mapped to Up");

    // Default Gamepad Mappings, seeded on every port.
    SDLTest_AssertCheck(lr->gamepadButtons[0][RETRO_DEVICE_ID_JOYPAD_B] == SDL_GAMEPAD_BUTTON_SOUTH, "B mapped to South");
    SDLTest_AssertCheck(lr->gamepadButtons[0][RETRO_DEVICE_ID_JOYPAD_L3] == SDL_GAMEPAD_BUTTON_LEFT_STICK, "L3 mapped to Left Stick");
    SDLTest_AssertCheck(lr->gamepadButtons[SDL_LIBRETRO_MAX_USERS - 1][RETRO_DEVICE_ID_JOYPAD_B] == SDL_GAMEPAD_BUTTON_SOUTH,
        "B mapped to South on the last port");

    // SetGamepadMapping bounds and per-port application.
    SDL_Libretro_SetGamepadMapping(NULL, 0, RETRO_DEVICE_ID_JOYPAD_B, SDL_GAMEPAD_BUTTON_NORTH);
    SDL_Libretro_SetGamepadMapping(lr, SDL_LIBRETRO_MAX_USERS, RETRO_DEVICE_ID_JOYPAD_B, SDL_GAMEPAD_BUTTON_NORTH);
    SDL_Libretro_SetGamepadMapping(lr, 0, -1, SDL_GAMEPAD_BUTTON_NORTH);
    SDL_Libretro_SetGamepadMapping(lr, 0, SDL_LIBRETRO_MAX_JOYPAD_BUTTONS, SDL_GAMEPAD_BUTTON_NORTH);
    SDL_Libretro_SetGamepadMapping(lr, 1, RETRO_DEVICE_ID_JOYPAD_B, SDL_GAMEPAD_BUTTON_NORTH);
    SDLTest_AssertCheck(lr->gamepadButtons[1][RETRO_DEVICE_ID_JOYPAD_B] == SDL_GAMEPAD_BUTTON_NORTH, "SetGamepadMapping applies to its port");
    SDLTest_AssertCheck(lr->gamepadButtons[0][RETRO_DEVICE_ID_JOYPAD_B] == SDL_GAMEPAD_BUTTON_SOUTH, "Other ports keep their binding");

    // Analog to Digital defaults, bounds, and per-port application.
    SDLTest_AssertCheck(SDL_Libretro_GetAnalogToDigital(lr, 0) == SDL_LIBRETRO_ANALOG_TO_DIGITAL_NONE, "Analog to Digital defaults to None");
    SDLTest_AssertCheck(SDL_Libretro_GetAnalogToDigital(NULL, 0) == SDL_LIBRETRO_ANALOG_TO_DIGITAL_NONE, "GetAnalogToDigital(NULL) reports None");
    SDLTest_AssertCheck(SDL_Libretro_GetAnalogToDigital(lr, SDL_LIBRETRO_MAX_USERS) == SDL_LIBRETRO_ANALOG_TO_DIGITAL_NONE,
        "GetAnalogToDigital rejects out-of-range ports");
    SDL_Libretro_SetAnalogToDigital(NULL, 0, SDL_LIBRETRO_ANALOG_TO_DIGITAL_LEFT);
    SDL_Libretro_SetAnalogToDigital(lr, SDL_LIBRETRO_MAX_USERS, SDL_LIBRETRO_ANALOG_TO_DIGITAL_LEFT);
    SDL_Libretro_SetAnalogToDigital(lr, 1, (SDL_LibretroAnalogToDigital)SDL_LIBRETRO_ANALOG_TO_DIGITAL_COUNT);
    SDL_Libretro_SetAnalogToDigital(lr, 1, SDL_LIBRETRO_ANALOG_TO_DIGITAL_RIGHT);
    SDLTest_AssertCheck(SDL_Libretro_GetAnalogToDigital(lr, 1) == SDL_LIBRETRO_ANALOG_TO_DIGITAL_RIGHT, "SetAnalogToDigital applies to its port");
    SDLTest_AssertCheck(SDL_Libretro_GetAnalogToDigital(lr, 0) == SDL_LIBRETRO_ANALOG_TO_DIGITAL_NONE, "Other ports keep None");

    // With no gamepad the axes read neutral, so nothing presses; None
    // short-circuits, and non-direction ids never press.
    SDLTest_AssertCheck(SDL_Libretro_AnalogToDigitalPressed(lr, 0, RETRO_DEVICE_ID_JOYPAD_UP) == false,
        "Analog to Digital None never presses");
    SDLTest_AssertCheck(SDL_Libretro_AnalogToDigitalPressed(lr, 1, RETRO_DEVICE_ID_JOYPAD_UP) == false,
        "A centered stick doesn't press");
    SDLTest_AssertCheck(SDL_Libretro_AnalogToDigitalPressed(lr, 1, RETRO_DEVICE_ID_JOYPAD_B) == false,
        "Non-direction buttons never press");

    SDL_Libretro_Destroy(lr);
    SDL_Libretro_Destroy(NULL);

    return TEST_COMPLETED;
}

static int SDLCALL test_StateQueries(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();
    int w = -1, h = -1;

    SDLTest_AssertCheck(SDL_Libretro_IsCoreReady(lr) == false, "IsCoreReady false on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == false, "IsGameReady false on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_ShouldQuit(lr) == false, "ShouldClose false on fresh context");
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
    SDLTest_AssertCheck(SDL_Libretro_GetSavestateContext(lr) == RETRO_SAVESTATE_CONTEXT_NORMAL, "GetSavestateContext NORMAL on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionCount(lr) == 0, "GetOptionCount 0 on fresh context");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

static int SDLCALL test_NullSafety(void *arg) {
    int w = -1, h = -1;

    SDLTest_AssertCheck(SDL_Libretro_IsCoreReady(NULL) == false, "IsCoreReady(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(NULL) == false, "IsGameReady(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_ShouldQuit(NULL) == false, "ShouldClose(NULL) false");
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
    SDLTest_AssertCheck(SDL_Libretro_GetSavestateContext(NULL) == RETRO_SAVESTATE_CONTEXT_NORMAL, "GetSavestateContext(NULL) NORMAL");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionCount(NULL) == 0, "GetOptionCount(NULL) 0");
    SDLTest_AssertCheck(SDL_Libretro_GetOption(NULL, "foo") == NULL, "GetOption(NULL) NULL");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionByIndex(NULL, 0) == NULL, "GetOptionByIndex(NULL) NULL");

    return TEST_COMPLETED;
}

static int SDLCALL test_DirectorySetters(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    SDLTest_AssertCheck(SDL_Libretro_SetCoreDirectory(lr, "/tmp/cores") == true, "SetCoreDirectory returns true");
    SDLTest_AssertCheck(SDL_strcmp(lr->coreDirectory, "/tmp/cores") == 0, "CoreDirectory stored correctly");
    // Leftovers from prior runs would mask the "not created" checks below.
    SDL_RemovePath("/tmp/saves");
    SDL_RemovePath("/tmp/system");
    SDLTest_AssertCheck(SDL_Libretro_SetSaveDirectory(lr, "/tmp/saves") == true, "SetSaveDirectory returns true");
    SDLTest_AssertCheck(SDL_strcmp(lr->saveDirectory, "/tmp/saves") == 0, "SaveDirectory stored correctly");
    SDLTest_AssertCheck(SDL_GetPathInfo("/tmp/saves", NULL) == false, "SetSaveDirectory does not create the directory (LoadCore does)");
    SDLTest_AssertCheck(SDL_Libretro_SetSaveDirectory(lr, "") == true, "SetSaveDirectory(\"\") returns true");
    SDLTest_AssertCheck(lr->saveDirectory[0] == '\0', "Empty path clears the save directory");
    SDLTest_AssertCheck(SDL_Libretro_SetSaveDirectory(lr, NULL) == true, "SetSaveDirectory(NULL) returns true");
    SDLTest_AssertCheck(SDL_strstr(lr->saveDirectory, "/saves") != NULL,
        "NULL resets the save directory to '<pref>/saves': '%s'", lr->saveDirectory);
    SDLTest_AssertCheck(SDL_Libretro_SetSystemDirectory(lr, "/tmp/system") == true, "SetSystemDirectory returns true");
    SDLTest_AssertCheck(SDL_strcmp(lr->systemDirectory, "/tmp/system") == 0, "SystemDirectory stored correctly");
    SDLTest_AssertCheck(SDL_GetPathInfo("/tmp/system", NULL) == false, "SetSystemDirectory does not create explicit paths");
    SDLTest_AssertCheck(SDL_Libretro_SetSystemDirectory(lr, "") == true, "SetSystemDirectory(\"\") returns true");
    SDLTest_AssertCheck(lr->systemDirectory[0] == '\0', "Empty path clears the system directory");
    SDLTest_AssertCheck(SDL_Libretro_SetSystemDirectory(lr, NULL) == true, "SetSystemDirectory(NULL) returns true");
    SDLTest_AssertCheck(SDL_strstr(lr->systemDirectory, "/system") != NULL,
        "NULL resets the system directory to '<pref>/system': '%s'", lr->systemDirectory);
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

static int SDLCALL test_DRCDisable(void *arg) {
    (void)arg;
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    // Force SDL's dummy audio driver so the stream opens on machines (and CI
    // runners) without a sound device. An SDL_AUDIO_DRIVER env var still wins.
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    SDL_Init(SDL_INIT_AUDIO);
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "LoadGame succeeds with test content");
    SDLTest_AssertCheck(lr->core.audioStream != NULL, "Audio stream open after load");

    // Turning DRC off must unwind a leftover nudge rather than freeze it in
    // the stream's frequency ratio.
    lr->core.drcAdjustment = 1.004f;
    SDL_SetAudioStreamFrequencyRatio(lr->core.audioStream, lr->core.drcAdjustment);
    lr->core.drcEnabled = false;
    SDL_Libretro_UpdateDRC(lr, 1.0f);
    SDLTest_AssertCheck(lr->core.drcAdjustment == 1.0f, "DRC adjustment reset when disabled");
    float ratio = SDL_GetAudioStreamFrequencyRatio(lr->core.audioStream);
    SDLTest_AssertCheck(ratio == 1.0f, "Frequency ratio back to plain speed, got %f", ratio);

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
#endif
}

// Records the last keyboard callback so tests can check what the core sees.
static bool test_keyDown;
static unsigned test_keyCode;
static uint32_t test_keyCharacter;
static void RETRO_CALLCONV test_KeyboardEvent(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers) {
    (void)key_modifiers;
    test_keyDown = down;
    test_keyCode = keycode;
    test_keyCharacter = character;
}

// Push a synthetic key event through SDL_Libretro_HandleEvent().
static void SendKey(SDL_Libretro* lr, bool down, SDL_Scancode scancode, SDL_Keymod mod) {
    SDL_Event ev;
    SDL_zero(ev);
    ev.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    ev.key.scancode = scancode;
    ev.key.key = SDL_GetKeyFromScancode(scancode, SDL_KMOD_NONE, true);
    ev.key.mod = mod;
    ev.key.down = down;
    SDL_Libretro_HandleEvent(lr, &ev);
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
    SDLTest_AssertCheck(lr->core.virtualJoypadState[0][RETRO_DEVICE_ID_JOYPAD_A] == true, "Virtual button A pressed");
    SDL_Libretro_SetVirtualButton(lr, 0, RETRO_DEVICE_ID_JOYPAD_A, false);
    SDLTest_AssertCheck(lr->core.virtualJoypadState[0][RETRO_DEVICE_ID_JOYPAD_A] == false, "Virtual button A released");
    /* Virtual buttons are now per-port: port 1 must not alias port 0. */
    SDL_Libretro_SetVirtualButton(lr, 1, RETRO_DEVICE_ID_JOYPAD_B, true);
    SDLTest_AssertCheck(lr->core.virtualJoypadState[1][RETRO_DEVICE_ID_JOYPAD_B] == true, "Virtual button on port 1 set");
    SDLTest_AssertCheck(lr->core.virtualJoypadState[0][RETRO_DEVICE_ID_JOYPAD_B] == false, "Port 1 does not alias port 0");
    SDL_Libretro_SetVirtualButton(lr, 16, 0, true);
    SDL_Libretro_SetVirtualButton(lr, 0, 16, true);
    SDL_Libretro_SetVirtualButton(NULL, 0, 0, true);

    SDLTest_AssertCheck(SDL_Libretro_GetPortDevice(lr, 0) == RETRO_DEVICE_JOYPAD, "Port 0 device defaults to JOYPAD");
    SDLTest_AssertCheck(SDL_Libretro_GetPortDevice(lr, SDL_LIBRETRO_MAX_USERS - 1) == RETRO_DEVICE_JOYPAD, "Last port device defaults to JOYPAD");
    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(lr, 0, RETRO_DEVICE_MOUSE) == true, "SetPortDevice port 0 true");
    SDLTest_AssertCheck(SDL_Libretro_GetPortDevice(lr, 0) == RETRO_DEVICE_MOUSE, "GetPortDevice returns stored device");
    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(lr, 0, RETRO_DEVICE_NONE) == true, "SetPortDevice port 0 NONE true");
    SDLTest_AssertCheck(SDL_Libretro_GetPortDevice(lr, 0) == RETRO_DEVICE_NONE, "Explicit NONE is preserved");
    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(lr, 1, RETRO_DEVICE_JOYPAD) == true, "SetPortDevice port 1 true");
    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(lr, 2, RETRO_DEVICE_JOYPAD) == true, "SetPortDevice port 2 true");
    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(lr, 16, RETRO_DEVICE_JOYPAD) == false, "SetPortDevice port 16 false");
    SDLTest_AssertCheck(SDL_Libretro_SetPortDevice(NULL, 0, RETRO_DEVICE_JOYPAD) == false, "SetPortDevice(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_GetPortDevice(lr, 16) == RETRO_DEVICE_NONE, "GetPortDevice out-of-range NONE");
    SDLTest_AssertCheck(SDL_Libretro_GetPortDevice(NULL, 0) == RETRO_DEVICE_NONE, "GetPortDevice(NULL) NONE");

    // Input descriptors (empty without a loaded core)
    SDLTest_AssertCheck(SDL_Libretro_GetInputDescriptorCount(lr) == 0, "Descriptor count 0 without core");
    SDLTest_AssertCheck(SDL_Libretro_GetInputDescriptor(lr, 0, NULL, NULL, NULL, NULL) == false, "GetInputDescriptor false without core");
    SDLTest_AssertCheck(SDL_Libretro_GetInputDescriptorCount(NULL) == 0, "GetInputDescriptorCount(NULL) 0");
    SDLTest_AssertCheck(SDL_Libretro_GetInputDescriptor(NULL, 0, NULL, NULL, NULL, NULL) == false, "GetInputDescriptor(NULL) false");

    // Keyboard callback characters: space is included and modifiers apply.
    lr->core.keyboard_event = test_KeyboardEvent;
    SendKey(lr, true, SDL_SCANCODE_SPACE, SDL_KMOD_NONE);
    SDLTest_AssertCheck(test_keyDown && test_keyCode == RETROK_SPACE && test_keyCharacter == ' ',
        "Space sends ' ', got key %u char %u", test_keyCode, (unsigned)test_keyCharacter);
    SendKey(lr, true, SDL_SCANCODE_1, SDL_KMOD_LSHIFT);
    SDLTest_AssertCheck(test_keyCode == RETROK_1 && test_keyCharacter == '!',
        "Shift+1 sends '!', got char %u", (unsigned)test_keyCharacter);
    SendKey(lr, true, SDL_SCANCODE_A, SDL_KMOD_NONE);
    SDLTest_AssertCheck(test_keyCharacter == 'a', "A sends 'a', got char %u", (unsigned)test_keyCharacter);
    SendKey(lr, true, SDL_SCANCODE_UP, SDL_KMOD_NONE);
    SDLTest_AssertCheck(test_keyCode == RETROK_UP && test_keyCharacter == 0,
        "Arrow keys send no character, got char %u", (unsigned)test_keyCharacter);
    SendKey(lr, false, SDL_SCANCODE_UP, SDL_KMOD_NONE);
    SDLTest_AssertCheck(!test_keyDown, "Key up reports released");
    lr->core.keyboard_event = NULL;

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

/* Drives SDL_Libretro_HandleEvent() with gamepad hotplug events backed by
   virtual joysticks, verifying gamepadCount tracks the highest occupied
   slot + 1 through additions and removals. */
static int SDLCALL test_GamepadEvents(void *arg) {
    SDL_Init(SDL_INIT_GAMEPAD);
    SDL_Libretro* lr = SDL_Libretro_Create();

    SDLTest_AssertCheck(lr->gamepadCount == 0, "gamepadCount 0 on fresh context");

    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    SDL_JoystickID jid1 = SDL_AttachVirtualJoystick(&desc);
    SDL_JoystickID jid2 = SDL_AttachVirtualJoystick(&desc);
    SDLTest_AssertCheck(jid1 != 0 && jid2 != 0, "Attached two virtual gamepads");

    SDL_Event ev;
    SDL_zero(ev);
    ev.type = SDL_EVENT_GAMEPAD_ADDED;
    ev.gdevice.which = jid1;
    SDL_Libretro_HandleEvent(lr, &ev);
    SDLTest_AssertCheck(lr->gamepads[0] != NULL, "Slot 0 filled after first add");
    SDLTest_AssertCheck(lr->gamepadCount == 1, "gamepadCount 1 after first add, got %u", lr->gamepadCount);

    ev.gdevice.which = jid2;
    SDL_Libretro_HandleEvent(lr, &ev);
    SDLTest_AssertCheck(lr->gamepads[1] != NULL, "Slot 1 filled after second add");
    SDLTest_AssertCheck(lr->gamepadCount == 2, "gamepadCount 2 after second add, got %u", lr->gamepadCount);

    // Removing slot 0 leaves slot 1 occupied, so the count stays 2.
    SDL_zero(ev);
    ev.type = SDL_EVENT_GAMEPAD_REMOVED;
    ev.gdevice.which = jid1;
    SDL_Libretro_HandleEvent(lr, &ev);
    SDLTest_AssertCheck(lr->gamepads[0] == NULL, "Slot 0 cleared after removal");
    SDLTest_AssertCheck(lr->gamepadCount == 2, "gamepadCount stays 2 while slot 1 occupied, got %u", lr->gamepadCount);

    // Removing the last gamepad drops the count to 0.
    ev.gdevice.which = jid2;
    SDL_Libretro_HandleEvent(lr, &ev);
    SDLTest_AssertCheck(lr->gamepads[1] == NULL, "Slot 1 cleared after removal");
    SDLTest_AssertCheck(lr->gamepadCount == 0, "gamepadCount 0 after all removed, got %u", lr->gamepadCount);

    // Once every port is full, an extra gamepad is closed rather than leaked.
    SDL_JoystickID jids[SDL_LIBRETRO_MAX_USERS + 1];
    SDL_zero(ev);
    ev.type = SDL_EVENT_GAMEPAD_ADDED;
    for (unsigned i = 0; i < SDL_LIBRETRO_MAX_USERS + 1; i++) {
        jids[i] = SDL_AttachVirtualJoystick(&desc);
        ev.gdevice.which = jids[i];
        SDL_Libretro_HandleEvent(lr, &ev);
    }
    SDLTest_AssertCheck(lr->gamepadCount == SDL_LIBRETRO_MAX_USERS, "Every port filled, got %u", lr->gamepadCount);
    SDLTest_AssertCheck(SDL_GetGamepadFromID(jids[SDL_LIBRETRO_MAX_USERS]) == NULL,
        "Gamepad without a free port is closed");
    SDL_Libretro_Destroy(lr);
    for (unsigned i = 0; i < SDL_LIBRETRO_MAX_USERS + 1; i++) {
        SDL_DetachVirtualJoystick(jids[i]);
    }

    SDL_DetachVirtualJoystick(jid1);
    SDL_DetachVirtualJoystick(jid2);
    return TEST_COMPLETED;
}

static int SDLCALL test_Options(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    SDLTest_AssertCheck(SDL_Libretro_GetOptionCount(lr) == 0, "Option count 0 on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionByIndex(lr, 0) == NULL, "GetOptionByIndex(0) NULL on empty");
    SDLTest_AssertCheck(SDL_Libretro_GetOption(lr, "foo") == NULL, "GetOption NULL on empty");
    SDLTest_AssertCheck(SDL_Libretro_AreOptionsDirty(lr) == false, "Options not dirty on fresh context");
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "key", "val") == false, "SetOptionValue false on empty");
    SDLTest_AssertCheck(SDL_Libretro_ResetOption(lr, "key") == false, "ResetOption false on empty");
    SDL_Libretro_ResetAllOptions(lr);

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

/* Exercises the deprecated RETRO_ENVIRONMENT_SET_VARIABLES path directly, which
   the test core (v2 only) never triggers. Drives the static env callback by
   pointing SDL_Libretro_active at a bare context. */
static int SDLCALL test_SetVariablesLegacy(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro* prevActive = SDL_Libretro_active;
    SDL_Libretro_active = lr;

    // Build a variable with more values than the old fixed cap to prove the
    // dynamic parser doesn't truncate. "v0" is the default (first value).
    const unsigned bigCount = RETRO_NUM_CORE_OPTION_VALUES_MAX + 50;
    size_t bufSize = 32 + bigCount * 8;
    char* big = (char*)SDL_malloc(bufSize);
    size_t off = (size_t)SDL_snprintf(big, bufSize, "Big Option; ");
    for (unsigned i = 0; i < bigCount; i++) {
        off += (size_t)SDL_snprintf(big + off, bufSize - off, "%sv%u", i ? "|" : "", i);
    }

    struct retro_variable vars[] = {
        // Extra spaces after ';' and around the standard single space are tolerated.
        { "leg_bool", "Enable Feature;  enabled|disabled" },
        { "leg_big", big },
        { NULL, NULL },
    };
    bool ok = SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_VARIABLES, vars);
    SDLTest_AssertCheck(ok == true, "SET_VARIABLES returns true");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionCount(lr) == 2, "Two legacy options registered");

    const SDL_LibretroOption* b = SDL_Libretro_GetOption(lr, "leg_bool");
    SDLTest_AssertCheck(b && SDL_strcmp(b->desc, "Enable Feature") == 0, "Label parsed, trailing space trimmed");
    SDLTest_AssertCheck(b && SDL_strcmp(b->value, "enabled") == 0, "First value is the default");
    SDLTest_AssertCheck(b && SDL_strcmp(b->defaultValue, "enabled") == 0, "Default value stored");
    SDLTest_AssertCheck(b && b->valuesCount == 2, "Two values parsed");
    SDLTest_AssertCheck(b && SDL_strcmp(b->values[0].value, "enabled") == 0, "Value 0 is enabled");
    SDLTest_AssertCheck(b && SDL_strcmp(b->values[1].value, "disabled") == 0, "Value 1 is disabled");
    SDLTest_AssertCheck(b && b->values[0].label == NULL, "Legacy values carry no label");

    // The whole point of the dynamic array: no truncation past the old cap.
    const SDL_LibretroOption* big_opt = SDL_Libretro_GetOption(lr, "leg_big");
    SDLTest_AssertCheck(big_opt && big_opt->valuesCount == bigCount, "All values parsed (no truncation)");
    SDLTest_AssertCheck(big_opt && SDL_strcmp(big_opt->value, "v0") == 0, "Big option default is v0");
    SDLTest_AssertCheck(big_opt && SDL_strcmp(big_opt->values[bigCount - 1].value, "v177") == 0,
        "Last value survives");

    // Values are validated against the declared list, same as v2 options.
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "leg_bool", "disabled") == true, "Set declared value");
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "leg_bool", "bogus") == false, "Reject undeclared value");

    SDL_free(big);
    SDL_Libretro_Destroy(lr);
    SDL_Libretro_active = prevActive;
    return TEST_COMPLETED;
}

/* A core re-registering an option replaces its values, default, and text,
   keeping the current value only while it's still declared. */
static int SDLCALL test_OptionReregister(void *arg) {
    (void)arg;
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro* prevActive = SDL_Libretro_active;
    SDL_Libretro_active = lr;
    bool updated = false;

    struct retro_variable first[] = { { "re_opt", "Re; a|b|c" }, { NULL, NULL } };
    SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_VARIABLES, first);
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "re_opt", "b") == true, "Set b on the first registration");
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "re_opt", "d") == false, "d isn't declared yet");
    SDL_Libretro_AreOptionsDirty(lr);
    SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated);

    // New values that still include the current one keep it.
    struct retro_variable second[] = { { "re_opt", "Re New; d|b" }, { NULL, NULL } };
    SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_VARIABLES, second);
    const SDL_LibretroOption* opt = SDL_Libretro_GetOption(lr, "re_opt");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionCount(lr) == 1, "Re-registration doesn't add an option");
    SDLTest_AssertCheck(opt && SDL_strcmp(opt->desc, "Re New") == 0, "Description updated");
    SDLTest_AssertCheck(opt && SDL_strcmp(opt->defaultValue, "d") == 0, "Default updated");
    SDLTest_AssertCheck(opt && opt->valuesCount == 2, "Values replaced");
    SDLTest_AssertCheck(opt && SDL_strcmp(opt->value, "b") == 0, "Still-valid current value kept");
    SDLTest_AssertCheck(lr->core.optionsDirtyCore == false, "Kept value isn't an update to the core");
    SDLTest_AssertCheck(SDL_Libretro_AreOptionsDirty(lr) == true, "App told to refresh the option");
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "re_opt", "d") == true, "Newly declared value accepted");
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "re_opt", "a") == false, "Dropped value rejected");
    SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated);

    // A current value that's no longer declared resets to the new default.
    static const struct retro_core_option_v2_definition defs[] = {
        { "re_opt", "Re V2", NULL, "Re info", NULL, "re_cat",
          { {"e", "E"}, {"f", "F"}, {NULL, NULL} }, "f" },
        { NULL, NULL, NULL, NULL, NULL, NULL, {{NULL, NULL}}, NULL },
    };
    static const struct retro_core_options_v2 opts = { NULL, (struct retro_core_option_v2_definition *)defs };
    SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, (void *)&opts);
    opt = SDL_Libretro_GetOption(lr, "re_opt");
    SDLTest_AssertCheck(opt && SDL_strcmp(opt->value, "f") == 0, "Invalid current value reset to the new default");
    SDLTest_AssertCheck(opt && SDL_strcmp(opt->info, "Re info") == 0, "Info updated");
    SDLTest_AssertCheck(opt && SDL_strcmp(opt->category, "re_cat") == 0, "Category updated");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetOptionValueLabel(lr, "re_opt"), "F") == 0, "Value labels updated");
    updated = false;
    SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated);
    SDLTest_AssertCheck(updated == true, "Core told the value changed");

    SDL_Libretro_Destroy(lr);
    SDL_Libretro_active = prevActive;
    return TEST_COMPLETED;
}

/* Exercises the perf interface directly through a bare active context. */
static int SDLCALL test_PerfInterface(void *arg) {
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro* prevActive = SDL_Libretro_active;
    SDL_Libretro_active = lr;

    struct retro_perf_callback perf = {0};
    SDLTest_AssertCheck(SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf), "GET_PERF_INTERFACE succeeds");
    SDLTest_AssertCheck(perf.get_cpu_features != NULL, "get_cpu_features provided");

    // CPU features mirror SDL's queries.
    if (perf.get_cpu_features) {
        uint64_t features = perf.get_cpu_features();
        SDLTest_AssertCheck(!SDL_HasNEON() || (features & RETRO_SIMD_NEON), "NEON reported when SDL_HasNEON()");
        SDLTest_AssertCheck(!SDL_HasSSE2() || (features & RETRO_SIMD_SSE2), "SSE2 reported when SDL_HasSSE2()");
        SDLTest_AssertCheck(!SDL_HasMMX() || (features & RETRO_SIMD_MMX), "MMX reported when SDL_HasMMX()");
        SDLTest_AssertCheck(!SDL_HasAltiVec() || (features & RETRO_SIMD_VMX), "VMX reported when SDL_HasAltiVec()");
        SDLTest_AssertCheck(!SDL_HasAVX512F() || (features & RETRO_SIMD_AVX512), "AVX512 reported when SDL_HasAVX512F()");
        SDLTest_AssertCheck(SDL_HasNEON() || !(features & RETRO_SIMD_NEON), "NEON not reported without SDL_HasNEON()");
#if defined(__aarch64__) || defined(_M_ARM64)
        // NEON is mandatory on AArch64.
        SDLTest_AssertCheck((features & RETRO_SIMD_NEON) && (features & RETRO_SIMD_ASIMD), "AArch64 reports NEON and ASIMD");
#endif
    }

    SDL_Libretro_Destroy(lr);
    SDL_Libretro_active = prevActive;
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
    SDLTest_AssertCheck(lr->core.speed == 0.0f, "Negative speed clamped without rewind");
    SDL_Libretro_SetRewindEnabled(lr, true, 100, 1);
    SDL_Libretro_SetSpeed(lr, -1.0f);
    SDLTest_AssertCheck(lr->core.speed == -1.0f, "Negative speed accepted with rewind");

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
    // Give the context a fake module handle so SDL_Libretro_IsCoreReady() is true
    // for the stub core below (cleared before Destroy so teardown is skipped).
    lr->core.symbols.handle = (SDL_SharedObject*)lr;
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

    // Duration-based budget: 1s at 60fps, interval 1, 32-byte state = 60 snapshots.
    SDLTest_AssertCheck(SDL_Libretro_SetRewindMemoryDuration(lr, 1.0) == true, "Set rewind budget by duration");
    SDLTest_AssertCheck(SDL_Libretro_GetRewindMemoryLimit(lr) == (size_t)60 * sizeof(g_rewindState), "Duration budget equals snapshots * state size");
    SDLTest_AssertCheck(SDL_Libretro_SetRewindMemoryDuration(lr, 0.0) == false, "Non-positive duration rejected");
    SDLTest_AssertCheck(SDL_Libretro_SetRewindMemoryDuration(NULL, 1.0) == false, "NULL context rejected");

    SDL_Libretro_SetRewindEnabled(lr, false, 0, 0);
    lr->core.symbols.handle = NULL; /* let Destroy skip core teardown (stub has no real module/symbols) */
    lr->core.gameLoaded = false;
    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

/* Rewind settings round-trip through the config file, and loading a config
   doesn't discard a snapshot count or interval the app already set. */
static int SDLCALL test_RewindConfig(void *arg) {
    (void)arg;
    const char* cfg = "rewind_test.cfg";
    SDL_RemovePath(cfg);

    // Enabled with custom settings.
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_InitConfigFile(lr, cfg);
    SDL_Libretro_SetRewindEnabled(lr, true, 600, 3);
    SDL_Libretro_Destroy(lr);

    lr = SDL_Libretro_Create();
    SDL_Libretro_InitConfigFile(lr, cfg);
    SDLTest_AssertCheck(SDL_Libretro_GetRewindEnabled(lr) == true, "Rewind enabled persists");
    SDLTest_AssertCheck(lr->rewindCapacity == 600, "Rewind frames persist");
    SDLTest_AssertCheck(lr->rewindCaptureInterval == 3, "Rewind interval persists");

    // Disabled settings are kept too, ready for re-enabling.
    SDL_Libretro_SetRewindEnabled(lr, true, 120, 2);
    SDL_Libretro_SetRewindEnabled(lr, false, lr->rewindCapacity, lr->rewindCaptureInterval);
    SDL_Libretro_Destroy(lr);

    lr = SDL_Libretro_Create();
    SDL_Libretro_InitConfigFile(lr, cfg);
    SDLTest_AssertCheck(SDL_Libretro_GetRewindEnabled(lr) == false, "Rewind disabled persists");
    SDLTest_AssertCheck(lr->rewindCapacity == 120, "Rewind frames persist while disabled");
    SDLTest_AssertCheck(lr->rewindCaptureInterval == 2, "Rewind interval persists while disabled");
    SDL_Libretro_Destroy(lr);

    // A config with only the enabled flag keeps what the app set.
    const char* text = "rewindenabled=true\n";
    SDLTest_AssertCheck(SDL_SaveFile(cfg, text, SDL_strlen(text)), "Write the config file");
    lr = SDL_Libretro_Create();
    SDL_Libretro_SetRewindEnabled(lr, true, 900, 4);
    SDL_Libretro_InitConfigFile(lr, cfg);
    SDLTest_AssertCheck(SDL_Libretro_GetRewindEnabled(lr) == true, "Rewind enabled from the config");
    SDLTest_AssertCheck(lr->rewindCapacity == 900, "App-set rewind frames kept");
    SDLTest_AssertCheck(lr->rewindCaptureInterval == 4, "App-set rewind interval kept");
    SDL_Libretro_Destroy(lr);

    SDL_RemovePath(cfg);
    return TEST_COMPLETED;
}

static int SDLCALL test_OptionVisibility(void *arg) {
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // test_core registers test_option_a (visible) and test_option_b (hidden), both
    // under test_category. Both carry desc_categorized; info_categorized is unset.
    SDLTest_AssertCheck(SDL_Libretro_GetOptionCount(lr) == 2, "Two options registered by test_core");

    const SDL_LibretroOption* a = SDL_Libretro_GetOption(lr, "test_option_a");
    SDLTest_AssertCheck(a != NULL, "GetOption(test_option_a) non-NULL");
    SDLTest_AssertCheck(a && a->visible == true, "Option A visible");
    SDLTest_AssertCheck(a && SDL_strcmp(a->desc, "Option A Category") == 0, "Option A desc uses desc_categorized");
    SDLTest_AssertCheck(a && SDL_strcmp(a->info, "First test option") == 0, "Option A info falls back to info");
    SDLTest_AssertCheck(a && SDL_strcmp(a->value, "on") == 0, "Option A default value on");
    SDLTest_AssertCheck(a && SDL_strcmp(a->category, "test_category") == 0, "Option A category");
    SDLTest_AssertCheck(a && a->valuesCount == 2, "Option A has 2 values");

    const SDL_LibretroOption* b = SDL_Libretro_GetOption(lr, "test_option_b");
    SDLTest_AssertCheck(b && b->visible == false, "Option B hidden via SET_CORE_OPTIONS_DISPLAY");
    SDLTest_AssertCheck(SDL_Libretro_AreOptionsDirty(lr) == true, "optionsDirty set after display change");
    SDLTest_AssertCheck(lr->core.optionsDirtyCore == false, "Display change doesn't flag GET_VARIABLE_UPDATE");

    // Toggling visibility again must not make GET_VARIABLE_UPDATE report true.
    struct retro_core_option_display showB = {"test_option_b", true};
    SDLTest_AssertCheck(SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &showB), "SET_CORE_OPTIONS_DISPLAY accepted");
    bool displayUpdated = true;
    SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &displayUpdated);
    SDLTest_AssertCheck(displayUpdated == false, "GET_VARIABLE_UPDATE false after display change");
    SDLTest_AssertCheck(b && b->visible == true, "Option B shown again");
    SDLTest_AssertCheck(SDL_Libretro_AreOptionsDirty(lr) == true, "App dirty after second display change");

    SDLTest_AssertCheck(SDL_Libretro_GetOptionByIndex(lr, 0) == a, "GetOptionByIndex(0) == GetOption(test_option_a)");
    SDLTest_AssertCheck(SDL_Libretro_GetOption(lr, "nope") == NULL, "Unknown option NULL");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionByIndex(lr, 5) == NULL, "GetOptionByIndex out-of-range NULL");
    SDLTest_AssertCheck(SDL_Libretro_GetOption(NULL, "test_option_a") == NULL, "GetOption(NULL lr) NULL");

    // Values are part of the option (test_option_a has on|off, no per-value labels).
    SDLTest_AssertCheck(a && SDL_strcmp(a->values[0].value, "on") == 0, "Value 0 is on");
    SDLTest_AssertCheck(a && SDL_strcmp(a->values[1].value, "off") == 0, "Value 1 is off");
    SDLTest_AssertCheck(a && a->values[0].label == NULL, "Value 0 label NULL (none provided)");

    // Categories: one (test_category) with both options attached.
    SDLTest_AssertCheck(SDL_Libretro_GetCategoryCount(lr) == 1, "One category registered by test_core");
    const SDL_LibretroCategory* c = SDL_Libretro_GetCategory(lr, "test_category");
    SDLTest_AssertCheck(c && SDL_strcmp(c->key, "test_category") == 0, "Category key");
    SDLTest_AssertCheck(c && SDL_strcmp(c->desc, "Test Category") == 0, "Category desc");
    SDLTest_AssertCheck(c && SDL_strcmp(c->info, "Category for test options") == 0, "Category info");
    SDLTest_AssertCheck(SDL_Libretro_GetCategoryByIndex(lr, 0) == c, "GetCategoryByIndex(0) == GetCategory");
    SDLTest_AssertCheck(SDL_Libretro_GetCategoryCount(NULL) == 0, "GetCategoryCount(NULL) 0");
    SDLTest_AssertCheck(SDL_Libretro_GetCategoryByIndex(lr, 5) == NULL, "GetCategoryByIndex out-of-range NULL");
    SDLTest_AssertCheck(SDL_Libretro_GetCategory(lr, "nope") == NULL, "Unknown category NULL");
    SDLTest_AssertCheck(SDL_Libretro_GetCategory(NULL, "test_category") == NULL, "GetCategory(NULL lr) NULL");

    // SetOptionValue validation against declared values; updates show through the held pointer.
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "test_option_a", "off") == true, "Set valid value succeeds");
    SDLTest_AssertCheck(a && SDL_strcmp(a->value, "off") == 0, "Held pointer reflects updated value");
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "test_option_a", "bogus") == false, "Set invalid value rejected");
    SDLTest_AssertCheck(SDL_strstr(SDL_GetError(), "not a valid value") != NULL, "Rejected set reports an error");
    SDLTest_AssertCheck(a && SDL_strcmp(a->value, "off") == 0, "Value unchanged after rejected set");
    SDL_ClearError();
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "nope", "on") == false, "Set unknown option fails");
    SDLTest_AssertCheck(SDL_strstr(SDL_GetError(), "No such option") != NULL, "Unknown option set reports an error");

    // GetOptionValueLabel falls back to the raw value when no label was supplied.
    SDLTest_AssertCheck(a && SDL_strcmp(SDL_Libretro_GetOptionValueLabel(lr, "test_option_a"), "off") == 0,
        "GetOptionValueLabel returns raw value when unlabeled");
    SDLTest_AssertCheck(SDL_Libretro_GetOptionValueLabel(lr, "nope") == NULL, "GetOptionValueLabel unknown NULL");

    // CycleOptionValue walks the declared values and wraps at the ends.
    SDL_Libretro_SetOptionValue(lr, "test_option_a", "on");
    SDLTest_AssertCheck(SDL_Libretro_CycleOptionValue(lr, "test_option_a", 1) == true, "Cycle forward succeeds");
    SDLTest_AssertCheck(a && SDL_strcmp(a->value, "off") == 0, "Cycle forward on -> off");
    SDLTest_AssertCheck(SDL_Libretro_CycleOptionValue(lr, "test_option_a", 1) == true, "Cycle forward wraps");
    SDLTest_AssertCheck(a && SDL_strcmp(a->value, "on") == 0, "Cycle forward off -> on (wrap)");
    SDL_Libretro_CycleOptionValue(lr, "test_option_a", -1);
    SDLTest_AssertCheck(a && SDL_strcmp(a->value, "off") == 0, "Cycle backward on -> off (wrap)");
    SDLTest_AssertCheck(SDL_Libretro_CycleOptionValue(lr, "test_option_a", 0) == false, "Cycle 0 direction fails");
    SDL_ClearError();
    SDLTest_AssertCheck(SDL_Libretro_CycleOptionValue(lr, "nope", 1) == false, "Cycle unknown option fails");
    SDLTest_AssertCheck(SDL_strstr(SDL_GetError(), "No such option") != NULL, "Cycle unknown option reports an error");

    // ResetAllOptions restores defaults and marks options dirty.
    SDL_Libretro_AreOptionsDirty(lr); // clear the dirty flag set by SetOptionValue
    SDL_Libretro_ResetAllOptions(lr);
    SDLTest_AssertCheck(a && SDL_strcmp(a->value, "on") == 0, "ResetAllOptions restores default");
    SDLTest_AssertCheck(SDL_Libretro_AreOptionsDirty(lr) == true, "ResetAllOptions marks options dirty");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_UpdateOptionVisibility(void *arg) {
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDLTest_AssertCheck(SDL_Libretro_UpdateOptionVisibility(NULL) == false, "UpdateOptionVisibility(NULL) false");

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDLTest_AssertCheck(SDL_Libretro_UpdateOptionVisibility(lr) == false, "UpdateOptionVisibility false without core");

    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    const SDL_LibretroOption* a = SDL_Libretro_GetOption(lr, "test_option_a");
    SDLTest_AssertCheck(a && a->visible == true, "Option A initially visible");

    // Set option B to "no", which should cause the callback to hide option A.
    SDL_Libretro_SetOptionValue(lr, "test_option_b", "no");
    SDLTest_AssertCheck(SDL_Libretro_UpdateOptionVisibility(lr) == true, "UpdateOptionVisibility returns true");
    SDLTest_AssertCheck(a && a->visible == false, "Option A hidden after update (B is no)");

    // Set option B back to "yes", option A should become visible again.
    SDL_Libretro_SetOptionValue(lr, "test_option_b", "yes");
    SDL_Libretro_UpdateOptionVisibility(lr);
    SDLTest_AssertCheck(a && a->visible == true, "Option A visible after update (B is yes)");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
#endif
}

/* Options a core registers after the core config was loaded (here, in
   retro_load_game) still pick up their saved value. */
static int SDLCALL test_LateOptionConfig(void *arg) {
    (void)arg;
#if !defined(TEST_CORE_PATH) || !defined(TEST_CONTENT_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH or TEST_CONTENT_PATH not defined");
    return TEST_COMPLETED;
#else
    const char* cfg = "late_option_test.cfg";
    const char* text = "[test_core]\ntest_option_a=off\ntest_option_late=late_b\ntest_option_bad=nope\n";
    SDL_RemovePath(cfg);
    SDLTest_AssertCheck(SDL_SaveFile(cfg, text, SDL_strlen(text)), "Write the config file");

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_InitConfigFile(lr, cfg);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetOptionValue(lr, "test_option_a"), "off") == 0,
        "Early option gets its saved value");
    SDLTest_AssertCheck(SDL_Libretro_GetOption(lr, "test_option_late") == NULL, "Late option not registered yet");

    // Drain the dirty state from the core load.
    bool updated = false;
    SDL_Libretro_AreOptionsDirty(lr);
    SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated);

    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "LoadGame succeeds");
    const char* late = SDL_Libretro_GetOptionValue(lr, "test_option_late");
    SDLTest_AssertCheck(late && SDL_strcmp(late, "late_b") == 0, "Late option gets its saved value");
    const unsigned char* probe = (const unsigned char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SYSTEM_RAM, NULL);
    SDLTest_AssertCheck(probe && probe[24] == 1, "Core reads the saved value right after registering");
    SDLTest_AssertCheck(lr->core.optionsDirtyCore == false, "Applying a saved value at registration isn't an update");

    // A saved value the option doesn't declare is ignored in favour of the default.
    struct retro_variable vars[] = {
        { "test_option_bad", "Bad Option; first|second" },
        { NULL, NULL },
    };
    SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_VARIABLES, vars);
    const char* bad = SDL_Libretro_GetOptionValue(lr, "test_option_bad");
    SDLTest_AssertCheck(bad && SDL_strcmp(bad, "first") == 0, "Invalid saved value falls back to the default");

    SDL_Libretro_Destroy(lr);
    SDL_RemovePath(cfg);
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_OptionsDirtyIndependent(void *arg) {
    (void)arg;
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // Drain both consumers of the dirty state left over from core load.
    bool updated = false;
    SDL_Libretro_AreOptionsDirty(lr);
    SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated);

    // App polls first; the core must still see the update.
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "test_option_a", "off") == true, "SetOptionValue succeeds");
    SDLTest_AssertCheck(SDL_Libretro_AreOptionsDirty(lr) == true, "App sees dirty flag");
    updated = false;
    SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated);
    SDLTest_AssertCheck(updated == true, "Core still sees update after app poll");

    // Core polls first; the app must still see the update.
    SDLTest_AssertCheck(SDL_Libretro_SetOptionValue(lr, "test_option_a", "on") == true, "SetOptionValue back succeeds");
    updated = false;
    SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated);
    SDLTest_AssertCheck(updated == true, "Core sees update first");
    SDLTest_AssertCheck(SDL_Libretro_AreOptionsDirty(lr) == true, "App still sees dirty after core poll");

    // Each consumer clears only its own flag.
    SDLTest_AssertCheck(SDL_Libretro_AreOptionsDirty(lr) == false, "App flag cleared after poll");
    updated = true;
    SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated);
    SDLTest_AssertCheck(updated == false, "Core flag cleared after poll");

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

    // LoadCore ensures the save and system directories exist before the core
    // needs them.
    SDL_RemovePath("/tmp/sdl_libretro_test_saves");
    SDL_RemovePath("/tmp/sdl_libretro_test_system");
    SDL_Libretro_SetSaveDirectory(lr, "/tmp/sdl_libretro_test_saves");
    SDL_Libretro_SetSystemDirectory(lr, "/tmp/sdl_libretro_test_system");
    SDLTest_AssertCheck(SDL_GetPathInfo("/tmp/sdl_libretro_test_saves", NULL) == false, "Save directory absent before LoadCore");
    SDLTest_AssertCheck(SDL_GetPathInfo("/tmp/sdl_libretro_test_system", NULL) == false, "System directory absent before LoadCore");

    SDLTest_AssertCheck(SDL_Libretro_LoadCore(lr, TEST_CORE_PATH) == true, "LoadCore succeeds for test_core");
    SDLTest_AssertCheck(SDL_GetPathInfo("/tmp/sdl_libretro_test_saves", NULL) == true, "LoadCore creates the save directory");
    SDLTest_AssertCheck(SDL_GetPathInfo("/tmp/sdl_libretro_test_system", NULL) == true, "LoadCore creates the system directory");
    SDLTest_AssertCheck(SDL_Libretro_IsCoreReady(lr) == true, "Core is ready after load");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetCoreName(lr), "test_core") == 0, "Core name is test_core");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetCoreVersion(lr), "1.0") == 0, "Core version is 1.0");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetValidExtensions(lr), "txt|vfs|cue") == 0, "Valid extensions is txt|vfs|cue");
    SDLTest_AssertCheck(SDL_Libretro_GetPerformanceLevel(lr) == 2, "Performance level reported by core is 2");
    SDLTest_AssertCheck(SDL_Libretro_GetPerformanceLevel(NULL) == 0, "GetPerformanceLevel(NULL) 0");
    SDLTest_AssertCheck(SDL_Libretro_GetSavestateContext(lr) == RETRO_SAVESTATE_CONTEXT_NORMAL, "Savestate context defaults to NORMAL");
    SDL_Libretro_SetSavestateContext(lr, RETRO_SAVESTATE_CONTEXT_RUNAHEAD_SAME_INSTANCE);
    SDLTest_AssertCheck(SDL_Libretro_GetSavestateContext(lr) == RETRO_SAVESTATE_CONTEXT_RUNAHEAD_SAME_INSTANCE, "Savestate context set to RUNAHEAD");
    SDL_Libretro_SetSavestateContext(lr, RETRO_SAVESTATE_CONTEXT_NORMAL);
    SDLTest_AssertCheck(SDL_Libretro_GetSavestateContext(lr) == RETRO_SAVESTATE_CONTEXT_NORMAL, "Savestate context restored to NORMAL");

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
    SDL_RemovePath("/tmp/sdl_libretro_test_saves");
    SDL_RemovePath("/tmp/sdl_libretro_test_system");
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_Subsystem(void *arg) {
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    SDLTest_AssertCheck(lr->core.subsystemCount == 1, "One subsystem registered");
    SDLTest_AssertCheck(SDL_Libretro_GetSubsystemById(lr, 999) == NULL, "GetSubsystemById unknown NULL");

    const struct retro_subsystem_info* sub = SDL_Libretro_GetSubsystemById(lr, 1);
    SDLTest_AssertCheck(sub != NULL, "GetSubsystemById(1) found");
    if (sub) {
        SDLTest_AssertCheck(SDL_strcmp(sub->desc, "Super Game Boy") == 0, "Subsystem desc matches");
        SDLTest_AssertCheck(SDL_strcmp(sub->ident, "sgb") == 0, "Subsystem ident matches");
        SDLTest_AssertCheck(sub->num_roms == 2, "Subsystem has 2 roms");
        SDLTest_AssertCheck(sub == SDL_Libretro_GetSubsystemByName(lr, "sgb"), "ByName matches ident");
        SDLTest_AssertCheck(sub == SDL_Libretro_GetSubsystemByName(lr, "Super Game Boy"), "ByName matches description");
        SDLTest_AssertCheck(SDL_Libretro_GetSubsystemByName(lr, "nope") == NULL, "GetSubsystemByName unknown NULL");
        SDLTest_AssertCheck(SDL_Libretro_GetSubsystemByName(lr, NULL) == NULL, "GetSubsystemByName NULL NULL");

        // First ROM carries a single memory descriptor (srm / 0x100).
        SDLTest_AssertCheck(sub->roms[0].required == true, "Rom 0 required");
        SDLTest_AssertCheck(sub->roms[0].need_fullpath == false, "Rom 0 need_fullpath false");
        SDLTest_AssertCheck(sub->roms[0].num_memory == 1, "Rom 0 has 1 memory region");
        if (sub->roms[0].num_memory == 1) {
            SDLTest_AssertCheck(SDL_strcmp(sub->roms[0].memory[0].extension, "srm") == 0, "Rom 0 memory extension srm");
            SDLTest_AssertCheck(sub->roms[0].memory[0].type == 0x100, "Rom 0 memory type 0x100");
        }

        // Second ROM has no memory descriptors.
        SDLTest_AssertCheck(sub->roms[1].need_fullpath == true, "Rom 1 need_fullpath true");
        SDLTest_AssertCheck(sub->roms[1].num_memory == 0, "Rom 1 has no memory regions");
        SDLTest_AssertCheck(sub->roms[1].memory == NULL, "Rom 1 memory NULL");
    }

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
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "LoadGame succeeds with test content");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game is ready after load");

    int w = 0, h = 0;
    SDL_Libretro_GetSize(lr, &w, &h);
    SDLTest_AssertCheck(w == 320 && h == 240, "Core reports 320x240, got %dx%d", w, h);
    SDLTest_AssertCheck(SDL_Libretro_GetFPS(lr) == 60.0, "FPS is 60.0");
    SDLTest_AssertCheck(SDL_Libretro_GetTexture(lr) != NULL, "Texture created");
    SDLTest_AssertCheck(SDL_Libretro_GetStateSize(lr) == 128, "Serialize size is 128");

    SDL_Libretro_Update(lr);
    SDL_Libretro_Update(lr);

    SDLTest_AssertCheck(SDL_Libretro_SaveState(lr, "test_state.sav") == true, "SaveState succeeds");
    SDLTest_AssertCheck(SDL_Libretro_LoadState(lr, "test_state.sav") == true, "LoadState succeeds");

    // Texture scale mode applies to the live texture and reads back.
    SDLTest_AssertCheck(SDL_Libretro_SetScaleMode(NULL, SDL_SCALEMODE_LINEAR) == false, "SetScaleMode(NULL) fails");
    SDLTest_AssertCheck(SDL_Libretro_SetScaleMode(lr, SDL_SCALEMODE_LINEAR) == true, "SetScaleMode(LINEAR) succeeds");
    SDLTest_AssertCheck(SDL_Libretro_GetScaleMode(lr) == SDL_SCALEMODE_LINEAR, "GetScaleMode returns LINEAR");

    // Rumble persists across frames until the core stops it, and is cleared on unload.
    struct retro_rumble_interface rumble = {0};
    SDLTest_AssertCheck(SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble) && rumble.set_rumble_state, "Rumble interface provided");
    if (rumble.set_rumble_state) {
        rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, 65535);
        SDL_Libretro_Update(lr);
        SDLTest_AssertCheck(lr->core.rumbleStrong[0] == 1.0f, "Rumble kept across frames");
        rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, 0);
        SDLTest_AssertCheck(lr->core.rumbleStrong[0] == 0.0f, "Rumble stopped by core");
        rumble.set_rumble_state(0, RETRO_RUMBLE_WEAK, 32768);
        SDL_Libretro_UnloadGame(lr);
        SDLTest_AssertCheck(lr->core.rumbleWeak[0] == 0.0f, "Rumble cleared on UnloadGame");
    }

    // get_time_usec reports microseconds on the SDL_GetTicksNS() clock.
    struct retro_perf_callback perf = {0};
    SDLTest_AssertCheck(SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf) && perf.get_time_usec, "Perf interface provided");
    if (perf.get_time_usec) {
        retro_time_t before = (retro_time_t)(SDL_GetTicksNS() / 1000);
        retro_time_t usec = perf.get_time_usec();
        retro_time_t after = (retro_time_t)(SDL_GetTicksNS() / 1000);
        SDLTest_AssertCheck(usec >= before && usec <= after, "get_time_usec matches SDL_GetTicksNS");
    }

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

// Records the deltas the frame-time callback receives.
static retro_usec_t test_frameTimes[16];
static int test_frameTimeCount;

static void RETRO_CALLCONV test_FrameTimeCallback(retro_usec_t usec) {
    if (test_frameTimeCount < (int)SDL_arraysize(test_frameTimes)) {
        test_frameTimes[test_frameTimeCount] = usec;
    }
    test_frameTimeCount++;
}

static bool test_FrameTimesAll(retro_usec_t expected) {
    if (test_frameTimeCount <= 0) return false;
    for (int i = 0; i < test_frameTimeCount && i < (int)SDL_arraysize(test_frameTimes); i++) {
        if (test_frameTimes[i] != expected) return false;
    }
    return true;
}

static int SDLCALL test_FrameTime(void *arg) {
#if !defined(TEST_CORE_PATH) || !defined(TEST_CONTENT_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH or TEST_CONTENT_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "LoadGame succeeds with test content");

    // A reference deliberately unlike 1/60s, so it can't be confused with the idealized frame period.
    const retro_usec_t coreReference = 12345;
    struct retro_frame_time_callback ftcb = { test_FrameTimeCallback, coreReference };
    SDLTest_AssertCheck(SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK, &ftcb), "SET_FRAME_TIME_CALLBACK succeeds");

    // First tick has no measured cadence: the core's reference.
    test_frameTimeCount = 0;
    SDL_Libretro_Update(lr);
    SDLTest_AssertCheck(test_frameTimeCount == 1 && test_frameTimes[0] == coreReference,
        "First tick reports the core reference, got %d calls, %lld", test_frameTimeCount, (long long)test_frameTimes[0]);

    // At 1x, a 50ms stall runs two catch-up ticks that share the measured time.
    test_frameTimeCount = 0;
    SDL_DelayNS(50 * SDL_NS_PER_MS);
    SDL_Libretro_Update(lr);
    SDLTest_AssertCheck(test_frameTimeCount == 2, "50ms stall runs two catch-up ticks, got %d", test_frameTimeCount);
    SDLTest_AssertCheck(test_frameTimeCount >= 1 && test_frameTimes[0] >= 24000 && test_frameTimes[0] < 1000000,
        "Catch-up ticks report their share of the measured time, got %lld", (long long)test_frameTimes[0]);

    // Off 1x speed the idealized per-tick period is reported.
    SDL_Libretro_SetSpeed(lr, 2.0f);
    test_frameTimeCount = 0;
    SDL_DelayNS(20 * SDL_NS_PER_MS);
    SDL_Libretro_Update(lr);
    retro_usec_t fastReference = (retro_usec_t)((1.0 / 60.0) * 1.0e6 / 2.0);
    SDLTest_AssertCheck(test_FrameTimesAll(fastReference), "Fast-forward reports the idealized period %lld, got %d calls, first %lld",
        (long long)fastReference, test_frameTimeCount, (long long)test_frameTimes[0]);

    // Pausing restarts the clock, so the pause isn't reported as one long frame.
    SDL_Libretro_SetSpeed(lr, 0.0f);
    SDL_Libretro_Update(lr);
    SDL_Libretro_SetSpeed(lr, 1.0f);
    test_frameTimeCount = 0;
    SDL_DelayNS(30 * SDL_NS_PER_MS);
    SDL_Libretro_Update(lr);
    SDLTest_AssertCheck(test_FrameTimesAll(coreReference), "First tick after a pause reports the core reference");

    // A reset restarts the clock too.
    SDLTest_AssertCheck(SDL_Libretro_Reset(lr), "Reset succeeds");
    test_frameTimeCount = 0;
    SDL_DelayNS(30 * SDL_NS_PER_MS);
    SDL_Libretro_Update(lr);
    SDLTest_AssertCheck(test_FrameTimesAll(coreReference), "First tick after a reset reports the core reference");

    // Once running, a single 1x tick reports the measured time, not the idealized 16666us.
    test_frameTimeCount = 0;
    SDL_DelayNS(16 * SDL_NS_PER_MS);
    SDL_Libretro_Update(lr);
    SDLTest_AssertCheck(test_frameTimeCount >= 1 && test_frameTimes[0] >= 16000 && test_frameTimes[0] != 16666,
        "1x tick reports measured time, got %d calls, %lld", test_frameTimeCount, (long long)test_frameTimes[0]);

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
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH);

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
        SDLTest_AssertCheck(probe[18] == 1, "GET_INPUT_MAX_USERS supported");
        SDLTest_AssertCheck(probe[19] == SDL_LIBRETRO_MAX_USERS,
            "GET_INPUT_MAX_USERS reports SDL_LIBRETRO_MAX_USERS (%d), got %d",
            SDL_LIBRETRO_MAX_USERS, (int)probe[19]);
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
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // The test core opts into no-content, so a NULL load runs.
    SDLTest_AssertCheck(SDL_Libretro_IsGameRequired(lr) == false, "Core does not require content");
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, NULL) == true, "LoadGame succeeds with NULL content");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready with no content");

    SDL_Libretro_Update(lr);
    SDL_Libretro_UnloadGame(lr);

    // A core that requires content must reject a NULL load (and not disturb state).
    lr->core.supportNoGame = false;
    SDLTest_AssertCheck(SDL_Libretro_IsGameRequired(lr) == true, "Core now reports content required");
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, NULL) == false, "LoadGame(NULL) rejected when content is required");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == false, "Game not ready after a rejected NULL load");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_LoadGameSpecial(void *arg) {
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // The "sgb" subsystem expects two ROMs. ROM 0 (need_fullpath=false) is read
    // into memory by the frontend; ROM 1 (need_fullpath=true) is passed by path.
    const char* rom0 = "test_sgb_rom0.gb";
    const char* rom1 = "test_sgb_rom1.sfc";
    const unsigned char rom0Bytes[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    SDL_IOStream* io0 = SDL_IOFromFile(rom0, "wb");
    SDL_WriteIO(io0, rom0Bytes, sizeof(rom0Bytes));
    SDL_CloseIO(io0);
    SDL_IOStream* io1 = SDL_IOFromFile(rom1, "wb");
    SDL_WriteIO(io1, "bios", 4);
    SDL_CloseIO(io1);

    const char* paths[] = { rom0, rom1 };

    // Bad arguments and unknown subsystems are rejected without loading anything.
    SDLTest_AssertCheck(SDL_Libretro_LoadGameSpecial(lr, "nope", paths, 2) == false, "Unknown subsystem name rejected");
    SDLTest_AssertCheck(SDL_Libretro_LoadGameSpecialById(lr, 999, paths, 2) == false, "Unknown subsystem id rejected");
    SDLTest_AssertCheck(SDL_Libretro_LoadGameSpecial(lr, "sgb", NULL, 2) == false, "NULL paths rejected");
    SDLTest_AssertCheck(SDL_Libretro_LoadGameSpecial(lr, "sgb", paths, 0) == false, "Zero paths rejected");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == false, "No game loaded after rejected calls");

    // Load by ident.
    SDLTest_AssertCheck(SDL_Libretro_LoadGameSpecial(lr, "sgb", paths, 2) == true, "LoadGameSpecial by ident succeeds");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready after special load");
    // ROM 0 was loaded into memory and copied into SAVE_RAM by the core.
    size_t sramSize = 0;
    const unsigned char* sram = (const unsigned char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, &sramSize);
    SDLTest_AssertCheck(sram != NULL && sramSize >= sizeof(rom0Bytes), "SAVE_RAM available");
    if (sram && sramSize >= sizeof(rom0Bytes)) {
        SDLTest_AssertCheck(SDL_memcmp(sram, rom0Bytes, sizeof(rom0Bytes)) == 0, "ROM 0 bytes reached the core in memory");
    }
    // The core probed GET_GAME_INFO_EXT during the load; SYSTEM_RAM exposes the result.
    const unsigned char* probe = (const unsigned char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SYSTEM_RAM, NULL);
    SDLTest_AssertCheck(probe != NULL, "GET_GAME_INFO_EXT probe available");
    if (probe) {
        SDLTest_AssertCheck(SDL_strcmp((const char*)probe, "gb") == 0, "Core saw primary ROM ext 'gb', got '%s'", (const char*)probe);
        SDLTest_AssertCheck(probe[17] == 1, "Core saw non-NULL game info data");
    }
    SDL_Libretro_UnloadGame(lr);

    // Load by description resolves to the same subsystem.
    SDLTest_AssertCheck(SDL_Libretro_LoadGameSpecial(lr, "Super Game Boy", paths, 2) == true, "LoadGameSpecial by description succeeds");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready after description load");
    SDL_Libretro_UnloadGame(lr);

    // Load by numeric id.
    SDLTest_AssertCheck(SDL_Libretro_LoadGameSpecialById(lr, 1, paths, 2) == true, "LoadGameSpecialById succeeds");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready after id load");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_RemovePath(rom0);
    SDL_RemovePath(rom1);
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
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // A missing ".txt" file fails the load (the core wants the data buffer).
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, "definitely_missing.txt") == false,
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
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true,
        "LoadGame succeeds after a prior failure");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_LoadGameIO(void *arg) {
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // Invalid arguments are rejected (and the stream still closed).
    SDLTest_AssertCheck(SDL_Libretro_LoadGame_IO(lr, NULL, "x.txt", true) == false, "NULL stream rejected");
    const unsigned char bytes[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    SDL_IOStream* io = SDL_IOFromConstMem(bytes, sizeof(bytes));
    SDLTest_AssertCheck(SDL_Libretro_LoadGame_IO(lr, io, NULL, true) == false, "NULL path rejected");

    // The ".txt" content is byte-oriented, so the stream is handed to the
    // core as a data buffer; the path is only the content identity.
    io = SDL_IOFromConstMem(bytes, sizeof(bytes));
    SDLTest_AssertCheck(SDL_Libretro_LoadGame_IO(lr, io, "virtual_game.txt", true) == true,
        "LoadGame_IO succeeds from a memory stream");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready after IO load");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetContentExtension(lr), "txt") == 0,
        "Content extension comes from the virtual path");

    // The core copied the stream bytes into SAVE_RAM.
    size_t sramSize = 0;
    const unsigned char* sram = (const unsigned char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, &sramSize);
    SDLTest_AssertCheck(sram != NULL && sramSize >= sizeof(bytes), "SAVE_RAM available");
    if (sram && sramSize >= sizeof(bytes)) {
        SDLTest_AssertCheck(SDL_memcmp(sram, bytes, sizeof(bytes)) == 0, "Stream bytes reached the core");
    }

    // GET_GAME_INFO_EXT saw a data buffer with the txt override's persistence.
    const unsigned char* probe = (const unsigned char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SYSTEM_RAM, NULL);
    SDLTest_AssertCheck(probe != NULL, "GET_GAME_INFO_EXT probe available");
    if (probe) {
        SDLTest_AssertCheck(SDL_strcmp((const char*)probe, "txt") == 0, "Core saw ext 'txt', got '%s'", (const char*)probe);
        SDLTest_AssertCheck(probe[16] == 1, "Content-info override kept the buffer persistent");
        SDLTest_AssertCheck(probe[17] == 1, "Core received a data buffer");
    }

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_LoadGameIOFullpath(void *arg) {
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_SetSaveDirectory(lr, NULL);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    const char* saveDir = SDL_Libretro_GetSaveDirectory(lr);
    SDLTest_AssertCheck(saveDir != NULL, "Default save directory applied");

    // The ".vfs" override requires a full path, so the stream is spilled to
    // the "<save>/tmp" directory, loaded from there, and the file removed
    // again on unload. A file already at the natural name forces a
    // uniquified temp name, which must not leak into the content identity.
    char tmpDir[1024], keepPath[1024], spillPath[1024];
    SDL_snprintf(tmpDir, sizeof(tmpDir), "%s/tmp", saveDir ? saveDir : ".");
    SDL_snprintf(keepPath, sizeof(keepPath), "%s/test_io_content.vfs", tmpDir);
    SDL_snprintf(spillPath, sizeof(spillPath), "%s/test_io_content-1.vfs", tmpDir);
    SDL_CreateDirectory(tmpDir);
    SDL_RemovePath(spillPath);
    SDLTest_AssertCheck(SDL_SaveFile(keepPath, "KEEP", 4) == true, "Pre-existing temp file created");

    const unsigned char bytes[] = { 'V', 'F', 'S', 'D', 'A', 'T', 'A', '1' };
    SDL_IOStream* io = SDL_IOFromConstMem(bytes, sizeof(bytes));
    SDLTest_AssertCheck(SDL_Libretro_LoadGame_IO(lr, io, "some/dir/test_io_content.vfs", true) == true,
        "LoadGame_IO spills need_fullpath content to disk");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready after spilled load");
    SDL_PathInfo info;
    SDLTest_AssertCheck(SDL_GetPathInfo(spillPath, &info) == true, "Spilled file exists in the temp directory while loaded");
    SDLTest_AssertCheck(SDL_strcmp(lr->core.contentTempPath, spillPath) == 0,
        "Temp path is '%s', got '%s'", spillPath, lr->core.contentTempPath);
    SDLTest_AssertCheck(SDL_strcmp(lr->core.contentPath, spillPath) == 0, "Core was handed the temp file");

    // The content identity comes from the path the caller gave.
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetGameName(lr), "test_io_content") == 0,
        "Game name from the original path, got '%s'", SDL_Libretro_GetGameName(lr));
    SDLTest_AssertCheck(SDL_strcmp(lr->core.contentDir, "some/dir") == 0,
        "Content dir from the original path, got '%s'", lr->core.contentDir);
    char srmPath[1024], expectedSrm[1024];
    SDL_Libretro_GetSavePath(lr, ".srm", srmPath, sizeof(srmPath));
    SDL_snprintf(expectedSrm, sizeof(expectedSrm), "%s/test_io_content.srm", saveDir ? saveDir : ".");
    SDLTest_AssertCheck(SDL_strcmp(srmPath, expectedSrm) == 0, "Save path is '%s', got '%s'", expectedSrm, srmPath);

    // The core received a path (no data buffer) and read it back through the VFS.
    const unsigned char* probe = (const unsigned char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SYSTEM_RAM, NULL);
    SDLTest_AssertCheck(probe != NULL, "GET_GAME_INFO_EXT probe available");
    if (probe) {
        SDLTest_AssertCheck(SDL_strcmp((const char*)probe, "vfs") == 0, "Core saw ext 'vfs', got '%s'", (const char*)probe);
        SDLTest_AssertCheck(probe[17] == 0, "Core received a path, not a data buffer");
    }
#ifndef SDL_LIBRETRO_DISABLE_VFS
    size_t sramSize = 0;
    const unsigned char* sram = (const unsigned char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, &sramSize);
    SDLTest_AssertCheck(sram != NULL && sramSize >= sizeof(bytes), "SAVE_RAM available");
    if (sram && sramSize >= sizeof(bytes)) {
        SDLTest_AssertCheck(SDL_memcmp(sram, bytes, sizeof(bytes)) == 0, "Core read the spilled file via the VFS");
    }
#endif

    SDL_Libretro_UnloadGame(lr);
    SDLTest_AssertCheck(SDL_GetPathInfo(spillPath, &info) == false, "Spilled file removed on unload");
    SDLTest_AssertCheck(SDL_GetPathInfo(keepPath, &info) == true, "Pre-existing temp file untouched");
    SDL_RemovePath(keepPath);

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

#ifdef TEST_CONTENT_ZIP_PATH
static int SDLCALL test_PhysFSLoadGameZip(void *arg) {
#if !defined(TEST_CORE_PATH) || !defined(TEST_CONTENT_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH or TEST_CONTENT_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // The zip's base name matches the .txt inside; the loader mounts the
    // archive, picks it, and hands the extracted bytes to the core.
    SDLTest_AssertCheck(SDL_Libretro_PhysFS_LoadGame(lr, TEST_CONTENT_ZIP_PATH) == true,
        "PhysFS_LoadGame succeeds with a zip");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready after zip load");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetContentExtension(lr), "txt") == 0,
        "Content extension is the file inside the zip, got '%s'", SDL_Libretro_GetContentExtension(lr));

    // The core received the file's bytes, not the archive's.
    size_t expectedSize = 0;
    unsigned char* expected = (unsigned char*)SDL_LoadFile(TEST_CONTENT_PATH, &expectedSize);
    size_t sramSize = 0;
    const unsigned char* sram = (const unsigned char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, &sramSize);
    SDLTest_AssertCheck(expected != NULL && sram != NULL && sramSize >= expectedSize, "SAVE_RAM and expected content available");
    if (expected && sram && sramSize >= expectedSize) {
        SDLTest_AssertCheck(SDL_memcmp(sram, expected, expectedSize) == 0, "Extracted bytes reached the core");
    }
    SDL_free(expected);

    SDL_Libretro_Update(lr);
    SDL_Libretro_Update(lr);

    SDL_Libretro_PhysFS_Quit(lr);
    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_PhysFSLoadGameZipFullpath(void *arg) {
#if !defined(TEST_CORE_PATH) || !defined(TEST_FULLPATH_ZIP_PATH) || !defined(TEST_CONTENT_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH, TEST_FULLPATH_ZIP_PATH, or TEST_CONTENT_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // The zip holds a ".vfs" file, which requires a full path: the loader
    // passes the virtual path and the core reads it through the
    // PhysFS-backed VFS straight out of the mounted archive.
    SDLTest_AssertCheck(SDL_Libretro_PhysFS_LoadGame(lr, TEST_FULLPATH_ZIP_PATH) == true,
        "PhysFS_LoadGame succeeds with a need_fullpath zip");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready after fullpath zip load");
    SDLTest_AssertCheck(lr->core.usedVFS == true, "Core requested the VFS early");
    SDLTest_AssertCheck(lr->core.contentTempPath[0] == '\0',
        "Content served by its virtual path, not spilled to disk");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetContentExtension(lr), "vfs") == 0,
        "Content extension is the file inside the zip, got '%s'", SDL_Libretro_GetContentExtension(lr));

    const unsigned char* probe = (const unsigned char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SYSTEM_RAM, NULL);
    SDLTest_AssertCheck(probe != NULL, "GET_GAME_INFO_EXT probe available");
    if (probe) {
        SDLTest_AssertCheck(probe[17] == 0, "Core received a path, not a data buffer");
    }

    // The core read the virtual path through the VFS; the bytes match the
    // file that was zipped up.
    size_t expectedSize = 0;
    unsigned char* expected = (unsigned char*)SDL_LoadFile(TEST_CONTENT_PATH, &expectedSize);
    size_t sramSize = 0;
    const unsigned char* sram = (const unsigned char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, &sramSize);
    SDLTest_AssertCheck(expected != NULL && sram != NULL && sramSize >= expectedSize, "SAVE_RAM and expected content available");
    if (expected && sram && sramSize >= expectedSize) {
        SDLTest_AssertCheck(SDL_memcmp(sram, expected, expectedSize) == 0, "Core read the zipped file via the PhysFS VFS");
    }
    SDL_free(expected);

    SDL_Libretro_PhysFS_Quit(lr);
    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_PhysFSBlockExtract(void *arg) {
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);

    SDLTest_AssertCheck(SDL_Libretro_GetBlockExtract(NULL) == false, "GetBlockExtract(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_GetBlockExtract(lr) == false, "GetBlockExtract false with no core");

    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDLTest_AssertCheck(SDL_Libretro_GetBlockExtract(lr) == false, "Test core does not block extraction");

    // A block_extract core receives the raw archive, not the contents.
    lr->core.blockExtract = true;
    SDLTest_AssertCheck(SDL_Libretro_GetBlockExtract(lr) == true, "GetBlockExtract reflects the core flag");
    SDLTest_AssertCheck(SDL_Libretro_PhysFS_LoadGame(lr, TEST_CONTENT_ZIP_PATH) == true,
        "PhysFS_LoadGame with block_extract succeeds");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetContentExtension(lr), "zip") == 0,
        "Content extension stays 'zip', got '%s'", SDL_Libretro_GetContentExtension(lr));
    size_t sramSize = 0;
    const unsigned char* sram = (const unsigned char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, &sramSize);
    SDLTest_AssertCheck(sram != NULL && sramSize >= 2, "SAVE_RAM available");
    if (sram && sramSize >= 2) {
        SDLTest_AssertCheck(sram[0] == 'P' && sram[1] == 'K', "Core received the raw zip (PK magic)");
    }

    SDL_Libretro_PhysFS_Quit(lr);
    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_PhysFSNonZip(void *arg) {
#if !defined(TEST_CORE_PATH) || !defined(TEST_CONTENT_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH or TEST_CONTENT_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // Non-archive content passes straight through to the plain loader.
    SDLTest_AssertCheck(SDL_Libretro_PhysFS_LoadGame(lr, TEST_CONTENT_PATH) == true,
        "PhysFS_LoadGame passes non-zip content through");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetContentExtension(lr), "txt") == 0,
        "Content extension is 'txt', got '%s'", SDL_Libretro_GetContentExtension(lr));
    SDL_Libretro_UnloadGame(lr);

    // So does a content-less load.
    SDLTest_AssertCheck(SDL_Libretro_PhysFS_LoadGame(lr, NULL) == true,
        "PhysFS_LoadGame passes NULL content through");

    SDL_Libretro_PhysFS_Quit(lr);
    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_PhysFSPickPriority(void *arg) {
#if !defined(TEST_CORE_PATH) || !defined(TEST_PRIORITY_ZIP_PATH) || !defined(TEST_DISC_ZIP_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH, TEST_PRIORITY_ZIP_PATH, or TEST_DISC_ZIP_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // test_priority.zip holds "aaa.txt" and "test_priority.txt"; the entry
    // whose base name matches the archive's must win over the other
    // extension-valid candidate.
    SDLTest_AssertCheck(SDL_Libretro_PhysFS_LoadGame(lr, TEST_PRIORITY_ZIP_PATH) == true,
        "PhysFS_LoadGame succeeds with the priority zip");
    size_t sramSize = 0;
    const char* sram = (const char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, &sramSize);
    SDLTest_AssertCheck(sram != NULL && sramSize >= 13, "SAVE_RAM available");
    if (sram && sramSize >= 13) {
        SDLTest_AssertCheck(SDL_memcmp(sram, "PRIORITY PICK", 13) == 0,
            "Base-name match beats the other candidate, got '%.13s'", sram);
    }

    // test_disc.zip holds "data.txt" and "disc.cue"; the core supports "cue",
    // so the disc metadata must win over the plain candidate.
    SDLTest_AssertCheck(SDL_Libretro_PhysFS_LoadGame(lr, TEST_DISC_ZIP_PATH) == true,
        "PhysFS_LoadGame succeeds with the disc zip");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetContentExtension(lr), "cue") == 0,
        "Cue sheet picked over the plain candidate, got '%s'", SDL_Libretro_GetContentExtension(lr));
    sram = (const char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, &sramSize);
    SDLTest_AssertCheck(sram != NULL && sramSize >= 14, "SAVE_RAM available");
    if (sram && sramSize >= 14) {
        SDLTest_AssertCheck(SDL_memcmp(sram, "CUE SHEET PICK", 14) == 0,
            "Cue sheet bytes reached the core, got '%.14s'", sram);
    }

    SDL_Libretro_PhysFS_Quit(lr);
    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_PhysFSNestedDir(void *arg) {
#if !defined(TEST_CORE_PATH) || !defined(TEST_NESTED_ZIP_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH or TEST_NESTED_ZIP_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // test_nested.zip only holds "sub/dir/nested.txt"; the recursive search
    // must find it.
    SDLTest_AssertCheck(SDL_Libretro_PhysFS_LoadGame(lr, TEST_NESTED_ZIP_PATH) == true,
        "PhysFS_LoadGame finds content in a subdirectory");
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetContentExtension(lr), "txt") == 0,
        "Content extension is 'txt', got '%s'", SDL_Libretro_GetContentExtension(lr));
    size_t sramSize = 0;
    const char* sram = (const char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, &sramSize);
    SDLTest_AssertCheck(sram != NULL && sramSize >= 11, "SAVE_RAM available");
    if (sram && sramSize >= 11) {
        SDLTest_AssertCheck(SDL_memcmp(sram, "NESTED PICK", 11) == 0,
            "Nested bytes reached the core, got '%.11s'", sram);
    }

    SDL_Libretro_PhysFS_Quit(lr);
    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_PhysFSLoadSecondZip(void *arg) {
#if !defined(TEST_CORE_PATH) || !defined(TEST_SECOND_ZIP_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH or TEST_SECOND_ZIP_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // Load zip A, then zip B on the same core without unloading in between.
    SDLTest_AssertCheck(SDL_Libretro_PhysFS_LoadGame(lr, TEST_CONTENT_ZIP_PATH) == true,
        "First zip loads");
    SDLTest_AssertCheck(SDL_strcmp(lr->physfsMountSource, TEST_CONTENT_ZIP_PATH) == 0,
        "Mount source is the first zip");

    // B holds need_fullpath content served by its virtual path, so the core
    // can only read it if B is still mounted when retro_load_game runs.
    SDLTest_AssertCheck(SDL_Libretro_PhysFS_LoadGame(lr, TEST_SECOND_ZIP_PATH) == true,
        "Second zip loads over the first");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready after second zip load");
    SDLTest_AssertCheck(SDL_strcmp(lr->physfsMountSource, TEST_SECOND_ZIP_PATH) == 0,
        "Mount source is the second zip, got '%s'", lr->physfsMountSource);

    const char expected[] = "SECOND ZIP PICK";
    size_t sramSize = 0;
    const unsigned char* sram = (const unsigned char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, &sramSize);
    SDLTest_AssertCheck(sram != NULL && sramSize >= sizeof(expected) - 1, "SAVE_RAM available");
    if (sram && sramSize >= sizeof(expected) - 1) {
        SDLTest_AssertCheck(SDL_memcmp(sram, expected, sizeof(expected) - 1) == 0,
            "Core read the second zip's file via the PhysFS VFS");
    }

    SDL_Libretro_PhysFS_Quit(lr);
    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_PhysFSSpillFallback(void *arg) {
#if !defined(TEST_CORE_NOVFS_PATH) || !defined(TEST_FULLPATH_ZIP_PATH) || !defined(TEST_CONTENT_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_NOVFS_PATH, TEST_FULLPATH_ZIP_PATH, or TEST_CONTENT_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);

    // Opt into the default save directory (<pref>/saves) before loading the
    // core, so LoadCore creates it; the spill lands there.
    SDL_Libretro_SetSaveDirectory(lr, NULL);
    const char* saveDir = SDL_Libretro_GetSaveDirectory(lr);
    SDLTest_AssertCheck(saveDir != NULL, "SetSaveDirectory(NULL) applies the pref-path default");

    // This core build skips the early GET_VFS_INTERFACE probe, so the loader
    // must not hand it a virtual path.
    SDL_Libretro_LoadCore(lr, TEST_CORE_NOVFS_PATH);
    SDLTest_AssertCheck(lr->core.usedVFS == false, "Core has not requested the VFS before load");
    SDLTest_AssertCheck(SDL_GetPathInfo(saveDir, NULL) == true, "LoadCore created the default save directory");

    // A file already sitting at the spill's natural name must survive the
    // load/unload cycle untouched. Clear any leftover from a crashed prior
    // run first so the uniquified name is deterministic.
    char tmpDir[1024], keepPath[1024], uniquePath[1024];
    SDL_snprintf(tmpDir, sizeof(tmpDir), "%s/tmp", saveDir);
    SDL_snprintf(keepPath, sizeof(keepPath), "%s/game.vfs", tmpDir);
    SDL_snprintf(uniquePath, sizeof(uniquePath), "%s/game-1.vfs", tmpDir);
    SDL_CreateDirectory(tmpDir);
    SDL_RemovePath(uniquePath);
    SDLTest_AssertCheck(SDL_SaveFile(keepPath, "KEEP", 4) == true, "Pre-existing 'game.vfs' created");

    SDLTest_AssertCheck(SDL_Libretro_PhysFS_LoadGame(lr, TEST_FULLPATH_ZIP_PATH) == true,
        "PhysFS_LoadGame succeeds for a full-path core that doesn't use the VFS");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready after spill-fallback load");
    SDLTest_AssertCheck(lr->core.contentTempPath[0] != '\0',
        "Content was spilled to a real file, not served virtually");
    SDLTest_AssertCheck(SDL_strcmp(lr->core.contentTempPath, uniquePath) == 0,
        "Spill avoided the pre-existing 'game.vfs': %s", lr->core.contentTempPath);
    SDLTest_AssertCheck(SDL_strcmp(SDL_Libretro_GetGameName(lr), "game") == 0,
        "Game name ignores the uniquified temp name, got '%s'", SDL_Libretro_GetGameName(lr));

    // The core read the spilled file; the bytes match the file that was zipped.
    size_t expectedSize = 0;
    unsigned char* expected = (unsigned char*)SDL_LoadFile(TEST_CONTENT_PATH, &expectedSize);
    size_t sramSize = 0;
    const unsigned char* sram = (const unsigned char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, &sramSize);
    SDLTest_AssertCheck(expected != NULL && sram != NULL && sramSize >= expectedSize, "SAVE_RAM and expected content available");
    if (expected && sram && sramSize >= expectedSize) {
        SDLTest_AssertCheck(SDL_memcmp(sram, expected, expectedSize) == 0, "Core read the spilled file");
    }
    SDL_free(expected);

    SDL_Libretro_UnloadGame(lr);
    SDLTest_AssertCheck(SDL_GetPathInfo(uniquePath, NULL) == false, "Spilled file removed on unload");

    size_t keepSize = 0;
    char* keep = (char*)SDL_LoadFile(keepPath, &keepSize);
    SDLTest_AssertCheck(keep != NULL && keepSize == 4 && SDL_memcmp(keep, "KEEP", 4) == 0,
        "Pre-existing 'game.vfs' untouched");
    SDL_free(keep);
    SDL_RemovePath(keepPath);

    SDL_Libretro_PhysFS_Quit(lr);
    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}
#endif /* TEST_CONTENT_ZIP_PATH */

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
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDL_Libretro_LoadGame(lr, NULL);

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
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_SetSaveDirectory(lr, "test_saves");
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH);

    SDLTest_AssertCheck(SDL_Libretro_GetSavePath(lr, ".srm", path, sizeof(path)) > 0, "GetSavePath derives a path with content");
    SDLTest_AssertCheck(SDL_strstr(path, "test_saves") != NULL && SDL_strstr(path, ".srm") != NULL, "Derived path uses save dir and extension: %s", path);
    SDL_RemovePath("test_saves"); // created by SetSaveDirectory above

    // LoadGame again should unload the existing game and reload cleanly.
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "LoadGame again succeeds (reloads over existing game)");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready after reload");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
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

static int SDLCALL test_LoadGameNoRenderer(void *arg) {
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);

    // Load a game with NO renderer set: the game runs and video is deferred.
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    SDLTest_AssertCheck(SDL_Libretro_GetRenderer(lr) == NULL, "No renderer set initially");
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, NULL) == true, "LoadGame succeeds without a renderer");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr) == true, "Game ready after renderer-less load");
    SDLTest_AssertCheck(SDL_Libretro_GetTexture(lr) == NULL, "No texture until a renderer is set");
    SDLTest_AssertCheck(SDL_Libretro_Render(NULL, lr, NULL) == false, "Render fails with a NULL renderer");

    // The core still runs with no video attached.
    SDL_Libretro_Update(lr);
    SDL_Libretro_Update(lr);

    // Rendering with a renderer adopts it and builds the texture (deferred init).
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDLTest_AssertCheck(SDL_Libretro_Render(renderer, lr, NULL) == true, "Render adopts the renderer and draws");
    SDLTest_AssertCheck(SDL_Libretro_GetRenderer(lr) == renderer, "Render stored the renderer");
    SDLTest_AssertCheck(SDL_Libretro_GetTexture(lr) != NULL, "Texture created via Render's deferred init");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

// Drive the pointer input callback for a rotation and assert POINTER_X/Y map as
// expected (within a rounding unit). renderDstRect and the mouse must be set first.
static void CheckPointer(SDL_Libretro* lr, int rot, int16_t expectX, int16_t expectY) {
    lr->core.rotation = rot;
    int16_t x = SDL_Libretro_InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
    int16_t y = SDL_Libretro_InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
    SDLTest_AssertCheck(SDL_abs(x - expectX) <= 2, "rot %d POINTER_X ~%d, got %d", rot, expectX, x);
    SDLTest_AssertCheck(SDL_abs(y - expectY) <= 2, "rot %d POINTER_Y ~%d, got %d", rot, expectY, y);
}

static void PushMouseMotion(SDL_Libretro* lr, float x, float y, float xrel, float yrel) {
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_EVENT_MOUSE_MOTION;
    e.motion.x = x;
    e.motion.y = y;
    e.motion.xrel = xrel;
    e.motion.yrel = yrel;
    SDL_Libretro_HandleEvent(lr, &e);
}

static int SDLCALL test_MouseMotion(void *arg) {
    (void)arg;
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // Relative motion accumulates between polls, even when the absolute
    // position doesn't change (pinned at a window edge or in relative mode).
    PushMouseMotion(lr, 0.0f, 0.0f, -5.0f, 3.0f);
    PushMouseMotion(lr, 0.0f, 0.0f, -7.0f, 4.0f);
    SDL_Libretro_InputPoll();
    int16_t dx = SDL_Libretro_InputState(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
    int16_t dy = SDL_Libretro_InputState(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
    SDLTest_AssertCheck(dx == -12, "MOUSE_X sums xrel, got %d", dx);
    SDLTest_AssertCheck(dy == 7, "MOUSE_Y sums yrel, got %d", dy);

    // Reading again within the same frame reports the same delta.
    dx = SDL_Libretro_InputState(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
    SDLTest_AssertCheck(dx == -12, "MOUSE_X stable within a frame, got %d", dx);

    // The next poll with no motion reports zero.
    SDL_Libretro_InputPoll();
    dx = SDL_Libretro_InputState(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
    dy = SDL_Libretro_InputState(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
    SDLTest_AssertCheck(dx == 0 && dy == 0, "No motion reports zero, got %d,%d", dx, dy);

    SDL_Libretro_Destroy(lr);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_MousePort(void *arg) {
    (void)arg;
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // Defaults, NULL handling, and bounds.
    SDLTest_AssertCheck(SDL_Libretro_GetMousePort(lr) == 0, "Mouse port defaults to 0");
    SDLTest_AssertCheck(SDL_Libretro_GetMousePort(NULL) == 0, "GetMousePort(NULL) reports 0");
    SDL_Libretro_SetMousePort(NULL, 1);
    SDL_Libretro_SetMousePort(lr, SDL_LIBRETRO_MAX_USERS);
    SDLTest_AssertCheck(SDL_Libretro_GetMousePort(lr) == 0, "SetMousePort rejects out-of-range ports");

    // With no render rect the pointer and lightgun report offscreen (1), and
    // relative motion drives MOUSE_X; only the mouse port answers.
    PushMouseMotion(lr, 0.0f, 0.0f, 9.0f, 0.0f);
    SDL_Libretro_InputPoll();
    SDLTest_AssertCheck(SDL_Libretro_InputState(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X) == 9, "Port 0 answers the mouse");
    SDLTest_AssertCheck(SDL_Libretro_InputState(0, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN) == 1, "Port 0 answers the lightgun");
    SDLTest_AssertCheck(SDL_Libretro_InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_IS_OFFSCREEN) == 1, "Port 0 answers the pointer");
    SDLTest_AssertCheck(SDL_Libretro_InputState(1, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X) == 0, "Port 1 ignores the mouse");
    SDLTest_AssertCheck(SDL_Libretro_InputState(1, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN) == 0, "Port 1 ignores the lightgun");
    SDLTest_AssertCheck(SDL_Libretro_InputState(1, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_IS_OFFSCREEN) == 0, "Port 1 ignores the pointer");

    // Moving the mouse to port 1 swaps which port answers.
    SDL_Libretro_SetMousePort(lr, 1);
    SDLTest_AssertCheck(SDL_Libretro_GetMousePort(lr) == 1, "SetMousePort applies");
    SDLTest_AssertCheck(SDL_Libretro_InputState(1, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X) == 9, "Port 1 now answers the mouse");
    SDLTest_AssertCheck(SDL_Libretro_InputState(1, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN) == 1, "Port 1 now answers the lightgun");
    SDLTest_AssertCheck(SDL_Libretro_InputState(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X) == 0, "Port 0 now ignores the mouse");
    SDLTest_AssertCheck(SDL_Libretro_InputState(0, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN) == 0, "Port 0 now ignores the lightgun");

    SDL_Libretro_Destroy(lr);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_Rotation(void *arg) {
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDL_Libretro_LoadGame(lr, NULL); // 320x240 (4:3) core, no content

    // --- SET_ROTATION clamps out-of-range values to 0-3 ---
    // The test core forwards (device - 0x1000) to SET_ROTATION; a rogue value
    // of 5 must be masked to 1 so GetRotation reports 90, not 450.
    SDL_Libretro_SetPortDevice(lr, 0, 0x1000 + 5);
    SDLTest_AssertCheck(SDL_Libretro_GetRotation(lr) == 90, "SET_ROTATION(5) clamps to 90 deg, got %d", SDL_Libretro_GetRotation(lr));
    SDL_Libretro_SetPortDevice(lr, 0, 0x1000 + 0);
    SDLTest_AssertCheck(SDL_Libretro_GetRotation(lr) == 0, "SET_ROTATION(0) restores 0 deg");

    // --- renderDstRect fit under rotation, into the full 320x240 output ---
    // 0deg: a 4:3 frame fills the 4:3 window.
    lr->core.rotation = 0;
    SDL_Libretro_Render(renderer, lr, NULL);
    SDL_FRect r0 = lr->core.renderDstRect;
    SDLTest_AssertCheck(SDL_fabsf(r0.w - 320.0f) < 1.0f && SDL_fabsf(r0.h - 240.0f) < 1.0f,
        "0deg fills 320x240, got %gx%g", r0.w, r0.h);

    // 90deg: the on-screen extents swap, so the fit is portrait (aspect inverted
    // to 3:4), centered and within the window bounds.
    lr->core.rotation = 1;
    SDL_Libretro_Render(renderer, lr, NULL);
    SDL_FRect r1 = lr->core.renderDstRect;
    SDLTest_AssertCheck(r1.w < r1.h, "90deg is portrait (w<h), got %gx%g", r1.w, r1.h);
    SDLTest_AssertCheck(SDL_fabsf(r1.w / r1.h - 0.75f) < 0.01f, "90deg aspect ~3:4, got %g", r1.w / r1.h);
    SDLTest_AssertCheck(r1.x >= 0.0f && r1.y >= 0.0f &&
        r1.x + r1.w <= 321.0f && r1.y + r1.h <= 241.0f, "90deg fit stays within the window");

    // --- POINTER_X/Y map through the inverse of the display rotation ---
    // Known on-screen rect and mouse at sx=0.25, sy=0.75.
    lr->core.renderDstRect = (SDL_FRect){ 0.0f, 0.0f, 100.0f, 200.0f };
    lr->core.inputMouseX = 25.0f;
    lr->core.inputMouseY = 150.0f;
    const int16_t LO = -16383, HI = 16383; // (0.25*2-1)*0x7FFF and (0.75*2-1)*0x7FFF
    CheckPointer(lr, 0, LO, HI); // u=sx=.25,   v=sy=.75
    CheckPointer(lr, 1, LO, LO); // u=1-sy=.25, v=sx=.25   (90deg CCW)
    CheckPointer(lr, 2, HI, LO); // u=1-sx=.75, v=1-sy=.25 (180deg)
    CheckPointer(lr, 3, HI, HI); // u=sy=.75,   v=1-sx=.75 (270deg CCW)

    // --- The mouse maps from window to render coordinates ---
    // A 160x120 logical presentation halves the 320x240 window, so the window
    // point (240,60) is render point (120,30): sx=.75, sy=.25 of the rect.
    SDL_SetRenderLogicalPresentation(renderer, 160, 120, SDL_LOGICAL_PRESENTATION_STRETCH);
    lr->core.renderDstRect = (SDL_FRect){ 0.0f, 0.0f, 160.0f, 120.0f };
    lr->core.inputMouseX = 240.0f;
    lr->core.inputMouseY = 60.0f;
    CheckPointer(lr, 0, HI, LO);

    // Rendering without a rect fits the logical size, not the output pixels.
    lr->core.rotation = 0;
    SDLTest_AssertCheck(SDL_Libretro_Render(renderer, lr, NULL), "Render with logical presentation");
    SDL_FRect logicalRect = lr->core.renderDstRect;
    SDLTest_AssertCheck(logicalRect.x >= 0.0f && logicalRect.y >= 0.0f &&
        logicalRect.x + logicalRect.w <= 160.0f && logicalRect.y + logicalRect.h <= 120.0f,
        "Render rect fits the 160x120 logical size, got %.0f,%.0f %.0fx%.0f",
        logicalRect.x, logicalRect.y, logicalRect.w, logicalRect.h);
    SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);

    // --- CreateSurface reads back the frame at its native size ---
    lr->core.rotation = 0;
    SDL_Libretro_Update(lr);
    SDL_Surface* shot = SDL_Libretro_CreateSurface(lr);
    SDLTest_AssertCheck(shot != NULL, "CreateSurface returns a surface");
    if (shot) {
        SDLTest_AssertCheck(shot->w == 320 && shot->h == 240,
            "CreateSurface is native size, got %dx%d", shot->w, shot->h);
        SDL_DestroySurface(shot);
    }

    // --- SaveScreenshot writes the frame as a PNG ---
    SDLTest_AssertCheck(SDL_Libretro_SaveScreenshot(NULL, NULL) == false, "SaveScreenshot(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_SaveScreenshot(lr, "/tmp/sdl_libretro_shot.png") == true,
        "SaveScreenshot writes a PNG");
    SDLTest_AssertCheck(SDL_GetPathInfo("/tmp/sdl_libretro_shot.png", NULL) == true, "Screenshot file exists");
    SDL_RemovePath("/tmp/sdl_libretro_shot.png");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_PortDeviceOnLoad(void *arg) {
    (void)arg;
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDLTest_AssertCheck(SDL_Libretro_LoadCore(lr, TEST_CORE_PATH) == true, "LoadCore succeeds");

    typedef unsigned (*GetPortDeviceFn)(unsigned);
    GetPortDeviceFn getDevice = (GetPortDeviceFn)SDL_LoadFunction(lr->core.symbols.handle, "test_core_get_port_device");
    SDLTest_AssertCheck(getDevice != NULL, "test_core exports test_core_get_port_device");
    if (!getDevice) {
        SDL_Libretro_Destroy(lr);
        return TEST_COMPLETED;
    }

    // Chosen before the game loads; the test core resets its devices in
    // retro_load_game(), so these only stick if the frontend re-applies them.
    SDL_Libretro_SetPortDevice(lr, 0, RETRO_DEVICE_ANALOG);
    SDL_Libretro_SetPortDevice(lr, 1, RETRO_DEVICE_MOUSE);
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "LoadGame succeeds");
    SDLTest_AssertCheck(getDevice(0) == RETRO_DEVICE_ANALOG, "Port 0 device applied on load, got %u", getDevice(0));
    SDLTest_AssertCheck(getDevice(1) == RETRO_DEVICE_MOUSE, "Port 1 device applied on load, got %u", getDevice(1));

    // Loading another game on the same core re-applies them again.
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "Second LoadGame succeeds");
    SDLTest_AssertCheck(getDevice(0) == RETRO_DEVICE_ANALOG, "Port 0 device re-applied on reload");
    SDLTest_AssertCheck(getDevice(1) == RETRO_DEVICE_MOUSE, "Port 1 device re-applied on reload");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_InputDescriptorCopy(void *arg) {
    (void)arg;
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDL_Libretro_LoadGame(lr, NULL);

    // The test core issues SET_INPUT_DESCRIPTORS from stack-allocated structs
    // and strings during retro_load_game, then scribbles over that stack.
    // Surviving text proves the frontend deep-copied it.
    SDLTest_AssertCheck(SDL_Libretro_GetInputDescriptorCount(lr) == 1, "One input descriptor registered");
    unsigned port = 99, device = 0, id = 0;
    const char* desc = NULL;
    SDLTest_AssertCheck(SDL_Libretro_GetInputDescriptor(lr, 0, &port, &device, &id, &desc) == true, "GetInputDescriptor(0) succeeds");
    SDLTest_AssertCheck(port == 0 && device == RETRO_DEVICE_JOYPAD && id == RETRO_DEVICE_ID_JOYPAD_UP,
        "Descriptor fields survive, got port=%u device=%u id=%u", port, device, id);
    SDLTest_AssertCheck(desc != NULL && SDL_strcmp(desc, "Stack Up") == 0,
        "Descriptor text deep-copied, got '%s'", desc ? desc : "(null)");

    // SET_CONTROLLER_INFO likewise deep-copies the types array and its strings.
    SDLTest_AssertCheck(lr->core.controllerInfoCount == 1, "One controller info entry");
    if (lr->core.controllerInfoCount == 1 && lr->core.controllerInfo[0].types != NULL) {
        SDLTest_AssertCheck(lr->core.controllerInfo[0].num_types == 1, "One controller type");
        SDLTest_AssertCheck(lr->core.controllerInfo[0].types[0].id == RETRO_DEVICE_JOYPAD, "Controller type id survives");
        SDLTest_AssertCheck(lr->core.controllerInfo[0].types[0].desc != NULL &&
            SDL_strcmp(lr->core.controllerInfo[0].types[0].desc, "Stack Pad") == 0,
            "Controller type desc deep-copied, got '%s'",
            lr->core.controllerInfo[0].types[0].desc ? lr->core.controllerInfo[0].types[0].desc : "(null)");
    } else {
        SDLTest_AssertCheck(false, "Controller info types missing");
    }

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_SetRenderer(void *arg) {
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* w1 = SDL_CreateWindow("test1", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* r1 = SDL_CreateRenderer(w1, NULL);
    SDL_Window* w2 = SDL_CreateWindow("test2", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* r2 = SDL_CreateRenderer(w2, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();

    // NULL is rejected for both the context and the renderer.
    SDLTest_AssertCheck(SDL_Libretro_SetRenderer(lr, NULL) == false, "SetRenderer(NULL renderer) rejected");
    SDLTest_AssertCheck(SDL_Libretro_SetRenderer(NULL, r1) == false, "SetRenderer(NULL context) rejected");

    SDLTest_AssertCheck(SDL_Libretro_SetRenderer(lr, r1) == true, "SetRenderer(r1) succeeds");
    SDLTest_AssertCheck(SDL_Libretro_GetRenderer(lr) == r1, "GetRenderer returns r1");

    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDL_Libretro_LoadGame(lr, NULL);
    SDLTest_AssertCheck(SDL_Libretro_GetTexture(lr) != NULL, "Texture built on r1");

    // Swapping renderers rebuilds the texture against the new renderer.
    SDLTest_AssertCheck(SDL_Libretro_SetRenderer(lr, r2) == true, "SetRenderer(r2) swap succeeds");
    SDLTest_AssertCheck(SDL_Libretro_GetRenderer(lr) == r2, "GetRenderer returns r2 after swap");
    SDLTest_AssertCheck(SDL_Libretro_GetTexture(lr) != NULL, "Texture rebuilt on r2");
    SDLTest_AssertCheck(SDL_Libretro_Render(r2, lr, NULL) == true, "Render works on the swapped renderer");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(r1);
    SDL_DestroyWindow(w1);
    SDL_DestroyRenderer(r2);
    SDL_DestroyWindow(w2);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_DiskImageIO(void *arg) {
    (void)arg;
#if !defined(TEST_CORE_PATH) || !defined(TEST_CONTENT_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH or TEST_CONTENT_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetSaveDirectory(lr, NULL);
    SDLTest_AssertCheck(SDL_Libretro_LoadCore(lr, TEST_CORE_PATH) == true, "LoadCore succeeds");
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "LoadGame succeeds");

    typedef bool (*GetDiskFn)(unsigned, char*, size_t, void*, size_t*);
    GetDiskFn getDisk = (GetDiskFn)SDL_LoadFunction(lr->core.symbols.handle, "test_core_get_disk");
    SDLTest_AssertCheck(getDisk != NULL, "test_core exports test_core_get_disk");
    const char* saveDir = SDL_Libretro_GetSaveDirectory(lr);
    if (!getDisk || !saveDir) {
        SDL_Libretro_Destroy(lr);
        return TEST_COMPLETED;
    }

    // Invalid arguments are rejected.
    const char bytes[] = "DISK IMAGE BYTES, LONGER THAN A MALLOC HEADER";
    SDLTest_AssertCheck(SDL_Libretro_AddDiskImage_IO(lr, NULL, "disk.txt", true) == false, "NULL stream rejected");
    SDLTest_AssertCheck(SDL_Libretro_AddDiskImage_IO(lr, SDL_IOFromConstMem(bytes, sizeof(bytes)), NULL, true) == false, "NULL path rejected");
    SDLTest_AssertCheck(SDL_Libretro_AddDiskImage_IO(lr, SDL_IOFromConstMem(bytes, sizeof(bytes)), "disk.txt", true) == false,
        "Rejected while the tray is closed");

    SDLTest_AssertCheck(SDL_Libretro_EjectDisk(lr) == true, "EjectDisk succeeds");
    unsigned base = SDL_Libretro_GetDiskCount(lr);

    // Byte-oriented image: the core gets the bytes and the caller's path, and
    // both stay valid after the call returns.
    SDLTest_AssertCheck(SDL_Libretro_AddDiskImage_IO(lr, SDL_IOFromConstMem(bytes, sizeof(bytes)), "virtual/disk2.txt", true) == true,
        "AddDiskImage_IO with a data image succeeds");
    SDLTest_AssertCheck(SDL_Libretro_GetDiskCount(lr) == base + 1, "Disk count grew by one");
    char path[1024];
    char data[128];
    size_t size = sizeof(data);
    SDL_memset(data, 0, sizeof(data));
    SDLTest_AssertCheck(getDisk(base, path, sizeof(path), data, &size) == true, "Core has the new disk");
    SDLTest_AssertCheck(SDL_strcmp(path, "virtual/disk2.txt") == 0, "Core got the path, got '%s'", path);
    SDLTest_AssertCheck(size == sizeof(bytes) && SDL_memcmp(data, bytes, sizeof(bytes)) == 0,
        "Core's data buffer is still valid after the call");

    // Full-path image: spilled to the temp directory, which the core gets.
    char tmpDir[1024], spillPath[1024];
    SDL_snprintf(tmpDir, sizeof(tmpDir), "%s/tmp/", saveDir);
    SDL_snprintf(spillPath, sizeof(spillPath), "%sdisk3.vfs", tmpDir);
    SDL_RemovePath(spillPath);
    SDLTest_AssertCheck(SDL_Libretro_AddDiskImage_IO(lr, SDL_IOFromConstMem(bytes, sizeof(bytes)), "virtual/disk3.vfs", true) == true,
        "AddDiskImage_IO with a full-path image succeeds");
    SDLTest_AssertCheck(SDL_Libretro_GetDiskCount(lr) == base + 2, "Disk count grew by two");
    size = sizeof(data);
    SDLTest_AssertCheck(getDisk(base + 1, path, sizeof(path), data, &size) == true, "Core has the full-path disk");
    SDLTest_AssertCheck(SDL_strcmp(path, spillPath) == 0, "Core got the temp file '%s', got '%s'", spillPath, path);
    SDLTest_AssertCheck(size == 0, "Full-path disk has no data buffer");
    size_t spilledSize = 0;
    void* spilled = SDL_LoadFile(spillPath, &spilledSize);
    SDLTest_AssertCheck(spilled != NULL && spilledSize == sizeof(bytes) && SDL_memcmp(spilled, bytes, sizeof(bytes)) == 0,
        "Temp file holds the image bytes");
    SDL_free(spilled);

    // A rejected image doesn't leave an empty slot behind, nor a temp file.
    SDLTest_AssertCheck(SDL_Libretro_AddDiskImage_IO(lr, SDL_IOFromConstMem(bytes, sizeof(bytes)), "reject.txt", true) == false,
        "Core-rejected data image fails");
    SDLTest_AssertCheck(SDL_Libretro_GetDiskCount(lr) == base + 2, "Rejected data image left no slot");
    char rejectSpill[1024];
    SDL_snprintf(rejectSpill, sizeof(rejectSpill), "%sreject.vfs", tmpDir);
    SDLTest_AssertCheck(SDL_Libretro_AddDiskImage_IO(lr, SDL_IOFromConstMem(bytes, sizeof(bytes)), "reject.vfs", true) == false,
        "Core-rejected full-path image fails");
    SDLTest_AssertCheck(SDL_Libretro_GetDiskCount(lr) == base + 2, "Rejected full-path image left no slot");
    SDLTest_AssertCheck(SDL_GetPathInfo(rejectSpill, NULL) == false, "Rejected image's temp file removed");
    SDLTest_AssertCheck(SDL_Libretro_AddDiskImage(lr, "reject.bin") == false, "Core-rejected path image fails");
    SDLTest_AssertCheck(SDL_Libretro_GetDiskCount(lr) == base + 2, "Rejected path image left no slot");

    // The temp file lives until the game unloads.
    SDL_Libretro_InsertDisk(lr);
    SDL_Libretro_UnloadGame(lr);
    SDLTest_AssertCheck(SDL_GetPathInfo(spillPath, NULL) == false, "Temp disk image removed on unload");
    SDLTest_AssertCheck(lr->core.diskImageCount == 0, "Disk images freed on unload");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_DiskControl(void *arg) {
    (void)arg;
    SDL_Libretro* lr = SDL_Libretro_Create();

    // No core loaded: all disk functions return safe defaults.
    SDLTest_AssertCheck(SDL_Libretro_GetDiskCount(lr) == 0, "GetDiskCount 0 without core");
    SDLTest_AssertCheck(SDL_Libretro_GetDiskIndex(lr) == 0, "GetDiskIndex 0 without core");
    SDLTest_AssertCheck(SDL_Libretro_EjectDisk(lr) == false, "EjectDisk false without core");
    SDLTest_AssertCheck(SDL_Libretro_InsertDisk(lr) == false, "InsertDisk false without core");
    SDLTest_AssertCheck(SDL_Libretro_SetDiskIndex(lr, 0) == false, "SetDiskIndex false without core");
    SDLTest_AssertCheck(SDL_Libretro_AddDiskImage(lr, "foo.bin") == false, "AddDiskImage false without core");

    // NULL safety.
    SDLTest_AssertCheck(SDL_Libretro_GetDiskCount(NULL) == 0, "GetDiskCount(NULL) 0");
    SDLTest_AssertCheck(SDL_Libretro_GetDiskIndex(NULL) == 0, "GetDiskIndex(NULL) 0");
    SDLTest_AssertCheck(SDL_Libretro_EjectDisk(NULL) == false, "EjectDisk(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_InsertDisk(NULL) == false, "InsertDisk(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_AddDiskImage(NULL, "foo.bin") == false, "AddDiskImage(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_AddDiskImage(lr, NULL) == false, "AddDiskImage(lr, NULL) false");

    // Load a core that provides disk control.
    SDLTest_AssertCheck(SDL_Libretro_LoadCore(lr, TEST_CORE_PATH) == true, "LoadCore succeeds");
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "LoadGame succeeds");

    // The test core starts with 1 disk.
    SDLTest_AssertCheck(SDL_Libretro_GetDiskCount(lr) == 1, "Initial disk count is 1");
    SDLTest_AssertCheck(SDL_Libretro_GetDiskIndex(lr) == 0, "Initial disk index is 0");

    // Can't change index without ejecting first.
    SDLTest_AssertCheck(SDL_Libretro_SetDiskIndex(lr, 0) == false, "SetDiskIndex fails while inserted");

    // Eject, then change index.
    SDLTest_AssertCheck(SDL_Libretro_EjectDisk(lr) == true, "EjectDisk succeeds");

    // Add a new disk image.
    SDLTest_AssertCheck(SDL_Libretro_AddDiskImage(lr, TEST_CONTENT_PATH) == true, "AddDiskImage succeeds");
    SDLTest_AssertCheck(SDL_Libretro_GetDiskCount(lr) == 2, "Disk count is now 2");

    // Switch to the new disk.
    SDLTest_AssertCheck(SDL_Libretro_SetDiskIndex(lr, 1) == true, "SetDiskIndex to 1 succeeds");
    SDLTest_AssertCheck(SDL_Libretro_GetDiskIndex(lr) == 1, "Disk index is now 1");

    // Insert the disk.
    SDLTest_AssertCheck(SDL_Libretro_InsertDisk(lr) == true, "InsertDisk succeeds");

    // Out-of-range index fails.
    SDLTest_AssertCheck(SDL_Libretro_EjectDisk(lr) == true, "EjectDisk again");
    SDLTest_AssertCheck(SDL_Libretro_SetDiskIndex(lr, 99) == false, "SetDiskIndex(99) fails");
    SDLTest_AssertCheck(SDL_Libretro_InsertDisk(lr) == true, "InsertDisk again");

    // The interface is registered once per core load, so it must survive
    // loading another game on the same core.
    SDL_Libretro_UnloadGame(lr);
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "Second LoadGame succeeds");
    SDLTest_AssertCheck(SDL_Libretro_GetDiskCount(lr) == 2, "Disk control kept across game reload");
    SDLTest_AssertCheck(SDL_Libretro_EjectDisk(lr) == true, "EjectDisk after game reload");
    SDLTest_AssertCheck(SDL_Libretro_InsertDisk(lr) == true, "InsertDisk after game reload");

    // Unloading the core drops the interface.
    SDL_Libretro_UnloadCore(lr);
    SDLTest_AssertCheck(lr->core.disk_control.get_num_images == NULL, "UnloadCore clears disk control");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

static int SDLCALL test_AudioLatency(void *arg) {
    (void)arg;
    SDL_Libretro* lr = SDL_Libretro_Create();

    // Default latency reports the compile-time default (100ms).
    SDLTest_AssertCheck(SDL_Libretro_GetAudioLatency(lr) == 100, "Default audio latency is 100");

    // Set and read back.
    SDL_Libretro_SetAudioLatency(lr, 64);
    SDLTest_AssertCheck(SDL_Libretro_GetAudioLatency(lr) == 64, "Audio latency set to 64");

    // Setting to 0 reverts to default.
    SDL_Libretro_SetAudioLatency(lr, 0);
    SDLTest_AssertCheck(SDL_Libretro_GetAudioLatency(lr) == 100, "Audio latency reverts to default");

    // NULL safety.
    SDL_Libretro_SetAudioLatency(NULL, 100);
    SDLTest_AssertCheck(SDL_Libretro_GetAudioLatency(NULL) == 0, "GetAudioLatency(NULL) 0");

    // Sample rate returns the default fallback without a core.
    SDLTest_AssertCheck(SDL_Libretro_GetSampleRate(lr) == 48000.0, "SampleRate falls back to default without core");
    SDLTest_AssertCheck(SDL_Libretro_GetSampleRate(NULL) == 48000.0, "SampleRate(NULL) returns default");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

static int SDLCALL test_AudioInitPausedRewind(void *arg) {
    (void)arg;
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    // Force SDL's dummy audio driver so the stream opens on machines (and CI
    // runners) without a sound device. An SDL_AUDIO_DRIVER env var still wins.
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    SDL_Init(SDL_INIT_AUDIO);
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "LoadGame succeeds with test content");
    SDLTest_AssertCheck(lr->core.audioStream != NULL, "Audio stream opens on load");

    // A deferred reinit (e.g. SET_SYSTEM_AV_INFO) can run while paused. SDL
    // rejects a frequency ratio of 0, so this used to tear the stream back
    // down and leave audio dead.
    SDL_Libretro_SetSpeed(lr, 0.0f);
    SDLTest_AssertCheck(SDL_Libretro_InitAudio(lr) == true, "InitAudio succeeds while paused");
    SDLTest_AssertCheck(lr->core.audioStream != NULL, "Audio stream open after paused reinit");

    // Same guard for a reinit during rewind (negative speed).
    SDL_Libretro_SetRewindEnabled(lr, true, 100, 1);
    SDL_Libretro_SetSpeed(lr, -1.0f);
    SDLTest_AssertCheck(SDL_Libretro_InitAudio(lr) == true, "InitAudio succeeds while rewinding");
    SDLTest_AssertCheck(lr->core.audioStream != NULL, "Audio stream open after rewind reinit");

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_AudioZeroSampleRate(void *arg) {
    (void)arg;
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    SDL_Init(SDL_INIT_AUDIO);
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "LoadGame succeeds with test content");
    SDLTest_AssertCheck(SDL_Libretro_GetSampleRate(lr) == 44100.0, "SampleRate reports the core rate");

    // A core reporting no sample rate falls back to the default instead of
    // opening the device with freq 0, which SDL rejects.
    lr->core.sampleRate = 0.0;
    SDLTest_AssertCheck(SDL_Libretro_GetSampleRate(lr) == 48000.0, "SampleRate 0 falls back to 48000");
    SDLTest_AssertCheck(SDL_Libretro_InitAudio(lr) == true, "InitAudio succeeds with sample rate 0");
    SDL_AudioSpec spec;
    SDL_zero(spec);
    SDLTest_AssertCheck(lr->core.audioStream && SDL_GetAudioStreamFormat(lr->core.audioStream, &spec, NULL) && spec.freq == 48000, "Audio stream opened at 48000 Hz (%d)", spec.freq);

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_AudioQueueOverflow(void *arg) {
    (void)arg;
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    // Force SDL's dummy audio driver so the stream opens on machines (and CI
    // runners) without a sound device. An SDL_AUDIO_DRIVER env var still wins.
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    SDL_Init(SDL_INIT_AUDIO);
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "LoadGame succeeds with test content");
    SDLTest_AssertCheck(lr->core.audioStream != NULL, "Audio stream open after load");

    // Pause the device so nothing drains while we measure the queue.
    SDL_PauseAudioStreamDevice(lr->core.audioStream);

    int threshold = lr->core.audioQueueThresholdBytes;
    int cap = threshold * 2;
    SDLTest_AssertCheck(threshold > 0, "Latency threshold computed (%d bytes)", threshold);

    // Push well past the hard cap in batch-sized chunks.
    static int16_t buf[512 * 2]; // Silence.
    SDL_zeroa(buf);
    int pushes = cap / (int)sizeof(buf) + 8;
    for (int i = 0; i < pushes; i++) {
        SDL_Libretro_QueueAudio(lr, buf, (int)sizeof(buf));
    }

    int queued = SDL_GetAudioStreamQueued(lr->core.audioStream);
    SDLTest_AssertCheck(queued > threshold, "Queue can exceed the latency threshold (%d > %d)", queued, threshold);
    SDLTest_AssertCheck(queued <= cap, "Queue clamped to the hard cap (%d <= %d)", queued, cap);
    // The final batch that crossed the cap is trimmed, not discarded whole.
    SDLTest_AssertCheck(queued > cap - (int)sizeof(buf), "Overflowing batch partially queued (%d > %d)", queued, cap - (int)sizeof(buf));

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_PixelFormats(void *arg) {
    (void)arg;
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_SetRenderer(lr, renderer);
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDL_Libretro_LoadGame(lr, NULL);

    // The test core sets RGB565.
    SDL_Texture* tex = SDL_Libretro_GetTexture(lr);
    SDLTest_AssertCheck(tex != NULL, "Texture created for RGB565");

    // Manually switch to XRGB8888 and trigger a re-init.
    lr->core.pixelFormat = RETRO_PIXEL_FORMAT_XRGB8888;
    lr->core.videoReinitPending = true;
    SDL_Libretro_Update(lr);
    tex = SDL_Libretro_GetTexture(lr);
    SDLTest_AssertCheck(tex != NULL, "Texture recreated for XRGB8888");

    // Switch to 0RGB1555.
    lr->core.pixelFormat = RETRO_PIXEL_FORMAT_0RGB1555;
    lr->core.videoReinitPending = true;
    SDL_Libretro_Update(lr);
    tex = SDL_Libretro_GetTexture(lr);
    SDLTest_AssertCheck(tex != NULL, "Texture recreated for 0RGB1555");

    // CloseVideo clears the software framebuffer pointer even when the texture is already gone.
    SDL_Libretro_CloseVideo(lr);
    SDLTest_AssertCheck(SDL_Libretro_GetTexture(lr) == NULL, "Texture destroyed by CloseVideo");
    lr->core.softwareFramebufferPixels = (void*)lr;
    SDL_Libretro_CloseVideo(lr);
    SDLTest_AssertCheck(lr->core.softwareFramebufferPixels == NULL, "CloseVideo clears stale framebuffer pointer without texture");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_Cheats(void *arg) {
    (void)arg;
#ifndef TEST_CORE_PATH
    SDLTest_AssertCheck(false, "TEST_CORE_PATH not defined");
    return TEST_COMPLETED;
#else
    SDL_Libretro* lr = SDL_Libretro_Create();

    // Cheats without a loaded core should fail gracefully.
    SDLTest_AssertCheck(SDL_Libretro_SetCheat(lr, 0, true, "ABC123") == false, "SetCheat fails without core");
    SDL_Libretro_ResetCheats(lr);

    // NULL safety.
    SDLTest_AssertCheck(SDL_Libretro_SetCheat(NULL, 0, true, "ABC123") == false, "SetCheat(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_SetCheat(lr, 0, true, NULL) == false, "SetCheat(NULL code) false");
    SDL_Libretro_ResetCheats(NULL);

    // Load core and game; cheats should now succeed.
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
    SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH);
    SDLTest_AssertCheck(SDL_Libretro_SetCheat(lr, 0, true, "DEADBEEF") == true, "SetCheat succeeds with core");
    SDLTest_AssertCheck(SDL_Libretro_SetCheat(lr, 1, false, "00FF00") == true, "SetCheat second code");
    SDL_Libretro_ResetCheats(lr);

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
#endif
}

static int SDLCALL test_OSD(void *arg) {
    (void)arg;
    SDL_Libretro* lr = SDL_Libretro_Create();

    // Empty queue.
    SDLTest_AssertCheck(SDL_Libretro_GetMessage(lr) == NULL, "No message initially");
    SDLTest_AssertCheck(SDL_Libretro_GetMessageCount(lr) == 0, "Message count 0 initially");
    SDLTest_AssertCheck(SDL_Libretro_GetMessageProgress(lr) == -1, "Progress -1 with no message");
    SDLTest_AssertCheck(SDL_Libretro_GetMessageType(lr) == 0, "Type 0 with no message");

    // NULL safety.
    SDLTest_AssertCheck(SDL_Libretro_GetMessage(NULL) == NULL, "GetMessage(NULL) NULL");
    SDLTest_AssertCheck(SDL_Libretro_GetMessageCount(NULL) == 0, "GetMessageCount(NULL) 0");
    SDL_Libretro_SetMessage(NULL, "test", 1.0);

    // Push a message with a long duration so it doesn't expire during the test.
    SDL_Libretro_SetMessage(lr, "Hello", 60.0);
    SDLTest_AssertCheck(SDL_Libretro_GetMessageCount(lr) == 1, "Message count is 1");
    const char* msg = SDL_Libretro_GetMessage(lr);
    SDLTest_AssertCheck(msg != NULL && SDL_strcmp(msg, "Hello") == 0, "Message is 'Hello'");

    // Push a second message.
    SDL_Libretro_SetMessage(lr, "World", 60.0);
    SDLTest_AssertCheck(SDL_Libretro_GetMessageCount(lr) == 2, "Message count is 2");

    // GetMessageByIndex.
    const char* byIdx = NULL;
    int prog = -1, typ = -1;
    SDLTest_AssertCheck(SDL_Libretro_GetMessageByIndex(lr, 0, &byIdx, &prog, &typ) == true, "GetMessageByIndex(0) succeeds");
    SDLTest_AssertCheck(byIdx != NULL && SDL_strcmp(byIdx, "Hello") == 0, "Index 0 is 'Hello'");
    SDLTest_AssertCheck(SDL_Libretro_GetMessageByIndex(lr, 1, &byIdx, NULL, NULL) == true, "GetMessageByIndex(1) succeeds");
    SDLTest_AssertCheck(byIdx != NULL && SDL_strcmp(byIdx, "World") == 0, "Index 1 is 'World'");
    SDLTest_AssertCheck(SDL_Libretro_GetMessageByIndex(lr, 99, &byIdx, NULL, NULL) == false, "GetMessageByIndex(99) fails");

    // GetMessageByIndex with -1 returns top message.
    SDLTest_AssertCheck(SDL_Libretro_GetMessageByIndex(lr, -1, &byIdx, NULL, NULL) == true, "GetMessageByIndex(-1) succeeds");

    // Duplicate message reuses the existing slot.
    SDL_Libretro_SetMessage(lr, "Hello", 60.0);
    SDLTest_AssertCheck(SDL_Libretro_GetMessageCount(lr) == 2, "Duplicate 'Hello' doesn't add a third");

    // Clear messages by setting NULL/empty.
    SDL_Libretro_SetMessage(lr, NULL, 0.0);
    SDLTest_AssertCheck(SDL_Libretro_GetMessage(lr) == NULL, "Messages cleared after SetMessage(NULL)");
    SDLTest_AssertCheck(SDL_Libretro_GetMessageCount(lr) == 0, "Message count 0 after clear");

    // Empty string also clears.
    SDL_Libretro_SetMessage(lr, "Temp", 60.0);
    SDLTest_AssertCheck(SDL_Libretro_GetMessageCount(lr) == 1, "One message added");
    SDL_Libretro_SetMessage(lr, "", 0.0);
    SDLTest_AssertCheck(SDL_Libretro_GetMessageCount(lr) == 0, "Empty string clears messages");

    // Negative durations are clamped to zero, so the message expires right away.
    SDL_Libretro_SetMessage(lr, "Negative", -5.0);
    SDL_Libretro_SetMessage(lr, "Very negative", -1e300);
    SDLTest_AssertCheck(lr->osdQueueCount == 2 && lr->osdQueue[0].endTimeMs <= SDL_GetTicks() && lr->osdQueue[1].endTimeMs <= SDL_GetTicks(),
        "Negative durations end no later than now");
    SDL_Delay(2);
    SDLTest_AssertCheck(SDL_Libretro_GetMessage(lr) == NULL, "Negative-duration messages have expired");

    // Absurdly long durations are capped rather than overflowing.
    SDL_Libretro_SetMessage(lr, "Forever", 1e300);
    msg = SDL_Libretro_GetMessage(lr);
    SDLTest_AssertCheck(msg != NULL && SDL_strcmp(msg, "Forever") == 0, "Huge-duration message is shown");
    SDL_Libretro_SetMessage(lr, NULL, 0.0);

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

static int SDLCALL test_VFSTruncate(void *arg) {
    (void)arg;
#ifdef SDL_LIBRETRO_DISABLE_VFS
    return TEST_SKIPPED;
#else
    char path[1024], tmpPath[1024];
    SDL_snprintf(path, sizeof(path), "%stest_vfs_truncate.bin", SDL_GetBasePath());
    SDL_snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path);
    SDLTest_AssertCheck(SDL_SaveFile(path, "0123456789", 10) == true, "Seed file written");

    unsigned mode = RETRO_VFS_FILE_ACCESS_READ_WRITE | RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING;
    struct retro_vfs_file_handle* h = SDL_Libretro_VFS_Open(path, mode, 0);
    SDLTest_AssertCheck(h != NULL, "VFS open succeeded");
    if (!h) return TEST_ABORTED;

    // Read-only handles refuse to truncate.
    struct retro_vfs_file_handle* ro = SDL_Libretro_VFS_Open(path, RETRO_VFS_FILE_ACCESS_READ, 0);
    SDLTest_AssertCheck(ro != NULL && SDL_Libretro_VFS_Truncate(ro, 2) == -1, "Read-only truncate fails");
    SDL_Libretro_VFS_Close(ro);

    // Shrink with the position past the new end: position clamps to the length.
    SDL_Libretro_VFS_Seek(h, 8, RETRO_VFS_SEEK_POSITION_START);
    SDLTest_AssertCheck(SDL_Libretro_VFS_Truncate(h, 4) == 0, "Shrink to 4 succeeds");
    SDLTest_AssertCheck(h->io != NULL, "Handle keeps a stream after shrink");
    SDLTest_AssertCheck(SDL_Libretro_VFS_Size(h) == 4, "Size is 4 after shrink");
    SDLTest_AssertCheck(SDL_Libretro_VFS_Tell(h) == 4, "Position clamped to 4");
    SDLTest_AssertCheck(SDL_GetPathInfo(tmpPath, NULL) == false, "No temp file left behind");

    // The handle stays writable and readable.
    SDLTest_AssertCheck(SDL_Libretro_VFS_Write(h, "AB", 2) == 2, "Write after shrink succeeds");
    char buf[16] = {0};
    SDL_Libretro_VFS_Seek(h, 0, RETRO_VFS_SEEK_POSITION_START);
    SDLTest_AssertCheck(SDL_Libretro_VFS_Read(h, buf, sizeof(buf)) == 6, "Read 6 bytes back");
    SDLTest_AssertCheck(SDL_memcmp(buf, "0123AB", 6) == 0, "Contents are '0123AB'");

    // Shrink with the position inside the kept range: position is preserved.
    SDL_Libretro_VFS_Seek(h, 1, RETRO_VFS_SEEK_POSITION_START);
    SDLTest_AssertCheck(SDL_Libretro_VFS_Truncate(h, 3) == 0, "Shrink to 3 succeeds");
    SDLTest_AssertCheck(SDL_Libretro_VFS_Tell(h) == 1, "Position 1 preserved");

    // Grow pads with zeros and keeps the position.
    SDLTest_AssertCheck(SDL_Libretro_VFS_Truncate(h, 5) == 0, "Grow to 5 succeeds");
    SDLTest_AssertCheck(SDL_Libretro_VFS_Tell(h) == 1, "Position 1 preserved after grow");

    // Shrink to empty.
    SDLTest_AssertCheck(SDL_Libretro_VFS_Truncate(h, 0) == 0, "Shrink to 0 succeeds");
    SDLTest_AssertCheck(SDL_Libretro_VFS_Size(h) == 0, "Size is 0");
    SDLTest_AssertCheck(SDL_Libretro_VFS_Write(h, "Z", 1) == 1, "Write after shrink to 0 succeeds");
    SDLTest_AssertCheck(SDL_Libretro_VFS_Truncate(h, -1) == -1, "Negative length fails");
    SDLTest_AssertCheck(SDL_Libretro_VFS_Close(h) == 0, "Close succeeds");

    size_t size = 0;
    char* data = (char*)SDL_LoadFile(path, &size);
    SDLTest_AssertCheck(data != NULL && size == 1 && data[0] == 'Z', "File on disk is 'Z'");
    SDL_free(data);

    // A mid-file shrink leaves exactly the kept prefix on disk.
    SDL_SaveFile(path, "0123456789", 10);
    h = SDL_Libretro_VFS_Open(path, mode, 0);
    SDLTest_AssertCheck(h != NULL && SDL_Libretro_VFS_Truncate(h, 7) == 0, "Shrink to 7 succeeds");
    SDL_Libretro_VFS_Close(h);
    data = (char*)SDL_LoadFile(path, &size);
    SDLTest_AssertCheck(data != NULL && size == 7 && SDL_memcmp(data, "0123456", 7) == 0, "File on disk is '0123456'");
    SDL_free(data);

    SDL_RemovePath(path);
    return TEST_COMPLETED;
#endif
}

/**
 * Drains the SDL event queue, counting events of the given type and keeping
 * the last one seen in *last (when non-NULL).
 */
static int test_DrainEvents(Uint32 type, SDL_Event* last) {
    int count = 0;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == type) {
            count++;
            if (last != NULL) {
                *last = event;
            }
        }
    }
    return count;
}

/**
 * An event watch that implements the env command in its userdata: marks the
 * event handled for the core by setting user.code.
 */
static bool SDLCALL test_EventsWatch(void* userdata, SDL_Event* event) {
    if (event->type == (Uint32)(uintptr_t)userdata) {
        event->user.code = 1;
    }
    return true;
}

static int SDLCALL test_Events(void *arg) {
    (void)arg;

    // The event space sits in the SDL user event range; the experimental
    // flag is masked off so every command fits below the lifecycle events.
    SDLTest_AssertCheck(SDL_EVENT_LIBRETRO >= SDL_EVENT_USER, "SDL_EVENT_LIBRETRO is a user event");
    SDLTest_AssertCheck(SDL_EVENT_LIBRETRO_MENU_CLOSED <= SDL_EVENT_LAST, "The libretro events fit the SDL event range");
    SDLTest_AssertCheck((SDL_EVENT_LIBRETRO | RETRO_ENVIRONMENT_GET_CAN_DUPE) <= SDL_EVENT_LAST,
        "Plain commands OR directly onto SDL_EVENT_LIBRETRO");
    SDLTest_AssertCheck((SDL_EVENT_LIBRETRO | (RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT & ~RETRO_ENVIRONMENT_EXPERIMENTAL)) < (Uint32)SDL_EVENT_LIBRETRO_CORE_LOADED,
        "Env events stay below the lifecycle events");

    SDL_Init(SDL_INIT_EVENTS);
    SDL_Libretro* lr = SDL_Libretro_Create();
    const Uint32 cameraEvent = SDL_EVENT_LIBRETRO | (RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE & ~RETRO_ENVIRONMENT_EXPERIMENTAL);

    // An unwatched env command lands in the queue with the context in data1
    // and the core's pointer in data2, and reports unhandled to the core.
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    int payload = 7;
    SDLTest_AssertCheck(SDL_Libretro_PushEnvEvent(lr, RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE, &payload) == false,
        "An unwatched env event reports unhandled");
    SDL_Event received;
    SDL_zero(received);
    SDLTest_AssertCheck(test_DrainEvents(cameraEvent, &received) == 1, "The env event is queued once");
    SDLTest_AssertCheck(received.user.data1 == lr, "data1 carries the context");
    SDLTest_AssertCheck(received.user.data2 == &payload, "data2 carries the core's data pointer");
    SDLTest_AssertCheck(received.user.code == 0, "An unclaimed event has a zero code");

    // A watch runs synchronously inside the push; setting user.code there
    // reports the command handled to the core.
    SDL_AddEventWatch(test_EventsWatch, (void*)(uintptr_t)cameraEvent);
    SDLTest_AssertCheck(SDL_Libretro_PushEnvEvent(lr, RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE, &payload) == true,
        "A watch that sets user.code marks the env command handled");
    SDL_RemoveEventWatch(test_EventsWatch, (void*)(uintptr_t)cameraEvent);
    SDLTest_AssertCheck(test_DrainEvents(cameraEvent, NULL) == 0,
        "A claimed env command is consumed by the watch, not queued");

#if defined(TEST_CORE_PATH) && defined(TEST_CONTENT_PATH)
    // Real loads push the lifecycle events, with the loaded name in data2.
    SDLTest_AssertCheck(SDL_Libretro_LoadCore(lr, TEST_CORE_PATH) == true, "LoadCore succeeds");
    SDLTest_AssertCheck(test_DrainEvents(SDL_EVENT_LIBRETRO_CORE_LOADED, &received) == 1,
        "CORE_LOADED is pushed after a core loads");
    SDLTest_AssertCheck(received.user.data1 == lr, "CORE_LOADED carries the context");
    SDLTest_AssertCheck(received.user.data2 == SDL_Libretro_GetCoreName(lr),
        "CORE_LOADED data2 is the core name");
    SDLTest_AssertCheck(SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH) == true, "LoadGame succeeds");
    SDLTest_AssertCheck(test_DrainEvents(SDL_EVENT_LIBRETRO_GAME_LOADED, &received) == 1,
        "GAME_LOADED is pushed after a game loads");
    SDLTest_AssertCheck(received.user.data2 == SDL_Libretro_GetGameName(lr),
        "GAME_LOADED data2 is the game name");

    // Commands the library handles get curated events after the handling.
    struct retro_game_geometry geometry = { 640, 480, 640, 480, 2.0f };
    SDLTest_AssertCheck(SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry) == true,
        "SET_GEOMETRY is handled");
    SDLTest_AssertCheck(test_DrainEvents(SDL_EVENT_LIBRETRO_GEOMETRY_CHANGED, NULL) == 1,
        "GEOMETRY_CHANGED is pushed for SET_GEOMETRY");
    SDLTest_AssertCheck(SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry) == true,
        "A repeated SET_GEOMETRY is handled");
    SDLTest_AssertCheck(test_DrainEvents(SDL_EVENT_LIBRETRO_GEOMETRY_CHANGED, NULL) == 0,
        "An unchanged geometry pushes nothing");
    SDLTest_AssertCheck(SDL_Libretro_EnvironmentCallback(RETRO_ENVIRONMENT_SHUTDOWN, NULL) == true,
        "SHUTDOWN is handled");
    SDLTest_AssertCheck(test_DrainEvents(SDL_EVENT_LIBRETRO_SHUTDOWN, NULL) == 1,
        "SHUTDOWN is pushed");
    SDLTest_AssertCheck(SDL_Libretro_ShouldQuit(lr) == true, "ShouldQuit reflects the shutdown");

    // Unloading pushes the unload pair: game first, then the core.
    SDL_Libretro_UnloadCore(lr);
    int gameUnloaded = 0, coreUnloaded = 0;
    SDL_Event drained;
    while (SDL_PollEvent(&drained)) {
        if (drained.type == SDL_EVENT_LIBRETRO_GAME_UNLOADED) {
            gameUnloaded++;
        } else if (drained.type == SDL_EVENT_LIBRETRO_CORE_UNLOADED) {
            coreUnloaded++;
        }
    }
    SDLTest_AssertCheck(gameUnloaded == 1 && coreUnloaded == 1,
        "Unloading pushes GAME_UNLOADED and CORE_UNLOADED once each, got %d/%d", gameUnloaded, coreUnloaded);
    SDL_Libretro_UnloadCore(lr);
    SDLTest_AssertCheck(test_DrainEvents(SDL_EVENT_LIBRETRO_CORE_UNLOADED, NULL) == 0,
        "Unloading without a core pushes nothing");
#endif

    SDL_Libretro_Destroy(lr);
    return TEST_COMPLETED;
}

#ifdef SDL_LIBRETRO_ENABLE_MENU
static void test_MenuCustomClicked(SDL_LibretroMenu* menu, void* userdata) {
    (void)menu;
    (*(int*)userdata)++;
}

/**
 * Applies the style and checks three probe colors in the resulting Nuklear
 * style: text, window background, and the toggle cursor accent.
 */
static bool test_MenuStyleHasColors(SDL_LibretroMenu* menu, SDL_LibretroMenuStyle style,
        struct nk_color text, struct nk_color window, struct nk_color toggleCursor) {
    if (!SDL_Libretro_SetMenuStyle(menu, style)) {
        return false;
    }
    return SDL_memcmp(&menu->ctx->style.text.color, &text, sizeof(text)) == 0 &&
        SDL_memcmp(&menu->ctx->style.window.background, &window, sizeof(window)) == 0 &&
        SDL_memcmp(&menu->ctx->style.checkbox.cursor_normal.data.color, &toggleCursor, sizeof(toggleCursor)) == 0;
}

static int SDLCALL test_Menu(void *arg) {
    // NULL safety
    SDLTest_AssertCheck(SDL_Libretro_CreateMenu(NULL) == NULL, "CreateMenu(NULL) returns NULL");
    SDL_Libretro_DestroyMenu(NULL);
    SDL_Libretro_UpdateMenu(NULL);
    SDL_Libretro_RenderMenu(NULL);
    SDL_Libretro_ToggleMenu(NULL);
    SDL_Libretro_SetMenuOpen(NULL, true);
    SDLTest_AssertCheck(SDL_Libretro_IsMenuOpen(NULL) == false, "IsMenuOpen(NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_HandleMenuEvent(NULL, NULL) == false, "HandleMenuEvent(NULL, NULL) false");
    SDLTest_AssertCheck(SDL_Libretro_GetMenuLibretro(NULL) == NULL, "GetMenuLibretro(NULL) NULL");
    SDL_Libretro_SetMenuUserData(NULL, (void*)1);
    SDLTest_AssertCheck(SDL_Libretro_GetMenuUserData(NULL) == NULL, "GetMenuUserData(NULL) NULL");
    SDL_Libretro_MenuScreenshotClicked(NULL, NULL);

    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDLTest_AssertCheck(SDL_Libretro_CreateMenu(lr) == NULL, "CreateMenu without a renderer returns NULL");

    SDL_Window* window = SDL_CreateWindow("test", 320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Libretro_SetRenderer(lr, renderer);

    SDL_LibretroMenu* menu = SDL_Libretro_CreateMenu(lr);
    SDLTest_AssertCheck(menu != NULL, "CreateMenu succeeds with a renderer");
    if (menu != NULL) {
        SDLTest_AssertCheck(SDL_Libretro_IsMenuOpen(menu) == false, "Menu starts closed");

        // Context getter and user data
        SDLTest_AssertCheck(SDL_Libretro_GetMenuLibretro(menu) == lr, "GetMenuLibretro returns the creating context");
        SDLTest_AssertCheck(SDL_Libretro_GetMenuUserData(menu) == NULL, "Menu user data starts NULL");
        int userValue = 42;
        SDL_Libretro_SetMenuUserData(menu, &userValue);
        SDLTest_AssertCheck(SDL_Libretro_GetMenuUserData(menu) == &userValue, "Menu user data round-trips");
        SDL_Libretro_SetMenuUserData(menu, NULL);
        SDLTest_AssertCheck(SDL_Libretro_GetMenuUserData(menu) == NULL, "Menu user data clears");

        // Styles
        SDLTest_AssertCheck(SDL_Libretro_GetMenuStyle(menu) == SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_MOCHA,
            "Menu starts with the default style");
        SDLTest_AssertCheck(SDL_Libretro_SetMenuStyle(NULL, SDL_LIBRETRO_MENU_STYLE_DARK) == false,
            "SetMenuStyle(NULL) fails");
        SDLTest_AssertCheck(SDL_Libretro_SetMenuStyle(menu, SDL_LIBRETRO_MENU_STYLE_COUNT) == false,
            "SetMenuStyle rejects an out-of-range style");
        bool stylesApplied = true;
        for (int style = 0; style < (int)SDL_LIBRETRO_MENU_STYLE_COUNT; style++) {
            stylesApplied = stylesApplied &&
                SDL_Libretro_SetMenuStyle(menu, (SDL_LibretroMenuStyle)style) &&
                SDL_Libretro_GetMenuStyle(menu) == (SDL_LibretroMenuStyle)style;
        }
        SDLTest_AssertCheck(stylesApplied, "Every menu style applies and reads back");

        // Regression probes for the theme colors.
        SDLTest_AssertCheck(test_MenuStyleHasColors(menu, SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_MOCHA,
            nk_rgba(205, 214, 244, 255), nk_rgba(30, 30, 46, 235), nk_rgba(180, 190, 254, 255)),
            "Mocha colors match");
        SDLTest_AssertCheck(test_MenuStyleHasColors(menu, SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_LATTE,
            nk_rgba(76, 79, 105, 255), nk_rgba(239, 241, 245, 235), nk_rgba(223, 142, 29, 255)),
            "Latte colors match");
        SDLTest_AssertCheck(test_MenuStyleHasColors(menu, SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_FRAPPE,
            nk_rgba(198, 208, 245, 255), nk_rgba(48, 52, 70, 235), nk_rgba(244, 184, 228, 255)),
            "Frappe colors match");
        SDLTest_AssertCheck(test_MenuStyleHasColors(menu, SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_MACCHIATO,
            nk_rgba(202, 211, 245, 255), nk_rgba(36, 39, 58, 235), nk_rgba(238, 212, 159, 255)),
            "Macchiato colors match");
        SDLTest_AssertCheck(test_MenuStyleHasColors(menu, SDL_LIBRETRO_MENU_STYLE_DRACULA,
            nk_rgba(248, 248, 242, 255), nk_rgba(40, 42, 54, 235), nk_rgba(255, 121, 198, 255)),
            "Dracula colors match");

        // UI scale: the resolution thresholds step the automatic scale.
        SDLTest_AssertCheck(SDL_Libretro_MenuAutoScale(1279) == 1.0f, "Auto scale 1x below 1280");
        SDLTest_AssertCheck(SDL_Libretro_MenuAutoScale(1280) == 2.0f, "Auto scale 2x at 1280");
        SDLTest_AssertCheck(SDL_Libretro_MenuAutoScale(2560) == 3.0f, "Auto scale 3x at 2560");
        SDLTest_AssertCheck(SDL_Libretro_MenuAutoScale(3840) == 4.0f, "Auto scale 4x at 3840");
        SDLTest_AssertCheck(menu->uiScaleIndex == 2, "UI scale defaults to 2x");
        SDLTest_AssertCheck(menu->renderScale > 0.0f, "Render scale is set at creation");

        // With nothing to run, the menu opens itself.
        SDL_Libretro_UpdateMenu(menu);
        SDL_Libretro_RenderMenu(menu);
        SDLTest_AssertCheck(SDL_Libretro_IsMenuOpen(menu) == true, "Menu auto-opens without a game");

        float defaultScale = menu->renderScale;
        menu->uiScaleIndex = 4;
        SDL_Libretro_UpdateMenu(menu);
        SDL_Libretro_RenderMenu(menu);
        SDLTest_AssertCheck(menu->renderScale == defaultScale * 2.0f, "UI scale 4x doubles the default 2x render scale");
        menu->uiScaleIndex = 2;
        SDL_Libretro_UpdateMenu(menu);
        SDL_Libretro_RenderMenu(menu);
        SDLTest_AssertCheck(menu->renderScale == defaultScale, "Returning to 2x restores the default render scale");

        // Gameplay input is swallowed while the menu is open, but lifecycle
        // events always pass through.
        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_EVENT_KEY_DOWN;
        event.key.key = SDLK_A;
        SDLTest_AssertCheck(SDL_Libretro_HandleMenuEvent(menu, &event) == true, "Open menu swallows gameplay input");
        SDL_zero(event);
        event.type = SDL_EVENT_QUIT;
        SDLTest_AssertCheck(SDL_Libretro_HandleMenuEvent(menu, &event) == false, "Quit passes through an open menu");

        // The toggle key closes it and both its edges are consumed; the
        // press alone doesn't toggle.
        SDL_zero(event);
        event.type = SDL_EVENT_KEY_DOWN;
        event.key.key = SDLK_F1;
        SDLTest_AssertCheck(SDL_Libretro_HandleMenuEvent(menu, &event) == true, "Toggle key press is consumed");
        SDLTest_AssertCheck(SDL_Libretro_IsMenuOpen(menu) == true, "Toggle key press doesn't toggle");
        SDL_zero(event);
        event.type = SDL_EVENT_KEY_UP;
        event.key.key = SDLK_F1;
        SDLTest_AssertCheck(SDL_Libretro_HandleMenuEvent(menu, &event) == true, "Toggle key event is consumed");
        SDLTest_AssertCheck(SDL_Libretro_IsMenuOpen(menu) == false, "Toggle key closes the menu");
        SDL_zero(event);
        event.type = SDL_EVENT_KEY_DOWN;
        event.key.key = SDLK_A;
        SDLTest_AssertCheck(SDL_Libretro_HandleMenuEvent(menu, &event) == false, "Closed menu ignores gameplay input");

        // Menu notifications arrive through the SDL event queue.
        SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
        SDL_Libretro_SetMenuOpen(menu, true);
        SDL_Libretro_SetMenuOpen(menu, true); // No change: no second event.
        SDL_Libretro_SetMenuOpen(menu, false);
        int menuOpened = 0, menuClosed = 0;
        SDL_Event menuEvent, lastOpened;
        SDL_zero(lastOpened);
        while (SDL_PollEvent(&menuEvent)) {
            if (menuEvent.type == SDL_EVENT_LIBRETRO_MENU_OPENED) {
                menuOpened++;
                lastOpened = menuEvent;
            } else if (menuEvent.type == SDL_EVENT_LIBRETRO_MENU_CLOSED) {
                menuClosed++;
            }
        }
        SDLTest_AssertCheck(menuOpened == 1 && menuClosed == 1,
            "Open/close each push one menu event, got %d/%d", menuOpened, menuClosed);
        SDLTest_AssertCheck(lastOpened.user.data1 == lr, "Menu events carry the context");
        SDLTest_AssertCheck(lastOpened.user.data2 == menu, "Menu events carry the menu in data2");

        // The menu passes SDL_libretro's own events through untouched.
        SDL_zero(event);
        event.type = SDL_EVENT_LIBRETRO_MENU_OPENED;
        SDLTest_AssertCheck(SDL_Libretro_HandleMenuEvent(menu, &event) == false,
            "Libretro events pass through the menu");

        // Application-added entries fire their callbacks and keep Quit last.
        int customClicks = 0;
        bool checkValue = true;
        SDLTest_AssertCheck(SDL_Libretro_AddMenuButton(NULL, "x", NULL, NULL) == false, "AddMenuButton(NULL) fails");
        SDLTest_AssertCheck(SDL_Libretro_AddMenuButton(menu, NULL, NULL, NULL) == false, "AddMenuButton without a label fails");
        SDLTest_AssertCheck(SDL_Libretro_AddMenuButton(menu, "Custom", &test_MenuCustomClicked, &customClicks) == true,
            "AddMenuButton succeeds");
        SDLTest_AssertCheck(SDL_Libretro_AddMenuCheckbox(menu, "Custom Check", &checkValue, &test_MenuCustomClicked, &customClicks) == true,
            "AddMenuCheckbox succeeds");
        size_t childCount = cvector_size(menu->console->children);
        SDLTest_AssertCheck(menu->console->children[childCount - 1] == menu->quitButton, "Quit stays the last entry");
        nk_console_trigger_event(menu->console->children[childCount - 3], NK_CONSOLE_EVENT_CLICKED);
        nk_console_trigger_event(menu->console->children[childCount - 2], NK_CONSOLE_EVENT_CHANGED);
        SDLTest_AssertCheck(customClicks == 2, "Custom entry callbacks fired, got %d", customClicks);
        SDLTest_AssertCheck(checkValue == true, "Checkbox value mirrors the widget state");

        // Deep links open the menu and navigate by label path.
        SDLTest_AssertCheck(SDL_Libretro_OpenMenuPath(NULL, "Settings") == false, "OpenMenuPath(NULL) fails");
        SDLTest_AssertCheck(SDL_Libretro_OpenMenuPath(menu, NULL) == false, "OpenMenuPath without a path fails");
        SDL_Libretro_SetMenuOpen(menu, false);
        SDLTest_AssertCheck(SDL_Libretro_OpenMenuPath(menu, "Settings/Audio & Video") == true,
            "OpenMenuPath resolves Settings/Audio & Video");
        SDLTest_AssertCheck(SDL_Libretro_IsMenuOpen(menu) == true, "OpenMenuPath opens the menu");
        SDLTest_AssertCheck(SDL_Libretro_OpenMenuPath(menu, "No Such Page") == false,
            "OpenMenuPath rejects unknown paths");
        SDLTest_AssertCheck(SDL_Libretro_OpenMenuPath(menu, "Settings") == true, "OpenMenuPath resolves Settings");
        SDL_Libretro_SetMenuOpen(menu, false);

        // Keyboard binding runes round-trip through SDL scancodes.
        SDLTest_AssertCheck(SDL_Libretro_MenuRuneFromScancode(SDL_SCANCODE_Z) == (nk_rune)'z', "Scancode Z maps to rune 'z'");
        SDLTest_AssertCheck(SDL_Libretro_MenuScancodeFromRune((nk_rune)'z') == SDL_SCANCODE_Z, "Rune 'z' maps back to scancode Z");
        SDLTest_AssertCheck(SDL_Libretro_MenuScancodeFromRune(SDL_Libretro_MenuRuneFromScancode(SDL_SCANCODE_RETURN)) == SDL_SCANCODE_RETURN,
            "Return round-trips through its rune");
        SDLTest_AssertCheck(SDL_Libretro_MenuScancodeFromRune(SDL_Libretro_MenuRuneFromScancode(SDL_SCANCODE_UP)) == SDL_SCANCODE_UP,
            "Up round-trips through its rune");
        SDLTest_AssertCheck(SDL_Libretro_MenuScancodeFromRune(SDL_Libretro_MenuRuneFromScancode(SDL_SCANCODE_F5)) == SDL_SCANCODE_F5,
            "F5 round-trips through its rune");
        SDLTest_AssertCheck(SDL_Libretro_MenuScancodeFromRune(NK_CONSOLE_KEY_NONE) == SDL_SCANCODE_UNKNOWN,
            "An empty capture maps to no scancode");

        // The capture handler applies keys to the live mapping, and captures
        // without an SDL equivalent revert to the current binding.
        menu->keyBinds[RETRO_DEVICE_ID_JOYPAD_B].rune = (nk_rune)'k';
        SDL_Libretro_MenuKeyBindChanged(NULL, &menu->keyBinds[RETRO_DEVICE_ID_JOYPAD_B]);
        SDLTest_AssertCheck(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_B] == SDL_SCANCODE_K,
            "A captured key applies to the mapping");
        menu->keyBinds[RETRO_DEVICE_ID_JOYPAD_B].rune = NK_CONSOLE_KEY_NONE;
        SDL_Libretro_MenuKeyBindChanged(NULL, &menu->keyBinds[RETRO_DEVICE_ID_JOYPAD_B]);
        SDLTest_AssertCheck(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_B] == SDL_SCANCODE_K,
            "An empty capture keeps the mapping");
        SDLTest_AssertCheck(menu->keyBinds[RETRO_DEVICE_ID_JOYPAD_B].rune == (nk_rune)'k',
            "An empty capture reverts to the current binding");
        SDL_Libretro_SetKeyboardMapping(lr, RETRO_DEVICE_ID_JOYPAD_B, SDL_SCANCODE_Z);

        // Gamepad captures translate through nk_gamepad, apply to their own
        // port, and revert when there's no SDL equivalent.
        SDLTest_AssertCheck(SDL_Libretro_MenuNkButtonFromSDL(SDL_GAMEPAD_BUTTON_SOUTH) == NK_GAMEPAD_BUTTON_A,
            "South maps to nk_gamepad A");
        SDLTest_AssertCheck(nk_gamepad_sdl3_map_button(NK_GAMEPAD_BUTTON_A) == SDL_GAMEPAD_BUTTON_SOUTH,
            "nk_gamepad A maps back to South");
        menu->padPorts[0].binds[RETRO_DEVICE_ID_JOYPAD_B].captured = NK_GAMEPAD_BUTTON_Y;
        SDL_Libretro_MenuPadBindChanged(NULL, &menu->padPorts[0].binds[RETRO_DEVICE_ID_JOYPAD_B]);
        SDLTest_AssertCheck(lr->gamepadButtons[0][RETRO_DEVICE_ID_JOYPAD_B] == SDL_GAMEPAD_BUTTON_NORTH,
            "A captured pad button applies to the mapping");
        SDLTest_AssertCheck(lr->gamepadButtons[1][RETRO_DEVICE_ID_JOYPAD_B] == SDL_GAMEPAD_BUTTON_SOUTH,
            "A captured pad button leaves other ports alone");
        menu->padPorts[0].binds[RETRO_DEVICE_ID_JOYPAD_B].captured = NK_GAMEPAD_BUTTON_INVALID;
        SDL_Libretro_MenuPadBindChanged(NULL, &menu->padPorts[0].binds[RETRO_DEVICE_ID_JOYPAD_B]);
        SDLTest_AssertCheck(lr->gamepadButtons[0][RETRO_DEVICE_ID_JOYPAD_B] == SDL_GAMEPAD_BUTTON_NORTH,
            "An empty pad capture keeps the mapping");
        SDLTest_AssertCheck(menu->padPorts[0].binds[RETRO_DEVICE_ID_JOYPAD_B].captured == NK_GAMEPAD_BUTTON_Y,
            "An empty pad capture reverts to the current binding");
        SDL_Libretro_SetGamepadMapping(lr, 0, RETRO_DEVICE_ID_JOYPAD_B, SDL_GAMEPAD_BUTTON_SOUTH);

        // The Analog to Digital combobox applies to its own port.
        SDLTest_AssertCheck(menu->padPorts[1].analogIndex == (int)SDL_LIBRETRO_ANALOG_TO_DIGITAL_NONE,
            "Analog to Digital starts on None");
        menu->padPorts[1].analogIndex = (int)SDL_LIBRETRO_ANALOG_TO_DIGITAL_RIGHT;
        SDL_Libretro_MenuAnalogToDigitalChanged(NULL, &menu->padPorts[1]);
        SDLTest_AssertCheck(SDL_Libretro_GetAnalogToDigital(lr, 1) == SDL_LIBRETRO_ANALOG_TO_DIGITAL_RIGHT,
            "The Analog to Digital choice applies to its port");
        SDLTest_AssertCheck(SDL_Libretro_GetAnalogToDigital(lr, 0) == SDL_LIBRETRO_ANALOG_TO_DIGITAL_NONE,
            "The Analog to Digital choice leaves other ports alone");
        SDL_Libretro_SetAnalogToDigital(lr, 1, SDL_LIBRETRO_ANALOG_TO_DIGITAL_NONE);
        SDL_Libretro_MenuSyncPadBinds(menu);
        SDLTest_AssertCheck(menu->padPorts[1].analogIndex == (int)SDL_LIBRETRO_ANALOG_TO_DIGITAL_NONE,
            "SyncPadBinds refreshes the Analog to Digital choice");

        // The Gamepad page holds one sub-page per player.
        SDLTest_AssertCheck(cvector_size(menu->gamepadButton->children) == (size_t)(SDL_LIBRETRO_MAX_USERS + 1),
            "The Gamepad page lists a sub-page per player after the back button");
        SDLTest_AssertCheck(SDL_strncmp(menu->padPorts[0].label, "1. ", 3) == 0,
            "Player pages are numbered by port");

        // Without disk control the Disks page stays hidden.
        SDLTest_AssertCheck(menu->disksButton->visible == nk_false, "Disks page hidden without disk control");

        // The Rewind toggle mirrors the live rewind state when settings sync.
        SDL_Libretro_SetRewindEnabled(lr, true, 8, 1);
        SDL_Libretro_MenuSyncSettings(menu);
        SDLTest_AssertCheck(menu->rewindChecked == nk_true, "Rewind toggle syncs from the context");
        menu->rewindChecked = nk_false;
        SDL_Libretro_MenuRewindChanged(menu, NULL);
        SDLTest_AssertCheck(SDL_Libretro_GetRewindEnabled(lr) == false, "Rewind toggle disables rewind");

        // The rewind buffer setting drives the memory limit, in MB.
        menu->rewindBufferMB = 16;
        SDL_Libretro_MenuRewindBufferChanged(NULL, menu);
        SDLTest_AssertCheck(SDL_Libretro_GetRewindMemoryLimit(lr) == (size_t)16 << 20,
            "Rewind buffer setting applies the memory limit");
        SDL_Libretro_SetRewindMemoryLimit(lr, 32 << 20);
        SDL_Libretro_MenuSyncSettings(menu);
        SDLTest_AssertCheck(menu->rewindBufferMB == 32, "Rewind buffer setting syncs from the context");
        SDL_Libretro_SetRewindMemoryLimit(lr, 0);

        // A progress-type OSD message surfaces as the top progress bar.
        SDL_Libretro_OsdPush(lr, "Working", 60.0, 0, RETRO_MESSAGE_TYPE_PROGRESS, 50);
        SDL_Libretro_SetMenuOpen(menu, true);
        SDL_Libretro_UpdateMenu(menu);
        SDL_Libretro_RenderMenu(menu);
        SDLTest_AssertCheck(menu->osdProgressWidget->visible == nk_true, "A progress OSD message shows the bar");
        SDLTest_AssertCheck(menu->osdProgressValue == 50, "The bar tracks the message progress, got %d", (int)menu->osdProgressValue);
        SDL_Libretro_SetMessage(lr, "", 0.0);
        SDL_Libretro_UpdateMenu(menu);
        SDL_Libretro_RenderMenu(menu);
        SDLTest_AssertCheck(menu->osdProgressWidget->visible == nk_false, "The bar hides when the message clears");
        SDL_Libretro_SetMenuOpen(menu, false);

#if defined(TEST_CORE_PATH) && defined(TEST_CONTENT_PATH)
        // With a game running the menu stays closed; opening it builds the
        // Core Options submenu from the test core's options.
        SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);
        SDL_Libretro_LoadGame(lr, TEST_CONTENT_PATH);
        SDL_Libretro_Update(lr);
        SDL_Libretro_UpdateMenu(menu);
        SDL_Libretro_RenderMenu(menu);
        SDLTest_AssertCheck(SDL_Libretro_IsMenuOpen(menu) == false, "Menu stays closed while a game runs");

        SDL_Libretro_SetMenuOpen(menu, true);
        for (int i = 0; i < 3; i++) {
            SDL_Libretro_UpdateMenu(menu);
            SDL_Libretro_RenderMenu(menu);
        }
        SDLTest_AssertCheck(SDL_Libretro_IsMenuOpen(menu) == true, "Menu stays open across frames");

        // Adding a second disk brings up the Disks page on the next open, and
        // picking a radio runs the eject/set/insert swap.
        SDLTest_AssertCheck(SDL_Libretro_EjectDisk(lr) == true, "EjectDisk before adding a disk");
        SDLTest_AssertCheck(SDL_Libretro_AddDiskImage(lr, TEST_CONTENT_PATH) == true, "AddDiskImage adds a second disk");
        SDLTest_AssertCheck(SDL_Libretro_InsertDisk(lr) == true, "InsertDisk after adding");
        SDL_Libretro_SetMenuOpen(menu, false);
        SDL_Libretro_UpdateMenu(menu);
        SDL_Libretro_SetMenuOpen(menu, true);
        SDL_Libretro_UpdateMenu(menu);
        SDL_Libretro_RenderMenu(menu);
        SDLTest_AssertCheck(menu->disksButton->visible == nk_true, "Disks page appears with two disks");
        menu->diskSelected = 1;
        SDL_Libretro_MenuDiskClicked(NULL, menu);
        SDLTest_AssertCheck(SDL_Libretro_GetDiskIndex(lr) == 1, "The menu swap changes the disk index");

        // About page with a loaded core: core and content lines appear.
        SDL_Libretro_MenuBuildAbout(menu);
        bool aboutHasCore = false;
        bool aboutHasContent = false;
        bool aboutHasSize = false;
        for (size_t offset = 0; offset < menu->aboutTextLength; offset += SDL_strlen(menu->aboutText + offset) + 1) {
            const char* line = menu->aboutText + offset;
            aboutHasCore |= SDL_strncmp(line, "Core: ", 6) == 0;
            aboutHasContent |= SDL_strncmp(line, "Content: ", 9) == 0;
            aboutHasSize |= SDL_strncmp(line, "Size: ", 6) == 0;
        }
        SDLTest_AssertCheck(aboutHasCore, "About shows the loaded core");
        SDLTest_AssertCheck(aboutHasContent, "About shows the loaded content");
        SDLTest_AssertCheck(aboutHasSize, "About shows the video size");
#endif

        SDL_Libretro_DestroyMenu(menu);
    }

    // Theme persistence round-trip through the config file.
    SDL_RemovePath("menu_test.cfg");
    SDL_Libretro* lrSave = SDL_Libretro_Create();
    SDL_Libretro_InitConfigFile(lrSave, "menu_test.cfg");
    SDL_Libretro_SetRenderer(lrSave, renderer);
    SDL_LibretroMenu* menuSave = SDL_Libretro_CreateMenu(lrSave);
    if (menuSave != NULL) {
        SDL_Libretro_SetMenuStyle(menuSave, SDL_LIBRETRO_MENU_STYLE_DRACULA);

        // UI scale persists like the theme.
        menuSave->uiScaleIndex = 3;
        SDL_Libretro_MenuUIScaleChanged(NULL, menuSave);

        // Mute captures the pre-mute volume and silences the output.
        SDL_Libretro_SetVolume(lrSave, 0.5f);
        menuSave->muteChecked = nk_true;
        SDL_Libretro_MenuMuteChanged(menuSave, NULL);
        SDLTest_AssertCheck(SDL_Libretro_GetVolume(lrSave) == 0.0f, "Mute drops the volume to zero");

        SDL_Libretro_DestroyMenu(menuSave);
    }
    SDL_strlcpy(lrSave->fileBrowserStartDirectory, "roms", sizeof(lrSave->fileBrowserStartDirectory));
    // A rebound key persists through the config file alongside the menu state.
    SDL_Libretro_SetKeyboardMapping(lrSave, RETRO_DEVICE_ID_JOYPAD_B, SDL_SCANCODE_K);
    SDL_Libretro_SetGamepadMapping(lrSave, 1, RETRO_DEVICE_ID_JOYPAD_B, SDL_GAMEPAD_BUTTON_NORTH);
    SDL_Libretro_SetAnalogToDigital(lrSave, 1, SDL_LIBRETRO_ANALOG_TO_DIGITAL_LEFT);
    SDL_Libretro_SetMousePort(lrSave, 1);
    SDL_Libretro_SetRewindMemoryLimit(lrSave, (size_t)48 << 20);
    SDL_Libretro_Destroy(lrSave); // Writes the config file.

    SDL_Libretro* lrLoad = SDL_Libretro_Create();
    SDL_Libretro_InitConfigFile(lrLoad, "menu_test.cfg");
    SDL_Libretro_SetRenderer(lrLoad, renderer);
    SDL_LibretroMenu* menuLoad = SDL_Libretro_CreateMenu(lrLoad);
    SDLTest_AssertCheck(menuLoad != NULL && SDL_Libretro_GetMenuStyle(menuLoad) == SDL_LIBRETRO_MENU_STYLE_DRACULA,
        "Menu theme persists through the config file");
    SDLTest_AssertCheck(menuLoad != NULL && menuLoad->muteChecked == nk_true, "Mute state persists through the config file");
    SDLTest_AssertCheck(SDL_Libretro_GetVolume(lrLoad) == 0.0f, "Volume stays muted after reload");
    SDLTest_AssertCheck(lrLoad->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_B] == SDL_SCANCODE_K,
        "Keyboard bindings persist through the config file");
    SDLTest_AssertCheck(lrLoad->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_X,
        "Untouched bindings keep their defaults");
    SDLTest_AssertCheck(lrLoad->gamepadButtons[1][RETRO_DEVICE_ID_JOYPAD_B] == SDL_GAMEPAD_BUTTON_NORTH,
        "Gamepad bindings persist through the config file per port");
    SDLTest_AssertCheck(lrLoad->gamepadButtons[0][RETRO_DEVICE_ID_JOYPAD_B] == SDL_GAMEPAD_BUTTON_SOUTH,
        "Untouched gamepad ports keep their defaults");
    SDLTest_AssertCheck(SDL_Libretro_GetAnalogToDigital(lrLoad, 1) == SDL_LIBRETRO_ANALOG_TO_DIGITAL_LEFT,
        "Analog to Digital persists through the config file per port");
    SDLTest_AssertCheck(SDL_Libretro_GetAnalogToDigital(lrLoad, 0) == SDL_LIBRETRO_ANALOG_TO_DIGITAL_NONE,
        "Analog to Digital elsewhere stays None");
    SDLTest_AssertCheck(SDL_Libretro_GetMousePort(lrLoad) == 1, "Mouse port persists through the config file");
    SDLTest_AssertCheck(SDL_Libretro_GetRewindMemoryLimit(lrLoad) == (size_t)48 << 20,
        "Rewind memory limit persists through the config file");
    if (menuLoad != NULL) {
        menuLoad->muteChecked = nk_false;
        SDL_Libretro_MenuMuteChanged(menuLoad, NULL);
        SDLTest_AssertCheck(SDL_fabsf(SDL_Libretro_GetVolume(lrLoad) - 0.5f) < 0.001f,
            "Unmute restores the pre-mute volume");
    }
    SDL_Libretro_DestroyMenu(menuLoad);
    SDL_Libretro_Destroy(lrLoad);
    SDL_RemovePath("menu_test.cfg");

    SDL_Libretro_Destroy(lr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TEST_COMPLETED;
}
#endif /* SDL_LIBRETRO_ENABLE_MENU */

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
    LIBRETRO_TEST_CASE(test_DRCDisable,       "Disabling DRC unwinds the ratio nudge"),
    LIBRETRO_TEST_CASE(test_Input,            "Keyboard mapping, virtual buttons, port device"),
    LIBRETRO_TEST_CASE(test_GamepadEvents,    "Gamepad hotplug events update gamepadCount"),
    LIBRETRO_TEST_CASE(test_Options,          "Core options on empty list"),
    LIBRETRO_TEST_CASE(test_SetVariablesLegacy, "Legacy SET_VARIABLES parsing without truncation"),
    LIBRETRO_TEST_CASE(test_PerfInterface,   "Perf interface CPU features and counters"),
    LIBRETRO_TEST_CASE(test_Rewind,           "Rewind buffer setup and speed"),
    LIBRETRO_TEST_CASE(test_RewindBuffer,     "Rewind codec round-trip and capture/step"),
    LIBRETRO_TEST_CASE(test_Memory,           "Memory get/set, save/load, and memory map"),
    LIBRETRO_TEST_CASE(test_SavePath,         "Derived save path and game reload"),
    LIBRETRO_TEST_CASE(test_OptionVisibility, "Core options, categories, and SET_CORE_OPTIONS_DISPLAY"),
    LIBRETRO_TEST_CASE(test_UpdateOptionVisibility, "Update option visibility via core callback"),
    LIBRETRO_TEST_CASE(test_OptionsDirtyIndependent, "Core and app consume independent dirty flags"),
    LIBRETRO_TEST_CASE(test_OptionReregister, "Re-registering an option updates it"),
    LIBRETRO_TEST_CASE(test_RewindConfig,     "Rewind settings round-trip through the config"),
    LIBRETRO_TEST_CASE(test_LateOptionConfig, "Late-registered options get their saved values"),
    LIBRETRO_TEST_CASE(test_LoadCore,         "Load test core and verify metadata"),
    LIBRETRO_TEST_CASE(test_Subsystem,        "Subsystem info incl. per-rom memory descriptors"),
    LIBRETRO_TEST_CASE(test_LoadGame,         "Load game, run frames, save/load state"),
    LIBRETRO_TEST_CASE(test_FrameTime,         "Frame-time callback receives measured deltas"),
    LIBRETRO_TEST_CASE(test_GameInfoExt,       "Extended game info via GET_GAME_INFO_EXT"),
    LIBRETRO_TEST_CASE(test_LoadGameNoContent, "Load game with no content file"),
    LIBRETRO_TEST_CASE(test_LoadGameSpecial,   "Load subsystem content by name, description, and id"),
    LIBRETRO_TEST_CASE(test_LoadGameIO,        "Load game from an SDL_IOStream"),
    LIBRETRO_TEST_CASE(test_LoadGameIOFullpath, "IO load spills need_fullpath content to disk"),
#ifdef TEST_CONTENT_ZIP_PATH
    LIBRETRO_TEST_CASE(test_PhysFSLoadGameZip, "Load game out of a zip via PhysFS"),
    LIBRETRO_TEST_CASE(test_PhysFSLoadGameZipFullpath, "need_fullpath content inside a zip served via the VFS"),
    LIBRETRO_TEST_CASE(test_PhysFSBlockExtract, "block_extract cores receive the raw archive"),
    LIBRETRO_TEST_CASE(test_PhysFSNonZip,      "PhysFS loader passes non-zip content through"),
    LIBRETRO_TEST_CASE(test_PhysFSPickPriority, "Content pick prefers disc metadata and base-name matches"),
    LIBRETRO_TEST_CASE(test_PhysFSNestedDir,   "Content in a zip subdirectory is found"),
    LIBRETRO_TEST_CASE(test_PhysFSLoadSecondZip, "Loading a second zip keeps it mounted"),
    LIBRETRO_TEST_CASE(test_PhysFSSpillFallback, "Full-path cores without VFS get an extracted file"),
#endif
    LIBRETRO_TEST_CASE(test_LoadGameNoRenderer, "Load game without a renderer, attach one later"),
    LIBRETRO_TEST_CASE(test_SetRenderer,       "SetRenderer NULL handling and renderer swap rebuild"),
    LIBRETRO_TEST_CASE(test_Rotation,          "Rotation-aware fit rect and pointer mapping"),
    LIBRETRO_TEST_CASE(test_MouseMotion,       "Mouse deltas come from relative motion events"),
    LIBRETRO_TEST_CASE(test_MousePort,         "Mouse, pointer, and lightgun answer one port"),
    LIBRETRO_TEST_CASE(test_InputDescriptorCopy, "Input descriptors and controller info are deep-copied"),
    LIBRETRO_TEST_CASE(test_LoadGameFailure,   "Failed load resets content state cleanly"),
    LIBRETRO_TEST_CASE(test_ContentExtension,  "Content extension extraction"),
    LIBRETRO_TEST_CASE(test_ExtensionInList,   "Extension-in-pipe-list matching"),
    LIBRETRO_TEST_CASE(test_PortDeviceOnLoad, "Port devices re-applied after game load"),
    LIBRETRO_TEST_CASE(test_DiskControl,      "Disk control eject/insert/swap/add"),
    LIBRETRO_TEST_CASE(test_DiskImageIO,      "Disk images from streams keep a path and buffer"),
    LIBRETRO_TEST_CASE(test_AudioLatency,     "Audio latency get/set and sample rate"),
    LIBRETRO_TEST_CASE(test_AudioInitPausedRewind, "Audio init survives paused and rewind speeds"),
    LIBRETRO_TEST_CASE(test_AudioZeroSampleRate, "Audio falls back to default sample rate when core reports 0"),
    LIBRETRO_TEST_CASE(test_AudioQueueOverflow, "Audio queue hard cap trims overflow"),
    LIBRETRO_TEST_CASE(test_PixelFormats,     "Pixel format switch (RGB565, XRGB8888, 0RGB1555)"),
    LIBRETRO_TEST_CASE(test_Cheats,           "Cheat set/reset with and without core"),
    LIBRETRO_TEST_CASE(test_OSD,              "OSD message push, query, duplicate, and clear"),
    LIBRETRO_TEST_CASE(test_VFSTruncate,      "VFS truncate keeps the handle usable"),
    LIBRETRO_TEST_CASE(test_Events,           "SDL event pushes: env dispatch, watches, and lifecycle"),
#ifdef SDL_LIBRETRO_ENABLE_MENU
    LIBRETRO_TEST_CASE(test_Menu,             "Menu create/toggle/update/render lifecycle"),
#endif
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
