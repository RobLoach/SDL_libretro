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

    // Settings state the widgets write into.
    int volumePercent;
    int fitModeIndex;

    // Load Game
    char loadGamePath[SDL_LIBRETRO_MAX_PATH];

    // Core Options rebuild tracking
    SDL_LibretroMenuOptionState* optionStates;
    unsigned optionStateCount;
    bool optionsStale; /** The Core Options submenu no longer matches the core's options. */
    unsigned builtOptionCount; /** Option count when the submenu was last built. */
    char builtCoreName[128]; /** Core library name when the submenu was last built. */
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
 * @internal
 */
static void SDL_Libretro_MenuGameFileChanged(nk_console* widget, void* user_data) {
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
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
        nk_console_show_message(nk_console_get_top(widget), "Failed to load game");
    }
    menu->loadGamePath[0] = '\0';
}

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

    // Backing out of the top level acts like Resume.
    nk_console_add_event_handler(menu->console, NK_CONSOLE_EVENT_BACK, &SDL_Libretro_MenuResumeClicked, menu, NULL);

    // Resume
    menu->resumeButton = nk_console_button_onclick_handler(menu->console, "Resume", &SDL_Libretro_MenuResumeClicked, menu, NULL);
    nk_console_button_set_symbol(menu->resumeButton, NK_SYMBOL_TRIANGLE_RIGHT);

    // Load Game
    nk_console* loadGame = nk_console_file_action(menu->console, "Load Game", menu->loadGamePath, sizeof(menu->loadGamePath));
    nk_console_add_event_handler(loadGame, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuGameFileChanged, menu, NULL);
    if (lr->fileBrowserStartDirectory[0] != '\0') {
        nk_console_file_set_directory(loadGame, lr->fileBrowserStartDirectory);
    }

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

    // Settings
    nk_console* settings = nk_console_button(menu->console, "Settings");
    nk_console_button_set_symbol(settings, NK_SYMBOL_HAMBURGER);
    {
        nk_console_button_set_symbol(
            nk_console_button_onclick(settings, "Settings", &nk_console_button_back),
            NK_SYMBOL_TRIANGLE_UP);

        menu->volumePercent = (int)(SDL_Libretro_GetVolume(lr) * 100.0f + 0.5f);
        nk_console* volume = nk_console_property_int(settings, "Volume", 0, &menu->volumePercent, 100, 5, 1.0f);
        nk_console_add_event_handler(volume, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuVolumeChanged, menu, NULL);

        menu->fitModeIndex = (int)SDL_Libretro_GetFitMode(lr);
        nk_console* fitMode = nk_console_combobox(settings, "Fit Mode", "Aspect|Integer|Stretch", '|', &menu->fitModeIndex);
        nk_console_add_event_handler(fitMode, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuFitModeChanged, menu, NULL);
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
