/**
 * SDL_libretro - menu system built on nuklear_console
 *
 * Provides an in-app menu (Load Game, save states, core options, settings,
 * About) rendered with Nuklear through the SDL3 renderer backend, driven by
 * nuklear_console for keyboard/gamepad-friendly navigation.
 *
 * Enabled by defining SDL_LIBRETRO_ENABLE_MENU alongside
 * SDL_LIBRETRO_IMPLEMENTATION. Requires the vendored Nuklear,
 * nuklear_console, nuklear_gamepad and c-vector submodules on the
 * include path (handled by the SDL_libretro_menu CMake target).
 *
 * Frame contract, with SDL_Libretro_HandleMenuEvent() called for each event:
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
 * @see SDL_Libretro_HandleMenuEvent()
 */
#define SDL_LIBRETRO_MENU_TOGGLE_KEY SDLK_F1
#endif

#ifndef SDL_LIBRETRO_MENU_FONT_HEIGHT
/**
 * Base height in pixels for the menu font, multiplied by the UI scale and a
 * whole-number display scale. The default font (ProggyClean) is a pixel font
 * that only renders cleanly at 13px and integer multiples of it, so the
 * height stays on that grid.
 */
#define SDL_LIBRETRO_MENU_FONT_HEIGHT 13
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
 * @internal
 * The style names, separated by a |, in SDL_LibretroMenuStyle order.
 */
#define SDL_LIBRETRO_MENU_STYLE_NAMES "Mocha|Latte|Frappe|Macchiato|Dracula|Dark"

/**
 * @internal
 * The Language combobox entries, separated by a |: Auto, then the
 * RETRO_LANGUAGE_* values in enum order.
 */
#define SDL_LIBRETRO_MENU_LANGUAGE_NAMES                                              \
    "Auto|English|Japanese|French|Spanish|German|Italian|Dutch|"                      \
    "Portuguese (Brazil)|Portuguese (Portugal)|Russian|Korean|"                       \
    "Chinese (Traditional)|Chinese (Simplified)|Esperanto|Polish|Vietnamese|Arabic|"  \
    "Greek|Turkish|Slovak|Persian|Hebrew|Asturian|Finnish|Indonesian|Swedish|"        \
    "Ukrainian|Czech|Catalan (Valencia)|Catalan|British English|Hungarian|"           \
    "Belarusian|Galician|Norwegian|Irish|Thai"

/**
 * @internal
 * Per-option UI state for the "Core Options" submenu.
 *
 * The comboboxes and checkboxes write directly into these fields, so the array
 * must stay allocated for as long as the widgets exist.
 */
typedef struct SDL_LibretroMenuOptionState {
    SDL_LibretroMenu* menu;
    unsigned index; /**< The option's index within the core's option list. */
    int selected; /**< Selected combobox entry. */
    nk_bool checked; /**< Checkbox state for boolean options. */
    char* displayList; /**< Pipe-separated labels backing the combobox; owned. */
} SDL_LibretroMenuOptionState;

/**
 * @internal
 * Per-port UI state for the "Controllers" submenu.
 *
 * @see SDL_LibretroMenuOptionState
 */
typedef struct SDL_LibretroMenuPortState {
    SDL_LibretroMenu* menu;
    unsigned port;
    int selected; /**< Selected combobox entry. */
    char* deviceList; /**< Pipe-separated device names backing the combobox; owned. */
    char label[16]; /**< The "Port N" combobox label; must outlive the widget. */
} SDL_LibretroMenuPortState;

/**
 * @internal
 * Per-button UI state for the "Keyboard" bindings submenu.
 */
typedef struct SDL_LibretroMenuKeyBind {
    SDL_LibretroMenu* menu;
    int button; /**< The RETRO_DEVICE_ID_JOYPAD_* index. */
    nk_rune rune; /**< The captured key, as an NK_CONSOLE_KEY_* value. */
} SDL_LibretroMenuKeyBind;

/**
 * @internal
 * Per-button UI state for a player's page of the "Gamepad" submenu.
 */
typedef struct SDL_LibretroMenuPadBind {
    SDL_LibretroMenu* menu;
    unsigned port; /**< The controller port the binding applies to. */
    int button; /**< The RETRO_DEVICE_ID_JOYPAD_* index. */
    enum nk_gamepad_button captured; /**< The captured gamepad button. */
} SDL_LibretroMenuPadBind;

/**
 * @internal
 * Per-player UI state for the "Gamepad" submenu.
 */
typedef struct SDL_LibretroMenuPadPort {
    SDL_LibretroMenu* menu;
    unsigned port;
    int analogIndex; /**< Selected Analog to Digital entry, in SDL_LibretroAnalogToDigital order. */
    char label[64]; /**< The player submenu label, holding the gamepad name; must outlive the widget. */
    SDL_LibretroMenuPadBind binds[SDL_LIBRETRO_MAX_JOYPAD_BUTTONS];
} SDL_LibretroMenuPadPort;

/**
 * @internal
 * The state behind the opaque SDL_LibretroMenu handle. Applications must use
 * the SDL_Libretro_*Menu* functions rather than these fields.
 */
struct SDL_LibretroMenu {
    SDL_Libretro* lr;
    struct nk_context* ctx;
    nk_console* console;
    struct nk_gamepads gamepads;
    struct nk_font_atlas* atlas; /**< The backend's font atlas. */
    float renderScale; /**< The current UI render scale applied via SDL_SetRenderScale. */
    void* userData; /**< Generic data available to the application. */

    bool open; /**< Whether the menu is currently shown. */
    bool wasOpen; /**< The open state of the previous update, to detect fresh opens. */
    bool keysHeldAtOpen[SDL_SCANCODE_COUNT]; /**< Keys held down when the menu opened, whose releases still go to the application and core. */
    unsigned int padButtonsHeldAtOpen[NK_GAMEPAD_MAX]; /**< nk_gamepad buttons held when the menu opened, hidden from the menu until released. */
    bool uiBuilt; /**< nk_begin ran this frame, so RenderMenu must flush and clear. */

    // Widgets whose visibility depends on the running game.
    nk_console* resumeButton;
    nk_console* stateRow;
    nk_console* screenshotButton;
    nk_console* resetButton;
    nk_console* optionsButton;
    nk_console* controllersButton;
    nk_console* loadGameButton;

    // Settings state the widgets write into.
    int volumePercent;
    int fitModeIndex;
    int styleIndex;
    int uiScaleIndex; /**< 0 = Auto (resolution-based), 1..4 = fixed multiplier; defaults to 2. */
    SDL_LibretroMenuStyle style; /**< The active menu style. */
    int filterIndex; /**< 0 = Nearest, 1 = Linear. */
    nk_bool fullscreenChecked;
    nk_bool vsyncChecked;
    nk_bool muteChecked;
    nk_bool rewindChecked;
    int rewindBufferMB; /**< Rewind memory limit in MB; 0 for no limit. */
    int mousePortIndex; /**< The controller port the mouse drives. */
    char mousePortList[16 * SDL_LIBRETRO_MAX_USERS]; /**< "Port 1|Port 2|..." for the Mouse Port combobox. */
    int languageIndex; /**< 0 = Auto (system locale), else RETRO_LANGUAGE_* + 1. */
    float preMuteVolume; /**< Volume to restore when unmuting. */

    // Load Game
    char loadGamePath[SDL_LIBRETRO_MAX_PATH];

    // Core Options rebuild tracking
    SDL_LibretroMenuOptionState* optionStates;
    unsigned optionStateCount;
    bool optionsStale; /**< The Core Options submenu no longer matches the core's options. */
    bool settingsDirty; /**< A menu-owned setting changed and awaits SDL_Libretro_MenuSaveState(). */
    unsigned builtOptionCount; /**< Option count when the submenu was last built. */
    unsigned builtCoreGeneration; /**< SDL_Libretro::coreGeneration when the core submenus were last built; the widgets borrow core-owned strings that an unload frees. */

    // Controllers
    SDL_LibretroMenuPortState portStates[SDL_LIBRETRO_MAX_USERS];

    // Keyboard bindings for player 1 (Settings > Keyboard).
    SDL_LibretroMenuKeyBind keyBinds[SDL_LIBRETRO_MAX_JOYPAD_BUTTONS];

    // Per-player gamepad bindings and analog-to-digital (Settings > Gamepad),
    // rebuilt when the menu opens so the gamepad names stay current.
    nk_console* gamepadButton;
    SDL_LibretroMenuPadPort padPorts[SDL_LIBRETRO_MAX_USERS];

    // Disks, rebuilt when the menu opens.
    nk_console* disksButton;
    int diskSelected; /**< Radio-selected disk index. */
    unsigned builtDiskCount; /**< Disk count when the submenu was last built; bounds diskLabels. */
    char** diskLabels; /**< Owned label copies backing the radios. */

    // Progress rows, refreshed every update.
    nk_console* osdProgressWidget; /**< Top-level bar for progress-type OSD messages. */
    nk_size osdProgressValue;
    char osdProgressLabel[128];

    nk_console* quitButton; /**< Kept as the last top-level entry when the app adds its own. */

    // Settings textedit buffers (directories + username).
    char coreDirBuffer[SDL_LIBRETRO_MAX_PATH];
    char saveDirBuffer[SDL_LIBRETRO_MAX_PATH];
    char systemDirBuffer[SDL_LIBRETRO_MAX_PATH];
    char browseDirBuffer[SDL_LIBRETRO_MAX_PATH];
    char usernameBuffer[64];

    // About page, rebuilt each time it opens. The labels read the arena
    // lazily, so it must stay allocated for as long as the menu.
    nk_console* aboutButton;
    char aboutText[2048]; /**< Packed NUL-separated label lines. */
    size_t aboutTextLength;
};

/**
 * @internal
 * Closes the menu when a game is available to resume.
 *
 * Also used as the BACK handler of the top level menu.
 */
static void SDL_Libretro_MenuResumeClicked(SDL_LibretroMenu* menu, void* userdata) {
    (void)userdata;
    if (SDL_Libretro_IsGameReady(menu->lr)) {
        SDL_Libretro_SetMenuOpen(menu, false);
    }
}

/**
 * @internal
 * BACK on the top level acts like Resume.
 */
static void SDL_Libretro_MenuBackClicked(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_Libretro_MenuResumeClicked((SDL_LibretroMenu*)user_data, NULL);
}

/**
 * @internal
 * Load the game waiting in loadGamePath, closing the menu on success.
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
        // The load path already pushed SDL_EVENT_LIBRETRO_GAME_LOADED.
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
 * @internal
 * Opens a browser file picker; the chosen file is written into the virtual
 * file system and handed back through SDL_Libretro_MenuLoadGameFromJS().
 *
 * Requires ccall and FS in -sEXPORTED_RUNTIME_METHODS.
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
 * @internal
 * Called from SDL_Libretro_MenuOpenWebFilePicker()'s JS once the picked file
 * lands in the virtual file system.
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
static void SDL_Libretro_MenuWebLoadGameClicked(SDL_LibretroMenu* menu, void* userdata) {
    (void)userdata;
    SDL_Libretro_MenuOpenWebFilePicker(menu);
}
#endif /* __EMSCRIPTEN__ */

/**
 * @internal
 */
static void SDL_Libretro_MenuSaveStateClicked(SDL_LibretroMenu* menu, void* userdata) {
    (void)userdata;
    char savePath[SDL_LIBRETRO_MAX_PATH];
    if (SDL_Libretro_GetSavePath(menu->lr, ".sav", savePath, sizeof(savePath)) > 0 &&
        SDL_Libretro_SaveState(menu->lr, savePath)) {
        nk_console_show_message(menu->console, "State saved");
    }
    else {
        nk_console_show_message(menu->console, "Save State failed");
    }
}

/**
 * @internal
 */
static void SDL_Libretro_MenuLoadStateClicked(SDL_LibretroMenu* menu, void* userdata) {
    (void)userdata;
    char savePath[SDL_LIBRETRO_MAX_PATH];
    if (SDL_Libretro_GetSavePath(menu->lr, ".sav", savePath, sizeof(savePath)) > 0 &&
        SDL_Libretro_LoadState(menu->lr, savePath)) {
        SDL_Libretro_SetMenuOpen(menu, false);
    }
    else {
        nk_console_show_message(menu->console, "Load State failed");
    }
}

/**
 * A ready-made menu callback that saves a PNG screenshot of the current
 * frame, reporting the result in the menu.
 *
 * Pass it to SDL_Libretro_AddMenuButton(). The menu's own Screenshot entry
 * uses it too.
 *
 * @param menu the menu the callback fired for. NULL is ignored.
 * @param userdata the destination path as a const char*, or NULL for
 *                 "screenshot.png".
 *
 * @see SDL_Libretro_AddMenuButton()
 */
void SDL_Libretro_MenuScreenshotClicked(SDL_LibretroMenu* menu, void* userdata) {
    if (menu == NULL) {
        return;
    }
    if (SDL_Libretro_SaveScreenshot(menu->lr, (const char*)userdata)) {
        nk_console_show_message(menu->console, "Screenshot saved");
    }
    else {
        SDL_Log("Failed to save screenshot: %s", SDL_GetError());
        nk_console_show_message(menu->console, "Screenshot failed");
    }
}

/**
 * @internal
 */
static void SDL_Libretro_MenuResetClicked(SDL_LibretroMenu* menu, void* userdata) {
    (void)userdata;
    if (SDL_Libretro_Reset(menu->lr)) {
        SDL_Libretro_SetMenuOpen(menu, false);
    }
}

/**
 * @internal
 * Human-readable name for the core's pixel format.
 */
static const char* SDL_Libretro_MenuPixelFormatName(enum retro_pixel_format format) {
    switch (format) {
        case RETRO_PIXEL_FORMAT_0RGB1555:
            return "0RGB1555";
        case RETRO_PIXEL_FORMAT_XRGB8888:
            return "XRGB8888";
        case RETRO_PIXEL_FORMAT_RGB565:
            return "RGB565";
        default:
            return "Unknown";
    }
}

/**
 * @internal
 * Standard submenu header: a back button labeled like the submenu itself.
 */
static void SDL_Libretro_MenuAddBackButton(nk_console* parent, const char* label) {
    nk_console_button_set_symbol(
        nk_console_button_onclick(parent, label, &nk_console_button_back),
        NK_SYMBOL_TRIANGLE_UP);
}

/**
 * @internal
 * Create a submenu button with a symbol and the standard back-button header.
 */
static nk_console* SDL_Libretro_MenuAddSubmenu(nk_console* parent, const char* label, enum nk_symbol_type symbol) {
    nk_console* button = nk_console_button(parent, label);
    nk_console_button_set_symbol(button, symbol);
    SDL_Libretro_MenuAddBackButton(button, label);
    return button;
}

/**
 * @internal
 * Wire a CHANGED handler onto a freshly created widget.
 *
 * @return The widget, for further configuration.
 */
static nk_console* SDL_Libretro_MenuOnChanged(nk_console* widget, nk_console_event callback, void* user_data) {
    nk_console_add_event_handler(widget, NK_CONSOLE_EVENT_CHANGED, callback, user_data, NULL);
    return widget;
}

/**
 * @internal
 * Move the active parent up to root when it sits inside root's subtree, so
 * rebuilding root's children can't leave it pointing at freed widgets.
 */
static void SDL_Libretro_MenuGuardActiveParent(SDL_LibretroMenu* menu, nk_console* root) {
    nk_console* active = nk_console_active_parent(menu->console);
    for (nk_console* it = active; it != NULL; it = it->parent) {
        if (it == root) {
            if (active != root) {
                nk_console_set_active_parent(root);
            }
            return;
        }
    }
}

/**
 * @internal
 * One menu entry created through the shared button/checkbox path; the menu's
 * own widgets and the application-added entries both use it.
 *
 * Owns the label copy; freed through the widget's event destructor.
 */
typedef struct SDL_LibretroMenuItem {
    SDL_LibretroMenu* menu;
    SDL_LibretroMenuCallback callback;
    void* userdata;
    char* label;
    nk_bool checked; /**< Backing checkbox state when the caller brings none. */
    bool* value; /**< The app's checkbox value, mirrored from checked; may be NULL. */
} SDL_LibretroMenuItem;

/**
 * @internal
 */
static void SDL_Libretro_MenuItemChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenuItem* item = (SDL_LibretroMenuItem*)user_data;
    if (item->value != NULL) {
        *item->value = item->checked == nk_true;
    }
    if (item->callback != NULL) {
        item->callback(item->menu, item->userdata);
    }
}

/**
 * @internal
 */
static void SDL_Libretro_MenuItemDestroy(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenuItem* item = (SDL_LibretroMenuItem*)user_data;
    SDL_free(item->label);
    SDL_free(item);
}

/**
 * @internal
 */
static SDL_LibretroMenuItem* SDL_Libretro_MenuCreateItem(SDL_LibretroMenu* menu, const char* label, SDL_LibretroMenuCallback callback, void* userdata) {
    SDL_LibretroMenuItem* item = (SDL_LibretroMenuItem*)SDL_calloc(1, sizeof(SDL_LibretroMenuItem));
    if (item == NULL) {
        return NULL;
    }
    item->menu = menu;
    item->callback = callback;
    item->userdata = userdata;
    item->label = SDL_strdup(label);
    if (item->label == NULL) {
        SDL_free(item);
        return NULL;
    }
    return item;
}

/**
 * @internal
 * Create a button under parent whose CLICKED handler is an
 * SDL_LibretroMenuCallback. Backs the menu's own entries and
 * SDL_Libretro_AddMenuButton().
 *
 * @return The widget, or NULL on allocation failure.
 */
static nk_console* SDL_Libretro_MenuAddButton(SDL_LibretroMenu* menu, nk_console* parent, const char* label, enum nk_symbol_type symbol, SDL_LibretroMenuCallback callback, void* userdata) {
    SDL_LibretroMenuItem* item = SDL_Libretro_MenuCreateItem(menu, label, callback, userdata);
    if (item == NULL) {
        return NULL;
    }
    nk_console* button = nk_console_button_onclick_handler(parent, item->label, &SDL_Libretro_MenuItemChanged, item, &SDL_Libretro_MenuItemDestroy);
    if (button == NULL) {
        SDL_Libretro_MenuItemDestroy(NULL, item);
        return NULL;
    }
    nk_console_button_set_symbol(button, symbol);
    return button;
}

/**
 * @internal
 * Create a checkbox under parent whose CHANGED handler is an
 * SDL_LibretroMenuCallback. checked is the widget's live storage, or NULL to
 * use the item's own, mirrored into the optional value bool. Backs the
 * menu's own entries and SDL_Libretro_AddMenuCheckbox().
 *
 * @return The widget, or NULL on allocation failure.
 */
static nk_console* SDL_Libretro_MenuAddCheckbox(SDL_LibretroMenu* menu, nk_console* parent, const char* label, nk_bool* checked, bool* value, SDL_LibretroMenuCallback callback, void* userdata) {
    SDL_LibretroMenuItem* item = SDL_Libretro_MenuCreateItem(menu, label, callback, userdata);
    if (item == NULL) {
        return NULL;
    }
    nk_bool* storage = checked != NULL ? checked : &item->checked;
    item->value = value;
    if (value != NULL) {
        *storage = (nk_bool)*value;
    }
    nk_console* checkbox = nk_console_checkbox(parent, item->label, storage);
    if (checkbox == NULL) {
        SDL_Libretro_MenuItemDestroy(NULL, item);
        return NULL;
    }
    nk_console_add_event_handler(checkbox, NK_CONSOLE_EVENT_CHANGED, &SDL_Libretro_MenuItemChanged, item, &SDL_Libretro_MenuItemDestroy);
    return checkbox;
}

/**
 * @internal
 * Append a formatted line to the About text arena and add a label for it.
 * Lines that no longer fit are dropped.
 */
static void SDL_Libretro_MenuAboutLine(SDL_LibretroMenu* menu, const char* format, ...) {
    char* line = menu->aboutText + menu->aboutTextLength;
    size_t available = sizeof(menu->aboutText) - menu->aboutTextLength;
    if (available < 2) {
        return;
    }
    va_list args;
    va_start(args, format);
    int written = SDL_vsnprintf(line, available, format, args);
    va_end(args);
    if (written < 0) {
        return;
    }
    if ((size_t)written >= available) {
        written = (int)available - 1;
    }
    menu->aboutTextLength += (size_t)written + 1;
    nk_console_label(menu->aboutButton, line);
}

/**
 * @internal
 * Rebuild the About page from the current frontend, core and content state,
 * adding only the lines that apply.
 */
static void SDL_Libretro_MenuBuildAbout(SDL_LibretroMenu* menu) {
    SDL_Libretro* lr = menu->lr;

    nk_console_free_children(menu->aboutButton);
    menu->aboutTextLength = 0;
    SDL_Libretro_MenuAddBackButton(menu->aboutButton, "About");

    // Frontend
    SDL_Libretro_MenuAboutLine(menu, "SDL_libretro %d.%d.%d",
        SDL_LIBRETRO_MAJOR_VERSION, SDL_LIBRETRO_MINOR_VERSION, SDL_LIBRETRO_MICRO_VERSION);
    int sdlVersion = SDL_GetVersion();
    SDL_Libretro_MenuAboutLine(menu, "SDL %d.%d.%d %s",
        SDL_VERSIONNUM_MAJOR(sdlVersion), SDL_VERSIONNUM_MINOR(sdlVersion), SDL_VERSIONNUM_MICRO(sdlVersion),
        SDL_GetRevision());
    const char* rendererName = SDL_GetRendererName(lr->renderer);
    SDL_Libretro_MenuAboutLine(menu, "Renderer: %s", rendererName != NULL ? rendererName : "Unknown");

    // Core
    if (SDL_Libretro_IsCoreReady(lr)) {
        SDL_Libretro_MenuAboutLine(menu, "Core: %s", SDL_Libretro_GetCoreName(lr));
        const char* coreVersion = SDL_Libretro_GetCoreVersion(lr);
        if (coreVersion[0] != '\0') {
            SDL_Libretro_MenuAboutLine(menu, "Core Version: %s", coreVersion);
        }
        const char* extensions = SDL_Libretro_GetValidExtensions(lr);
        SDL_Libretro_MenuAboutLine(menu, "Extensions: %s", extensions[0] != '\0' ? extensions : "(any)");
        SDL_Libretro_MenuAboutLine(menu, "Pixel Format: %s", SDL_Libretro_MenuPixelFormatName(lr->core.pixelFormat));
        if (lr->core.sampleRate > 0.0) {
            SDL_Libretro_MenuAboutLine(menu, "Sample Rate: %g Hz", lr->core.sampleRate);
        }
    }

    // Content
    if (SDL_Libretro_IsGameReady(lr)) {
        char contentName[128] = "";
        SDL_Libretro_GetFileName(contentName, sizeof(contentName), lr->core.contentPath, true);
        SDL_Libretro_MenuAboutLine(menu, "Content: %s", contentName[0] != '\0' ? contentName : "(none)");
        int width = 0;
        int height = 0;
        SDL_Libretro_GetSize(lr, &width, &height);
        SDL_Libretro_MenuAboutLine(menu, "Size: %dx%d @ %.2f FPS", width, height, SDL_Libretro_GetFPS(lr));
        int rotation = SDL_Libretro_GetRotation(lr);
        if (rotation != 0) {
            SDL_Libretro_MenuAboutLine(menu, "Aspect Ratio: %.3f (rotated %d)",
                SDL_Libretro_GetAspectRatio(lr), rotation * 90);
        }
        else {
            SDL_Libretro_MenuAboutLine(menu, "Aspect Ratio: %.3f", SDL_Libretro_GetAspectRatio(lr));
        }
    }

    nk_console_label(menu->aboutButton, "https://github.com/RobLoach/SDL_libretro");
}

/**
 * @internal
 * Rebuilds the About page and enters it. A CLICKED handler suppresses the
 * button's default submenu navigation, so the handler navigates itself.
 */
static void SDL_Libretro_MenuAboutOpened(SDL_LibretroMenu* menu, void* userdata) {
    (void)userdata;
    SDL_Libretro_MenuBuildAbout(menu);
    nk_console_set_active_parent(menu->aboutButton);
}

/**
 * @internal
 */
static void SDL_Libretro_MenuQuitClicked(SDL_LibretroMenu* menu, void* userdata) {
    (void)menu;
    (void)userdata;
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
    SDL_Libretro_SetScaleMode(menu->lr, menu->filterIndex == 1 ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
}

/**
 * @internal
 */
static void SDL_Libretro_MenuFullscreenChanged(SDL_LibretroMenu* menu, void* userdata) {
    (void)userdata;
    SDL_SetWindowFullscreen(menu->lr->window, menu->fullscreenChecked == nk_true);
    menu->settingsDirty = true;
}

/**
 * @internal
 */
static void SDL_Libretro_MenuVSyncChanged(SDL_LibretroMenu* menu, void* userdata) {
    (void)userdata;
    SDL_SetRenderVSync(menu->lr->renderer, menu->vsyncChecked == nk_true ? 1 : 0);
    menu->settingsDirty = true;
}

/**
 * @internal
 */
static void SDL_Libretro_MenuRewindChanged(SDL_LibretroMenu* menu, void* userdata) {
    (void)userdata;
    // Keep the capacity and interval the app configured.
    SDL_Libretro_SetRewindEnabled(menu->lr, menu->rewindChecked == nk_true,
                                  menu->lr->rewindCapacity, menu->lr->rewindCaptureInterval);
}

/**
 * @internal
 */
static void SDL_Libretro_MenuMousePortChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_Libretro_SetMousePort(menu->lr, (unsigned)menu->mousePortIndex);
}

/**
 * @internal
 */
static void SDL_Libretro_MenuLanguageChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_Libretro_SetLanguage(menu->lr, menu->languageIndex - 1);
}

/**
 * @internal
 */
static void SDL_Libretro_MenuRewindBufferChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_Libretro_SetRewindMemoryLimit(menu->lr, (size_t)menu->rewindBufferMB << 20);
}

/**
 * @internal
 * Mute drops the volume to zero and restores the previous level on unmute.
 */
static void SDL_Libretro_MenuMuteChanged(SDL_LibretroMenu* menu, void* userdata) {
    (void)userdata;
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
 * @internal
 * Special keys that map between SDL scancodes and NK_CONSOLE_KEY_* runes.
 *
 * Aliases (keypad enter, right-hand modifiers) come after their primary, so
 * rune-to-scancode lookups resolve to the primary. Printable keys are
 * handled by the ASCII fallbacks in the lookups below.
 */
static const struct { SDL_Scancode scancode; nk_rune rune; } SDL_Libretro_MenuKeyRunes[] = {
    { SDL_SCANCODE_RETURN, NK_CONSOLE_KEY_ENTER },
    { SDL_SCANCODE_KP_ENTER, NK_CONSOLE_KEY_ENTER },
    { SDL_SCANCODE_TAB, NK_CONSOLE_KEY_TAB },
    { SDL_SCANCODE_BACKSPACE, NK_CONSOLE_KEY_BACKSPACE },
    { SDL_SCANCODE_ESCAPE, NK_CONSOLE_KEY_ESCAPE },
    { SDL_SCANCODE_DELETE, NK_CONSOLE_KEY_DELETE },
    { SDL_SCANCODE_UP, NK_CONSOLE_KEY_UP },
    { SDL_SCANCODE_DOWN, NK_CONSOLE_KEY_DOWN },
    { SDL_SCANCODE_LEFT, NK_CONSOLE_KEY_LEFT },
    { SDL_SCANCODE_RIGHT, NK_CONSOLE_KEY_RIGHT },
    { SDL_SCANCODE_LSHIFT, NK_CONSOLE_KEY_SHIFT },
    { SDL_SCANCODE_RSHIFT, NK_CONSOLE_KEY_SHIFT },
    { SDL_SCANCODE_LCTRL, NK_CONSOLE_KEY_CTRL },
    { SDL_SCANCODE_RCTRL, NK_CONSOLE_KEY_CTRL },
    { SDL_SCANCODE_LALT, NK_CONSOLE_KEY_ALT },
    { SDL_SCANCODE_RALT, NK_CONSOLE_KEY_ALT },
    { SDL_SCANCODE_F1, NK_CONSOLE_KEY_F1 },
    { SDL_SCANCODE_F2, NK_CONSOLE_KEY_F2 },
    { SDL_SCANCODE_F3, NK_CONSOLE_KEY_F3 },
    { SDL_SCANCODE_F4, NK_CONSOLE_KEY_F4 },
    { SDL_SCANCODE_F5, NK_CONSOLE_KEY_F5 },
    { SDL_SCANCODE_F6, NK_CONSOLE_KEY_F6 },
    { SDL_SCANCODE_F7, NK_CONSOLE_KEY_F7 },
    { SDL_SCANCODE_F8, NK_CONSOLE_KEY_F8 },
    { SDL_SCANCODE_F9, NK_CONSOLE_KEY_F9 },
    { SDL_SCANCODE_F10, NK_CONSOLE_KEY_F10 },
    { SDL_SCANCODE_F11, NK_CONSOLE_KEY_F11 },
    { SDL_SCANCODE_F12, NK_CONSOLE_KEY_F12 },
};

/**
 * @internal
 * The NK_CONSOLE_KEY_* rune for an SDL scancode, to show the current binding
 * in a key-capture widget. NK_CONSOLE_KEY_NONE when the key has no rune; it
 * can still be rebound, just not displayed.
 */
static nk_rune SDL_Libretro_MenuRuneFromScancode(SDL_Scancode scancode) {
    for (size_t i = 0; i < SDL_arraysize(SDL_Libretro_MenuKeyRunes); i++) {
        if (SDL_Libretro_MenuKeyRunes[i].scancode == scancode) {
            return SDL_Libretro_MenuKeyRunes[i].rune;
        }
    }
    SDL_Keycode key = SDL_GetKeyFromScancode(scancode, SDL_KMOD_NONE, false);
    if (key >= 32 && key < 127) {
        return (nk_rune)key;
    }
    return NK_CONSOLE_KEY_NONE;
}

/**
 * @internal
 * The SDL scancode for a captured NK_CONSOLE_KEY_* rune, or
 * SDL_SCANCODE_UNKNOWN when it has no equivalent.
 */
static SDL_Scancode SDL_Libretro_MenuScancodeFromRune(nk_rune rune) {
    for (size_t i = 0; i < SDL_arraysize(SDL_Libretro_MenuKeyRunes); i++) {
        if (SDL_Libretro_MenuKeyRunes[i].rune == rune) {
            return SDL_Libretro_MenuKeyRunes[i].scancode;
        }
    }
    if (rune >= 'A' && rune <= 'Z') {
        rune += 'a' - 'A';
    }
    if (rune >= 32 && rune < 127) {
        return SDL_GetScancodeFromKey((SDL_Keycode)rune, NULL);
    }
    return SDL_SCANCODE_UNKNOWN;
}

/**
 * @internal
 * Refresh the Keyboard page's captured runes from the live mapping.
 */
static void SDL_Libretro_MenuSyncKeyBinds(SDL_LibretroMenu* menu) {
    for (int i = 0; i < SDL_LIBRETRO_MAX_JOYPAD_BUTTONS; i++) {
        menu->keyBinds[i].rune = SDL_Libretro_MenuRuneFromScancode(menu->lr->keyboardPlayer1[i]);
    }
}

/**
 * @internal
 * Apply a captured key to the player 1 keyboard mapping. Captures without an
 * SDL equivalent (including a timed-out prompt) revert to the current
 * binding.
 */
static void SDL_Libretro_MenuKeyBindChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenuKeyBind* bind = (SDL_LibretroMenuKeyBind*)user_data;
    SDL_LibretroMenu* menu = bind->menu;
    SDL_Scancode scancode = SDL_Libretro_MenuScancodeFromRune(bind->rune);
    if (scancode == SDL_SCANCODE_UNKNOWN) {
        bind->rune = SDL_Libretro_MenuRuneFromScancode(menu->lr->keyboardPlayer1[bind->button]);
        return;
    }
    SDL_Libretro_SetKeyboardMapping(menu->lr, bind->button, scancode);
}

/**
 * @internal
 * The nk_gamepad button whose SDL equivalent is the given button, for
 * showing the current binding in a capture widget. NK_GAMEPAD_BUTTON_INVALID
 * when nk_gamepad has no equivalent (e.g. the trigger paddle defaults).
 */
static enum nk_gamepad_button SDL_Libretro_MenuNkButtonFromSDL(SDL_GamepadButton button) {
    for (int i = NK_GAMEPAD_BUTTON_FIRST; i <= NK_GAMEPAD_BUTTON_R3; i++) {
        if (nk_gamepad_sdl3_map_button(i) == button) {
            return (enum nk_gamepad_button)i;
        }
    }
    return NK_GAMEPAD_BUTTON_INVALID;
}

/**
 * @internal
 * Refresh every Gamepad page's captured buttons and analog-to-digital
 * choices from the live per-port state.
 */
static void SDL_Libretro_MenuSyncPadBinds(SDL_LibretroMenu* menu) {
    for (unsigned port = 0; port < SDL_LIBRETRO_MAX_USERS; port++) {
        SDL_LibretroMenuPadPort* padPort = &menu->padPorts[port];
        padPort->analogIndex = (int)menu->lr->analogToDigital[port];
        for (int i = 0; i < SDL_LIBRETRO_MAX_JOYPAD_BUTTONS; i++) {
            padPort->binds[i].captured = SDL_Libretro_MenuNkButtonFromSDL(menu->lr->gamepadButtons[port][i]);
        }
    }
}

/**
 * @internal
 * Apply a captured gamepad button to its port's mapping; captures without an
 * SDL equivalent (including a timed-out prompt) revert to the current
 * binding.
 */
static void SDL_Libretro_MenuPadBindChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenuPadBind* bind = (SDL_LibretroMenuPadBind*)user_data;
    SDL_LibretroMenu* menu = bind->menu;
    SDL_GamepadButton button = nk_gamepad_sdl3_map_button((int)bind->captured);
    if (button == SDL_GAMEPAD_BUTTON_INVALID) {
        bind->captured = SDL_Libretro_MenuNkButtonFromSDL(menu->lr->gamepadButtons[bind->port][bind->button]);
        return;
    }
    SDL_Libretro_SetGamepadMapping(menu->lr, bind->port, bind->button, button);
}

/**
 * @internal
 * Apply the Analog to Digital combobox choice to its port.
 */
static void SDL_Libretro_MenuAnalogToDigitalChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenuPadPort* padPort = (SDL_LibretroMenuPadPort*)user_data;
    SDL_Libretro_SetAnalogToDigital(padPort->menu->lr, padPort->port, (SDL_LibretroAnalogToDigital)padPort->analogIndex);
}

/**
 * @internal
 * Populate the "Gamepad" submenu with one page per player, titled with the
 * connected gamepad's name, holding that port's button bindings and its
 * Analog to Digital stick choice. Rebuilt when the menu opens so the names
 * stay current.
 */
static void SDL_Libretro_MenuBuildGamepads(SDL_LibretroMenu* menu) {
    if (menu->gamepadButton == NULL) {
        return;
    }
    SDL_Libretro* lr = menu->lr;

    SDL_Libretro_MenuGuardActiveParent(menu, menu->gamepadButton);
    nk_console_free_children(menu->gamepadButton);
    SDL_Libretro_MenuAddBackButton(menu->gamepadButton, "Gamepad");

    SDL_Libretro_MenuSyncPadBinds(menu);
    for (unsigned port = 0; port < SDL_LIBRETRO_MAX_USERS; port++) {
        SDL_LibretroMenuPadPort* padPort = &menu->padPorts[port];
        padPort->menu = menu;
        padPort->port = port;

        const char* name = lr->gamepads[port] != NULL ? SDL_GetGamepadName(lr->gamepads[port]) : NULL;
        SDL_snprintf(padPort->label, sizeof(padPort->label), "%u. %s", port + 1, name != NULL ? name : "No Gamepad");
        nk_console* player = SDL_Libretro_MenuAddSubmenu(menu->gamepadButton, padPort->label, NK_SYMBOL_TRIANGLE_RIGHT);
        nk_console_set_tooltip(player, "Gamepad buttons for this player's retro controller; the triggers stay analog");

        nk_console_set_tooltip(
            SDL_Libretro_MenuOnChanged(nk_console_combobox(player, "Analog to Digital", "None|Left|Right", '|', &padPort->analogIndex),
                                       &SDL_Libretro_MenuAnalogToDigitalChanged, padPort),
            "The chosen analog stick also presses the D-Pad directions");

        for (int i = 0; i < SDL_LIBRETRO_MAX_JOYPAD_BUTTONS; i++) {
            padPort->binds[i].menu = menu;
            padPort->binds[i].port = port;
            padPort->binds[i].button = i;
            SDL_Libretro_MenuOnChanged(
                nk_console_input_gamepad(player, SDL_Libretro_JoypadButtonNames[i], -1, NULL, &padPort->binds[i].captured),
                &SDL_Libretro_MenuPadBindChanged, &padPort->binds[i]);
        }
    }
}

/**
 * @internal
 * Read the menu's own persisted state from the config, applying it to the
 * window, renderer and volume before the widgets snapshot them.
 *
 * The counterpart of SDL_Libretro_MenuSaveState(); menu-owned config keys
 * live in these two functions only.
 */
static void SDL_Libretro_MenuLoadState(SDL_LibretroMenu* menu) {
    SDL_Libretro* lr = menu->lr;
    SDL_LibretroMenuStyle style = SDL_LIBRETRO_MENU_DEFAULT_STYLE;
    // Default the UI scale to 2x; a saved choice (including an explicit Auto)
    // overrides it below.
    menu->uiScaleIndex = 2;
    if (lr->ini != NULL) {
        Sint64 savedStyle = INI_GetInt(lr->ini, NULL, "menutheme", (Sint64)style);
        if (savedStyle >= 0 && savedStyle < (Sint64)SDL_LIBRETRO_MENU_STYLE_COUNT) {
            style = (SDL_LibretroMenuStyle)savedStyle;
        }
        if (INI_HasValue(lr->ini, NULL, "menuuiscale")) {
            Sint64 savedScale = INI_GetInt(lr->ini, NULL, "menuuiscale", 2);
            if (savedScale >= 0 && savedScale <= 4) {
                menu->uiScaleIndex = (int)savedScale;
            }
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
 * @internal
 * Write every menu-owned config value; the file itself is saved when the
 * context closes.
 *
 * One choke point, flushed on settingsDirty, so the change handlers stay
 * free of persistence code.
 */
static void SDL_Libretro_MenuSaveState(SDL_LibretroMenu* menu) {
    SDL_Libretro* lr = menu->lr;
    menu->settingsDirty = false;
    if (lr->ini == NULL) {
        return;
    }
    INI_SetInt(lr->ini, NULL, "menutheme", (Sint64)menu->style);
    INI_SetInt(lr->ini, NULL, "menuuiscale", (Sint64)menu->uiScaleIndex);
    INI_SetBoolean(lr->ini, NULL, "menufullscreen", menu->fullscreenChecked == nk_true);
    INI_SetBoolean(lr->ini, NULL, "menuvsync", menu->vsyncChecked == nk_true);
    INI_SetBoolean(lr->ini, NULL, "menumuted", menu->muteChecked == nk_true);
    INI_SetFloat(lr->ini, NULL, "menumutevolume", menu->preMuteVolume);
}

/**
 * @internal
 * Whether the value reads as the "on" half of a boolean core option.
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
 * @internal
 * Whether the option is exactly an on/off pair, in either order.
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
 * @internal
 * Marks the Core Options submenu for a rebuild when the change flipped any
 * option visibility, consuming the dirty flag our own write raised.
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
static void SDL_Libretro_MenuOptionCheckboxChanged(SDL_LibretroMenu* menu, void* userdata) {
    SDL_LibretroMenuOptionState* state = (SDL_LibretroMenuOptionState*)userdata;
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
static void SDL_Libretro_MenuResetOptionsClicked(SDL_LibretroMenu* menu, void* userdata) {
    (void)userdata;
    SDL_Libretro_ResetAllOptions(menu->lr);
    SDL_Libretro_MenuOptionWritten(menu);
    menu->optionsStale = true;
    nk_console_show_message(menu->console, "Core options reset to defaults");
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
 * @internal
 * Join count items into a freshly allocated "a|b|c" string for a combobox.
 *
 * @param count the number of items.
 * @param item Returns the text for an index; must never return NULL.
 * @param userdata passed to item.
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
 * @internal
 * Whether the option should appear in the Core Options submenu.
 */
static bool SDL_Libretro_MenuOptionShown(const SDL_LibretroOption* option) {
    return option != NULL && option->visible && option->valuesCount > 0;
}

/**
 * @internal
 * The display text for one core option value: its label, or the raw value.
 *
 * @see SDL_Libretro_MenuJoinPipeList()
 */
static const char* SDL_Libretro_MenuOptionValueText(void* userdata, unsigned index) {
    const SDL_LibretroOption* option = (const SDL_LibretroOption*)userdata;
    const char* label = option->values[index].label;
    return (label != NULL && label[0] != '\0') ? label : option->values[index].value;
}

/**
 * @internal
 * Build the checkbox/combobox widget for the given option under parent.
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
        widget = SDL_Libretro_MenuAddCheckbox(menu, parent, label, &state->checked, NULL,
                                              &SDL_Libretro_MenuOptionCheckboxChanged, state);
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

        widget = SDL_Libretro_MenuOnChanged(nk_console_combobox(parent, label, state->displayList, '|', &state->selected),
                                            &SDL_Libretro_MenuOptionChanged, state);
    }

    if (option->info != NULL && option->info[0] != '\0') {
        nk_console_set_tooltip(widget, option->info);
    }
}

/**
 * @internal
 * Category index of the option, or -1 when uncategorized (or the category was
 * never registered, in which case the option is treated as flat).
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
 * @internal
 * Populate the "Core Options" submenu from the loaded core's options.
 */
static void SDL_Libretro_MenuBuildOptions(SDL_LibretroMenu* menu) {
    if (menu->optionsButton == NULL) {
        return;
    }
    SDL_Libretro* lr = menu->lr;

    // Drop the previous widgets before the option states they point into.
    SDL_Libretro_MenuGuardActiveParent(menu, menu->optionsButton);
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
        // The page is going away; don't strand the user on it with nothing
        // left to navigate, not even a way back.
        if (nk_console_active_parent(menu->console) == menu->optionsButton) {
            nk_console_set_active_parent(menu->optionsButton->parent);
        }
        menu->optionsButton->visible = nk_false;
        return;
    }
    menu->optionsButton->visible = nk_true;

    menu->optionStates = (SDL_LibretroMenuOptionState*)SDL_calloc(count, sizeof(SDL_LibretroMenuOptionState));
    if (menu->optionStates == NULL) {
        if (nk_console_active_parent(menu->console) == menu->optionsButton) {
            nk_console_set_active_parent(menu->optionsButton->parent);
        }
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
        nk_console* categoryButton = SDL_Libretro_MenuAddSubmenu(menu->optionsButton, categoryLabel, NK_SYMBOL_TRIANGLE_RIGHT);
        if (category->info != NULL && category->info[0] != '\0') {
            nk_console_set_tooltip(categoryButton, category->info);
        }

        for (unsigned i = 0; i < count; i++) {
            const SDL_LibretroOption* option = SDL_Libretro_GetOptionByIndex(lr, i);
            if (SDL_Libretro_MenuOptionShown(option) && SDL_Libretro_MenuOptionCategory(lr, option) == (int)c) {
                SDL_Libretro_MenuBuildOptionWidget(menu, categoryButton, i);
            }
        }
    }

    SDL_Libretro_MenuAddButton(menu, menu->optionsButton, "Reset to Defaults", NK_SYMBOL_TRIANGLE_LEFT, &SDL_Libretro_MenuResetOptionsClicked, NULL);
}

#ifndef __EMSCRIPTEN__
/**
 * @internal
 * Append each extension from a pipe-separated list (e.g. "gb|gbc") to a
 * semicolon-separated file filter (e.g. ".gb;.GB;.gbc;.GBC"), deduplicated.
 *
 * The filter matching is case-sensitive, so both lower and upper case
 * variants are added.
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
 * @internal
 * Restrict the Load Game file browser to content the scanned cores (and the
 * loaded core) can open.
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
 * Refresh the Settings textedit buffers from the current context values.
 */
static void SDL_Libretro_MenuSyncSettingsBuffers(SDL_LibretroMenu* menu) {
    SDL_Libretro* lr = menu->lr;
    const char* value = SDL_Libretro_GetCoreDirectory(lr);
    SDL_strlcpy(menu->coreDirBuffer, value != NULL ? value : "", sizeof(menu->coreDirBuffer));
    value = SDL_Libretro_GetSaveDirectory(lr);
    SDL_strlcpy(menu->saveDirBuffer, value != NULL ? value : "", sizeof(menu->saveDirBuffer));
    value = SDL_Libretro_GetSystemDirectory(lr);
    SDL_strlcpy(menu->systemDirBuffer, value != NULL ? value : "", sizeof(menu->systemDirBuffer));
    SDL_strlcpy(menu->browseDirBuffer, lr->fileBrowserStartDirectory, sizeof(menu->browseDirBuffer));
    value = SDL_Libretro_GetUsername(lr);
    SDL_strlcpy(menu->usernameBuffer, value != NULL ? value : "", sizeof(menu->usernameBuffer));
}

/**
 * @internal
 * Consolidated callback for all Settings textedits (directories + username).
 *
 * Applies every settings buffer at once, so a single handler serves all the
 * textedits without per-field state.
 */
static void SDL_Libretro_MenuSettingChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_Libretro_SetCoreDirectory(menu->lr, menu->coreDirBuffer);
    SDL_Libretro_MenuUpdateLoadGameFilter(menu);
    SDL_Libretro_SetSaveDirectory(menu->lr, menu->saveDirBuffer);
    SDL_Libretro_SetSystemDirectory(menu->lr, menu->systemDirBuffer);
    SDL_strlcpy(menu->lr->fileBrowserStartDirectory, menu->browseDirBuffer, sizeof(menu->lr->fileBrowserStartDirectory));
#ifndef __EMSCRIPTEN__
    if (menu->loadGameButton != NULL && menu->browseDirBuffer[0] != '\0') {
        nk_console_file_set_directory(menu->loadGameButton, menu->browseDirBuffer);
    }
#endif
    SDL_Libretro_SetUsername(menu->lr, menu->usernameBuffer);
}

/**
 * @internal
 * Create a textedit widget under @p parent and wire it to the consolidated
 * Settings callback.
 */
static nk_console* SDL_Libretro_MenuAddSettingTextedit(nk_console* parent, const char* label,
                                                       SDL_LibretroMenu* menu,
                                                       char* buffer, size_t bufferSize) {
    return SDL_Libretro_MenuOnChanged(nk_console_textedit(parent, label, buffer, bufferSize),
                                      &SDL_Libretro_MenuSettingChanged, menu);
}

/**
 * @internal
 * Create a directory-picker widget under @p parent and wire it to the
 * consolidated Settings callback.
 */
static nk_console* SDL_Libretro_MenuAddSettingDir(nk_console* parent, const char* label,
                                                  SDL_LibretroMenu* menu,
                                                  char* buffer, size_t bufferSize) {
#ifdef __EMSCRIPTEN__
    // The web build's virtual file system isn't worth browsing.
    return SDL_Libretro_MenuAddSettingTextedit(parent, label, menu, buffer, bufferSize);
#else
    // nk_console_dir (not dir_action) keeps the label in its own column, so
    // the directory type stays visible next to the picked path.
    return SDL_Libretro_MenuOnChanged(nk_console_dir(parent, label, buffer, (int)bufferSize),
                                      &SDL_Libretro_MenuSettingChanged, menu);
#endif
}

/**
 * @internal
 */
static void SDL_Libretro_MenuFreePortStates(SDL_LibretroMenu* menu) {
    for (unsigned i = 0; i < SDL_LIBRETRO_MAX_USERS; i++) {
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
 * @internal
 * Populate the "Controllers" submenu with a device combobox per port that
 * offers more than one controller type.
 */
static void SDL_Libretro_MenuBuildControllers(SDL_LibretroMenu* menu) {
    if (menu->controllersButton == NULL) {
        return;
    }
    SDL_Libretro* lr = menu->lr;

    // Drop the previous widgets before the port states they point into.
    SDL_Libretro_MenuGuardActiveParent(menu, menu->controllersButton);
    nk_console_free_children(menu->controllersButton);
    SDL_Libretro_MenuFreePortStates(menu);

    SDL_Libretro_MenuAddBackButton(menu->controllersButton, "Controllers");

    unsigned count = lr->core.controllerInfoCount;
    if (count > SDL_LIBRETRO_MAX_USERS) {
        count = SDL_LIBRETRO_MAX_USERS;
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
        SDL_Libretro_MenuOnChanged(nk_console_combobox(menu->controllersButton, state->label, state->deviceList, '|', &state->selected),
                                   &SDL_Libretro_MenuPortDeviceChanged, state);
        anyWidget = true;
    }

    menu->controllersButton->visible = (nk_bool)anyWidget;
}

/**
 * @internal
 * A menu color theme, expressed as the distinct colors the themes vary on.
 *
 * SDL_Libretro_MenuApplyPalette() fans these out over the full Nuklear color
 * table, so adding a theme is ~20 colors instead of ~30 table assignments.
 */
typedef struct SDL_LibretroMenuPalette {
    struct nk_color text;
    struct nk_color window; /**< Window background. */
    struct nk_color frame; /**< Header and border. */
    struct nk_color widget; /**< Buttons, selects, properties, edits, combos, charts, tab headers. */
    struct nk_color widgetHover;
    struct nk_color widgetActive;
    struct nk_color selectActive;
    struct nk_color toggle; /**< Checkbox/toggle track. */
    struct nk_color toggleHover;
    struct nk_color toggleCursor;
    struct nk_color slider; /**< Slider and knob track. */
    struct nk_color sliderCursor;
    struct nk_color sliderCursorHover;
    struct nk_color sliderCursorActive;
    struct nk_color editCursor;
    struct nk_color chartColor;
    struct nk_color chartHighlight;
    struct nk_color scrollbar; /**< Scrollbar track. */
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
 * @internal
 * The palette backing a built-in style, or NULL for the Nuklear default.
 */
static const SDL_LibretroMenuPalette* SDL_Libretro_MenuGetPalette(SDL_LibretroMenuStyle style) {
    static const SDL_LibretroMenuPalette* const palettes[SDL_LIBRETRO_MENU_STYLE_COUNT] = {
        &SDL_Libretro_menuPaletteMocha,
        &SDL_Libretro_menuPaletteLatte,
        &SDL_Libretro_menuPaletteFrappe,
        &SDL_Libretro_menuPaletteMacchiato,
        &SDL_Libretro_menuPaletteDracula,
        NULL, // SDL_LIBRETRO_MENU_STYLE_DARK is the Nuklear default.
    };
    return ((int)style >= 0 && style < SDL_LIBRETRO_MENU_STYLE_COUNT) ? palettes[style] : NULL;
}

/**
 * @internal
 * The automatic UI scale step for a window of the given logical width,
 * following raylib-libretro's resolution thresholds adapted to the 13px base
 * font.
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
 * @internal
 * The UI render scale for the current window: the UI scale (manual override
 * or resolution-based step) times the window's display scale.
 */
static float SDL_Libretro_MenuRenderScale(SDL_LibretroMenu* menu) {
    float displayScale = SDL_GetWindowDisplayScale(menu->lr->window);
    int scale = (displayScale > 0.0f) ? (int)(displayScale + 0.5f) : 1;
    if (scale < 1) {
        scale = 1;
    }
    int step;
    if (menu->uiScaleIndex > 0) {
        step = menu->uiScaleIndex;
    }
    else {
        int width = 0;
        int height = 0;
        SDL_GetWindowSize(menu->lr->window, &width, &height);
        step = (int)SDL_Libretro_MenuAutoScale(width);
    }
    return (float)(step * scale);
}

/**
 * @internal
 * Bakes the default font at the base pixel height and makes it the active
 * style font. The font is baked once at SDL_LIBRETRO_MENU_FONT_HEIGHT;
 * scaling is handled by SDL_SetRenderScale in the render path.
 */
static void SDL_Libretro_MenuBakeFont(SDL_LibretroMenu* menu) {
    if (menu->atlas != NULL) {
        nk_font_atlas_clear(menu->atlas);
        menu->atlas = NULL;
    }
    struct nk_sdl* sdl = (struct nk_sdl*)menu->ctx->userdata.ptr;
    struct nk_font_config fontConfig = nk_font_config(0);
    fontConfig.oversample_h = 1;
    fontConfig.oversample_v = 1;
    fontConfig.pixel_snap = nk_true;
    menu->atlas = nk_sdl_font_stash_begin(menu->ctx);
    struct nk_font* font = nk_font_atlas_add_default(menu->atlas, (float)SDL_LIBRETRO_MENU_FONT_HEIGHT, &fontConfig);

    int atlasWidth = 0, atlasHeight = 0;
    const void* fontImage = nk_font_atlas_bake(menu->atlas, &atlasWidth, &atlasHeight, NK_FONT_ATLAS_RGBA32);
    if (fontImage != NULL) {
        Uint8* texels = (Uint8*)fontImage;
        for (int i = 3; i < atlasWidth * atlasHeight * 4; i += 4) {
            texels[i] = texels[i] >= 128 ? 255 : 0;
        }
    }
    nk_sdl_device_upload_atlas(menu->ctx, fontImage, atlasWidth, atlasHeight);
    nk_font_atlas_end(menu->atlas, nk_handle_ptr(sdl->ogl.font_tex), &sdl->ogl.tex_null);
    if (sdl->ogl.font_tex != NULL) {
        SDL_SetTextureScaleMode(sdl->ogl.font_tex, SDL_SCALEMODE_NEAREST);
    }
    if (font != NULL) {
        nk_style_set_font(menu->ctx, &font->handle);
    }
}

/**
 * @internal
 * Fan the palette out over the Nuklear color table.
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

/**
 * @internal
 */
static void SDL_Libretro_MenuUIScaleChanged(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    menu->settingsDirty = true;
}

/**
 * Applies a color theme to the menu.
 *
 * The choice is saved to the config with the other menu settings, and
 * restored by SDL_Libretro_CreateMenu(). The initial style is
 * SDL_LIBRETRO_MENU_DEFAULT_STYLE.
 *
 * @param menu the menu to style.
 * @param style the theme; must be below SDL_LIBRETRO_MENU_STYLE_COUNT.
 * @returns true on success, false when menu is NULL or style is out of
 *          range.
 *
 * @see SDL_Libretro_GetMenuStyle()
 */
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

/**
 * Gets the menu's active color theme.
 *
 * @param menu the menu to query.
 * @returns the active style, or SDL_LIBRETRO_MENU_DEFAULT_STYLE when menu is
 *          NULL.
 *
 * @see SDL_Libretro_SetMenuStyle()
 */
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
 * @internal
 * Refresh the Settings widget state from the current context values.
 *
 * Runs at creation and every time the menu opens, so changes made outside
 * the menu (a hotkey volume change, an app fullscreen toggle) show up.
 */
static void SDL_Libretro_MenuSyncSettings(SDL_LibretroMenu* menu) {
    SDL_Libretro* lr = menu->lr;
    menu->volumePercent = (int)(SDL_Libretro_GetVolume(lr) * 100.0f + 0.5f);
    menu->fitModeIndex = (int)SDL_Libretro_GetFitMode(lr);
    menu->filterIndex = SDL_Libretro_GetScaleMode(lr) == SDL_SCALEMODE_LINEAR ? 1 : 0;
    menu->fullscreenChecked = (nk_bool)((SDL_GetWindowFlags(lr->window) & SDL_WINDOW_FULLSCREEN) != 0);
    int vsync = 0;
    SDL_GetRenderVSync(lr->renderer, &vsync);
    menu->vsyncChecked = (nk_bool)(vsync != 0);
    menu->rewindChecked = (nk_bool)SDL_Libretro_GetRewindEnabled(lr);
    // Clamped to the widget's range: nk_property clamps the bound value while
    // rendering, and the CHANGED writeback would shrink a larger app budget.
    size_t rewindLimitMB = SDL_Libretro_GetRewindMemoryLimit(lr) >> 20;
    menu->rewindBufferMB = rewindLimitMB > 1024 ? 1024 : (int)rewindLimitMB;
    menu->mousePortIndex = (int)SDL_Libretro_GetMousePort(lr);
    // The raw override, so a locale-detected language still shows as Auto.
    menu->languageIndex = lr->language >= 0 ? lr->language + 1 : 0;
}

/**
 * @internal
 * Swap to the radio-selected disk: eject, change the index, insert.
 */
static void SDL_Libretro_MenuDiskClicked(nk_console* widget, void* user_data) {
    (void)widget;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)user_data;
    SDL_Libretro* lr = menu->lr;
    if (menu->diskSelected < 0 || (unsigned)menu->diskSelected == SDL_Libretro_GetDiskIndex(lr)) {
        return;
    }
    bool changed = false;
    if (SDL_Libretro_EjectDisk(lr)) {
        changed = SDL_Libretro_SetDiskIndex(lr, (unsigned)menu->diskSelected);
        // Close the tray even when the index change failed.
        changed = SDL_Libretro_InsertDisk(lr) && changed;
    }
    if (changed) {
        nk_console_show_message(menu->console, "Disk changed");
    }
    else {
        nk_console_show_message(menu->console, "Disk change failed");
        menu->diskSelected = (int)SDL_Libretro_GetDiskIndex(lr);
    }
}

/**
 * @internal
 */
static void SDL_Libretro_MenuFreeDiskLabels(SDL_LibretroMenu* menu) {
    if (menu->diskLabels == NULL) {
        return;
    }
    for (unsigned i = 0; i < menu->builtDiskCount; i++) {
        SDL_free(menu->diskLabels[i]);
    }
    SDL_free(menu->diskLabels);
    menu->diskLabels = NULL;
}

/**
 * @internal
 * Rebuild the Disks submenu: one radio per disk in the core's disk control,
 * hidden unless the running game has disks to swap.
 */
static void SDL_Libretro_MenuBuildDisks(SDL_LibretroMenu* menu) {
    if (menu->disksButton == NULL) {
        return;
    }
    SDL_Libretro* lr = menu->lr;

    // Drop the previous widgets before the labels they point into.
    SDL_Libretro_MenuGuardActiveParent(menu, menu->disksButton);
    nk_console_free_children(menu->disksButton);
    SDL_Libretro_MenuFreeDiskLabels(menu);

    unsigned count = SDL_Libretro_GetDiskCount(lr);
    menu->builtDiskCount = count;
    if (count < 2) {
        // The page is going away; don't strand the user on an empty parent.
        if (nk_console_active_parent(menu->console) == menu->disksButton) {
            nk_console_set_active_parent(menu->console);
        }
        menu->disksButton->visible = nk_false;
        return;
    }

    menu->diskLabels = (char**)SDL_calloc(count, sizeof(char*));
    if (menu->diskLabels == NULL) {
        menu->disksButton->visible = nk_false;
        return;
    }

    menu->disksButton->visible = nk_true;
    SDL_Libretro_MenuAddBackButton(menu->disksButton, "Disks");
    menu->diskSelected = (int)SDL_Libretro_GetDiskIndex(lr);
    for (unsigned i = 0; i < count; i++) {
        char label[128];
        if (!SDL_Libretro_GetDiskLabel(lr, i, label, sizeof(label)) || label[0] == '\0') {
            SDL_snprintf(label, sizeof(label), "Disk %u", i + 1);
        }
        menu->diskLabels[i] = SDL_strdup(label);
        nk_console* radio = nk_console_radio(menu->disksButton, menu->diskLabels[i] != NULL ? menu->diskLabels[i] : "Disk", &menu->diskSelected);
        // Radios report a pick as CLICKED.
        nk_console_add_event_handler(radio, NK_CONSOLE_EVENT_CLICKED, &SDL_Libretro_MenuDiskClicked, menu, NULL);
    }
}

/**
 * @internal
 * Build the Settings submenu; the widgets write into the fields that
 * SDL_Libretro_MenuSyncSettings() keeps current.
 */
static void SDL_Libretro_MenuBuildSettings(SDL_LibretroMenu* menu) {
    nk_console* settings = SDL_Libretro_MenuAddSubmenu(menu->console, "Settings", NK_SYMBOL_HAMBURGER);

    // General
    nk_console* general = SDL_Libretro_MenuAddSubmenu(settings, "General", NK_SYMBOL_TRIANGLE_RIGHT);
    SDL_Libretro_MenuSyncSettingsBuffers(menu);
    nk_console_set_tooltip(
        SDL_Libretro_MenuAddSettingTextedit(general, "Username", menu,
                                            menu->usernameBuffer, sizeof(menu->usernameBuffer)),
        "Reported to cores that ask for a username");

    // Rewind toggle, with its memory budget below it.
    nk_console_set_tooltip(
        SDL_Libretro_MenuAddCheckbox(menu, general, "Rewind", &menu->rewindChecked, NULL, &SDL_Libretro_MenuRewindChanged, NULL),
        "Capture state snapshots so gameplay can rewind");
    nk_console_set_tooltip(
        SDL_Libretro_MenuOnChanged(nk_console_property_int(general, "Rewind Buffer (MB)", 0, &menu->rewindBufferMB, 1024, 8, 1.0f),
                                   &SDL_Libretro_MenuRewindBufferChanged, menu),
        "Memory the rewind buffer may use; 0 for no limit");
    nk_console_set_tooltip(
        SDL_Libretro_MenuOnChanged(nk_console_combobox(general, "Language", SDL_LIBRETRO_MENU_LANGUAGE_NAMES, '|', &menu->languageIndex),
                                   &SDL_Libretro_MenuLanguageChanged, menu),
        "Reported to cores; applies when the next core loads. Auto follows the system locale");

    // Audio & Video
    nk_console* audioVideo = SDL_Libretro_MenuAddSubmenu(settings, "Audio & Video", NK_SYMBOL_TRIANGLE_RIGHT);

    SDL_Libretro_MenuOnChanged(nk_console_knob_int(audioVideo, "Volume", 0, &menu->volumePercent, 100, 5, 1.0f),
                               &SDL_Libretro_MenuVolumeChanged, menu);
    nk_console_set_tooltip(
        SDL_Libretro_MenuAddCheckbox(menu, audioVideo, "Mute", &menu->muteChecked, NULL, &SDL_Libretro_MenuMuteChanged, NULL),
        "Silence the audio; unmuting restores the volume");
    SDL_Libretro_MenuAddCheckbox(menu, audioVideo, "Fullscreen", &menu->fullscreenChecked, NULL, &SDL_Libretro_MenuFullscreenChanged, NULL);
    SDL_Libretro_MenuAddCheckbox(menu, audioVideo, "VSync", &menu->vsyncChecked, NULL, &SDL_Libretro_MenuVSyncChanged, NULL);
    nk_console_set_tooltip(
        SDL_Libretro_MenuOnChanged(nk_console_combobox(audioVideo, "Filter", "Nearest|Linear", '|', &menu->filterIndex),
                                   &SDL_Libretro_MenuFilterChanged, menu),
        "Nearest keeps pixels sharp; Linear smooths the scaling");
    nk_console_set_tooltip(
        SDL_Libretro_MenuOnChanged(nk_console_combobox(audioVideo, "Fit Mode", "Aspect|Integer|Stretch", '|', &menu->fitModeIndex),
                                   &SDL_Libretro_MenuFitModeChanged, menu),
        "Aspect keeps the ratio; Integer locks whole-number scales; Stretch fills the window");
    SDL_Libretro_MenuOnChanged(nk_console_combobox(audioVideo, "Theme", SDL_LIBRETRO_MENU_STYLE_NAMES, '|', &menu->styleIndex),
                               &SDL_Libretro_MenuStyleChanged, menu);
    nk_console_set_tooltip(
        SDL_Libretro_MenuOnChanged(nk_console_combobox(audioVideo, "UI Scale", "Auto|1x|2x|3x|4x", '|', &menu->uiScaleIndex),
                                   &SDL_Libretro_MenuUIScaleChanged, menu),
        "Menu size; Auto picks one from the resolution");

    // Core Options, populated lazily once a core registers options.
    menu->optionsButton = nk_console_button(settings, "Core Options");
    nk_console_button_set_symbol(menu->optionsButton, NK_SYMBOL_TRIANGLE_RIGHT);
    menu->optionsButton->visible = nk_false;

    // Controllers, populated lazily once a core registers controller info.
    menu->controllersButton = nk_console_button(settings, "Controllers");
    nk_console_button_set_symbol(menu->controllersButton, NK_SYMBOL_TRIANGLE_RIGHT);
    menu->controllersButton->visible = nk_false;

    // Keyboard bindings for player 1's virtual controller.
    nk_console* keyboard = SDL_Libretro_MenuAddSubmenu(settings, "Keyboard", NK_SYMBOL_TRIANGLE_RIGHT);
    nk_console_set_tooltip(keyboard, "Keyboard keys for Player 1's controller, and the mouse's port");
    SDL_Libretro_MenuSyncKeyBinds(menu);

    // The port the mouse, pointer and lightgun answer on.
    menu->mousePortList[0] = '\0';
    for (unsigned port = 0; port < SDL_LIBRETRO_MAX_USERS; port++) {
        char item[16];
        SDL_snprintf(item, sizeof(item), port == 0 ? "Port %u" : "|Port %u", port + 1);
        SDL_strlcat(menu->mousePortList, item, sizeof(menu->mousePortList));
    }
    nk_console_set_tooltip(
        SDL_Libretro_MenuOnChanged(nk_console_combobox(keyboard, "Mouse Port", menu->mousePortList, '|', &menu->mousePortIndex),
                                   &SDL_Libretro_MenuMousePortChanged, menu),
        "The controller port the mouse, pointer and lightgun drive");

    for (int i = 0; i < SDL_LIBRETRO_MAX_JOYPAD_BUTTONS; i++) {
        menu->keyBinds[i].menu = menu;
        menu->keyBinds[i].button = i;
        SDL_Libretro_MenuOnChanged(
            nk_console_input_key(keyboard, SDL_Libretro_JoypadButtonNames[i], &menu->keyBinds[i].rune),
            &SDL_Libretro_MenuKeyBindChanged, &menu->keyBinds[i]);
    }

    // Gamepad bindings and analog-to-digital, one page per player.
    menu->gamepadButton = nk_console_button(settings, "Gamepad");
    nk_console_button_set_symbol(menu->gamepadButton, NK_SYMBOL_TRIANGLE_RIGHT);
    nk_console_set_tooltip(menu->gamepadButton, "Per-player gamepad buttons for the retro controllers");
    SDL_Libretro_MenuBuildGamepads(menu);

    // Directories
    nk_console* directories = SDL_Libretro_MenuAddSubmenu(settings, "Directories", NK_SYMBOL_TRIANGLE_RIGHT);

    SDL_Libretro_MenuAddSettingDir(directories, "Cores", menu,
                                   menu->coreDirBuffer, sizeof(menu->coreDirBuffer));
    SDL_Libretro_MenuAddSettingDir(directories, "Saves", menu,
                                   menu->saveDirBuffer, sizeof(menu->saveDirBuffer));
    SDL_Libretro_MenuAddSettingDir(directories, "System", menu,
                                   menu->systemDirBuffer, sizeof(menu->systemDirBuffer));
    SDL_Libretro_MenuAddSettingDir(directories, "Content", menu,
                                   menu->browseDirBuffer, sizeof(menu->browseDirBuffer));
}

/**
 * @internal
 * Build the top-level menu tree.
 */
static void SDL_Libretro_MenuBuildWidgets(SDL_LibretroMenu* menu) {
    SDL_Libretro* lr = menu->lr;
    (void)lr;

    // Backing out of the top level acts like Resume.
    nk_console_add_event_handler(menu->console, NK_CONSOLE_EVENT_BACK, &SDL_Libretro_MenuBackClicked, menu, NULL);

    // Progress reported by the core through a progress-type OSD message.
    menu->osdProgressWidget = nk_console_progress(menu->console, "Progress", &menu->osdProgressValue, 100);
    menu->osdProgressWidget->visible = nk_false;

    // Resume
    menu->resumeButton = SDL_Libretro_MenuAddButton(menu, menu->console, "Resume", NK_SYMBOL_TRIANGLE_RIGHT, &SDL_Libretro_MenuResumeClicked, NULL);

    // Load Game
#ifdef __EMSCRIPTEN__
    // Browsing the virtual file system isn't useful on the web; a JS file
    // picker brings the user's own files in instead.
    menu->loadGameButton = SDL_Libretro_MenuAddButton(menu, menu->console, "Load Game", NK_SYMBOL_NONE, &SDL_Libretro_MenuWebLoadGameClicked, NULL);
#else
    menu->loadGameButton = SDL_Libretro_MenuOnChanged(
        nk_console_file_action(menu->console, "Load Game", menu->loadGamePath, sizeof(menu->loadGamePath)),
        &SDL_Libretro_MenuGameFileChanged, menu);
    if (lr->fileBrowserStartDirectory[0] != '\0') {
        nk_console_file_set_directory(menu->loadGameButton, lr->fileBrowserStartDirectory);
    }
    SDL_Libretro_MenuUpdateLoadGameFilter(menu);
#endif

    // Save State / Load State
    menu->stateRow = nk_console_row_begin(menu->console);
    {
        SDL_Libretro_MenuAddButton(menu, menu->stateRow, "Save State", NK_SYMBOL_RECT_SOLID, &SDL_Libretro_MenuSaveStateClicked, NULL);
        SDL_Libretro_MenuAddButton(menu, menu->stateRow, "Load State", NK_SYMBOL_RECT_OUTLINE, &SDL_Libretro_MenuLoadStateClicked, NULL);
    }
    nk_console_row_end(menu->stateRow);

    // Screenshot
    menu->screenshotButton = SDL_Libretro_MenuAddButton(menu, menu->console, "Screenshot", NK_SYMBOL_CIRCLE_OUTLINE, &SDL_Libretro_MenuScreenshotClicked, NULL);

    // Reset
    menu->resetButton = SDL_Libretro_MenuAddButton(menu, menu->console, "Reset", NK_SYMBOL_CIRCLE_SOLID, &SDL_Libretro_MenuResetClicked, NULL);

    // Disks, populated lazily for cores with multi-disk games.
    menu->disksButton = SDL_Libretro_MenuAddSubmenu(menu->console, "Disks", NK_SYMBOL_CIRCLE_OUTLINE);
    nk_console_set_tooltip(menu->disksButton, "Swap disks for multi-disk games");
    menu->disksButton->visible = nk_false;

    SDL_Libretro_MenuBuildSettings(menu);

    // About, rebuilt by the CLICKED handler whenever the page opens.
    menu->aboutButton = SDL_Libretro_MenuAddButton(menu, menu->console, "About", NK_SYMBOL_TRIANGLE_RIGHT, &SDL_Libretro_MenuAboutOpened, NULL);

    // Quit, kept as the last top-level entry when the app adds its own.
    menu->quitButton = SDL_Libretro_MenuAddButton(menu, menu->console, "Quit", NK_SYMBOL_X, &SDL_Libretro_MenuQuitClicked, NULL);
}

/**
 * Creates the in-app menu for a libretro context.
 *
 * Sets up Nuklear on the context's window and renderer, restores the menu's
 * persisted settings (style, UI scale, volume, and the like) from the config,
 * and builds the menu pages. The menu starts closed, but
 * SDL_Libretro_UpdateMenu() opens it whenever no game is running.
 *
 * Requires the implementation to be compiled with SDL_LIBRETRO_ENABLE_MENU.
 * Each frame, route events through SDL_Libretro_HandleMenuEvent(), then call
 * SDL_Libretro_UpdateMenu() and SDL_Libretro_RenderMenu() after rendering the
 * game and before presenting.
 *
 * @param lr the libretro context. SDL_Libretro_SetRenderer() must have been
 *           called on it first, and it must outlive the menu.
 * @returns the new menu, or NULL on failure; call SDL_GetError() for more
 *          information. Free it with SDL_Libretro_DestroyMenu().
 *
 * @see SDL_Libretro_DestroyMenu()
 * @see SDL_Libretro_UpdateMenu()
 * @see SDL_Libretro_RenderMenu()
 * @see SDL_Libretro_HandleMenuEvent()
 */
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

    // Bake the default font once at the base height; UI scaling happens
    // through SDL_SetRenderScale in the render path.
    SDL_Libretro_MenuBakeFont(menu);

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
    menu->renderScale = SDL_Libretro_MenuRenderScale(menu);
    SDL_Libretro_MenuSyncSettings(menu);
    SDL_Libretro_MenuBuildWidgets(menu);

    nk_input_begin(menu->ctx);

    return menu;
}

/**
 * Destroys a menu created with SDL_Libretro_CreateMenu().
 *
 * Writes any changed menu settings to the config, frees the Nuklear context
 * and every widget, including the entries added with
 * SDL_Libretro_AddMenuButton() and SDL_Libretro_AddMenuCheckbox(), and drops
 * any queued SDL_EVENT_LIBRETRO_MENU_OPENED and SDL_EVENT_LIBRETRO_MENU_CLOSED
 * events that still refer to it. The libretro context is left untouched;
 * destroy the menu before destroying the context.
 *
 * @param menu the menu to destroy. NULL is ignored.
 *
 * @see SDL_Libretro_CreateMenu()
 */
void SDL_Libretro_DestroyMenu(SDL_LibretroMenu* menu) {
    if (menu == NULL) {
        return;
    }
    // Queued menu events carry this menu in data2; drop them before it goes away.
    SDL_FlushEvent(SDL_EVENT_LIBRETRO_MENU_OPENED);
    SDL_FlushEvent(SDL_EVENT_LIBRETRO_MENU_CLOSED);
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
    SDL_Libretro_MenuFreeDiskLabels(menu);
    nk_gamepad_free(&menu->gamepads);
    if (menu->ctx != NULL) {
        nk_sdl_shutdown(menu->ctx);
    }
    SDL_free(menu);
}

/**
 * Checks whether the menu is currently shown.
 *
 * While it is open, the application should skip SDL_Libretro_Update() so
 * the game stays paused.
 *
 * @param menu the menu to query.
 * @returns true when the menu is open, false when it is closed or menu is
 *          NULL.
 *
 * @see SDL_Libretro_SetMenuOpen()
 * @see SDL_Libretro_ToggleMenu()
 */
bool SDL_Libretro_IsMenuOpen(const SDL_LibretroMenu* menu) {
    return menu != NULL && menu->open;
}

/**
 * Gets the libretro context the menu was created for.
 *
 * Handy inside an SDL_LibretroMenuCallback.
 *
 * @param menu the menu to query.
 * @returns the SDL_Libretro context passed to SDL_Libretro_CreateMenu(), or
 *          NULL when menu is NULL.
 *
 * @see SDL_Libretro_CreateMenu()
 */
SDL_Libretro* SDL_Libretro_GetMenuLibretro(const SDL_LibretroMenu* menu) {
    return menu != NULL ? menu->lr : NULL;
}

/**
 * Attaches generic application data to the menu.
 *
 * The menu never reads or frees it.
 *
 * @param menu the menu to update. NULL is ignored.
 * @param userData the data to store, or NULL to clear it.
 *
 * @see SDL_Libretro_GetMenuUserData()
 */
void SDL_Libretro_SetMenuUserData(SDL_LibretroMenu* menu, void* userData) {
    if (menu != NULL) {
        menu->userData = userData;
    }
}

/**
 * Gets the data attached with SDL_Libretro_SetMenuUserData().
 *
 * @param menu the menu to query.
 * @returns the stored data, or NULL when none was set or menu is NULL.
 *
 * @see SDL_Libretro_SetMenuUserData()
 */
void* SDL_Libretro_GetMenuUserData(const SDL_LibretroMenu* menu) {
    return menu != NULL ? menu->userData : NULL;
}

/**
 * Opens or closes the menu.
 *
 * When the state changes, pushes SDL_EVENT_LIBRETRO_MENU_OPENED or
 * SDL_EVENT_LIBRETRO_MENU_CLOSED with the menu in data2, and closing stops
 * text input on the window. Setting the current state again does nothing.
 * Note that SDL_Libretro_UpdateMenu() reopens the menu while no game is
 * running.
 *
 * @param menu the menu to update. NULL is ignored.
 * @param open true to show the menu, false to hide it.
 *
 * @see SDL_Libretro_ToggleMenu()
 * @see SDL_Libretro_IsMenuOpen()
 */
void SDL_Libretro_SetMenuOpen(SDL_LibretroMenu* menu, bool open) {
    if (menu == NULL || menu->open == open) {
        return;
    }
    menu->open = open;

    // Input held while the menu opens belongs to whatever saw it pressed.
    // The application and core still get the key releases, so nothing like
    // a held fast-forward key sticks, and the menu doesn't take a held
    // gamepad button's release as a click.
    if (open) {
        int numKeys = 0;
        const bool* keys = SDL_GetKeyboardState(&numKeys);
        for (int i = 0; i < SDL_SCANCODE_COUNT; i++) {
            menu->keysHeldAtOpen[i] = keys != NULL && i < numKeys && keys[i];
        }
        // Buttons still hidden from an earlier open are held too.
        for (int i = 0; i < NK_GAMEPAD_MAX; i++) {
            menu->padButtonsHeldAtOpen[i] |= menu->gamepads.gamepads[i].buttons;
        }
    }
    if (!open && menu->lr != NULL && menu->lr->window != NULL) {
        SDL_StopTextInput(menu->lr->window);
    }
    SDL_Libretro_PushEvent(menu->lr, open ? SDL_EVENT_LIBRETRO_MENU_OPENED : SDL_EVENT_LIBRETRO_MENU_CLOSED, menu);
}

/**
 * Opens the menu when it is closed, or closes it when it is open.
 *
 * This is what the toggle key (SDL_LIBRETRO_MENU_TOGGLE_KEY) and the gamepad
 * Guide button do in SDL_Libretro_HandleMenuEvent().
 *
 * @param menu the menu to toggle. NULL is ignored.
 *
 * @see SDL_Libretro_SetMenuOpen()
 */
void SDL_Libretro_ToggleMenu(SDL_LibretroMenu* menu) {
    if (menu != NULL) {
        SDL_Libretro_SetMenuOpen(menu, !menu->open);
    }
}

/**
 * Routes an event to the menu; call this for every event, before
 * SDL_Libretro_HandleEvent().
 *
 * The toggle key (SDL_LIBRETRO_MENU_TOGGLE_KEY) and the gamepad Guide button
 * flip the menu open or closed, and are always consumed. Gamepad connection
 * changes are tracked even while the menu is closed.
 *
 * @param menu the menu to feed.
 * @param event the event to handle; it is not modified.
 * @returns true when the menu consumed the event: do not forward it to
 *          SDL_Libretro_HandleEvent(). This happens for the toggle inputs
 *          above, and for gameplay input swallowed while the menu is open.
 *          false when the event should be handled as usual: the menu is
 *          closed, menu or event is NULL, the event is an application event
 *          (SDL_EVENT_USER and above), or it is a lifecycle event (quit,
 *          drop-file, window, and gamepad/joystick add/remove) which always
 *          passes through so the application and frontend still see it. The
 *          release of a key that was already held when the menu opened passes
 *          through too, so the press it answers isn't left stuck down.
 *
 * @see SDL_Libretro_UpdateMenu()
 */
bool SDL_Libretro_HandleMenuEvent(SDL_LibretroMenu* menu, const SDL_Event* event) {
    if (menu == NULL || menu->ctx == NULL || event == NULL) {
        return false;
    }

    // Gamepad connection bookkeeping runs even while the menu is closed.
    nk_gamepad_sdl3_handle_event(&menu->gamepads, (SDL_Event*)event);

    // Toggle on the menu key or the gamepad Guide button.
    // Both edges of the toggle key are consumed so the core never sees an
    // unmatched press; only the release toggles.
    if ((event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) &&
        event->key.key == SDL_LIBRETRO_MENU_TOGGLE_KEY) {
        if (event->type == SDL_EVENT_KEY_UP) {
            SDL_Libretro_ToggleMenu(menu);
        }
        return true;
    }
    if (event->type == SDL_EVENT_GAMEPAD_BUTTON_UP && event->gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE) {
        SDL_Libretro_ToggleMenu(menu);
        return true;
    }

    if (!menu->open) {
        return false;
    }

    // Application events, including the menu's own notifications, are never
    // swallowed and mean nothing to the UI.
    if (event->type >= SDL_EVENT_USER) {
        return false;
    }

    // A key held since before the menu opened releases to the application and
    // core, which saw it pressed. A fresh press makes it the menu's.
    if ((event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) &&
        event->key.scancode > SDL_SCANCODE_UNKNOWN && event->key.scancode < SDL_SCANCODE_COUNT) {
        bool* held = &menu->keysHeldAtOpen[event->key.scancode];
        if (event->type == SDL_EVENT_KEY_UP && *held) {
            *held = false;
            return false;
        }
        if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
            *held = false;
        }
    }

    // Feed a copy so coordinate conversion doesn't mutate the caller's event.
    SDL_SetRenderScale(menu->lr->renderer, menu->renderScale, menu->renderScale);
    SDL_Event converted = *event;
    SDL_ConvertEventToRenderCoordinates(menu->lr->renderer, &converted);
    nk_sdl_handle_event(menu->ctx, &converted);
    SDL_SetRenderScale(menu->lr->renderer, 1.0f, 1.0f);

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
 * @internal
 * Rebuild the core-derived submenus (Core Options, Controllers, and the file
 * filter) when they no longer match the loaded core:
 *
 *  * an option changed outside the menu (the app-side dirty flag),
 *  * a core unloaded since the last build, freeing the strings the widgets
 *    borrow, even if the same core loaded again; or the option set changed size,
 *  * the menu just opened; visibility may have shifted while it was closed,
 *    so the core's display callback is re-run first.
 */
static void SDL_Libretro_MenuRebuildCoreMenus(SDL_LibretroMenu* menu, bool justOpened) {
    SDL_Libretro* lr = menu->lr;
    if (SDL_Libretro_AreOptionsDirty(lr)) {
        menu->optionsStale = true;
    }
    // Any unload frees the strings the widgets borrow, even when the same core
    // loads again, so compare generations rather than core names.
    bool coreChanged = menu->builtCoreGeneration != lr->coreGeneration;
    bool countChanged = menu->builtOptionCount != lr->core.optionCount;
    if (!justOpened && !coreChanged && !countChanged && !menu->optionsStale) {
        return;
    }

    if (justOpened && SDL_Libretro_UpdateOptionVisibility(lr)) {
        SDL_Libretro_AreOptionsDirty(lr);
    }
    SDL_Libretro_MenuBuildOptions(menu);
    SDL_Libretro_MenuBuildControllers(menu);
    SDL_Libretro_MenuBuildDisks(menu);
    // Not core-derived, but gamepads can come and go while the menu is
    // closed, and the player pages are titled with their names.
    SDL_Libretro_MenuBuildGamepads(menu);
    SDL_Libretro_MenuUpdateLoadGameFilter(menu);
    menu->optionsStale = false;
    menu->builtOptionCount = lr->core.optionCount;
    menu->builtCoreGeneration = lr->coreGeneration;
}

/**
 * Opens the menu and navigates to a page by its slash-separated label path,
 * e.g. "Settings/Audio & Video", "Settings/Core Options" or "Disks".
 *
 * The pages built from the loaded core are refreshed first, so their paths
 * resolve before the next frame.
 *
 * @param menu the menu to navigate.
 * @param path the label path of the page to show.
 * @returns true when the path resolved; the menu stays open either way.
 *          false when menu or path is NULL; call SDL_GetError() for more
 *          information.
 *
 * @see SDL_Libretro_SetMenuOpen()
 */
bool SDL_Libretro_OpenMenuPath(SDL_LibretroMenu* menu, const char* path) {
    if (menu == NULL || menu->console == NULL || path == NULL) {
        return SDL_InvalidParamError("menu");
    }
    SDL_Libretro_SetMenuOpen(menu, true);
    // Do the just-opened work here so the next SDL_Libretro_UpdateMenu()
    // doesn't rebuild the lazy pages again underneath the navigation, and so
    // their paths resolve before the first frame.
    SDL_Libretro_MenuRebuildCoreMenus(menu, true);
    SDL_Libretro_MenuSyncSettings(menu);
    SDL_Libretro_MenuSyncSettingsBuffers(menu);
    SDL_Libretro_MenuSyncKeyBinds(menu);
    SDL_Libretro_MenuSyncPadBinds(menu);
    menu->wasOpen = true;

    return nk_console_navigate_to_path(menu->console, path) == nk_true;
}

/**
 * Processes input and lays out the menu for this frame.
 *
 * Call once per frame, after the frame's events went through
 * SDL_Libretro_HandleMenuEvent() and after SDL_Libretro_Render(), then
 * follow it with SDL_Libretro_RenderMenu(). Opens the menu whenever no game
 * is running. While open, it refreshes the pages that depend on the loaded
 * core, shows or hides the game-only entries, and writes changed menu
 * settings to the config. Does nothing else while the menu is closed.
 *
 * @param menu the menu to update. NULL is ignored.
 *
 * @see SDL_Libretro_RenderMenu()
 * @see SDL_Libretro_HandleMenuEvent()
 */
void SDL_Libretro_UpdateMenu(SDL_LibretroMenu* menu) {
    if (menu == NULL || menu->ctx == NULL) {
        return;
    }
    SDL_Libretro* lr = menu->lr;

    nk_input_end(menu->ctx);
    nk_gamepad_update(&menu->gamepads);

    // Hide gamepad buttons held since the menu opened until they're let go,
    // so their release doesn't click whatever has focus.
    for (int i = 0; i < NK_GAMEPAD_MAX; i++) {
        struct nk_gamepad* pad = &menu->gamepads.gamepads[i];
        unsigned int held = menu->padButtonsHeldAtOpen[i];
        menu->padButtonsHeldAtOpen[i] = held & pad->buttons;
        pad->buttons &= ~held;
        pad->buttons_prev &= ~held;
    }

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
    menu->screenshotButton->visible = (nk_bool)gameReady;
    menu->resetButton->visible = (nk_bool)gameReady;

    // Progress-type OSD messages get a live bar at the top of the menu.
    int osdProgress = SDL_Libretro_GetMessageProgress(lr);
    if (osdProgress >= 0) {
        const char* osdMessage = SDL_Libretro_GetMessage(lr);
        SDL_strlcpy(menu->osdProgressLabel, osdMessage != NULL ? osdMessage : "", sizeof(menu->osdProgressLabel));
        menu->osdProgressValue = (nk_size)(osdProgress > 100 ? 100 : osdProgress);
        nk_console_progress_update(menu->osdProgressWidget, menu->osdProgressLabel, &menu->osdProgressValue, 100);
        menu->osdProgressWidget->visible = nk_true;
    }
    else {
        menu->osdProgressWidget->visible = nk_false;
    }

    // Settings that can change outside the menu.
    if (justOpened) {
        SDL_Libretro_MenuSyncSettings(menu);
        SDL_Libretro_MenuSyncSettingsBuffers(menu);
        SDL_Libretro_MenuSyncKeyBinds(menu);
        SDL_Libretro_MenuSyncPadBinds(menu);
    }

    // Flush changed menu settings into the config.
    if (menu->settingsDirty) {
        SDL_Libretro_MenuSaveState(menu);
    }

    menu->renderScale = SDL_Libretro_MenuRenderScale(menu);

    int width = 0;
    int height = 0;
    SDL_GetRenderOutputSize(lr->renderer, &width, &height);
    float logicalW = (float)width / menu->renderScale;
    float logicalH = (float)height / menu->renderScale;

    menu->uiBuilt = true;
    nk_console_render_window(menu->console, "SDL_libretro", nk_rect(0.0f, 0.0f, logicalW, logicalH), NK_WINDOW_SCROLL_AUTO_HIDE |
                                                                                                                  // Show the window title only on the top level.
                                                                                                                  ((nk_console_active_parent(menu->console) == menu->console) ? NK_WINDOW_TITLE : 0));
}

/**
 * Draws the menu laid out by SDL_Libretro_UpdateMenu() to the renderer.
 *
 * Call once per frame, after SDL_Libretro_UpdateMenu() and before
 * SDL_RenderPresent(). Draws nothing while the menu is closed, but must still
 * be called every frame so input is collected for the next one.
 *
 * @param menu the menu to draw. NULL is ignored.
 *
 * @see SDL_Libretro_UpdateMenu()
 */
void SDL_Libretro_RenderMenu(SDL_LibretroMenu* menu) {
    if (menu == NULL || menu->ctx == NULL) {
        return;
    }
    // Flush whenever nk_begin ran this frame, even if a callback closed the
    // menu mid-frame, so nk_clear always pairs with it.
    if (menu->uiBuilt) {
        SDL_SetRenderScale(menu->lr->renderer, menu->renderScale, menu->renderScale);
        nk_sdl_render(menu->ctx, NK_ANTI_ALIASING_ON);
        SDL_SetRenderScale(menu->lr->renderer, 1.0f, 1.0f);
        nk_console_sdl_update_text_input(menu->console, menu->lr->window);
        menu->uiBuilt = false;
    }
    nk_input_begin(menu->ctx);
}

/**
 * @internal
 * New entries append after Quit; swap them so Quit stays the last entry.
 */
static void SDL_Libretro_MenuKeepQuitLast(SDL_LibretroMenu* menu) {
    nk_console** children = menu->console->children;
    size_t count = cvector_size(children);
    if (menu->quitButton == NULL || count < 2 || children[count - 2] != menu->quitButton) {
        return;
    }
    children[count - 2] = children[count - 1];
    children[count - 1] = menu->quitButton;
}

/**
 * Adds a button to the main menu, just above Quit.
 *
 * The menu copies the label and frees the button in
 * SDL_Libretro_DestroyMenu().
 *
 * @param menu the menu to add to.
 * @param label the button text.
 * @param callback called when the button is clicked, or NULL.
 * @param userdata passed to the callback.
 * @returns true on success, false when menu or label is NULL or the button
 *          could not be created; call SDL_GetError() for more information.
 *
 * @see SDL_Libretro_AddMenuCheckbox()
 * @see SDL_Libretro_MenuScreenshotClicked()
 */
bool SDL_Libretro_AddMenuButton(SDL_LibretroMenu* menu, const char* label, SDL_LibretroMenuCallback callback, void* userdata) {
    if (menu == NULL || menu->console == NULL || label == NULL) {
        return SDL_InvalidParamError("menu");
    }
    nk_console* button = SDL_Libretro_MenuAddButton(menu, menu->console, label, NK_SYMBOL_NONE, callback, userdata);
    SDL_Libretro_MenuKeepQuitLast(menu);
    return button != NULL;
}

/**
 * Adds a checkbox to the main menu, just above Quit.
 *
 * The checkbox starts from the current value, and the value pointer is
 * written on every toggle, so it must outlive the menu. The menu copies the
 * label and frees the checkbox in SDL_Libretro_DestroyMenu().
 *
 * @param menu the menu to add to.
 * @param label the checkbox text.
 * @param value the bool the checkbox mirrors, or NULL to keep the state
 *              internal.
 * @param callback called after each toggle, once value is updated, or NULL.
 * @param userdata passed to the callback.
 * @returns true on success, false when menu or label is NULL or the checkbox
 *          could not be created; call SDL_GetError() for more information.
 *
 * @see SDL_Libretro_AddMenuButton()
 */
bool SDL_Libretro_AddMenuCheckbox(SDL_LibretroMenu* menu, const char* label, bool* value, SDL_LibretroMenuCallback callback, void* userdata) {
    if (menu == NULL || menu->console == NULL || label == NULL) {
        return SDL_InvalidParamError("menu");
    }
    nk_console* checkbox = SDL_Libretro_MenuAddCheckbox(menu, menu->console, label, NULL, value, callback, userdata);
    SDL_Libretro_MenuKeepQuitLast(menu);
    return checkbox != NULL;
}

#endif /* SDL_LIBRETRO_MENU_IMPL_ONCE */
