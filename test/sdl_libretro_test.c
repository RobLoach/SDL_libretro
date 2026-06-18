#define SDL_LIBRETRO_IMPLEMENTATION
#include "SDL_libretro.h"

static void test_create_destroy(void) {
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_assert(lr != NULL);

    SDL_assert(lr->volume == 1.0f);
    SDL_assert(lr->speed == 1.0f);
    SDL_assert(SDL_strcmp(lr->username, "SDL_libretro") == 0);

    SDL_assert(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_B] == SDL_SCANCODE_Z);
    SDL_assert(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_X);
    SDL_assert(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_START] == SDL_SCANCODE_RETURN);
    SDL_assert(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_UP] == SDL_SCANCODE_UP);

    SDL_Libretro_Destroy(lr);
    SDL_Libretro_Destroy(NULL);
}

static void test_state_queries(void) {
    SDL_Libretro* lr = SDL_Libretro_Create();
    int w = -1, h = -1;

    SDL_assert(SDL_Libretro_IsCoreReady(lr) == false);
    SDL_assert(SDL_Libretro_IsGameReady(lr) == false);
    SDL_assert(SDL_Libretro_ShouldClose(lr) == false);
    SDL_assert(SDL_Libretro_GetTexture(lr) == NULL);
    SDL_Libretro_GetSize(lr, &w, &h);
    SDL_assert(w == 0);
    SDL_assert(h == 0);
    SDL_assert(SDL_Libretro_GetAspectRatio(lr) == 0.0f);
    SDL_assert(SDL_Libretro_GetFPS(lr) == 0.0);
    SDL_assert(SDL_Libretro_GetRotation(lr) == 0);
    SDL_assert(SDL_strcmp(SDL_Libretro_GetCoreName(lr), "") == 0);
    SDL_assert(SDL_strcmp(SDL_Libretro_GetCoreVersion(lr), "") == 0);
    SDL_assert(SDL_strcmp(SDL_Libretro_GetValidExtensions(lr), "") == 0);
    SDL_assert(SDL_Libretro_GetSerializeSize(lr) == 0);
    SDL_assert(SDL_Libretro_GetOptionCount(lr) == 0);

    /* NULL safety */
    SDL_assert(SDL_Libretro_IsCoreReady(NULL) == false);
    SDL_assert(SDL_Libretro_IsGameReady(NULL) == false);
    SDL_assert(SDL_Libretro_ShouldClose(NULL) == false);
    SDL_assert(SDL_Libretro_GetTexture(NULL) == NULL);
    w = -1; h = -1;
    SDL_Libretro_GetSize(NULL, &w, &h);
    SDL_assert(w == 0);
    SDL_assert(h == 0);
    SDL_assert(SDL_Libretro_GetAspectRatio(NULL) == 0.0f);
    SDL_assert(SDL_Libretro_GetFPS(NULL) == 0.0);
    SDL_assert(SDL_Libretro_GetRotation(NULL) == 0);
    SDL_assert(SDL_strcmp(SDL_Libretro_GetCoreName(NULL), "") == 0);
    SDL_assert(SDL_Libretro_GetVolume(NULL) == 0.0f);
    SDL_assert(SDL_Libretro_GetSpeed(NULL) == 1.0f);
    SDL_assert(SDL_Libretro_GetSerializeSize(NULL) == 0);

    SDL_Libretro_Destroy(lr);
}

static void test_directory_setters(void) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    SDL_assert(SDL_Libretro_SetCoreDirectory(lr, "/tmp/cores") == true);
    SDL_assert(SDL_strcmp(lr->coreDirectory, "/tmp/cores") == 0);
    SDL_assert(SDL_Libretro_SetSaveDirectory(lr, "/tmp/saves") == true);
    SDL_assert(SDL_strcmp(lr->saveDirectory, "/tmp/saves") == 0);
    SDL_assert(SDL_Libretro_SetSystemDirectory(lr, "/tmp/system") == true);
    SDL_assert(SDL_strcmp(lr->systemDirectory, "/tmp/system") == 0);
    SDL_assert(SDL_Libretro_SetCoreAssetsDirectory(lr, "/tmp/assets") == true);
    SDL_assert(SDL_strcmp(lr->coreAssetsDirectory, "/tmp/assets") == 0);
    SDL_assert(SDL_Libretro_SetUsername(lr, "TestUser") == true);
    SDL_assert(SDL_strcmp(lr->username, "TestUser") == 0);

    /* NULL lr */
    SDL_assert(SDL_Libretro_SetCoreDirectory(NULL, "/tmp") == false);
    SDL_assert(SDL_Libretro_SetSaveDirectory(NULL, "/tmp") == false);
    SDL_assert(SDL_Libretro_SetSystemDirectory(NULL, "/tmp") == false);
    SDL_assert(SDL_Libretro_SetCoreAssetsDirectory(NULL, "/tmp") == false);
    SDL_assert(SDL_Libretro_SetUsername(NULL, "x") == false);

    /* NULL path stores empty string */
    SDL_assert(SDL_Libretro_SetCoreDirectory(lr, NULL) == true);
    SDL_assert(SDL_strcmp(lr->coreDirectory, "") == 0);
    SDL_assert(SDL_Libretro_SetUsername(lr, NULL) == true);
    SDL_assert(SDL_strcmp(lr->username, "") == 0);

    SDL_Libretro_Destroy(lr);
}

static void test_volume_speed(void) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    /* Volume defaults and clamping */
    SDL_assert(SDL_Libretro_GetVolume(lr) == 1.0f);
    SDL_Libretro_SetVolume(lr, 0.5f);
    SDL_assert(SDL_Libretro_GetVolume(lr) == 0.5f);
    SDL_Libretro_SetVolume(lr, -1.0f);
    SDL_assert(SDL_Libretro_GetVolume(lr) == 0.0f);
    SDL_Libretro_SetVolume(lr, 2.0f);
    SDL_assert(SDL_Libretro_GetVolume(lr) == 1.0f);
    SDL_Libretro_SetVolume(NULL, 0.5f);

    /* Speed defaults and clamping */
    SDL_assert(SDL_Libretro_GetSpeed(lr) == 1.0f);
    SDL_Libretro_SetSpeed(lr, 2.0f);
    SDL_assert(SDL_Libretro_GetSpeed(lr) == 2.0f);
    SDL_Libretro_SetSpeed(lr, 0.0f);
    SDL_assert(SDL_Libretro_GetSpeed(lr) == 0.1f);
    SDL_Libretro_SetSpeed(lr, -5.0f);
    SDL_assert(SDL_Libretro_GetSpeed(lr) == 0.1f);
    SDL_Libretro_SetSpeed(lr, 10.0f);
    SDL_assert(SDL_Libretro_GetSpeed(lr) == 10.0f);
    SDL_Libretro_SetSpeed(NULL, 2.0f);

    SDL_Libretro_Destroy(lr);
}

static void test_input(void) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    /* Keyboard mapping */
    SDL_assert(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_X);
    SDL_Libretro_SetKeyboardMapping(lr, RETRO_DEVICE_ID_JOYPAD_A, SDL_SCANCODE_SPACE);
    SDL_assert(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_SPACE);
    SDL_Libretro_SetKeyboardMapping(lr, -1, SDL_SCANCODE_Z);
    SDL_assert(lr->keyboardPlayer1[RETRO_DEVICE_ID_JOYPAD_A] == SDL_SCANCODE_SPACE);
    SDL_Libretro_SetKeyboardMapping(NULL, 0, SDL_SCANCODE_Z);

    /* Virtual button */
    SDL_Libretro_SetVirtualButton(lr, 0, RETRO_DEVICE_ID_JOYPAD_A, true);
    SDL_assert(lr->core.virtualJoypadState[RETRO_DEVICE_ID_JOYPAD_A] == true);
    SDL_Libretro_SetVirtualButton(lr, 0, RETRO_DEVICE_ID_JOYPAD_A, false);
    SDL_assert(lr->core.virtualJoypadState[RETRO_DEVICE_ID_JOYPAD_A] == false);
    SDL_Libretro_SetVirtualButton(lr, 16, 0, true);
    SDL_Libretro_SetVirtualButton(lr, 0, 16, true);
    SDL_Libretro_SetVirtualButton(NULL, 0, 0, true);

    /* Port device */
    SDL_assert(SDL_Libretro_SetPortDevice(lr, 0, RETRO_DEVICE_JOYPAD) == true);
    SDL_assert(lr->core.portDeviceMap[0] == RETRO_DEVICE_JOYPAD);
    SDL_assert(SDL_Libretro_SetPortDevice(lr, 15, RETRO_DEVICE_JOYPAD) == true);
    SDL_assert(SDL_Libretro_SetPortDevice(lr, 16, RETRO_DEVICE_JOYPAD) == false);
    SDL_assert(SDL_Libretro_SetPortDevice(NULL, 0, RETRO_DEVICE_JOYPAD) == false);

    SDL_Libretro_Destroy(lr);
}

static void test_options(void) {
    SDL_Libretro* lr = SDL_Libretro_Create();

    SDL_assert(SDL_Libretro_GetOptionCount(lr) == 0);
    SDL_assert(SDL_Libretro_GetOptionKey(lr, 0) == NULL);
    SDL_assert(SDL_Libretro_GetOptionValue(lr, "foo") == NULL);
    SDL_assert(SDL_Libretro_AreOptionsDirty(lr) == false);
    SDL_assert(SDL_Libretro_SetOptionValue(lr, "key", "val") == false);
    SDL_assert(SDL_Libretro_ResetOption(lr, "key") == false);
    SDL_Libretro_ResetAllOptions(lr);

    /* NULL safety */
    SDL_assert(SDL_Libretro_GetOptionCount(NULL) == 0);
    SDL_assert(SDL_Libretro_GetOptionKey(NULL, 0) == NULL);
    SDL_assert(SDL_Libretro_GetOptionValue(NULL, "foo") == NULL);

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

    SDL_Log("All tests passed");
    SDL_Quit();
    return 0;
}
