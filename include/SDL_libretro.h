/*
 * SDL_libretro - SDL3 libretro frontend library
 *
 * Copyright (c) 2020-2026 Rob Loach (@RobLoach)
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

#endif /* SDL_LIBRETRO_H */
