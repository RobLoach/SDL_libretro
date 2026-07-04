/**
 * SDL_libretro_menu - Optional menu system for SDL_libretro using nuklear_console.
 *
 * This header is completely optional. To use it:
 *
 * 1. Include Nuklear with the required defines and NK_IMPLEMENTATION in one TU.
 * 2. Include the SDL3 renderer backend with NK_SDL3_RENDERER_IMPLEMENTATION.
 * 3. Include nuklear_gamepad with NK_GAMEPAD_IMPLEMENTATION.
 * 4. Include nuklear_console with NK_CONSOLE_IMPLEMENTATION.
 * 5. Define SDL_LIBRETRO_MENU_IMPLEMENTATION before including this header in
 *    that same TU.
 *
 * Every other TU includes this header normally (no IMPLEMENTATION define).
 *
 * @file SDL_libretro_menu.h
 */

#ifndef SDL_LIBRETRO_MENU_H
#define SDL_LIBRETRO_MENU_H

#include <SDL3/SDL.h>

typedef struct SDL_LibretroMenu SDL_LibretroMenu;

struct SDL_Libretro;

#ifdef __cplusplus
extern "C" {
#endif

SDL_LibretroMenu* SDL_Libretro_CreateMenu(struct SDL_Libretro* lr);
void SDL_Libretro_DestroyMenu(SDL_LibretroMenu* menu);
bool SDL_Libretro_IsMenuOpen(const SDL_LibretroMenu* menu);
void SDL_Libretro_SetMenuOpen(SDL_LibretroMenu* menu, bool open);
bool SDL_Libretro_MenuHandleEvent(SDL_LibretroMenu* menu, SDL_Event* event);
void SDL_Libretro_UpdateMenu(SDL_LibretroMenu* menu);
void SDL_Libretro_RenderMenu(SDL_LibretroMenu* menu);

#ifdef __cplusplus
}
#endif

/* ===================================================================== */
/*  Implementation                                                       */
/* ===================================================================== */
#ifdef SDL_LIBRETRO_MENU_IMPLEMENTATION
#ifndef SDL_LIBRETRO_MENU_IMPLEMENTATION_ONCE
#define SDL_LIBRETRO_MENU_IMPLEMENTATION_ONCE

#include "SDL_libretro.h"

#ifndef SDL_LIBRETRO_MENU_MAX_OPTIONS
#define SDL_LIBRETRO_MENU_MAX_OPTIONS 256
#endif

#ifndef SDL_LIBRETRO_MENU_MAX_OPTION_VALUES_LEN
#define SDL_LIBRETRO_MENU_MAX_OPTION_VALUES_LEN 2048
#endif

#ifndef SDL_LIBRETRO_MENU_FONT_SIZE
#define SDL_LIBRETRO_MENU_FONT_SIZE 18
#endif

typedef struct SDL_LibretroMenuOptionBinding {
    char key[128];
    int selectedIndex;
    char valuesList[SDL_LIBRETRO_MENU_MAX_OPTION_VALUES_LEN];
    int separator;
    int count;
    nk_console* widget;
} SDL_LibretroMenuOptionBinding;

struct SDL_LibretroMenu {
    struct SDL_Libretro* lr;
    struct nk_context* ctx;
    nk_console* console;
    struct nk_gamepads gamepads;
    bool open;
    bool shouldQuit;
    float fontScale;

    int volumePercent;
    int fitModeIndex;

    SDL_LibretroMenuOptionBinding optionBindings[SDL_LIBRETRO_MENU_MAX_OPTIONS];
    unsigned optionBindingCount;

    nk_console* optionsParent;
};

static void SDL_LibretroMenu_OnResume(nk_console* widget, void* user_data) {
    (void)user_data;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)nk_console_user_data(widget);
    if (menu) {
        menu->open = false;
    }
}

static void SDL_LibretroMenu_OnReset(nk_console* widget, void* user_data) {
    (void)user_data;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)nk_console_user_data(widget);
    if (menu && menu->lr) {
        SDL_Libretro_Reset(menu->lr);
        menu->open = false;
    }
}

static void SDL_LibretroMenu_OnQuit(nk_console* widget, void* user_data) {
    (void)user_data;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)nk_console_user_data(widget);
    if (menu) {
        menu->shouldQuit = true;
    }
}

static void SDL_LibretroMenu_OnVolumeChanged(nk_console* widget, void* user_data) {
    (void)user_data;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)nk_console_user_data(widget);
    if (menu && menu->lr) {
        SDL_Libretro_SetVolume(menu->lr, (float)menu->volumePercent / 100.0f);
    }
}

static void SDL_LibretroMenu_OnFitModeChanged(nk_console* widget, void* user_data) {
    (void)user_data;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)nk_console_user_data(widget);
    if (menu && menu->lr) {
        SDL_Libretro_SetFitMode(menu->lr, (SDL_LibretroFitMode)menu->fitModeIndex);
    }
}

static void SDL_LibretroMenu_OnOptionChanged(nk_console* widget, void* user_data) {
    (void)user_data;
    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)nk_console_user_data(widget);
    if (!menu || !menu->lr) return;

    for (unsigned i = 0; i < menu->optionBindingCount; i++) {
        SDL_LibretroMenuOptionBinding* binding = &menu->optionBindings[i];
        if (binding->widget == widget) {
            const SDL_LibretroOption* opt = SDL_Libretro_GetOption(menu->lr, binding->key);
            if (opt && binding->selectedIndex >= 0 && (unsigned)binding->selectedIndex < opt->valuesCount) {
                SDL_Libretro_SetOptionValue(menu->lr, binding->key, opt->values[binding->selectedIndex].value);
            }
            break;
        }
    }
}

static void SDL_LibretroMenu_BuildOptionBindings(SDL_LibretroMenu* menu) {
    menu->optionBindingCount = 0;
    unsigned optionCount = SDL_Libretro_GetOptionCount(menu->lr);

    for (unsigned i = 0; i < optionCount && menu->optionBindingCount < SDL_LIBRETRO_MENU_MAX_OPTIONS; i++) {
        const SDL_LibretroOption* opt = SDL_Libretro_GetOptionByIndex(menu->lr, i);
        if (!opt || !opt->visible) continue;

        SDL_LibretroMenuOptionBinding* binding = &menu->optionBindings[menu->optionBindingCount];
        SDL_strlcpy(binding->key, opt->key, sizeof(binding->key));
        binding->separator = '|';
        binding->selectedIndex = 0;
        binding->count = (int)opt->valuesCount;

        char* dst = binding->valuesList;
        size_t remaining = sizeof(binding->valuesList);
        for (unsigned v = 0; v < opt->valuesCount; v++) {
            const char* label = opt->values[v].label ? opt->values[v].label : opt->values[v].value;
            if (SDL_strcmp(opt->values[v].value, opt->value) == 0) {
                binding->selectedIndex = (int)v;
            }
            size_t len = SDL_strlen(label);
            if (v > 0) {
                if (remaining < 2) break;
                *dst++ = '|';
                remaining--;
            }
            if (len >= remaining) break;
            SDL_memcpy(dst, label, len);
            dst += len;
            remaining -= len;
        }
        *dst = '\0';

        menu->optionBindingCount++;
    }
}

static void SDL_LibretroMenu_BuildConsole(SDL_LibretroMenu* menu) {
    if (menu->console) {
        nk_console_free(menu->console);
    }

    menu->console = nk_console_init(menu->ctx);
    nk_console_set_gamepads(menu->console, &menu->gamepads);
    nk_console_set_user_data(menu->console, menu);

    nk_console_button_onclick(menu->console, "Resume", SDL_LibretroMenu_OnResume);

    if (SDL_Libretro_IsCoreReady(menu->lr)) {
        // Core Options
        SDL_LibretroMenu_BuildOptionBindings(menu);
        if (menu->optionBindingCount > 0) {
            unsigned categoryCount = SDL_Libretro_GetCategoryCount(menu->lr);
            nk_console* optionsParent;

            if (categoryCount > 0) {
                optionsParent = nk_console_button(menu->console, "Core Options");
                nk_console_button_onclick(optionsParent, "Back", nk_console_button_back);

                for (unsigned c = 0; c < categoryCount; c++) {
                    const SDL_LibretroCategory* cat = SDL_Libretro_GetCategoryByIndex(menu->lr, c);
                    if (!cat) continue;

                    nk_console* catParent = nk_console_button(optionsParent, cat->desc);
                    nk_console_button_onclick(catParent, "Back", nk_console_button_back);

                    for (unsigned i = 0; i < menu->optionBindingCount; i++) {
                        SDL_LibretroMenuOptionBinding* binding = &menu->optionBindings[i];
                        const SDL_LibretroOption* opt = SDL_Libretro_GetOption(menu->lr, binding->key);
                        if (!opt || !opt->category || SDL_strcmp(opt->category, cat->key) != 0) continue;

                        binding->widget = nk_console_combobox(catParent, opt->desc,
                            binding->valuesList, binding->separator, &binding->selectedIndex);
                        if (opt->info && opt->info[0]) {
                            nk_console_set_tooltip(binding->widget, opt->info);
                        }
                        nk_console_add_event(binding->widget, NK_CONSOLE_EVENT_CHANGED, SDL_LibretroMenu_OnOptionChanged);
                    }
                }

                // Uncategorized options
                nk_console* uncatParent = NULL;
                for (unsigned i = 0; i < menu->optionBindingCount; i++) {
                    SDL_LibretroMenuOptionBinding* binding = &menu->optionBindings[i];
                    const SDL_LibretroOption* opt = SDL_Libretro_GetOption(menu->lr, binding->key);
                    if (!opt || (opt->category && opt->category[0])) continue;

                    if (!uncatParent) {
                        uncatParent = nk_console_button(optionsParent, "Other");
                        nk_console_button_onclick(uncatParent, "Back", nk_console_button_back);
                    }
                    binding->widget = nk_console_combobox(uncatParent, opt->desc,
                        binding->valuesList, binding->separator, &binding->selectedIndex);
                    if (opt->info && opt->info[0]) {
                        nk_console_set_tooltip(binding->widget, opt->info);
                    }
                    nk_console_add_event(binding->widget, NK_CONSOLE_EVENT_CHANGED, SDL_LibretroMenu_OnOptionChanged);
                }
            } else {
                optionsParent = nk_console_button(menu->console, "Core Options");
                nk_console_button_onclick(optionsParent, "Back", nk_console_button_back);

                for (unsigned i = 0; i < menu->optionBindingCount; i++) {
                    SDL_LibretroMenuOptionBinding* binding = &menu->optionBindings[i];
                    const SDL_LibretroOption* opt = SDL_Libretro_GetOption(menu->lr, binding->key);
                    if (!opt) continue;

                    binding->widget = nk_console_combobox(optionsParent, opt->desc,
                        binding->valuesList, binding->separator, &binding->selectedIndex);
                    if (opt->info && opt->info[0]) {
                        nk_console_set_tooltip(binding->widget, opt->info);
                    }
                    nk_console_add_event(binding->widget, NK_CONSOLE_EVENT_CHANGED, SDL_LibretroMenu_OnOptionChanged);
                }
            }

            menu->optionsParent = optionsParent;
        }

        // Audio
        nk_console* audio = nk_console_button(menu->console, "Audio");
        nk_console_button_onclick(audio, "Back", nk_console_button_back);
        nk_console* volWidget = nk_console_property_int(audio, "Volume", 0, &menu->volumePercent, 100, 5, 1.0f);
        nk_console_add_event(volWidget, NK_CONSOLE_EVENT_CHANGED, SDL_LibretroMenu_OnVolumeChanged);

        // Video
        nk_console* video = nk_console_button(menu->console, "Video");
        nk_console_button_onclick(video, "Back", nk_console_button_back);
        nk_console* fitWidget = nk_console_combobox(video, "Fit Mode", "Aspect|Integer|Stretch", '|', &menu->fitModeIndex);
        nk_console_add_event(fitWidget, NK_CONSOLE_EVENT_CHANGED, SDL_LibretroMenu_OnFitModeChanged);

        // Reset
        nk_console_button_onclick(menu->console, "Reset", SDL_LibretroMenu_OnReset);
    }

    nk_console_button_onclick(menu->console, "Quit", SDL_LibretroMenu_OnQuit);
}

SDL_LibretroMenu* SDL_Libretro_CreateMenu(SDL_Libretro* lr) {
    if (!lr) return NULL;

    SDL_Renderer* renderer = SDL_Libretro_GetRenderer(lr);
    if (!renderer) return NULL;

    SDL_Window* window = SDL_GetRenderWindow(renderer);
    if (!window) return NULL;

    SDL_LibretroMenu* menu = (SDL_LibretroMenu*)SDL_calloc(1, sizeof(SDL_LibretroMenu));
    if (!menu) return NULL;

    menu->lr = lr;
    menu->open = false;
    menu->shouldQuit = false;

    menu->fontScale = SDL_GetWindowDisplayScale(window);
    if (menu->fontScale < 1.0f) menu->fontScale = 1.0f;

    menu->ctx = nk_sdl_init(window, renderer, nk_sdl_allocator());
    if (!menu->ctx) {
        SDL_free(menu);
        return NULL;
    }

    {
        struct nk_font_config config = nk_font_config(0);
        struct nk_font_atlas* atlas = nk_sdl_font_stash_begin(menu->ctx);
        struct nk_font* font = nk_font_atlas_add_default(atlas, SDL_LIBRETRO_MENU_FONT_SIZE * menu->fontScale, &config);
        nk_sdl_font_stash_end(menu->ctx);
        font->handle.height /= menu->fontScale;
        nk_style_set_font(menu->ctx, &font->handle);
    }

    nk_gamepad_init(&menu->gamepads, menu->ctx, NULL);

    menu->volumePercent = (int)(SDL_Libretro_GetVolume(lr) * 100.0f);
    menu->fitModeIndex = (int)SDL_Libretro_GetFitMode(lr);

    SDL_LibretroMenu_BuildConsole(menu);

    nk_input_begin(menu->ctx);

    return menu;
}

void SDL_Libretro_DestroyMenu(SDL_LibretroMenu* menu) {
    if (!menu) return;

    nk_input_end(menu->ctx);

    if (menu->console) {
        nk_console_free(menu->console);
    }
    nk_gamepad_free(&menu->gamepads);
    nk_sdl_shutdown(menu->ctx);
    SDL_free(menu);
}

bool SDL_Libretro_IsMenuOpen(const SDL_LibretroMenu* menu) {
    return menu && menu->open;
}

void SDL_Libretro_SetMenuOpen(SDL_LibretroMenu* menu, bool open) {
    if (!menu) return;

    if (open && !menu->open) {
        menu->volumePercent = (int)(SDL_Libretro_GetVolume(menu->lr) * 100.0f);
        menu->fitModeIndex = (int)SDL_Libretro_GetFitMode(menu->lr);
        SDL_LibretroMenu_BuildConsole(menu);
    }

    menu->open = open;
}

bool SDL_Libretro_MenuHandleEvent(SDL_LibretroMenu* menu, SDL_Event* event) {
    if (!menu) return false;

    if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
        if (event->key.key == SDLK_F1 || event->key.key == SDLK_ESCAPE) {
            menu->open = !menu->open;
            if (menu->open) {
                menu->volumePercent = (int)(SDL_Libretro_GetVolume(menu->lr) * 100.0f);
                menu->fitModeIndex = (int)SDL_Libretro_GetFitMode(menu->lr);
                SDL_LibretroMenu_BuildConsole(menu);
            }
            return true;
        }
    }
    else if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        if (event->gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE) {
            menu->open = !menu->open;
            if (menu->open) {
                menu->volumePercent = (int)(SDL_Libretro_GetVolume(menu->lr) * 100.0f);
                menu->fitModeIndex = (int)SDL_Libretro_GetFitMode(menu->lr);
                SDL_LibretroMenu_BuildConsole(menu);
            }
            return true;
        }
    }

    if (menu->open) {
        SDL_Renderer* renderer = SDL_Libretro_GetRenderer(menu->lr);
        if (renderer) {
            SDL_ConvertEventToRenderCoordinates(renderer, event);
        }
        nk_sdl_handle_event(menu->ctx, event);
        nk_gamepad_sdl3_handle_event(&menu->gamepads, event);
        return true;
    }

    return false;
}

void SDL_Libretro_UpdateMenu(SDL_LibretroMenu* menu) {
    if (!menu || !menu->open) return;

    nk_input_end(menu->ctx);
    nk_gamepad_update(&menu->gamepads);
}

void SDL_Libretro_RenderMenu(SDL_LibretroMenu* menu) {
    if (!menu || !menu->open) return;

    SDL_Renderer* renderer = SDL_Libretro_GetRenderer(menu->lr);
    SDL_Window* window = SDL_GetRenderWindow(renderer);

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    float scale = SDL_GetWindowDisplayScale(window);
    float lw = (float)w / scale;
    float lh = (float)h / scale;

    nk_uint flags = NK_WINDOW_SCROLL_AUTO_HIDE | NK_WINDOW_TITLE;
    nk_console_render_window(menu->console, SDL_Libretro_GetCoreName(menu->lr) ? SDL_Libretro_GetCoreName(menu->lr) : "Menu", nk_rect(0, 0, lw, lh), flags);

    nk_sdl_render(menu->ctx, NK_ANTI_ALIASING_ON);

    if (menu->console) {
        nk_console_sdl_update_text_input(menu->console, window);
    }

    nk_input_begin(menu->ctx);

    if (menu->shouldQuit) {
        SDL_Event quit_event;
        SDL_zero(quit_event);
        quit_event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quit_event);
    }
}

#endif /* SDL_LIBRETRO_MENU_IMPLEMENTATION_ONCE */
#endif /* SDL_LIBRETRO_MENU_IMPLEMENTATION */

#endif /* SDL_LIBRETRO_MENU_H */
