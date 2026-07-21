/**
 * SDL_libretro - menu system built on nuklear_console
 *
 * Provides an in-app menu (Load Game, save states, core options, settings)
 * rendered with Nuklear through the SDL3 renderer backend, driven by
 * nuklear_console for keyboard/gamepad-friendly navigation.
 *
 * Enabled by defining SDL_LIBRETRO_ENABLE_MENU alongside
 * SDL_LIBRETRO_IMPLEMENTATION. Requires the vendored Nuklear,
 * nuklear_console, nuklear_gamepad, c-vector and tinydir submodules on the
 * include path (handled by the SDL_libretro_menu CMake target).
 *
 * Frame contract, with SDL_Libretro_MenuHandleEvent() called for each event:
 *
 *     if (!SDL_Libretro_IsMenuOpen(menu)) SDL_Libretro_Update(lr);
 *     SDL_Libretro_Render(renderer, lr, NULL);
 *     SDL_Libretro_UpdateMenu(menu);
 *     SDL_Libretro_RenderMenu(menu);
 *     SDL_RenderPresent(renderer);
 *
 * @file SDL_libretro_menu.h
 */

#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_MENU_IMPL_ONCE) && defined(SDL_LIBRETRO_ENABLE_MENU) && !defined(SDL_LIBRETRO_DISABLE_MENU)
#define SDL_LIBRETRO_MENU_IMPL_ONCE

// Nuklear, with the feature set the SDL3 renderer backend and nuklear_console
// require. Each define is guarded so an application can pre-configure Nuklear.
#ifndef NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_FIXED_TYPES
#endif
#ifndef NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_VARARGS
#endif
#ifndef NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#endif
#ifndef NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_IO
#endif
#ifndef NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_FONT_BAKING
#endif
#ifndef NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_DEFAULT_FONT
#endif
#ifndef NK_INCLUDE_COMMAND_USERDATA
#define NK_INCLUDE_COMMAND_USERDATA
#endif
#ifndef NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#endif
#ifndef NK_BUTTON_TRIGGER_ON_RELEASE
#define NK_BUTTON_TRIGGER_ON_RELEASE
#endif

#ifndef SDL_LIBRETRO_MENU_NO_NK_IMPLEMENTATION
#define NK_IMPLEMENTATION
#endif
#ifndef SDL_LIBRETRO_MENU_NUKLEAR_H
#define SDL_LIBRETRO_MENU_NUKLEAR_H "nuklear.h"
#endif
#include SDL_LIBRETRO_MENU_NUKLEAR_H

#ifndef SDL_LIBRETRO_MENU_NO_NK_IMPLEMENTATION
#define NK_SDL3_RENDERER_IMPLEMENTATION
#endif
#ifndef SDL_LIBRETRO_MENU_NUKLEAR_SDL3_RENDERER_H
#define SDL_LIBRETRO_MENU_NUKLEAR_SDL3_RENDERER_H "demo/sdl3_renderer/nuklear_sdl3_renderer.h"
#endif
#include SDL_LIBRETRO_MENU_NUKLEAR_SDL3_RENDERER_H

#ifndef SDL_LIBRETRO_MENU_NO_NK_IMPLEMENTATION
#define NK_GAMEPAD_IMPLEMENTATION
#endif
#ifndef SDL_LIBRETRO_MENU_NUKLEAR_GAMEPAD_H
#define SDL_LIBRETRO_MENU_NUKLEAR_GAMEPAD_H "nuklear_gamepad.h"
#endif
#include SDL_LIBRETRO_MENU_NUKLEAR_GAMEPAD_H

// The file browser enumerates directories through nuklear_console's SDL3
// backend, selected automatically since SDL is included first.
#ifndef SDL_LIBRETRO_MENU_NO_NK_IMPLEMENTATION
#define NK_CONSOLE_IMPLEMENTATION
#endif
#ifndef NK_CONSOLE_FILE_SDL_NATIVE_DIALOG
#define NK_CONSOLE_FILE_SDL_NATIVE_DIALOG
#endif
#ifndef SDL_LIBRETRO_MENU_NUKLEAR_CONSOLE_H
#define SDL_LIBRETRO_MENU_NUKLEAR_CONSOLE_H "nuklear_console.h"
#endif
#include SDL_LIBRETRO_MENU_NUKLEAR_CONSOLE_H
#include "nuklear_console_sdl.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifndef SDL_LIBRETRO_MENU_TOGGLE_KEY
/**
 * The keyboard key that toggles the menu.
 *
 * @see SDL_Libretro_MenuHandleEvent()
 */
#define SDL_LIBRETRO_MENU_TOGGLE_KEY SDLK_F1
#endif

#ifndef SDL_LIBRETRO_MENU_FONT_HEIGHT
/**
 * Base height in pixels for the menu font, multiplied by the window's display scale.
 */
#define SDL_LIBRETRO_MENU_FONT_HEIGHT 16
#endif

#ifndef SDL_LIBRETRO_MENU_DEFAULT_STYLE
/**
 * The style applied when the menu is created.
 *
 * @see SDL_Libretro_SetMenuStyle()
 */
#define SDL_LIBRETRO_MENU_DEFAULT_STYLE SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_MOCHA
#endif

/**
 * The style names, separated by a |, in SDL_LibretroMenuStyle order.
 */
#define SDL_LIBRETRO_MENU_STYLE_NAMES "Mocha|Latte|Frappe|Macchiato|Dracula|Dark"

/**
 * Per-option UI state for the "Core Options" submenu.
 *
 * The comboboxes and checkboxes write directly into these fields, so the array
 * must stay allocated for as long as the widgets exist.
 *
 * @internal
 */
typedef struct SDL_LibretroMenuOptionState {
    SDL_LibretroMenu* menu;
    unsigned index; /** The option's index within the core's option list. */
    int selected; /** Selected combobox entry. */
    nk_bool checked; /** Checkbox state for boolean options. */
    char* displayList; /** Pipe-separated labels backing the combobox; owned. */
} SDL_LibretroMenuOptionState;

/**
 * Per-port UI state for the "Controllers" submenu.
 *
 * @see SDL_LibretroMenuOptionState
 * @internal
 */
typedef struct SDL_LibretroMenuPortState {
    SDL_LibretroMenu* menu;
    unsigned port;
    int selected; /** Selected combobox entry. */
    char* deviceList; /** Pipe-separated device names backing the combobox; owned. */
    char label[16]; /** The "Port N" combobox label; must outlive the widget. */
} SDL_LibretroMenuPortState;

struct SDL_LibretroMenu {
    SDL_Libretro* lr;
    struct nk_context* ctx;
    nk_console* console;
    struct nk_gamepads gamepads;

    bool open; /** Whether the menu is currently shown. */
    bool wasOpen; /** The open state of the previous update, to detect fresh opens. */
    bool uiBuilt; /** nk_begin ran this frame, so RenderMenu must flush and clear. */

    // Widgets whose visibility depends on the running game.
    nk_console* resumeButton;
    nk_console* stateRow;
    nk_console* resetButton;
    nk_console* optionsButton;
    nk_console* controllersButton;
    nk_console* loadGameButton;

    // Settings state the widgets write into.
    int volumePercent;
    int fitModeIndex;
    int styleIndex;
    SDL_LibretroMenuStyle style; /** The active menu style. */
    int filterIndex; /** 0 = Nearest (Pixel Art where available), 1 = Linear. */
    nk_bool fullscreenChecked;
    nk_bool vsyncChecked;
    nk_bool muteChecked;
    float preMuteVolume; /** Volume to restore when unmuting. */

    // Load Game
    char loadGamePath[SDL_LIBRETRO_MAX_PATH];

    // Core Options rebuild tracking
    SDL_LibretroMenuOptionState* optionStates;
    unsigned optionStateCount;
    bool optionsStale; /** The Core Options submenu no longer matches the core's options. */
    bool settingsDirty; /** A menu-owned setting changed and awaits SDL_Libretro_MenuSaveState(). */
    unsigned builtOptionCount; /** Option count when the submenu was last built. */
    char builtCoreName[128]; /** Core library name when the submenu was last built. */

    // Controllers
    SDL_LibretroMenuPortState portStates[SDL_LIBRETRO_MAX_GAMEPADS];
};

/**
 * Closes the menu when a game is available to resume.
 *
 * Also used as the BACK handler of the top level menu.
 *
 * @internal
 */
static void SDL_Libretro_MenuResumeClicked(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    if (SDL_Libretro_IsGameReady(menu->lr)) {
        SDL_Libretro_SetMenuOpen(menu, false);
    }
}

/**
 * Load the game waiting in loadGamePath, closing the menu on success.
 *
 * @internal
 */
static void SDL_Libretro_MenuLoadPendingGame(SDL_LibretroMenu* menu) {
    if (menu->loadGamePath[0] == '\0') {
        return;
    }

    // Drop the previous core so the loader picks the right one by extension.
    SDL_Libretro_UnloadCore(menu->lr);

#if defined(SDL_LIBRETRO_ENABLE_PHYSFS) && !defined(SDL_LIBRETRO_DISABLE_PHYSFS)
    bool loaded = SDL_Libretro_PhysFS_LoadGame(menu->lr, menu->loadGamePath);
#else
    bool loaded = SDL_Libretro_LoadGame(menu->lr, menu->loadGamePath);
#endif
    if (loaded) {
        SDL_Libretro_SetMenuOpen(menu, false);
    }
    else {
        SDL_Log("Failed to load game: %s", SDL_GetError());
        nk_console_show_message(menu->console, "Failed to load game");
    }
    menu->loadGamePath[0] = '\0';
}

#ifndef __EMSCRIPTEN__
/**
 * @internal
 */
static void SDL_Libretro_MenuGameFileChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_Libretro_MenuLoadPendingGame((SDL_LibretroMenu*)user_data);
}
#endif

#ifdef __EMSCRIPTEN__
/**
 * Opens a browser file picker; the chosen file is written into the virtual
 * file system and handed back through SDL_Libretro_MenuLoadGameFromJS().
 *
 * Requires ccall and FS in -sEXPORTED_RUNTIME_METHODS.
 *
 * @internal
 */
EM_JS(void, SDL_Libretro_MenuOpenWebFilePicker, (SDL_LibretroMenu * menu), {
    var input = document.createElement('input');
    input.type = 'file';
    input.onchange = function(e) {
        var file = e.target.files[0];
        if (!file) return;
        var reader = new FileReader();
        reader.onload = function() {
            var path = '/' + file.name;
            try {
                Module.FS.writeFile(path, new Uint8Array(reader.result));
            } catch (err) {
                console.error('SDL_libretro: failed to write picked file to FS:', err);
                return;
            }
            Module.ccall('SDL_Libretro_MenuLoadGameFromJS', null, ['number', 'string'], [menu, path]);
        };
        reader.onerror = function() {
            console.error('SDL_libretro: failed to read picked file:', reader.error);
        };
        reader.readAsArrayBuffer(file);
    };
    input.click();
});

/**
 * Called from SDL_Libretro_MenuOpenWebFilePicker()'s JS once the picked file
 * lands in the virtual file system.
 *
 * @internal
 */
EMSCRIPTEN_KEEPALIVE void SDL_Libretro_MenuLoadGameFromJS(SDL_LibretroMenu* menu, const char* path) {
    if (menu == NULL || path == NULL) {
        return;
    }
    SDL_strlcpy(menu->loadGamePath, path, sizeof(menu->loadGamePath));
    SDL_Libretro_MenuLoadPendingGame(menu);
}

/**
 * @internal
 */
static void SDL_Libretro_MenuWebLoadGameClicked(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_Libretro_MenuOpenWebFilePicker((SDL_LibretroMenu*)user_data);
}
#endif /* __EMSCRIPTEN__ */

/**
 * @internal
 */
static void SDL_Libretro_MenuSaveStateClicked(nk_console* widget, void* user_data) {
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    char savePath[SDL_LIBRETRO_MAX_PATH];
    if (SDL_Libretro_GetSavePath(menu->lr, ".sav", savePath, sizeof(savePath)) > 0 &&
        SDL_Libretro_SaveState(menu->lr, savePath)) {
        nk_console_show_message(nk_console_get_top(widget), "State saved");
    }
    else {
        nk_console_show_message(nk_console_get_top(widget), "Save State failed");
    }
}

/**
 * @internal
 */
static void SDL_Libretro_MenuLoadStateClicked(nk_console* widget, void* user_data) {
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    char savePath[SDL_LIBRETRO_MAX_PATH];
    if (SDL_Libretro_GetSavePath(menu->lr, ".sav", savePath, sizeof(savePath)) > 0 &&
        SDL_Libretro_LoadState(menu->lr, savePath)) {
        SDL_Libretro_SetMenuOpen(menu, false);
    }
    else {
        nk_console_show_message(nk_console_get_top(widget), "Load State failed");
    }
}

/**
 * @internal
 */
static void SDL_Libretro_MenuResetClicked(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    if (SDL_Libretro_Reset(menu->lr)) {
        SDL_Libretro_SetMenuOpen(menu, false);
    }
}

/**
 * @internal
 */
static void SDL_Libretro_MenuQuitClicked(nk_console* widget, void* user_data) {
    (void)widget;
    (void)user_data;
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_EVENT_QUIT;
    event.quit.timestamp = SDL_GetTicksNS();
    SDL_PushEvent(&event);
}

/**
 * @internal
 */
static void SDL_Libretro_MenuVolumeChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_Libretro_SetVolume(menu->lr, (float)menu->volumePercent / 100.0f);
}

/**
 * @internal
 */
static void SDL_Libretro_MenuFitModeChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_Libretro_SetFitMode(menu->lr, (SDL_LibretroFitMode)menu->fitModeIndex);
}

/**
 * @internal
 */
static void SDL_Libretro_MenuFilterChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_Libretro_SetTextureScaleMode(menu->lr, menu->filterIndex == 1 ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
}

/**
 * @internal
 */
static void SDL_Libretro_MenuFullscreenChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_SetWindowFullscreen(menu->lr->window, menu->fullscreenChecked == nk_true);
    menu->settingsDirty = true;
}

/**
 * @internal
 */
static void SDL_Libretro_MenuVSyncChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_SetRenderVSync(menu->lr->renderer, menu->vsyncChecked == nk_true ? 1 : 0);
    menu->settingsDirty = true;
}

/**
 * Mute drops the volume to zero and restores the previous level on unmute.
 *
 * @internal
 */
static void SDL_Libretro_MenuMuteChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_Libretro* lr = menu->lr;
    if (menu->muteChecked) {
        menu->preMuteVolume = SDL_Libretro_GetVolume(lr);
        SDL_Libretro_SetVolume(lr, 0.0f);
    }
    else {
        SDL_Libretro_SetVolume(lr, menu->preMuteVolume > 0.0f ? menu->preMuteVolume : 1.0f);
        menu->volumePercent = (int)(SDL_Libretro_GetVolume(lr) * 100.0f + 0.5f);
    }
    menu->settingsDirty = true;
}

/**
 * Read the menu's own persisted state from the config, applying it to the
 * window, renderer and volume before the widgets snapshot them.
 *
 * The counterpart of SDL_Libretro_MenuSaveState(); menu-owned config keys
 * live in these two functions only.
 *
 * @internal
 */
static void SDL_Libretro_MenuLoadState(SDL_LibretroMenu* menu) {
    SDL_Libretro* lr = menu->lr;
    SDL_LibretroMenuStyle style = SDL_LIBRETRO_MENU_DEFAULT_STYLE;
    if (lr->ini != NULL) {
        Sint64 savedStyle = INI_GetInt(lr->ini, NULL, "menutheme", (Sint64)style);
        if (savedStyle >= 0 && savedStyle < (Sint64)SDL_LIBRETRO_MENU_STYLE_COUNT) {
            style = (SDL_LibretroMenuStyle)savedStyle;
        }
        if (INI_HasValue(lr->ini, NULL, "menufullscreen")) {
            SDL_SetWindowFullscreen(lr->window, INI_GetBoolean(lr->ini, NULL, "menufullscreen", false));
        }
        if (INI_HasValue(lr->ini, NULL, "menuvsync")) {
            SDL_SetRenderVSync(lr->renderer, INI_GetBoolean(lr->ini, NULL, "menuvsync", false) ? 1 : 0);
        }
        if (INI_GetBoolean(lr->ini, NULL, "menumuted", false)) {
            menu->muteChecked = nk_true;
            menu->preMuteVolume = INI_GetFloat(lr->ini, NULL, "menumutevolume", 1.0f);
            SDL_Libretro_SetVolume(lr, 0.0f);
        }
    }
    SDL_Libretro_SetMenuStyle(menu, style);
}

/**
 * Write every menu-owned config value; the file itself is saved when the
 * context closes.
 *
 * One choke point, flushed on settingsDirty, so the change handlers stay
 * free of persistence code.
 *
 * @internal
 */
static void SDL_Libretro_MenuSaveState(SDL_LibretroMenu* menu) {
    SDL_Libretro* lr = menu->lr;
    menu->settingsDirty = false;
    if (lr->ini == NULL) {
        return;
    }
    INI_SetInt(lr->ini, NULL, "menutheme", (Sint64)menu->style);
    INI_SetBoolean(lr->ini, NULL, "menufullscreen", menu->fullscreenChecked == nk_true);
    INI_SetBoolean(lr->ini, NULL, "menuvsync", menu->vsyncChecked == nk_true);
    INI_SetBoolean(lr->ini, NULL, "menumuted", menu->muteChecked == nk_true);
    INI_SetFloat(lr->ini, NULL, "menumutevolume", menu->preMuteVolume);
}

/**
 * Whether the value reads as the "on" half of a boolean core option.
 *
 * @internal
 */
static bool SDL_Libretro_MenuValueTruthy(const char* value) {
    return value != NULL && (SDL_strcasecmp(value, "enabled") == 0 ||
                             SDL_strcasecmp(value, "true") == 0 ||
                             SDL_strcasecmp(value, "on") == 0 ||
                             SDL_strcasecmp(value, "yes") == 0 ||
                             SDL_strcmp(value, "1") == 0);
}

/**
 * @internal
 * @see SDL_Libretro_MenuValueTruthy()
 */
static bool SDL_Libretro_MenuValueFalsy(const char* value) {
    return value != NULL && (SDL_strcasecmp(value, "disabled") == 0 ||
                             SDL_strcasecmp(value, "false") == 0 ||
                             SDL_strcasecmp(value, "off") == 0 ||
                             SDL_strcasecmp(value, "no") == 0 ||
                             SDL_strcmp(value, "0") == 0);
}

/**
 * Whether the option is exactly an on/off pair, in either order.
 *
 * @internal
 */
static bool SDL_Libretro_MenuOptionIsBoolean(const SDL_LibretroOption* option) {
    if (option->valuesCount != 2) {
        return false;
    }
    const char* first = option->values[0].value;
    const char* second = option->values[1].value;
    return (SDL_Libretro_MenuValueTruthy(first) && SDL_Libretro_MenuValueFalsy(second)) ||
           (SDL_Libretro_MenuValueFalsy(first) && SDL_Libretro_MenuValueTruthy(second));
}

/**
 * Marks the Core Options submenu for a rebuild when the change flipped any
 * option visibility, consuming the dirty flag our own write raised.
 *
 * @internal
 */
static void SDL_Libretro_MenuOptionWritten(SDL_LibretroMenu* menu) {
    SDL_Libretro_AreOptionsDirty(menu->lr);
    if (SDL_Libretro_UpdateOptionVisibility(menu->lr)) {
        SDL_Libretro_AreOptionsDirty(menu->lr);
        menu->optionsStale = true;
    }
}

/**
 * @internal
 */
static void SDL_Libretro_MenuOptionChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenuOptionState* state = (SDL_LibretroMenuOptionState*)user_data;
    SDL_LibretroMenu* menu = state->menu;
    const SDL_LibretroOption* option = SDL_Libretro_GetOptionByIndex(menu->lr, state->index);
    if (option == NULL || state->selected < 0 || (unsigned)state->selected >= option->valuesCount) {
        return;
    }
    SDL_Libretro_SetOptionValue(menu->lr, option->key, option->values[state->selected].value);
    SDL_Libretro_MenuOptionWritten(menu);
}

/**
 * @internal
 */
static void SDL_Libretro_MenuOptionCheckboxChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenuOptionState* state = (SDL_LibretroMenuOptionState*)user_data;
    SDL_LibretroMenu* menu = state->menu;
    const SDL_LibretroOption* option = SDL_Libretro_GetOptionByIndex(menu->lr, state->index);
    if (option == NULL || option->valuesCount != 2) {
        return;
    }
    // Write back the token the core declared rather than assuming a fixed pair.
    bool firstTruthy = SDL_Libretro_MenuValueTruthy(option->values[0].value);
    const char* truthy = firstTruthy ? option->values[0].value : option->values[1].value;
    const char* falsy = firstTruthy ? option->values[1].value : option->values[0].value;
    SDL_Libretro_SetOptionValue(menu->lr, option->key, state->checked ? truthy : falsy);
    SDL_Libretro_MenuOptionWritten(menu);
}

/**
 * @internal
 */
static void SDL_Libretro_MenuResetOptionsClicked(nk_console* widget, void* user_data) {
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_Libretro_ResetAllOptions(menu->lr);
    SDL_Libretro_MenuOptionWritten(menu);
    menu->optionsStale = true;
    nk_console_show_message(nk_console_get_top(widget), "Core options reset to defaults");
}

/**
 * @internal
 */
static void SDL_Libretro_MenuFreeOptionStates(SDL_LibretroMenu* menu) {
    if (menu->optionStates != NULL) {
        for (unsigned i = 0; i < menu->optionStateCount; i++) {
            SDL_free(menu->optionStates[i].displayList);
        }
        SDL_free(menu->optionStates);
        menu->optionStates = NULL;
    }
    menu->optionStateCount = 0;
}

/**
 * Standard submenu header: a back button labeled like the submenu itself.
 *
 * @internal
 */
static void SDL_Libretro_MenuAddBackButton(nk_console* parent, const char* label) {
    nk_console_button_set_symbol(
        nk_console_button_onclick(parent, label, &nk_console_button_back),
        NK_SYMBOL_TRIANGLE_UP);
}

/**
 * Join count items into a freshly allocated "a|b|c" string for a combobox.
 *
 * @param item Returns the text for an index; must never return NULL.
 *
 * @internal
 */
static char* SDL_Libretro_MenuJoinPipeList(unsigned count, const char* (*item)(void* userdata, unsigned index), void* userdata) {
    size_t length = 0;
    for (unsigned i = 0; i < count; i++) {
        length += SDL_strlen(item(userdata, i)) + 1;
    }
    char* list = (char*)SDL_malloc(length + 1);
    if (list == NULL) {
        return NULL;
    }
    list[0] = '\0';
    size_t offset = 0;
    for (unsigned i = 0; i < count; i++) {
        if (i > 0) {
            list[offset++] = '|';
        }
        offset += SDL_strlcpy(list + offset, item(userdata, i), length + 1 - offset);
    }
    return list;
}

/**
 * Whether the option should appear in the Core Options submenu.
 *
 * @internal
 */
static bool SDL_Libretro_MenuOptionShown(const SDL_LibretroOption* option) {
    return option != NULL && option->visible && option->valuesCount > 0;
}

/**
 * The display text for one core option value: its label, or the raw value.
 *
 * @internal
 * @see SDL_Libretro_MenuJoinPipeList()
 */
static const char* SDL_Libretro_MenuOptionValueText(void* userdata, unsigned index) {
    const SDL_LibretroOption* option = (const SDL_LibretroOption*)userdata;
    const char* label = option->values[index].label;
    return (label != NULL && label[0] != '\0') ? label : option->values[index].value;
}

/**
 * Build the checkbox/combobox widget for the given option under parent.
 *
 * @internal
 */
static void SDL_Libretro_MenuBuildOptionWidget(SDL_LibretroMenu* menu, nk_console* parent, unsigned index) {
    const SDL_LibretroOption* option = SDL_Libretro_GetOptionByIndex(menu->lr, index);
    SDL_LibretroMenuOptionState* state = &menu->optionStates[index];
    state->menu = menu;
    state->index = index;

    const char* label = (option->desc != NULL && option->desc[0] != '\0') ? option->desc : option->key;
    nk_console* widget;

    if (SDL_Libretro_MenuOptionIsBoolean(option)) {
        state->checked = (nk_bool)SDL_Libretro_MenuValueTruthy(option->value);
        widget = nk_console_checkbox(parent, label, &state->checked);
        nk_console_add_event_handler(widget, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuOptionCheckboxChanged, state, NULL);
    }
    else {
        // The combobox reads the pipe-separated list lazily, so keep it alive in the state.
        state->displayList = SDL_Libretro_MenuJoinPipeList(option->valuesCount, &SDL_Libretro_MenuOptionValueText, (void*)option);
        if (state->displayList == NULL) {
            return;
        }
        state->selected = 0;
        for (unsigned v = 0; v < option->valuesCount; v++) {
            if (option->value != NULL && SDL_strcmp(option->values[v].value, option->value) == 0) {
                state->selected = (int)v;
            }
        }

        widget = nk_console_combobox(parent, label, state->displayList, '|', &state->selected);
        nk_console_add_event_handler(widget, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuOptionChanged, state, NULL);
    }

    if (option->info != NULL && option->info[0] != '\0') {
        widget->tooltip = option->info;
    }
}

/**
 * Category index of the option, or -1 when uncategorized (or the category was
 * never registered, in which case the option is treated as flat).
 *
 * @internal
 */
static int SDL_Libretro_MenuOptionCategory(SDL_Libretro* lr, const SDL_LibretroOption* option) {
    if (option->category == NULL || option->category[0] == '\0') {
        return -1;
    }
    unsigned count = SDL_Libretro_GetCategoryCount(lr);
    for (unsigned c = 0; c < count; c++) {
        const SDL_LibretroCategory* category = SDL_Libretro_GetCategoryByIndex(lr, c);
        if (category != NULL && category->key != NULL && SDL_strcmp(option->category, category->key) == 0) {
            return (int)c;
        }
    }
    return -1;
}

/**
 * Populate the "Core Options" submenu from the loaded core's options.
 *
 * @internal
 */
static void SDL_Libretro_MenuBuildOptions(SDL_LibretroMenu* menu) {
    if (menu->optionsButton == NULL) {
        return;
    }
    SDL_Libretro* lr = menu->lr;

    // Drop the previous widgets before the option states they point into.
    nk_console_free_children(menu->optionsButton);
    SDL_Libretro_MenuFreeOptionStates(menu);

    unsigned count = SDL_Libretro_GetOptionCount(lr);
    unsigned shown = 0;
    for (unsigned i = 0; i < count; i++) {
        if (SDL_Libretro_MenuOptionShown(SDL_Libretro_GetOptionByIndex(lr, i))) {
            shown++;
        }
    }
    if (shown == 0) {
        menu->optionsButton->visible = nk_false;
        return;
    }
    menu->optionsButton->visible = nk_true;

    menu->optionStates = (SDL_LibretroMenuOptionState*)SDL_calloc(count, sizeof(SDL_LibretroMenuOptionState));
    if (menu->optionStates == NULL) {
        menu->optionsButton->visible = nk_false;
        return;
    }
    menu->optionStateCount = count;

    SDL_Libretro_MenuAddBackButton(menu->optionsButton, "Core Options");

    // Uncategorized options first, directly under Core Options.
    for (unsigned i = 0; i < count; i++) {
        const SDL_LibretroOption* option = SDL_Libretro_GetOptionByIndex(lr, i);
        if (!SDL_Libretro_MenuOptionShown(option) || SDL_Libretro_MenuOptionCategory(lr, option) >= 0) {
            continue;
        }
        SDL_Libretro_MenuBuildOptionWidget(menu, menu->optionsButton, i);
    }

    // Then a submenu per category holding its options.
    unsigned categoryCount = SDL_Libretro_GetCategoryCount(lr);
    for (unsigned c = 0; c < categoryCount; c++) {
        const SDL_LibretroCategory* category = SDL_Libretro_GetCategoryByIndex(lr, c);
        bool hasVisible = false;
        for (unsigned i = 0; i < count; i++) {
            const SDL_LibretroOption* option = SDL_Libretro_GetOptionByIndex(lr, i);
            if (SDL_Libretro_MenuOptionShown(option) && SDL_Libretro_MenuOptionCategory(lr, option) == (int)c) {
                hasVisible = true;
                break;
            }
        }
        if (!hasVisible) {
            continue;
        }

        const char* categoryLabel = (category->desc != NULL && category->desc[0] != '\0') ? category->desc : category->key;
        nk_console* categoryButton = nk_console_button(menu->optionsButton, categoryLabel);
        nk_console_button_set_symbol(categoryButton, NK_SYMBOL_TRIANGLE_RIGHT);
        SDL_Libretro_MenuAddBackButton(categoryButton, categoryLabel);

        for (unsigned i = 0; i < count; i++) {
            const SDL_LibretroOption* option = SDL_Libretro_GetOptionByIndex(lr, i);
            if (SDL_Libretro_MenuOptionShown(option) && SDL_Libretro_MenuOptionCategory(lr, option) == (int)c) {
                SDL_Libretro_MenuBuildOptionWidget(menu, categoryButton, i);
            }
        }
    }

    nk_console_button_set_symbol(
        nk_console_button_onclick_handler(menu->optionsButton, "Reset to Defaults", &SDL_Libretro_MenuResetOptionsClicked, menu, NULL),
        NK_SYMBOL_TRIANGLE_LEFT);
}

#ifndef __EMSCRIPTEN__
/**
 * Append each extension from a pipe-separated list (e.g. "gb|gbc") to a
 * semicolon-separated file filter (e.g. ".gb;.GB;.gbc;.GBC"), deduplicated.
 *
 * The filter matching is case-sensitive, so both lower and upper case
 * variants are added.
 *
 * @internal
 */
static void SDL_Libretro_MenuAppendExtensions(char* filter, size_t filterSize, const char* extensions) {
    if (extensions == NULL) {
        return;
    }
    for (const char* token = extensions; *token != '\0';) {
        const char* end = token;
        while (*end != '\0' && *end != '|') {
            end++;
        }
        size_t tokenLen = (size_t)(end - token);
        char lower[34];
        char upper[34];
        if (tokenLen > 0 && tokenLen + 2 <= sizeof(lower)) {
            lower[0] = '.';
            upper[0] = '.';
            for (size_t i = 0; i < tokenLen; i++) {
                lower[i + 1] = (char)SDL_tolower((int)(unsigned char)token[i]);
                upper[i + 1] = (char)SDL_toupper((int)(unsigned char)token[i]);
            }
            lower[tokenLen + 1] = '\0';
            upper[tokenLen + 1] = '\0';

            // Skip if the extension is already in the filter.
            bool found = false;
            for (const char* existing = filter; *existing != '\0' && !found;) {
                const char* existingEnd = existing;
                while (*existingEnd != '\0' && *existingEnd != ';') {
                    existingEnd++;
                }
                size_t existingLen = (size_t)(existingEnd - existing);
                if (existingLen == tokenLen + 1 && SDL_strncasecmp(existing, lower, existingLen) == 0) {
                    found = true;
                }
                existing = (*existingEnd == ';') ? existingEnd + 1 : existingEnd;
            }

            if (!found) {
                size_t length = SDL_strlen(filter);
                // Reserve room for ";lower;UPPER" and the terminator.
                if (length + (tokenLen + 2) * 2 + 2 < filterSize) {
                    if (length > 0) {
                        filter[length++] = ';';
                    }
                    length += SDL_strlcpy(filter + length, lower, filterSize - length);
                    filter[length++] = ';';
                    SDL_strlcpy(filter + length, upper, filterSize - length);
                }
            }
        }
        token = (*end == '|') ? end + 1 : end;
    }
}
#endif /* !__EMSCRIPTEN__ */

/**
 * Restrict the Load Game file browser to content the scanned cores (and the
 * loaded core) can open.
 *
 * @internal
 */
static void SDL_Libretro_MenuUpdateLoadGameFilter(SDL_LibretroMenu* menu) {
#ifndef __EMSCRIPTEN__
    if (menu->loadGameButton == NULL) {
        return;
    }
    SDL_Libretro* lr = menu->lr;

    char filter[1024] = "";
    for (unsigned i = 0; i < lr->coreLibraryCount; i++) {
        SDL_Libretro_MenuAppendExtensions(filter, sizeof(filter), lr->coreLibrary[i].supported_extensions);
    }
    if (SDL_Libretro_IsCoreReady(lr)) {
        SDL_Libretro_MenuAppendExtensions(filter, sizeof(filter), lr->core.validExtensions);
    }
#if defined(SDL_LIBRETRO_ENABLE_PHYSFS) && !defined(SDL_LIBRETRO_DISABLE_PHYSFS)
    SDL_Libretro_MenuAppendExtensions(filter, sizeof(filter), "zip");
#endif

    // With nothing to filter against, show every file.
    nk_console_file_set_filter(menu->loadGameButton, filter[0] != '\0' ? filter : NULL);
#else
    (void)menu;
#endif
}

/**
 * @internal
 */
static void SDL_Libretro_MenuFreePortStates(SDL_LibretroMenu* menu) {
    for (unsigned i = 0; i < SDL_LIBRETRO_MAX_GAMEPADS; i++) {
        SDL_free(menu->portStates[i].deviceList);
        menu->portStates[i].deviceList = NULL;
    }
}

/**
 * @internal
 */
static void SDL_Libretro_MenuPortDeviceChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenuPortState* state = (SDL_LibretroMenuPortState*)user_data;
    SDL_Libretro* lr = state->menu->lr;
    if (state->port >= lr->core.controllerInfoCount) {
        return;
    }
    const struct retro_controller_info* info = &lr->core.controllerInfo[state->port];
    if (state->selected < 0 || (unsigned)state->selected >= info->num_types) {
        return;
    }
    SDL_Libretro_SetPortDevice(lr, state->port, info->types[state->selected].id);
}

/**
 * @internal
 * @see SDL_Libretro_MenuJoinPipeList()
 */
static const char* SDL_Libretro_MenuPortDeviceText(void* userdata, unsigned index) {
    const struct retro_controller_info* info = (const struct retro_controller_info*)userdata;
    return info->types[index].desc != NULL ? info->types[index].desc : "Unknown";
}

/**
 * Populate the "Controllers" submenu with a device combobox per port that
 * offers more than one controller type.
 *
 * @internal
 */
static void SDL_Libretro_MenuBuildControllers(SDL_LibretroMenu* menu) {
    if (menu->controllersButton == NULL) {
        return;
    }
    SDL_Libretro* lr = menu->lr;

    // Drop the previous widgets before the port states they point into.
    nk_console_free_children(menu->controllersButton);
    SDL_Libretro_MenuFreePortStates(menu);

    SDL_Libretro_MenuAddBackButton(menu->controllersButton, "Controllers");

    unsigned count = lr->core.controllerInfoCount;
    if (count > SDL_LIBRETRO_MAX_GAMEPADS) {
        count = SDL_LIBRETRO_MAX_GAMEPADS;
    }

    bool anyWidget = false;
    for (unsigned port = 0; port < count; port++) {
        const struct retro_controller_info* info = &lr->core.controllerInfo[port];
        if (info->types == NULL || info->num_types <= 1) {
            continue;
        }

        // The combobox reads the pipe-separated list lazily, so keep it alive in the state.
        SDL_LibretroMenuPortState* state = &menu->portStates[port];
        state->deviceList = SDL_Libretro_MenuJoinPipeList(info->num_types, &SDL_Libretro_MenuPortDeviceText, (void*)info);
        if (state->deviceList == NULL) {
            continue;
        }
        state->menu = menu;
        state->port = port;
        state->selected = 0;

        unsigned currentDevice = SDL_Libretro_GetPortDevice(lr, port);
        for (unsigned i = 0; i < info->num_types; i++) {
            if (info->types[i].id == currentDevice) {
                state->selected = (int)i;
            }
        }

        SDL_snprintf(state->label, sizeof(state->label), "Port %u", port + 1);
        nk_console* combobox = nk_console_combobox(menu->controllersButton, state->label, state->deviceList, '|', &state->selected);
        nk_console_add_event_handler(combobox, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuPortDeviceChanged, state, NULL);
        anyWidget = true;
    }

    menu->controllersButton->visible = (nk_bool)anyWidget;
}

/**
 * A menu color theme, expressed as the distinct colors the themes vary on.
 *
 * SDL_Libretro_MenuApplyPalette() fans these out over the full Nuklear color
 * table, so adding a theme is ~20 colors instead of ~30 table assignments.
 *
 * @internal
 */
typedef struct SDL_LibretroMenuPalette {
    struct nk_color text;
    struct nk_color window; /** Window background. */
    struct nk_color frame; /** Header and border. */
    struct nk_color widget; /** Buttons, selects, properties, edits, combos, charts, tab headers. */
    struct nk_color widgetHover;
    struct nk_color widgetActive;
    struct nk_color selectActive;
    struct nk_color toggle; /** Checkbox/toggle track. */
    struct nk_color toggleHover;
    struct nk_color toggleCursor;
    struct nk_color slider; /** Slider and knob track. */
    struct nk_color sliderCursor;
    struct nk_color sliderCursorHover;
    struct nk_color sliderCursorActive;
    struct nk_color editCursor;
    struct nk_color chartColor;
    struct nk_color chartHighlight;
    struct nk_color scrollbar; /** Scrollbar track. */
    struct nk_color scrollbarCursor;
    struct nk_color scrollbarCursorHover;
    struct nk_color scrollbarCursorActive;
    struct nk_color knobCursor;
    struct nk_color knobCursorHover;
    struct nk_color knobCursorActive;
} SDL_LibretroMenuPalette;

// Catppuccin Mocha: text/base/mantle/surface/overlay neutrals with lavender,
// green and pink accents. https://catppuccin.com/palette/
static const SDL_LibretroMenuPalette SDL_Libretro_menuPaletteMocha = {
    .text = {205, 214, 244, 255},
    .window = {30, 30, 46, 235},
    .frame = {24, 24, 37, 255},
    .widget = {49, 50, 68, 255},
    .widgetHover = {127, 132, 156, 255},
    .widgetActive = {108, 112, 134, 255},
    .selectActive = {108, 112, 134, 255},
    .toggle = {88, 91, 112, 255},
    .toggleHover = {147, 153, 178, 255},
    .toggleCursor = {180, 190, 254, 255},
    .slider = {69, 71, 90, 255},
    .sliderCursor = {166, 227, 161, 255},
    .sliderCursorHover = {166, 227, 161, 255},
    .sliderCursorActive = {166, 227, 161, 255},
    .editCursor = {180, 190, 254, 255},
    .chartColor = {180, 190, 254, 255},
    .chartHighlight = {245, 194, 231, 255},
    .scrollbar = {49, 50, 68, 255},
    .scrollbarCursor = {108, 112, 134, 255},
    .scrollbarCursorHover = {180, 190, 254, 255},
    .scrollbarCursorActive = {245, 194, 231, 255},
    .knobCursor = {245, 194, 231, 255},
    .knobCursorHover = {245, 194, 231, 255},
    .knobCursorActive = {245, 194, 231, 255},
};

// Catppuccin Latte: the light variant, with teal, mauve and pink accents.
static const SDL_LibretroMenuPalette SDL_Libretro_menuPaletteLatte = {
    .text = {76, 79, 105, 255},
    .window = {239, 241, 245, 235},
    .frame = {230, 233, 239, 255},
    .widget = {204, 208, 218, 255},
    .widgetHover = {124, 127, 147, 55},
    .widgetActive = {156, 160, 176, 255},
    .selectActive = {156, 160, 176, 255},
    .toggle = {172, 176, 190, 255},
    .toggleHover = {124, 127, 147, 55},
    .toggleCursor = {223, 142, 29, 255},
    .slider = {188, 192, 204, 255},
    .sliderCursor = {23, 146, 153, 255},
    .sliderCursorHover = {23, 146, 153, 255},
    .sliderCursorActive = {23, 146, 153, 255},
    .editCursor = {136, 57, 239, 255},
    .chartColor = {23, 146, 153, 255},
    .chartHighlight = {136, 57, 239, 255},
    .scrollbar = {204, 208, 218, 255},
    .scrollbarCursor = {156, 160, 176, 255},
    .scrollbarCursorHover = {136, 57, 239, 255},
    .scrollbarCursorActive = {136, 57, 239, 255},
    .knobCursor = {234, 118, 203, 255},
    .knobCursorHover = {234, 118, 203, 255},
    .knobCursorActive = {234, 118, 203, 255},
};

// Catppuccin Frappe, with green, lavender and pink accents.
static const SDL_LibretroMenuPalette SDL_Libretro_menuPaletteFrappe = {
    .text = {198, 208, 245, 255},
    .window = {48, 52, 70, 235},
    .frame = {41, 44, 60, 255},
    .widget = {65, 69, 89, 255},
    .widgetHover = {131, 139, 167, 255},
    .widgetActive = {115, 121, 148, 255},
    .selectActive = {115, 121, 148, 255},
    .toggle = {98, 104, 128, 255},
    .toggleHover = {148, 156, 187, 255},
    .toggleCursor = {244, 184, 228, 255},
    .slider = {81, 87, 109, 255},
    .sliderCursor = {166, 209, 137, 255},
    .sliderCursorHover = {166, 209, 137, 255},
    .sliderCursorActive = {166, 209, 137, 255},
    .editCursor = {244, 184, 228, 255},
    .chartColor = {186, 187, 241, 255},
    .chartHighlight = {244, 184, 228, 255},
    .scrollbar = {65, 69, 89, 255},
    .scrollbarCursor = {115, 121, 148, 255},
    .scrollbarCursorHover = {186, 187, 241, 255},
    .scrollbarCursorActive = {186, 187, 241, 255},
    .knobCursor = {244, 184, 228, 255},
    .knobCursorHover = {244, 184, 228, 255},
    .knobCursorActive = {244, 184, 228, 255},
};

// Catppuccin Macchiato, with yellow, green, lavender and pink accents.
static const SDL_LibretroMenuPalette SDL_Libretro_menuPaletteMacchiato = {
    .text = {202, 211, 245, 255},
    .window = {36, 39, 58, 235},
    .frame = {30, 32, 48, 255},
    .widget = {54, 58, 79, 255},
    .widgetHover = {128, 135, 162, 255},
    .widgetActive = {110, 115, 141, 255},
    .selectActive = {110, 115, 141, 255},
    .toggle = {91, 96, 120, 255},
    .toggleHover = {147, 154, 183, 255},
    .toggleCursor = {238, 212, 159, 255},
    .slider = {73, 77, 100, 255},
    .sliderCursor = {166, 218, 149, 255},
    .sliderCursorHover = {166, 218, 149, 255},
    .sliderCursorActive = {166, 218, 149, 255},
    .editCursor = {245, 189, 230, 255},
    .chartColor = {183, 189, 248, 255},
    .chartHighlight = {238, 212, 159, 255},
    .scrollbar = {54, 58, 79, 255},
    .scrollbarCursor = {110, 115, 141, 255},
    .scrollbarCursorHover = {183, 189, 248, 255},
    .scrollbarCursorActive = {183, 189, 248, 255},
    .knobCursor = {245, 189, 230, 255},
    .knobCursorHover = {245, 189, 230, 255},
    .knobCursorActive = {245, 189, 230, 255},
};

// Dracula: background/current-line/comment neutrals with pink and purple
// accents. https://draculatheme.com/contribute
static const SDL_LibretroMenuPalette SDL_Libretro_menuPaletteDracula = {
    .text = {248, 248, 242, 255},
    .window = {40, 42, 54, 235},
    .frame = {68, 71, 90, 255},
    .widget = {68, 71, 90, 255},
    .widgetHover = {98, 114, 164, 255},
    .widgetActive = {189, 147, 249, 255},
    .selectActive = {98, 114, 164, 255},
    .toggle = {68, 71, 90, 255},
    .toggleHover = {98, 114, 164, 255},
    .toggleCursor = {255, 121, 198, 255},
    .slider = {40, 42, 54, 235},
    .sliderCursor = {68, 71, 90, 255},
    .sliderCursorHover = {98, 114, 164, 255},
    .sliderCursorActive = {98, 114, 164, 255},
    .editCursor = {248, 248, 242, 255},
    .chartColor = {98, 114, 164, 255},
    .chartHighlight = {189, 147, 249, 255},
    .scrollbar = {40, 42, 54, 235},
    .scrollbarCursor = {68, 71, 90, 255},
    .scrollbarCursorHover = {98, 114, 164, 255},
    .scrollbarCursorActive = {189, 147, 249, 255},
    .knobCursor = {68, 71, 90, 255},
    .knobCursorHover = {98, 114, 164, 255},
    .knobCursorActive = {98, 114, 164, 255},
};

/**
 * The palette backing a built-in style, or NULL for the Nuklear default.
 *
 * @internal
 */
static const SDL_LibretroMenuPalette* SDL_Libretro_MenuGetPalette(SDL_LibretroMenuStyle style) {
    switch (style) {
        case SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_MOCHA:
            return &SDL_Libretro_menuPaletteMocha;
        case SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_LATTE:
            return &SDL_Libretro_menuPaletteLatte;
        case SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_FRAPPE:
            return &SDL_Libretro_menuPaletteFrappe;
        case SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_MACCHIATO:
            return &SDL_Libretro_menuPaletteMacchiato;
        case SDL_LIBRETRO_MENU_STYLE_DRACULA:
            return &SDL_Libretro_menuPaletteDracula;
        default:
            return NULL;
    }
}

/**
 * Fan the palette out over the Nuklear color table.
 *
 * @internal
 */
static void SDL_Libretro_MenuApplyPalette(struct nk_context* ctx, const SDL_LibretroMenuPalette* palette) {
    struct nk_color table[NK_COLOR_COUNT];
    table[NK_COLOR_TEXT] = palette->text;
    table[NK_COLOR_WINDOW] = palette->window;
    table[NK_COLOR_HEADER] = palette->frame;
    table[NK_COLOR_BORDER] = palette->frame;
    table[NK_COLOR_BUTTON] = palette->widget;
    table[NK_COLOR_BUTTON_HOVER] = palette->widgetHover;
    table[NK_COLOR_BUTTON_ACTIVE] = palette->widgetActive;
    table[NK_COLOR_TOGGLE] = palette->toggle;
    table[NK_COLOR_TOGGLE_HOVER] = palette->toggleHover;
    table[NK_COLOR_TOGGLE_CURSOR] = palette->toggleCursor;
    table[NK_COLOR_SELECT] = palette->widget;
    table[NK_COLOR_SELECT_ACTIVE] = palette->selectActive;
    table[NK_COLOR_SLIDER] = palette->slider;
    table[NK_COLOR_SLIDER_CURSOR] = palette->sliderCursor;
    table[NK_COLOR_SLIDER_CURSOR_HOVER] = palette->sliderCursorHover;
    table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = palette->sliderCursorActive;
    table[NK_COLOR_PROPERTY] = palette->widget;
    table[NK_COLOR_EDIT] = palette->widget;
    table[NK_COLOR_EDIT_CURSOR] = palette->editCursor;
    table[NK_COLOR_COMBO] = palette->widget;
    table[NK_COLOR_CHART] = palette->widget;
    table[NK_COLOR_CHART_COLOR] = palette->chartColor;
    table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = palette->chartHighlight;
    table[NK_COLOR_SCROLLBAR] = palette->scrollbar;
    table[NK_COLOR_SCROLLBAR_CURSOR] = palette->scrollbarCursor;
    table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = palette->scrollbarCursorHover;
    table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = palette->scrollbarCursorActive;
    table[NK_COLOR_TAB_HEADER] = palette->widget;
    table[NK_COLOR_KNOB] = palette->slider;
    table[NK_COLOR_KNOB_CURSOR] = palette->knobCursor;
    table[NK_COLOR_KNOB_CURSOR_HOVER] = palette->knobCursorHover;
    table[NK_COLOR_KNOB_CURSOR_ACTIVE] = palette->knobCursorActive;
    nk_style_from_table(ctx, table);
}

bool SDL_Libretro_SetMenuStyle(SDL_LibretroMenu* menu, SDL_LibretroMenuStyle style) {
    if (menu == NULL || menu->ctx == NULL || (int)style < 0 || style >= SDL_LIBRETRO_MENU_STYLE_COUNT) {
        return false;
    }

    // Reset to the Nuklear default first; the Dark style is exactly that.
    nk_style_default(menu->ctx);
    const SDL_LibretroMenuPalette* palette = SDL_Libretro_MenuGetPalette(style);
    if (palette != NULL) {
        SDL_Libretro_MenuApplyPalette(menu->ctx, palette);
    }

    menu->style = style;
    menu->styleIndex = (int)style;
    menu->settingsDirty = true;
    return true;
}

SDL_LibretroMenuStyle SDL_Libretro_GetMenuStyle(const SDL_LibretroMenu* menu) {
    return menu != NULL ? menu->style : SDL_LIBRETRO_MENU_DEFAULT_STYLE;
}

/**
 * @internal
 */
static void SDL_Libretro_MenuStyleChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_Libretro_SetMenuStyle(menu, (SDL_LibretroMenuStyle)menu->styleIndex);
}

/**
 * Refresh the Settings widget state from the current context values.
 *
 * Runs at creation and every time the menu opens, so changes made outside
 * the menu (a hotkey volume change, an app fullscreen toggle) show up.
 *
 * @internal
 */
static void SDL_Libretro_MenuSyncSettings(SDL_LibretroMenu* menu) {
    SDL_Libretro* lr = menu->lr;
    menu->volumePercent = (int)(SDL_Libretro_GetVolume(lr) * 100.0f + 0.5f);
    menu->fitModeIndex = (int)SDL_Libretro_GetFitMode(lr);
    menu->filterIndex = SDL_Libretro_GetTextureScaleMode(lr) == SDL_SCALEMODE_LINEAR ? 1 : 0;
    menu->fullscreenChecked = (nk_bool)((SDL_GetWindowFlags(lr->window) & SDL_WINDOW_FULLSCREEN) != 0);
    int vsync = 0;
    SDL_GetRenderVSync(lr->renderer, &vsync);
    menu->vsyncChecked = (nk_bool)(vsync != 0);
}

/**
 * Build the Settings submenu; the widgets write into the fields that
 * SDL_Libretro_MenuSyncSettings() keeps current.
 *
 * @internal
 */
static void SDL_Libretro_MenuBuildSettings(SDL_LibretroMenu* menu) {
    nk_console* settings = nk_console_button(menu->console, "Settings");
    nk_console_button_set_symbol(settings, NK_SYMBOL_HAMBURGER);
    SDL_Libretro_MenuAddBackButton(settings, "Settings");

    // Audio & Video
    nk_console* audioVideo = nk_console_button(settings, "Audio & Video");
    nk_console_button_set_symbol(audioVideo, NK_SYMBOL_TRIANGLE_RIGHT);
    SDL_Libretro_MenuAddBackButton(audioVideo, "Audio & Video");

    nk_console* volume = nk_console_property_int(audioVideo, "Volume", 0, &menu->volumePercent, 100, 5, 1.0f);
    nk_console_add_event_handler(volume, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuVolumeChanged, menu, NULL);

    nk_console* mute = nk_console_checkbox(audioVideo, "Mute", &menu->muteChecked);
    nk_console_add_event_handler(mute, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuMuteChanged, menu, NULL);

    nk_console* fullscreen = nk_console_checkbox(audioVideo, "Fullscreen", &menu->fullscreenChecked);
    nk_console_add_event_handler(fullscreen, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuFullscreenChanged, menu, NULL);

    nk_console* vsync = nk_console_checkbox(audioVideo, "VSync", &menu->vsyncChecked);
    nk_console_add_event_handler(vsync, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuVSyncChanged, menu, NULL);

    nk_console* filter = nk_console_combobox(audioVideo, "Filter", "Nearest|Linear", '|', &menu->filterIndex);
    nk_console_add_event_handler(filter, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuFilterChanged, menu, NULL);

    nk_console* fitMode = nk_console_combobox(audioVideo, "Fit Mode", "Aspect|Integer|Stretch", '|', &menu->fitModeIndex);
    nk_console_add_event_handler(fitMode, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuFitModeChanged, menu, NULL);

    nk_console* theme = nk_console_combobox(audioVideo, "Theme", SDL_LIBRETRO_MENU_STYLE_NAMES, '|', &menu->styleIndex);
    nk_console_add_event_handler(theme, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuStyleChanged, menu, NULL);
}

/**
 * Build the top-level menu tree.
 *
 * @internal
 */
static void SDL_Libretro_MenuBuildWidgets(SDL_LibretroMenu* menu) {
    SDL_Libretro* lr = menu->lr;
    (void)lr;

    // Backing out of the top level acts like Resume.
    nk_console_add_event_handler(menu->console, NK_CONSOLE_EVENT_BACK, &SDL_Libretro_MenuResumeClicked, menu, NULL);

    // Resume
    menu->resumeButton = nk_console_button_onclick_handler(menu->console, "Resume", &SDL_Libretro_MenuResumeClicked, menu, NULL);
    nk_console_button_set_symbol(menu->resumeButton, NK_SYMBOL_TRIANGLE_RIGHT);

    // Load Game
#ifdef __EMSCRIPTEN__
    // Browsing the virtual file system isn't useful on the web; a JS file
    // picker brings the user's own files in instead.
    menu->loadGameButton = nk_console_button_onclick_handler(menu->console, "Load Game", &SDL_Libretro_MenuWebLoadGameClicked, menu, NULL);
#else
    menu->loadGameButton = nk_console_file_action(menu->console, "Load Game", menu->loadGamePath, sizeof(menu->loadGamePath));
    nk_console_add_event_handler(menu->loadGameButton, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuGameFileChanged, menu, NULL);
    if (lr->fileBrowserStartDirectory[0] != '\0') {
        nk_console_file_set_directory(menu->loadGameButton, lr->fileBrowserStartDirectory);
    }
    SDL_Libretro_MenuUpdateLoadGameFilter(menu);
#endif

    // Save State / Load State
    menu->stateRow = nk_console_row_begin(menu->console);
    {
        nk_console* saveState = nk_console_button(menu->stateRow, "Save State");
        nk_console_add_event_handler(saveState, NK_CONSOLE_EVENT_CLICKED, &SDL_Libretro_MenuSaveStateClicked, menu, NULL);
        nk_console_button_set_symbol(saveState, NK_SYMBOL_RECT_SOLID);

        nk_console* loadState = nk_console_button(menu->stateRow, "Load State");
        nk_console_add_event_handler(loadState, NK_CONSOLE_EVENT_CLICKED, &SDL_Libretro_MenuLoadStateClicked, menu, NULL);
        nk_console_button_set_symbol(loadState, NK_SYMBOL_RECT_OUTLINE);
    }
    nk_console_row_end(menu->stateRow);

    // Reset
    menu->resetButton = nk_console_button_onclick_handler(menu->console, "Reset", &SDL_Libretro_MenuResetClicked, menu, NULL);
    nk_console_button_set_symbol(menu->resetButton, NK_SYMBOL_CIRCLE_SOLID);

    // Core Options, populated lazily once a core registers options.
    menu->optionsButton = nk_console_button(menu->console, "Core Options");
    nk_console_button_set_symbol(menu->optionsButton, NK_SYMBOL_TRIANGLE_RIGHT);
    menu->optionsButton->visible = nk_false;

    // Controllers, populated lazily once a core registers controller info.
    menu->controllersButton = nk_console_button(menu->console, "Controllers");
    nk_console_button_set_symbol(menu->controllersButton, NK_SYMBOL_TRIANGLE_RIGHT);
    menu->controllersButton->visible = nk_false;

    SDL_Libretro_MenuBuildSettings(menu);

    // Quit
    nk_console* quit = nk_console_button_onclick_handler(menu->console, "Quit", &SDL_Libretro_MenuQuitClicked, menu, NULL);
    nk_console_button_set_symbol(quit, NK_SYMBOL_X);
}

SDL_LibretroMenu* SDL_Libretro_CreateMenu(SDL_Libretro* lr) {
    if (lr == NULL) {
        SDL_InvalidParamError("lr");
        return NULL;
    }
    if (lr->renderer == NULL || lr->window == NULL) {
        SDL_SetError("SDL_Libretro_CreateMenu() requires SDL_Libretro_SetRenderer() to be called first");
        return NULL;
    }

    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)SDL_calloc(1, sizeof(SDL_LibretroMenu));
    if (menu == NULL) {
        return NULL;
    }
    menu->lr = lr;

    menu->ctx = nk_sdl_init(lr->window, lr->renderer, nk_sdl_allocator());
    if (menu->ctx == NULL) {
        SDL_free(menu);
        SDL_SetError("Failed to initialize Nuklear");
        return NULL;
    }

    // Bake the default font, scaled to the window's display scale.
    float scale = SDL_GetWindowDisplayScale(lr->window);
    if (scale <= 0.0f) {
        scale = 1.0f;
    }
    struct nk_font_config fontConfig = nk_font_config(0);
    struct nk_font_atlas* atlas = nk_sdl_font_stash_begin(menu->ctx);
    struct nk_font* font = nk_font_atlas_add_default(atlas, (float)SDL_LIBRETRO_MENU_FONT_HEIGHT * scale, &fontConfig);
    nk_sdl_font_stash_end(menu->ctx);
    if (font != NULL) {
        nk_style_set_font(menu->ctx, &font->handle);
    }

    menu->console = nk_console_init(menu->ctx);
    if (menu->console == NULL) {
        nk_sdl_shutdown(menu->ctx);
        SDL_free(menu);
        SDL_SetError("Failed to initialize nuklear_console");
        return NULL;
    }

    nk_gamepad_init(&menu->gamepads, menu->ctx, NULL);
    nk_console_set_gamepads(menu->console, &menu->gamepads);

    SDL_Libretro_MenuLoadState(menu);
    SDL_Libretro_MenuSyncSettings(menu);
    SDL_Libretro_MenuBuildWidgets(menu);

    nk_input_begin(menu->ctx);

    return menu;
}

void SDL_Libretro_DestroyMenu(SDL_LibretroMenu* menu) {
    if (menu == NULL) {
        return;
    }
    if (menu->settingsDirty) {
        SDL_Libretro_MenuSaveState(menu);
    }
    if (menu->ctx != NULL) {
        nk_input_end(menu->ctx);
    }
    if (menu->console != NULL) {
        nk_console_free(menu->console);
    }
    SDL_Libretro_MenuFreeOptionStates(menu);
    SDL_Libretro_MenuFreePortStates(menu);
    nk_gamepad_free(&menu->gamepads);
    if (menu->ctx != NULL) {
        nk_sdl_shutdown(menu->ctx);
    }
    SDL_free(menu);
}

bool SDL_Libretro_IsMenuOpen(const SDL_LibretroMenu* menu) {
    return menu != NULL && menu->open;
}

void SDL_Libretro_SetMenuOpen(SDL_LibretroMenu* menu, bool open) {
    if (menu == NULL || menu->open == open) {
        return;
    }
    menu->open = open;
    if (!open && menu->lr != NULL && menu->lr->window != NULL) {
        SDL_StopTextInput(menu->lr->window);
    }
}

void SDL_Libretro_ToggleMenu(SDL_LibretroMenu* menu) {
    if (menu != NULL) {
        SDL_Libretro_SetMenuOpen(menu, !menu->open);
    }
}

bool SDL_Libretro_MenuHandleEvent(SDL_LibretroMenu* menu, const SDL_Event* event) {
    if (menu == NULL || menu->ctx == NULL || event == NULL) {
        return false;
    }

    // Gamepad connection bookkeeping runs even while the menu is closed.
    nk_gamepad_sdl3_handle_event(&menu->gamepads, (SDL_Event*)event);

    // Toggle on the menu key or the gamepad Guide button.
    if (event->type == SDL_EVENT_KEY_UP && event->key.key == SDL_LIBRETRO_MENU_TOGGLE_KEY) {
        SDL_Libretro_ToggleMenu(menu);
        return true;
    }
    if (event->type == SDL_EVENT_GAMEPAD_BUTTON_UP && event->gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE) {
        SDL_Libretro_ToggleMenu(menu);
        return true;
    }

    if (!menu->open) {
        return false;
    }

    // Feed a copy so coordinate conversion doesn't mutate the caller's event.
    SDL_Event converted = *event;
    SDL_ConvertEventToRenderCoordinates(menu->lr->renderer, &converted);
    nk_sdl_handle_event(menu->ctx, &converted);

    // Lifecycle events stay visible to the application and the frontend even
    // while the menu swallows gameplay input.
    switch (event->type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_DROP_FILE:
        case SDL_EVENT_GAMEPAD_ADDED:
        case SDL_EVENT_GAMEPAD_REMOVED:
        case SDL_EVENT_JOYSTICK_ADDED:
        case SDL_EVENT_JOYSTICK_REMOVED:
            return false;
        default:
            break;
    }
    if (event->type >= SDL_EVENT_WINDOW_FIRST && event->type <= SDL_EVENT_WINDOW_LAST) {
        return false;
    }
    return true;
}

/**
 * Rebuild the core-derived submenus (Core Options, Controllers, and the file
 * filter) when they no longer match the loaded core:
 *
 *  * an option changed outside the menu (the app-side dirty flag),
 *  * the loaded core changed (library name) or its option set changed size,
 *  * the menu just opened; visibility may have shifted while it was closed,
 *    so the core's display callback is re-run first.
 *
 * @internal
 */
static void SDL_Libretro_MenuRebuildCoreMenus(SDL_LibretroMenu* menu, bool justOpened) {
    SDL_Libretro* lr = menu->lr;
    if (SDL_Libretro_AreOptionsDirty(lr)) {
        menu->optionsStale = true;
    }
    bool coreChanged = SDL_strcmp(menu->builtCoreName, lr->core.libraryName) != 0;
    bool countChanged = menu->builtOptionCount != lr->core.optionCount;
    if (!justOpened && !coreChanged && !countChanged && !menu->optionsStale) {
        return;
    }

    if (justOpened && SDL_Libretro_UpdateOptionVisibility(lr)) {
        SDL_Libretro_AreOptionsDirty(lr);
    }
    SDL_Libretro_MenuBuildOptions(menu);
    SDL_Libretro_MenuBuildControllers(menu);
    SDL_Libretro_MenuUpdateLoadGameFilter(menu);
    menu->optionsStale = false;
    menu->builtOptionCount = lr->core.optionCount;
    SDL_strlcpy(menu->builtCoreName, lr->core.libraryName, sizeof(menu->builtCoreName));
}

void SDL_Libretro_UpdateMenu(SDL_LibretroMenu* menu) {
    if (menu == NULL || menu->ctx == NULL) {
        return;
    }
    SDL_Libretro* lr = menu->lr;

    nk_input_end(menu->ctx);
    nk_gamepad_update(&menu->gamepads);

    // Show the menu whenever there's nothing to run.
    if (!menu->open && !SDL_Libretro_IsGameReady(lr)) {
        SDL_Libretro_SetMenuOpen(menu, true);
    }

    bool justOpened = menu->open && !menu->wasOpen;
    menu->wasOpen = menu->open;
    if (!menu->open) {
        return;
    }

    SDL_Libretro_MenuRebuildCoreMenus(menu, justOpened);

    // Game-dependent entries.
    bool gameReady = SDL_Libretro_IsGameReady(lr);
    menu->resumeButton->visible = (nk_bool)gameReady;
    menu->stateRow->visible = (nk_bool)gameReady;
    menu->resetButton->visible = (nk_bool)gameReady;

    // Settings that can change outside the menu.
    if (justOpened) {
        SDL_Libretro_MenuSyncSettings(menu);
    }

    // Flush changed menu settings into the config.
    if (menu->settingsDirty) {
        SDL_Libretro_MenuSaveState(menu);
    }

    int width = 0;
    int height = 0;
    SDL_GetRenderOutputSize(lr->renderer, &width, &height);

    menu->uiBuilt = true;
    nk_console_render_window(menu->console, "SDL_libretro", nk_rect(0.0f, 0.0f, (float)width, (float)height), NK_WINDOW_SCROLL_AUTO_HIDE |
                                                                                                                  // Show the window title only on the top level.
                                                                                                                  ((nk_console_active_parent(menu->console) == menu->console) ? NK_WINDOW_TITLE : 0));
}

void SDL_Libretro_RenderMenu(SDL_LibretroMenu* menu) {
    if (menu == NULL || menu->ctx == NULL) {
        return;
    }
    // Flush whenever nk_begin ran this frame, even if a callback closed the
    // menu mid-frame, so nk_clear always pairs with it.
    if (menu->uiBuilt) {
        nk_sdl_render(menu->ctx, NK_ANTI_ALIASING_ON);
        nk_console_sdl_update_text_input(menu->console, menu->lr->window);
        menu->uiBuilt = false;
    }
    nk_input_begin(menu->ctx);
}

#endif /* SDL_LIBRETRO_MENU_IMPL_ONCE */
