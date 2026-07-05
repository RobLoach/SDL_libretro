#define SDL_LIBRETRO_IMPLEMENTATION
#include "SDL_libretro.h"
#include "SDL_libretro_minizip.h"

#include <SDL3/SDL_main.h>
#include <SDL3/SDL_test.h>

/* The fixture archive (test/test_content.zip) contains:
 *   - test_content.txt  ("SDL_libretro test content\n") — loaded as content
 *   - extra.dat         ("EXTRA data payload\n")        — read by the core via VFS
 * The mock core (test_core.c) claims the "txt" extension and, during
 * retro_load_game, publishes GET_GAME_INFO_EXT and reads "extra.dat" through the
 * libretro VFS, recording the results in RETRO_MEMORY_SYSTEM_RAM (game_info_probe):
 *   [0..15] lower-case ext, [16] persistent_data, [17] data != NULL,
 *   [18] read "extra.dat" from the archive via the VFS. */

static int SDLCALL test_LoadGameZip(void *arg) {
    (void)arg;
#if !defined(TEST_CORE_PATH) || !defined(TEST_ZIP_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH / TEST_ZIP_PATH not defined");
#else
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDLTest_AssertCheck(lr != NULL, "Create context");
    SDLTest_AssertCheck(SDL_Libretro_LoadCore(lr, TEST_CORE_PATH), "Load test core");

    SDLTest_AssertCheck(SDL_Libretro_LoadGame_Zip(lr, TEST_ZIP_PATH),
                        "LoadGame_Zip auto-detects and loads content");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr), "Game is ready after zip load");

    // The archive is mounted for the life of the game.
    SDLTest_AssertCheck(lr->zipMount != NULL, "Archive stays mounted after load");

    // Content was extracted from the archive and handed to the core (copied to SAVE_RAM).
    size_t sramSz = 0;
    const char* sram = (const char*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SAVE_RAM, &sramSz);
    SDLTest_AssertCheck(sram != NULL && SDL_strncmp(sram, "SDL_libretro", 12) == 0,
                        "Content extracted from zip into SAVE_RAM");

    // The extended game info and the post-load VFS read the core recorded.
    size_t sz = 0;
    const Uint8* probe = (const Uint8*)SDL_Libretro_GetMemoryData(lr, RETRO_MEMORY_SYSTEM_RAM, &sz);
    SDLTest_AssertCheck(probe != NULL && sz >= 19, "Probe memory available");
    if (probe) {
        SDLTest_AssertCheck(SDL_strncmp((const char*)probe, "txt", 3) == 0, "GET_GAME_INFO_EXT ext is 'txt'");
        SDLTest_AssertCheck(probe[16] == 1, "GET_GAME_INFO_EXT persistent_data is true");
        SDLTest_AssertCheck(probe[17] == 1, "GET_GAME_INFO_EXT data is non-NULL during load");
        SDLTest_AssertCheck(probe[18] == 1, "Core read 'extra.dat' from the archive via the VFS");
        // Directory enumeration over the archive root.
        SDLTest_AssertCheck(probe[19] == 1, "opendir/readdir listed 'extra.dat' in the archive root");
        SDLTest_AssertCheck(probe[20] == 1, "dirent_is_dir reported 'data/' as a directory");
        SDLTest_AssertCheck(probe[21] == 3, "Enumerated 3 root entries (got %u)", probe[21]);
    }

    // Unloading releases the mount and restores the default VFS.
    SDL_Libretro_UnloadGame(lr);
    SDLTest_AssertCheck(lr->zipMount == NULL, "Archive unmounted after UnloadGame");

    SDL_Libretro_Destroy(lr);
#endif
    return TEST_COMPLETED;
}

static int SDLCALL test_LoadGameZipEntry(void *arg) {
    (void)arg;
#if !defined(TEST_CORE_PATH) || !defined(TEST_ZIP_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH / TEST_ZIP_PATH not defined");
#else
    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    // Explicitly name the inner entry.
    SDLTest_AssertCheck(SDL_Libretro_LoadGame_ZipEntry(lr, TEST_ZIP_PATH, "test_content.txt"),
                        "LoadGame_ZipEntry loads the named entry");
    SDLTest_AssertCheck(SDL_Libretro_IsGameReady(lr), "Game is ready");

    // A missing entry fails cleanly and leaves no game/mount behind.
    SDLTest_AssertCheck(!SDL_Libretro_LoadGame_ZipEntry(lr, TEST_ZIP_PATH, "does_not_exist.bin"),
                        "Missing entry fails");
    SDLTest_AssertCheck(!SDL_Libretro_IsGameReady(lr), "No game after failed entry load");
    SDLTest_AssertCheck(lr->zipMount == NULL, "No mount after failed entry load");

    SDL_Libretro_Destroy(lr);
#endif
    return TEST_COMPLETED;
}

static int SDLCALL test_LoadGameZipInvalid(void *arg) {
    (void)arg;
#if !defined(TEST_CORE_PATH) || !defined(TEST_ZIP_PATH)
    SDLTest_AssertCheck(false, "TEST_CORE_PATH / TEST_ZIP_PATH not defined");
#else
    // NULL-argument safety.
    SDLTest_AssertCheck(!SDL_Libretro_LoadGame_Zip(NULL, TEST_ZIP_PATH), "LoadGame_Zip(NULL, ...) false");

    SDL_Libretro* lr = SDL_Libretro_Create();
    SDL_Libretro_LoadCore(lr, TEST_CORE_PATH);

    SDLTest_AssertCheck(!SDL_Libretro_LoadGame_Zip(lr, NULL), "LoadGame_Zip(lr, NULL) false");
    SDLTest_AssertCheck(!SDL_Libretro_LoadGame_Zip(lr, "/no/such/file.zip"), "Nonexistent archive fails");
    SDLTest_AssertCheck(!SDL_Libretro_IsGameReady(lr), "No game after failed archive open");
    SDLTest_AssertCheck(lr->zipMount == NULL, "No mount after failed archive open");

    SDL_Libretro_Destroy(lr);
#endif
    return TEST_COMPLETED;
}

#define LIBRETRO_TEST_CASE(fn, desc) \
    &(const SDLTest_TestCaseReference){ fn, #fn, desc, TEST_ENABLED }

static const SDLTest_TestCaseReference *testCases[] = {
    LIBRETRO_TEST_CASE(test_LoadGameZip,        "Load content from a .zip, verify VFS reads post-load"),
    LIBRETRO_TEST_CASE(test_LoadGameZipEntry,   "Load a named entry; missing entry fails cleanly"),
    LIBRETRO_TEST_CASE(test_LoadGameZipInvalid, "Invalid arguments and archives fail cleanly"),
    NULL
};

static const SDLTest_TestSuiteReference testSuite = {
    "SDL_libretro_minizip",
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
