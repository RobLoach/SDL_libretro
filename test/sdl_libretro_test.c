#define SDL_LIBRETRO_IMPLEMENTATION
#include "SDL_libretro.h"

static int test_pass = 0;
static int test_fail = 0;

#define TEST_ASSERT(expr) do { \
    if (expr) { test_pass++; } \
    else { test_fail++; SDL_Log("FAIL: %s:%d: %s", __FILE__, __LINE__, #expr); } \
} while (0)

#define TEST_ASSERT_STR(expected, actual) do { \
    const char* _e = (expected); const char* _a = (actual); \
    if (_e && _a && SDL_strcmp(_e, _a) == 0) { test_pass++; } \
    else if (!_e && !_a) { test_pass++; } \
    else { test_fail++; SDL_Log("FAIL: %s:%d: expected \"%s\", got \"%s\"", \
        __FILE__, __LINE__, _e ? _e : "(null)", _a ? _a : "(null)"); } \
} while (0)

static void test_create_destroy(void) {
    SDL_Libretro* lr = SDL_Libretro_Create();
    TEST_ASSERT(lr != NULL);

    TEST_ASSERT(lr->volume == 1.0f);
    TEST_ASSERT(lr->speed == 1.0f);
    TEST_ASSERT_STR("SDL_libretro", lr->username);

    TEST_ASSERT(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_B] == SDL_SCANCODE_Z);
    TEST_ASSERT(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_X);
    TEST_ASSERT(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_START] == SDL_SCANCODE_RETURN);
    TEST_ASSERT(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_UP] == SDL_SCANCODE_UP);

    SDL_Libretro_Destroy(lr);
    SDL_Libretro_Destroy(NULL);
}

static void test_state_queries(void) {
    SDL_Libretro* lr = SDL_Libretro_Create();
    int w = -1, h = -1;

    TEST_ASSERT(SDL_Libretro_IsCoreReady(lr) == false);
    TEST_ASSERT(SDL_Libretro_IsGameReady(lr) == false);
    TEST_ASSERT(SDL_Libretro_ShouldClose(lr) == false);
    TEST_ASSERT(SDL_Libretro_GetTexture(lr) == NULL);
    SDL_Libretro_GetSize(lr, &w, &h);
    TEST_ASSERT(w == 0);
    TEST_ASSERT(h == 0);
    TEST_ASSERT(SDL_Libretro_GetAspectRatio(lr) == 0.0f);
    TEST_ASSERT(SDL_Libretro_GetFPS(lr) == 0.0);
    TEST_ASSERT(SDL_Libretro_GetRotation(lr) == 0);
    TEST_ASSERT_STR("", SDL_Libretro_GetCoreName(lr));
    TEST_ASSERT_STR("", SDL_Libretro_GetCoreVersion(lr));
    TEST_ASSERT_STR("", SDL_Libretro_GetValidExtensions(lr));
    TEST_ASSERT(SDL_Libretro_GetSerializeSize(lr) == 0);
    TEST_ASSERT(SDL_Libretro_GetOptionCount(lr) == 0);

    /* NULL safety */
    TEST_ASSERT(SDL_Libretro_IsCoreReady(NULL) == false);
    TEST_ASSERT(SDL_Libretro_IsGameReady(NULL) == false);
    TEST_ASSERT(SDL_Libretro_ShouldClose(NULL) == false);
    TEST_ASSERT(SDL_Libretro_GetTexture(NULL) == NULL);
    w = -1; h = -1;
    SDL_Libretro_GetSize(NULL, &w, &h);
    TEST_ASSERT(w == 0);
    TEST_ASSERT(h == 0);
    TEST_ASSERT(SDL_Libretro_GetAspectRatio(NULL) == 0.0f);
    TEST_ASSERT(SDL_Libretro_GetFPS(NULL) == 0.0);
    TEST_ASSERT(SDL_Libretro_GetRotation(NULL) == 0);
    TEST_ASSERT_STR("", SDL_Libretro_GetCoreName(NULL));
    TEST_ASSERT(SDL_Libretro_GetVolume(NULL) == 0.0f);
    TEST_ASSERT(SDL_Libretro_GetSpeed(NULL) == 1.0f);
    TEST_ASSERT(SDL_Libretro_GetSerializeSize(NULL) == 0);

    SDL_Libretro_Destroy(lr);
}

static void test_directory_setters(void) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    TEST_ASSERT(SDL_Libretro_SetCoreDirectory(lr, "/tmp/cores") == true);
    TEST_ASSERT_STR("/tmp/cores", lr->coreDirectory);
    TEST_ASSERT(SDL_Libretro_SetSaveDirectory(lr, "/tmp/saves") == true);
    TEST_ASSERT_STR("/tmp/saves", lr->saveDirectory);
    TEST_ASSERT(SDL_Libretro_SetSystemDirectory(lr, "/tmp/system") == true);
    TEST_ASSERT_STR("/tmp/system", lr->systemDirectory);
    TEST_ASSERT(SDL_Libretro_SetCoreAssetsDirectory(lr, "/tmp/assets") == true);
    TEST_ASSERT_STR("/tmp/assets", lr->coreAssetsDirectory);
    TEST_ASSERT(SDL_Libretro_SetUsername(lr, "TestUser") == true);
    TEST_ASSERT_STR("TestUser", lr->username);

    /* NULL lr */
    TEST_ASSERT(SDL_Libretro_SetCoreDirectory(NULL, "/tmp") == false);
    TEST_ASSERT(SDL_Libretro_SetSaveDirectory(NULL, "/tmp") == false);
    TEST_ASSERT(SDL_Libretro_SetSystemDirectory(NULL, "/tmp") == false);
    TEST_ASSERT(SDL_Libretro_SetCoreAssetsDirectory(NULL, "/tmp") == false);
    TEST_ASSERT(SDL_Libretro_SetUsername(NULL, "x") == false);

    /* NULL path stores empty string */
    TEST_ASSERT(SDL_Libretro_SetCoreDirectory(lr, NULL) == true);
    TEST_ASSERT_STR("", lr->coreDirectory);
    TEST_ASSERT(SDL_Libretro_SetUsername(lr, NULL) == true);
    TEST_ASSERT_STR("", lr->username);

    SDL_Libretro_Destroy(lr);
}

static void test_volume_speed(void) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    /* Volume defaults and clamping */
    TEST_ASSERT(SDL_Libretro_GetVolume(lr) == 1.0f);
    SDL_Libretro_SetVolume(lr, 0.5f);
    TEST_ASSERT(SDL_Libretro_GetVolume(lr) == 0.5f);
    SDL_Libretro_SetVolume(lr, -1.0f);
    TEST_ASSERT(SDL_Libretro_GetVolume(lr) == 0.0f);
    SDL_Libretro_SetVolume(lr, 2.0f);
    TEST_ASSERT(SDL_Libretro_GetVolume(lr) == 1.0f);
    SDL_Libretro_SetVolume(NULL, 0.5f);

    /* Speed defaults and clamping */
    TEST_ASSERT(SDL_Libretro_GetSpeed(lr) == 1.0f);
    SDL_Libretro_SetSpeed(lr, 2.0f);
    TEST_ASSERT(SDL_Libretro_GetSpeed(lr) == 2.0f);
    SDL_Libretro_SetSpeed(lr, 0.0f);
    TEST_ASSERT(SDL_Libretro_GetSpeed(lr) == 0.1f);
    SDL_Libretro_SetSpeed(lr, -5.0f);
    TEST_ASSERT(SDL_Libretro_GetSpeed(lr) == 0.1f);
    SDL_Libretro_SetSpeed(lr, 10.0f);
    TEST_ASSERT(SDL_Libretro_GetSpeed(lr) == 10.0f);
    SDL_Libretro_SetSpeed(NULL, 2.0f);

    SDL_Libretro_Destroy(lr);
}

static void test_input(void) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    /* Keyboard mapping */
    TEST_ASSERT(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_X);
    SDL_Libretro_SetKeyboardMapping(lr, RETRO_DEVICE_ID_JOYPAD_A, SDL_SCANCODE_SPACE);
    TEST_ASSERT(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_SPACE);
    SDL_Libretro_SetKeyboardMapping(lr, -1, SDL_SCANCODE_Z);
    TEST_ASSERT(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_SPACE);
    SDL_Libretro_SetKeyboardMapping(NULL, 0, SDL_SCANCODE_Z);

    /* Virtual button */
    SDL_Libretro_SetVirtualButton(lr, 0, RETRO_DEVICE_ID_JOYPAD_A, true);
    TEST_ASSERT(lr->core.virtualJoypadState[RETRO_DEVICE_ID_JOYPAD_A] == true);
    SDL_Libretro_SetVirtualButton(lr, 0, RETRO_DEVICE_ID_JOYPAD_A, false);
    TEST_ASSERT(lr->core.virtualJoypadState[RETRO_DEVICE_ID_JOYPAD_A] == false);
    SDL_Libretro_SetVirtualButton(lr, 16, 0, true);
    SDL_Libretro_SetVirtualButton(lr, 0, 16, true);
    SDL_Libretro_SetVirtualButton(NULL, 0, 0, true);

    /* Port device */
    TEST_ASSERT(SDL_Libretro_SetPortDevice(lr, 0, RETRO_DEVICE_JOYPAD) == true);
    TEST_ASSERT(lr->core.portDeviceMap[0] == RETRO_DEVICE_JOYPAD);
    TEST_ASSERT(SDL_Libretro_SetPortDevice(lr, 15, RETRO_DEVICE_JOYPAD) == true);
    TEST_ASSERT(SDL_Libretro_SetPortDevice(lr, 16, RETRO_DEVICE_JOYPAD) == false);
    TEST_ASSERT(SDL_Libretro_SetPortDevice(NULL, 0, RETRO_DEVICE_JOYPAD) == false);

    SDL_Libretro_Destroy(lr);
}

static void test_options(void) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    TEST_ASSERT(SDL_Libretro_GetOptionCount(lr) == 0);
    TEST_ASSERT(SDL_Libretro_GetOptionKey(lr, 0) == NULL);
    TEST_ASSERT(SDL_Libretro_GetOptionValue(lr, "foo") == NULL);
    TEST_ASSERT(SDL_Libretro_AreOptionsDirty(lr) == false);
    TEST_ASSERT(SDL_Libretro_SetOptionValue(lr, "key", "val") == false);
    TEST_ASSERT(SDL_Libretro_ResetOption(lr, "key") == false);
    SDL_Libretro_ResetAllOptions(lr);

    /* NULL safety */
    TEST_ASSERT(SDL_Libretro_GetOptionCount(NULL) == 0);
    TEST_ASSERT(SDL_Libretro_GetOptionKey(NULL, 0) == NULL);
    TEST_ASSERT(SDL_Libretro_GetOptionValue(NULL, "foo") == NULL);

    SDL_Libretro_Destroy(lr);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    SDL_Init(0);

    test_create_destroy();
    test_state_queries();
    test_directory_setters();
    test_volume_speed();
    test_input();
    test_options();

    SDL_Log("Tests: %d passed, %d failed", test_pass, test_fail);
    SDL_Quit();
    return test_fail > 0 ? 1 : 0;
}
