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

#ifndef SDL_LIBRETRO_MENU_NO_NK_IMPLEMENTATION
#define NK_CONSOLE_IMPLEMENTATION
#endif
#ifndef NK_CONSOLE_ENABLE_TINYDIR
#define NK_CONSOLE_ENABLE_TINYDIR
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
 * Base height in pixels for the menu font, multiplied by the UI scale and the
 * window's display scale.
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
    struct nk_font_atlas* atlas; /** The backend's font atlas, cleared before each rebake. */
    float bakedFontHeight; /** The font pixel height of the current bake, to detect scale changes. */

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
    int uiScaleIndex; /** 0 = Auto (resolution-based), 1..4 = fixed multiplier. */
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
    if (menu->lr->ini != NULL) {
        INI_SetBoolean(menu->lr->ini, NULL, "menufullscreen", menu->fullscreenChecked == nk_true);
    }
}

/**
 * @internal
 */
static void SDL_Libretro_MenuVSyncChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_SetRenderVSync(menu->lr->renderer, menu->vsyncChecked == nk_true ? 1 : 0);
    if (menu->lr->ini != NULL) {
        INI_SetBoolean(menu->lr->ini, NULL, "menuvsync", menu->vsyncChecked == nk_true);
    }
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
    if (lr->ini != NULL) {
        INI_SetBoolean(lr->ini, NULL, "menumuted", menu->muteChecked == nk_true);
        INI_SetFloat(lr->ini, NULL, "menumutevolume", menu->preMuteVolume);
    }
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
 * Whether the option should appear in the Core Options submenu.
 *
 * @internal
 */
static bool SDL_Libretro_MenuOptionShown(const SDL_LibretroOption* option) {
    return option != NULL && option->visible && option->valuesCount > 0;
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
        size_t length = 0;
        for (unsigned v = 0; v < option->valuesCount; v++) {
            const char* item = (option->values[v].label != NULL && option->values[v].label[0] != '\0')
                                   ? option->values[v].label
                                   : option->values[v].value;
            length += SDL_strlen(item) + 1;
        }
        state->displayList = (char*)SDL_malloc(length + 1);
        if (state->displayList == NULL) {
            return;
        }
        state->displayList[0] = '\0';
        state->selected = 0;
        size_t offset = 0;
        for (unsigned v = 0; v < option->valuesCount; v++) {
            const char* item = (option->values[v].label != NULL && option->values[v].label[0] != '\0')
                                   ? option->values[v].label
                                   : option->values[v].value;
            if (v > 0) {
                state->displayList[offset++] = '|';
            }
            offset += SDL_strlcpy(state->displayList + offset, item, length + 1 - offset);
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

    nk_console_button_set_symbol(
        nk_console_button_onclick(menu->optionsButton, "Core Options", &nk_console_button_back),
        NK_SYMBOL_TRIANGLE_UP);

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
        nk_console_button_set_symbol(
            nk_console_button_onclick(categoryButton, categoryLabel, &nk_console_button_back),
            NK_SYMBOL_TRIANGLE_UP);

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

    nk_console_button_set_symbol(
        nk_console_button_onclick(menu->controllersButton, "Controllers", &nk_console_button_back),
        NK_SYMBOL_TRIANGLE_UP);

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
        size_t length = 0;
        for (unsigned i = 0; i < info->num_types; i++) {
            length += SDL_strlen(info->types[i].desc != NULL ? info->types[i].desc : "Unknown") + 1;
        }
        SDL_LibretroMenuPortState* state = &menu->portStates[port];
        state->deviceList = (char*)SDL_malloc(length + 1);
        if (state->deviceList == NULL) {
            continue;
        }
        state->menu = menu;
        state->port = port;
        state->selected = 0;
        state->deviceList[0] = '\0';

        unsigned currentDevice = SDL_Libretro_GetPortDevice(lr, port);
        size_t offset = 0;
        for (unsigned i = 0; i < info->num_types; i++) {
            if (i > 0) {
                state->deviceList[offset++] = '|';
            }
            offset += SDL_strlcpy(state->deviceList + offset, info->types[i].desc != NULL ? info->types[i].desc : "Unknown", length + 1 - offset);
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
 * The automatic UI scale step for a window of the given logical width,
 * following raylib-libretro's resolution thresholds adapted to the 16px base
 * font.
 *
 * @internal
 */
static float SDL_Libretro_MenuAutoScale(int width) {
    if (width >= 3840) {
        return 4.0f;
    }
    if (width >= 2560) {
        return 3.0f;
    }
    if (width >= 1280) {
        return 2.0f;
    }
    return 1.0f;
}

/**
 * The font pixel height for the current window: the base height times the UI
 * scale (the manual override, or the resolution-based step) times the
 * window's display scale.
 *
 * @internal
 */
static float SDL_Libretro_MenuFontHeight(SDL_LibretroMenu* menu) {
    float displayScale = SDL_GetWindowDisplayScale(menu->lr->window);
    if (displayScale <= 0.0f) {
        displayScale = 1.0f;
    }
    float step;
    if (menu->uiScaleIndex > 0) {
        step = (float)menu->uiScaleIndex;
    }
    else {
        int width = 0;
        int height = 0;
        SDL_GetWindowSize(menu->lr->window, &width, &height);
        step = SDL_Libretro_MenuAutoScale(width);
    }
    return (float)SDL_LIBRETRO_MENU_FONT_HEIGHT * step * displayScale;
}

/**
 * (Re)bakes the default font at the given pixel height and makes it the
 * active style font. The previous atlas is cleared first so rebakes on
 * resolution or scale changes don't accumulate.
 *
 * @internal
 */
static void SDL_Libretro_MenuBakeFont(SDL_LibretroMenu* menu, float fontHeight) {
    if (menu->atlas != NULL) {
        nk_font_atlas_clear(menu->atlas);
        menu->atlas = NULL;
    }
    struct nk_font_config fontConfig = nk_font_config(0);
    menu->atlas = nk_sdl_font_stash_begin(menu->ctx);
    struct nk_font* font = nk_font_atlas_add_default(menu->atlas, fontHeight, &fontConfig);
    nk_sdl_font_stash_end(menu->ctx);
    if (font != NULL) {
        nk_style_set_font(menu->ctx, &font->handle);
    }
    menu->bakedFontHeight = fontHeight;
}

/**
 * @internal
 */
static void SDL_Libretro_MenuUIScaleChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    // The font rebakes on the next update; only the choice persists here.
    if (menu->lr != NULL && menu->lr->ini != NULL) {
        INI_SetInt(menu->lr->ini, NULL, "menuuiscale", (Sint64)menu->uiScaleIndex);
    }
}

bool SDL_Libretro_SetMenuStyle(SDL_LibretroMenu* menu, SDL_LibretroMenuStyle style) {
    if (menu == NULL || menu->ctx == NULL || (int)style < 0 || style >= SDL_LIBRETRO_MENU_STYLE_COUNT) {
        return false;
    }
    struct nk_context* ctx = menu->ctx;
    struct nk_color table[NK_COLOR_COUNT];

    // Reset the styles to default first.
    nk_style_default(ctx);

    switch (style) {
        case SDL_LIBRETRO_MENU_STYLE_DRACULA: {
            struct nk_color background = nk_rgba(40, 42, 54, 235);
            struct nk_color currentline = nk_rgba(68, 71, 90, 255);
            struct nk_color foreground = nk_rgba(248, 248, 242, 255);
            struct nk_color comment = nk_rgba(98, 114, 164, 255);
            /* struct nk_color cyan = nk_rgba(139, 233, 253, 255); */
            /* struct nk_color green = nk_rgba(80, 250, 123, 255); */
            /* struct nk_color orange = nk_rgba(255, 184, 108, 255); */
            struct nk_color pink = nk_rgba(255, 121, 198, 255);
            struct nk_color purple = nk_rgba(189, 147, 249, 255);
            /* struct nk_color red = nk_rgba(255, 85, 85, 255); */
            /* struct nk_color yellow = nk_rgba(241, 250, 140, 255); */
            table[NK_COLOR_TEXT] = foreground;
            table[NK_COLOR_WINDOW] = background;
            table[NK_COLOR_HEADER] = currentline;
            table[NK_COLOR_BORDER] = currentline;
            table[NK_COLOR_BUTTON] = currentline;
            table[NK_COLOR_BUTTON_HOVER] = comment;
            table[NK_COLOR_BUTTON_ACTIVE] = purple;
            table[NK_COLOR_TOGGLE] = currentline;
            table[NK_COLOR_TOGGLE_HOVER] = comment;
            table[NK_COLOR_TOGGLE_CURSOR] = pink;
            table[NK_COLOR_SELECT] = currentline;
            table[NK_COLOR_SELECT_ACTIVE] = comment;
            table[NK_COLOR_SLIDER] = background;
            table[NK_COLOR_SLIDER_CURSOR] = currentline;
            table[NK_COLOR_SLIDER_CURSOR_HOVER] = comment;
            table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = comment;
            table[NK_COLOR_PROPERTY] = currentline;
            table[NK_COLOR_EDIT] = currentline;
            table[NK_COLOR_EDIT_CURSOR] = foreground;
            table[NK_COLOR_COMBO] = currentline;
            table[NK_COLOR_CHART] = currentline;
            table[NK_COLOR_CHART_COLOR] = comment;
            table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = purple;
            table[NK_COLOR_SCROLLBAR] = background;
            table[NK_COLOR_SCROLLBAR_CURSOR] = currentline;
            table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = comment;
            table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = purple;
            table[NK_COLOR_TAB_HEADER] = currentline;
            table[NK_COLOR_KNOB] = table[NK_COLOR_SLIDER];
            table[NK_COLOR_KNOB_CURSOR] = table[NK_COLOR_SLIDER_CURSOR];
            table[NK_COLOR_KNOB_CURSOR_HOVER] = table[NK_COLOR_SLIDER_CURSOR_HOVER];
            table[NK_COLOR_KNOB_CURSOR_ACTIVE] = table[NK_COLOR_SLIDER_CURSOR_ACTIVE];
            nk_style_from_table(ctx, table);
            break;
        }
        case SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_LATTE: {
            /* struct nk_color rosewater = nk_rgba(220, 138, 120, 255); */
            /* struct nk_color flamingo = nk_rgba(221, 120, 120, 255); */
            struct nk_color pink = nk_rgba(234, 118, 203, 255);
            struct nk_color mauve = nk_rgba(136, 57, 239, 255);
            /* struct nk_color red = nk_rgba(210, 15, 57, 255); */
            /* struct nk_color maroon = nk_rgba(230, 69, 83, 255); */
            /* struct nk_color peach = nk_rgba(254, 100, 11, 255); */
            struct nk_color yellow = nk_rgba(223, 142, 29, 255);
            /* struct nk_color green = nk_rgba(64, 160, 43, 255); */
            struct nk_color teal = nk_rgba(23, 146, 153, 255);
            /* struct nk_color sky = nk_rgba(4, 165, 229, 255); */
            /* struct nk_color sapphire = nk_rgba(32, 159, 181, 255); */
            /* struct nk_color blue = nk_rgba(30, 102, 245, 255); */
            /* struct nk_color lavender = nk_rgba(114, 135, 253, 255); */
            struct nk_color text = nk_rgba(76, 79, 105, 255);
            /* struct nk_color subtext1 = nk_rgba(92, 95, 119, 255); */
            /* struct nk_color subtext0 = nk_rgba(108, 111, 133, 255); */
            struct nk_color overlay2 = nk_rgba(124, 127, 147, 55);
            /* struct nk_color overlay1 = nk_rgba(140, 143, 161, 255); */
            struct nk_color overlay0 = nk_rgba(156, 160, 176, 255);
            struct nk_color surface2 = nk_rgba(172, 176, 190, 255);
            struct nk_color surface1 = nk_rgba(188, 192, 204, 255);
            struct nk_color surface0 = nk_rgba(204, 208, 218, 255);
            struct nk_color base = nk_rgba(239, 241, 245, 235);
            struct nk_color mantle = nk_rgba(230, 233, 239, 255);
            /* struct nk_color crust = nk_rgba(220, 224, 232, 255); */
            table[NK_COLOR_TEXT] = text;
            table[NK_COLOR_WINDOW] = base;
            table[NK_COLOR_HEADER] = mantle;
            table[NK_COLOR_BORDER] = mantle;
            table[NK_COLOR_BUTTON] = surface0;
            table[NK_COLOR_BUTTON_HOVER] = overlay2;
            table[NK_COLOR_BUTTON_ACTIVE] = overlay0;
            table[NK_COLOR_TOGGLE] = surface2;
            table[NK_COLOR_TOGGLE_HOVER] = overlay2;
            table[NK_COLOR_TOGGLE_CURSOR] = yellow;
            table[NK_COLOR_SELECT] = surface0;
            table[NK_COLOR_SELECT_ACTIVE] = overlay0;
            table[NK_COLOR_SLIDER] = surface1;
            table[NK_COLOR_SLIDER_CURSOR] = teal;
            table[NK_COLOR_SLIDER_CURSOR_HOVER] = teal;
            table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = teal;
            table[NK_COLOR_PROPERTY] = surface0;
            table[NK_COLOR_EDIT] = surface0;
            table[NK_COLOR_EDIT_CURSOR] = mauve;
            table[NK_COLOR_COMBO] = surface0;
            table[NK_COLOR_CHART] = surface0;
            table[NK_COLOR_CHART_COLOR] = teal;
            table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = mauve;
            table[NK_COLOR_SCROLLBAR] = surface0;
            table[NK_COLOR_SCROLLBAR_CURSOR] = overlay0;
            table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = mauve;
            table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = mauve;
            table[NK_COLOR_TAB_HEADER] = surface0;
            table[NK_COLOR_KNOB] = table[NK_COLOR_SLIDER];
            table[NK_COLOR_KNOB_CURSOR] = pink;
            table[NK_COLOR_KNOB_CURSOR_HOVER] = pink;
            table[NK_COLOR_KNOB_CURSOR_ACTIVE] = pink;
            nk_style_from_table(ctx, table);
            break;
        }
        case SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_FRAPPE: {
            /* struct nk_color rosewater = nk_rgba(242, 213, 207, 255); */
            /* struct nk_color flamingo = nk_rgba(238, 190, 190, 255); */
            struct nk_color pink = nk_rgba(244, 184, 228, 255);
            /* struct nk_color mauve = nk_rgba(202, 158, 230, 255); */
            /* struct nk_color red = nk_rgba(231, 130, 132, 255); */
            /* struct nk_color maroon = nk_rgba(234, 153, 156, 255); */
            /* struct nk_color peach = nk_rgba(239, 159, 118, 255); */
            /* struct nk_color yellow = nk_rgba(229, 200, 144, 255); */
            struct nk_color green = nk_rgba(166, 209, 137, 255);
            /* struct nk_color teal = nk_rgba(129, 200, 190, 255); */
            /* struct nk_color sky = nk_rgba(153, 209, 219, 255); */
            /* struct nk_color sapphire = nk_rgba(133, 193, 220, 255); */
            /* struct nk_color blue = nk_rgba(140, 170, 238, 255); */
            struct nk_color lavender = nk_rgba(186, 187, 241, 255);
            struct nk_color text = nk_rgba(198, 208, 245, 255);
            /* struct nk_color subtext1 = nk_rgba(181, 191, 226, 255); */
            /* struct nk_color subtext0 = nk_rgba(165, 173, 206, 255); */
            struct nk_color overlay2 = nk_rgba(148, 156, 187, 255);
            struct nk_color overlay1 = nk_rgba(131, 139, 167, 255);
            struct nk_color overlay0 = nk_rgba(115, 121, 148, 255);
            struct nk_color surface2 = nk_rgba(98, 104, 128, 255);
            struct nk_color surface1 = nk_rgba(81, 87, 109, 255);
            struct nk_color surface0 = nk_rgba(65, 69, 89, 255);
            struct nk_color base = nk_rgba(48, 52, 70, 235);
            struct nk_color mantle = nk_rgba(41, 44, 60, 255);
            /* struct nk_color crust = nk_rgba(35, 38, 52, 255); */
            table[NK_COLOR_TEXT] = text;
            table[NK_COLOR_WINDOW] = base;
            table[NK_COLOR_HEADER] = mantle;
            table[NK_COLOR_BORDER] = mantle;
            table[NK_COLOR_BUTTON] = surface0;
            table[NK_COLOR_BUTTON_HOVER] = overlay1;
            table[NK_COLOR_BUTTON_ACTIVE] = overlay0;
            table[NK_COLOR_TOGGLE] = surface2;
            table[NK_COLOR_TOGGLE_HOVER] = overlay2;
            table[NK_COLOR_TOGGLE_CURSOR] = pink;
            table[NK_COLOR_SELECT] = surface0;
            table[NK_COLOR_SELECT_ACTIVE] = overlay0;
            table[NK_COLOR_SLIDER] = surface1;
            table[NK_COLOR_SLIDER_CURSOR] = green;
            table[NK_COLOR_SLIDER_CURSOR_HOVER] = green;
            table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = green;
            table[NK_COLOR_PROPERTY] = surface0;
            table[NK_COLOR_EDIT] = surface0;
            table[NK_COLOR_EDIT_CURSOR] = pink;
            table[NK_COLOR_COMBO] = surface0;
            table[NK_COLOR_CHART] = surface0;
            table[NK_COLOR_CHART_COLOR] = lavender;
            table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = pink;
            table[NK_COLOR_SCROLLBAR] = surface0;
            table[NK_COLOR_SCROLLBAR_CURSOR] = overlay0;
            table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = lavender;
            table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = lavender;
            table[NK_COLOR_TAB_HEADER] = surface0;
            table[NK_COLOR_KNOB] = table[NK_COLOR_SLIDER];
            table[NK_COLOR_KNOB_CURSOR] = pink;
            table[NK_COLOR_KNOB_CURSOR_HOVER] = pink;
            table[NK_COLOR_KNOB_CURSOR_ACTIVE] = pink;
            nk_style_from_table(ctx, table);
            break;
        }
        case SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_MACCHIATO: {
            /* struct nk_color rosewater = nk_rgba(244, 219, 214, 255); */
            /* struct nk_color flamingo = nk_rgba(240, 198, 198, 255); */
            struct nk_color pink = nk_rgba(245, 189, 230, 255);
            /* struct nk_color mauve = nk_rgba(198, 160, 246, 255); */
            /* struct nk_color red = nk_rgba(237, 135, 150, 255); */
            /* struct nk_color maroon = nk_rgba(238, 153, 160, 255); */
            /* struct nk_color peach = nk_rgba(245, 169, 127, 255); */
            struct nk_color yellow = nk_rgba(238, 212, 159, 255);
            struct nk_color green = nk_rgba(166, 218, 149, 255);
            /* struct nk_color teal = nk_rgba(139, 213, 202, 255); */
            /* struct nk_color sky = nk_rgba(145, 215, 227, 255); */
            /* struct nk_color sapphire = nk_rgba(125, 196, 228, 255); */
            /* struct nk_color blue = nk_rgba(138, 173, 244, 255); */
            struct nk_color lavender = nk_rgba(183, 189, 248, 255);
            struct nk_color text = nk_rgba(202, 211, 245, 255);
            /* struct nk_color subtext1 = nk_rgba(184, 192, 224, 255); */
            /* struct nk_color subtext0 = nk_rgba(165, 173, 203, 255); */
            struct nk_color overlay2 = nk_rgba(147, 154, 183, 255);
            struct nk_color overlay1 = nk_rgba(128, 135, 162, 255);
            struct nk_color overlay0 = nk_rgba(110, 115, 141, 255);
            struct nk_color surface2 = nk_rgba(91, 96, 120, 255);
            struct nk_color surface1 = nk_rgba(73, 77, 100, 255);
            struct nk_color surface0 = nk_rgba(54, 58, 79, 255);
            struct nk_color base = nk_rgba(36, 39, 58, 235);
            struct nk_color mantle = nk_rgba(30, 32, 48, 255);
            /* struct nk_color crust = nk_rgba(24, 25, 38, 255); */
            table[NK_COLOR_TEXT] = text;
            table[NK_COLOR_WINDOW] = base;
            table[NK_COLOR_HEADER] = mantle;
            table[NK_COLOR_BORDER] = mantle;
            table[NK_COLOR_BUTTON] = surface0;
            table[NK_COLOR_BUTTON_HOVER] = overlay1;
            table[NK_COLOR_BUTTON_ACTIVE] = overlay0;
            table[NK_COLOR_TOGGLE] = surface2;
            table[NK_COLOR_TOGGLE_HOVER] = overlay2;
            table[NK_COLOR_TOGGLE_CURSOR] = yellow;
            table[NK_COLOR_SELECT] = surface0;
            table[NK_COLOR_SELECT_ACTIVE] = overlay0;
            table[NK_COLOR_SLIDER] = surface1;
            table[NK_COLOR_SLIDER_CURSOR] = green;
            table[NK_COLOR_SLIDER_CURSOR_HOVER] = green;
            table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = green;
            table[NK_COLOR_PROPERTY] = surface0;
            table[NK_COLOR_EDIT] = surface0;
            table[NK_COLOR_EDIT_CURSOR] = pink;
            table[NK_COLOR_COMBO] = surface0;
            table[NK_COLOR_CHART] = surface0;
            table[NK_COLOR_CHART_COLOR] = lavender;
            table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = yellow;
            table[NK_COLOR_SCROLLBAR] = surface0;
            table[NK_COLOR_SCROLLBAR_CURSOR] = overlay0;
            table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = lavender;
            table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = lavender;
            table[NK_COLOR_TAB_HEADER] = surface0;
            table[NK_COLOR_KNOB] = table[NK_COLOR_SLIDER];
            table[NK_COLOR_KNOB_CURSOR] = pink;
            table[NK_COLOR_KNOB_CURSOR_HOVER] = pink;
            table[NK_COLOR_KNOB_CURSOR_ACTIVE] = pink;
            nk_style_from_table(ctx, table);
            break;
        }
        case SDL_LIBRETRO_MENU_STYLE_CATPPUCCIN_MOCHA: {
            /* struct nk_color rosewater = nk_rgba(245, 224, 220, 255); */
            /* struct nk_color flamingo = nk_rgba(242, 205, 205, 255); */
            struct nk_color pink = nk_rgba(245, 194, 231, 255);
            /* struct nk_color mauve = nk_rgba(203, 166, 247, 255); */
            /* struct nk_color red = nk_rgba(243, 139, 168, 255); */
            /* struct nk_color maroon = nk_rgba(235, 160, 172, 255); */
            /* struct nk_color peach = nk_rgba(250, 179, 135, 255); */
            /* struct nk_color yellow = nk_rgba(249, 226, 175, 255); */
            struct nk_color green = nk_rgba(166, 227, 161, 255);
            /* struct nk_color teal = nk_rgba(148, 226, 213, 255); */
            /* struct nk_color sky = nk_rgba(137, 220, 235, 255); */
            /* struct nk_color sapphire = nk_rgba(116, 199, 236, 255); */
            /* struct nk_color blue = nk_rgba(137, 180, 250, 255); */
            struct nk_color lavender = nk_rgba(180, 190, 254, 255);
            struct nk_color text = nk_rgba(205, 214, 244, 255);
            /* struct nk_color subtext1 = nk_rgba(186, 194, 222, 255); */
            /* struct nk_color subtext0 = nk_rgba(166, 173, 200, 255); */
            struct nk_color overlay2 = nk_rgba(147, 153, 178, 255);
            struct nk_color overlay1 = nk_rgba(127, 132, 156, 255);
            struct nk_color overlay0 = nk_rgba(108, 112, 134, 255);
            struct nk_color surface2 = nk_rgba(88, 91, 112, 255);
            struct nk_color surface1 = nk_rgba(69, 71, 90, 255);
            struct nk_color surface0 = nk_rgba(49, 50, 68, 255);
            struct nk_color base = nk_rgba(30, 30, 46, 235);
            struct nk_color mantle = nk_rgba(24, 24, 37, 255);
            /* struct nk_color crust = nk_rgba(17, 17, 27, 255); */
            table[NK_COLOR_TEXT] = text;
            table[NK_COLOR_WINDOW] = base;
            table[NK_COLOR_HEADER] = mantle;
            table[NK_COLOR_BORDER] = mantle;
            table[NK_COLOR_BUTTON] = surface0;
            table[NK_COLOR_BUTTON_HOVER] = overlay1;
            table[NK_COLOR_BUTTON_ACTIVE] = overlay0;
            table[NK_COLOR_TOGGLE] = surface2;
            table[NK_COLOR_TOGGLE_HOVER] = overlay2;
            table[NK_COLOR_TOGGLE_CURSOR] = lavender;
            table[NK_COLOR_SELECT] = surface0;
            table[NK_COLOR_SELECT_ACTIVE] = overlay0;
            table[NK_COLOR_SLIDER] = surface1;
            table[NK_COLOR_SLIDER_CURSOR] = green;
            table[NK_COLOR_SLIDER_CURSOR_HOVER] = green;
            table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = green;
            table[NK_COLOR_PROPERTY] = surface0;
            table[NK_COLOR_EDIT] = surface0;
            table[NK_COLOR_EDIT_CURSOR] = lavender;
            table[NK_COLOR_COMBO] = surface0;
            table[NK_COLOR_CHART] = surface0;
            table[NK_COLOR_CHART_COLOR] = lavender;
            table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = pink;
            table[NK_COLOR_SCROLLBAR] = surface0;
            table[NK_COLOR_SCROLLBAR_CURSOR] = overlay0;
            table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = lavender;
            table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = pink;
            table[NK_COLOR_TAB_HEADER] = surface0;
            table[NK_COLOR_KNOB] = table[NK_COLOR_SLIDER];
            table[NK_COLOR_KNOB_CURSOR] = pink;
            table[NK_COLOR_KNOB_CURSOR_HOVER] = pink;
            table[NK_COLOR_KNOB_CURSOR_ACTIVE] = pink;
            nk_style_from_table(ctx, table);
            break;
        }
        case SDL_LIBRETRO_MENU_STYLE_DARK:
        default: {
            // The Nuklear default style.
            break;
        }
    }

    menu->style = style;
    menu->styleIndex = (int)style;

    // Persist the theme; the config is written to disk when the context closes.
    if (menu->lr != NULL && menu->lr->ini != NULL) {
        INI_SetInt(menu->lr->ini, NULL, "menutheme", (Sint64)style);
    }
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

    // Apply the saved UI scale when the config has one, then bake the default
    // font for the current resolution and display scale.
    if (lr->ini != NULL) {
        Sint64 savedScale = INI_GetInt(lr->ini, NULL, "menuuiscale", 0);
        if (savedScale > 0 && savedScale <= 4) {
            menu->uiScaleIndex = (int)savedScale;
        }
    }
    SDL_Libretro_MenuBakeFont(menu, SDL_Libretro_MenuFontHeight(menu));

    menu->console = nk_console_init(menu->ctx);
    if (menu->console == NULL) {
        nk_sdl_shutdown(menu->ctx);
        SDL_free(menu);
        SDL_SetError("Failed to initialize nuklear_console");
        return NULL;
    }

    nk_gamepad_init(&menu->gamepads, menu->ctx, NULL);
    nk_console_set_gamepads(menu->console, &menu->gamepads);

    // Apply the saved theme when the config has one, the default otherwise.
    SDL_LibretroMenuStyle initialStyle = SDL_LIBRETRO_MENU_DEFAULT_STYLE;
    if (lr->ini != NULL && INI_HasValue(lr->ini, NULL, "menutheme")) {
        Sint64 savedStyle = INI_GetInt(lr->ini, NULL, "menutheme", (Sint64)initialStyle);
        if (savedStyle >= 0 && savedStyle < (Sint64)SDL_LIBRETRO_MENU_STYLE_COUNT) {
            initialStyle = (SDL_LibretroMenuStyle)savedStyle;
        }
    }
    SDL_Libretro_SetMenuStyle(menu, initialStyle);

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

    // Settings
    nk_console* settings = nk_console_button(menu->console, "Settings");
    nk_console_button_set_symbol(settings, NK_SYMBOL_HAMBURGER);
    {
        nk_console_button_set_symbol(
            nk_console_button_onclick(settings, "Settings", &nk_console_button_back),
            NK_SYMBOL_TRIANGLE_UP);

        // Apply the saved window/audio state before the widgets snapshot it.
        if (lr->ini != NULL) {
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

        // Audio & Video
        nk_console* audioVideo = nk_console_button(settings, "Audio & Video");
        nk_console_button_set_symbol(audioVideo, NK_SYMBOL_TRIANGLE_RIGHT);
        {
            nk_console_button_set_symbol(
                nk_console_button_onclick(audioVideo, "Audio & Video", &nk_console_button_back),
                NK_SYMBOL_TRIANGLE_UP);

            menu->volumePercent = (int)(SDL_Libretro_GetVolume(lr) * 100.0f + 0.5f);
            nk_console* volume = nk_console_property_int(audioVideo, "Volume", 0, &menu->volumePercent, 100, 5, 1.0f);
            nk_console_add_event_handler(volume, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuVolumeChanged, menu, NULL);

            nk_console* mute = nk_console_checkbox(audioVideo, "Mute", &menu->muteChecked);
            nk_console_add_event_handler(mute, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuMuteChanged, menu, NULL);

            menu->fullscreenChecked = (SDL_GetWindowFlags(lr->window) & SDL_WINDOW_FULLSCREEN) != 0;
            nk_console* fullscreen = nk_console_checkbox(audioVideo, "Fullscreen", &menu->fullscreenChecked);
            nk_console_add_event_handler(fullscreen, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuFullscreenChanged, menu, NULL);

            int vsync = 0;
            SDL_GetRenderVSync(lr->renderer, &vsync);
            menu->vsyncChecked = vsync != 0;
            nk_console* vsyncBox = nk_console_checkbox(audioVideo, "VSync", &menu->vsyncChecked);
            nk_console_add_event_handler(vsyncBox, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuVSyncChanged, menu, NULL);

            menu->filterIndex = SDL_Libretro_GetTextureScaleMode(lr) == SDL_SCALEMODE_LINEAR ? 1 : 0;
            nk_console* filter = nk_console_combobox(audioVideo, "Filter", "Nearest|Linear", '|', &menu->filterIndex);
            nk_console_add_event_handler(filter, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuFilterChanged, menu, NULL);

            menu->fitModeIndex = (int)SDL_Libretro_GetFitMode(lr);
            nk_console* fitMode = nk_console_combobox(audioVideo, "Fit Mode", "Aspect|Integer|Stretch", '|', &menu->fitModeIndex);
            nk_console_add_event_handler(fitMode, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuFitModeChanged, menu, NULL);

            nk_console* theme = nk_console_combobox(audioVideo, "Theme", SDL_LIBRETRO_MENU_STYLE_NAMES, '|', &menu->styleIndex);
            nk_console_add_event_handler(theme, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuStyleChanged, menu, NULL);

            nk_console* uiScale = nk_console_combobox(audioVideo, "UI Scale", "Auto|1x|2x|3x|4x", '|', &menu->uiScaleIndex);
            nk_console_add_event_handler(uiScale, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuUIScaleChanged, menu, NULL);
        }
    }

    // Quit
    nk_console* quit = nk_console_button_onclick_handler(menu->console, "Quit", &SDL_Libretro_MenuQuitClicked, menu, NULL);
    nk_console_button_set_symbol(quit, NK_SYMBOL_X);

    nk_input_begin(menu->ctx);

    return menu;
}

void SDL_Libretro_DestroyMenu(SDL_LibretroMenu* menu) {
    if (menu == NULL) {
        return;
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

    // Rebuild the Core Options submenu when it went stale: an option changed
    // outside the menu, the core changed, the option set changed size, or the
    // menu just opened (visibility may have shifted while it was closed).
    if (SDL_Libretro_AreOptionsDirty(lr)) {
        menu->optionsStale = true;
    }
    bool coreChanged = SDL_strcmp(menu->builtCoreName, lr->core.libraryName) != 0;
    bool countChanged = menu->builtOptionCount != lr->core.optionCount;
    if (justOpened || coreChanged || countChanged || menu->optionsStale) {
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

    // Game-dependent entries.
    bool gameReady = SDL_Libretro_IsGameReady(lr);
    menu->resumeButton->visible = (nk_bool)gameReady;
    menu->stateRow->visible = (nk_bool)gameReady;
    menu->resetButton->visible = (nk_bool)gameReady;

    // Settings that can change outside the menu.
    if (justOpened) {
        menu->volumePercent = (int)(SDL_Libretro_GetVolume(lr) * 100.0f + 0.5f);
        menu->fitModeIndex = (int)SDL_Libretro_GetFitMode(lr);
        menu->filterIndex = SDL_Libretro_GetTextureScaleMode(lr) == SDL_SCALEMODE_LINEAR ? 1 : 0;
        menu->fullscreenChecked = (SDL_GetWindowFlags(lr->window) & SDL_WINDOW_FULLSCREEN) != 0;
    }

    // Track resolution, display-scale and UI-scale changes while open, so a
    // resize or a move between monitors rebakes the font at the right size.
    float fontHeight = SDL_Libretro_MenuFontHeight(menu);
    if (fontHeight != menu->bakedFontHeight) {
        SDL_Libretro_MenuBakeFont(menu, fontHeight);
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
