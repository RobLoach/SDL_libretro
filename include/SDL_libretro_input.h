#if defined(SDL_LIBRETRO_IMPLEMENTATION) && !defined(SDL_LIBRETRO_INPUT_IMPL_ONCE)
#define SDL_LIBRETRO_INPUT_IMPL_ONCE

/*
 * SDL_libretro - input subsystem
 */

/**
 * Ports a RETRO_DEVICE_ID_JOYPAD_* to a SDL_GamepadButton.
 *
 * @see RETRO_DEVICE_ID_JOYPAD_B
 * @see SDL_GAMEPAD_BUTTON_SOUTH
 */
static SDL_GamepadButton SDL_Libretro_RetroJoypadToGamepadButton(unsigned button) {
    switch (button) {
        case RETRO_DEVICE_ID_JOYPAD_B:       return SDL_GAMEPAD_BUTTON_SOUTH;
        case RETRO_DEVICE_ID_JOYPAD_Y:       return SDL_GAMEPAD_BUTTON_WEST;
        case RETRO_DEVICE_ID_JOYPAD_SELECT:  return SDL_GAMEPAD_BUTTON_BACK;
        case RETRO_DEVICE_ID_JOYPAD_START:   return SDL_GAMEPAD_BUTTON_START;
        case RETRO_DEVICE_ID_JOYPAD_UP:      return SDL_GAMEPAD_BUTTON_DPAD_UP;
        case RETRO_DEVICE_ID_JOYPAD_DOWN:    return SDL_GAMEPAD_BUTTON_DPAD_DOWN;
        case RETRO_DEVICE_ID_JOYPAD_LEFT:    return SDL_GAMEPAD_BUTTON_DPAD_LEFT;
        case RETRO_DEVICE_ID_JOYPAD_RIGHT:   return SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
        case RETRO_DEVICE_ID_JOYPAD_A:       return SDL_GAMEPAD_BUTTON_EAST;
        case RETRO_DEVICE_ID_JOYPAD_X:       return SDL_GAMEPAD_BUTTON_NORTH;
        case RETRO_DEVICE_ID_JOYPAD_L:       return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
        case RETRO_DEVICE_ID_JOYPAD_R:       return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
        case RETRO_DEVICE_ID_JOYPAD_L3:      return SDL_GAMEPAD_BUTTON_LEFT_STICK;
        case RETRO_DEVICE_ID_JOYPAD_R3:      return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
        default:                             return SDL_GAMEPAD_BUTTON_INVALID;
    }
}

/**
 * Ports a RETROK_* to a SDL_SCANCODE_*
 *
 * @see RETROK_BACKSPACE
 * @see SDL_SCANCODE_BACKSPACE
 * @see SDL_Libretro_ScancodeToRetroKey()
 */
static SDL_Scancode SDL_Libretro_RetroKeyToScancode(unsigned key) {
    switch (key) {
        case RETROK_BACKSPACE:    return SDL_SCANCODE_BACKSPACE;
        case RETROK_TAB:          return SDL_SCANCODE_TAB;
        case RETROK_RETURN:       return SDL_SCANCODE_RETURN;
        case RETROK_ESCAPE:       return SDL_SCANCODE_ESCAPE;
        case RETROK_SPACE:        return SDL_SCANCODE_SPACE;
        case RETROK_EXCLAIM:      return SDL_SCANCODE_1;
        case RETROK_QUOTEDBL:     return SDL_SCANCODE_APOSTROPHE;
        case RETROK_HASH:         return SDL_SCANCODE_3;
        case RETROK_DOLLAR:       return SDL_SCANCODE_4;
        case RETROK_AMPERSAND:    return SDL_SCANCODE_7;
        case RETROK_QUOTE:        return SDL_SCANCODE_APOSTROPHE;
        case RETROK_LEFTPAREN:    return SDL_SCANCODE_9;
        case RETROK_RIGHTPAREN:   return SDL_SCANCODE_0;
        case RETROK_ASTERISK:     return SDL_SCANCODE_8;
        case RETROK_PLUS:         return SDL_SCANCODE_EQUALS;
        case RETROK_COMMA:        return SDL_SCANCODE_COMMA;
        case RETROK_MINUS:        return SDL_SCANCODE_MINUS;
        case RETROK_PERIOD:       return SDL_SCANCODE_PERIOD;
        case RETROK_SLASH:        return SDL_SCANCODE_SLASH;
        case RETROK_0:            return SDL_SCANCODE_0;
        case RETROK_1:            return SDL_SCANCODE_1;
        case RETROK_2:            return SDL_SCANCODE_2;
        case RETROK_3:            return SDL_SCANCODE_3;
        case RETROK_4:            return SDL_SCANCODE_4;
        case RETROK_5:            return SDL_SCANCODE_5;
        case RETROK_6:            return SDL_SCANCODE_6;
        case RETROK_7:            return SDL_SCANCODE_7;
        case RETROK_8:            return SDL_SCANCODE_8;
        case RETROK_9:            return SDL_SCANCODE_9;
        case RETROK_SEMICOLON:    return SDL_SCANCODE_SEMICOLON;
        case RETROK_EQUALS:       return SDL_SCANCODE_EQUALS;
        case RETROK_LEFTBRACKET:  return SDL_SCANCODE_LEFTBRACKET;
        case RETROK_BACKSLASH:    return SDL_SCANCODE_BACKSLASH;
        case RETROK_RIGHTBRACKET: return SDL_SCANCODE_RIGHTBRACKET;
        case RETROK_BACKQUOTE:    return SDL_SCANCODE_GRAVE;
        case RETROK_a:            return SDL_SCANCODE_A;
        case RETROK_b:            return SDL_SCANCODE_B;
        case RETROK_c:            return SDL_SCANCODE_C;
        case RETROK_d:            return SDL_SCANCODE_D;
        case RETROK_e:            return SDL_SCANCODE_E;
        case RETROK_f:            return SDL_SCANCODE_F;
        case RETROK_g:            return SDL_SCANCODE_G;
        case RETROK_h:            return SDL_SCANCODE_H;
        case RETROK_i:            return SDL_SCANCODE_I;
        case RETROK_j:            return SDL_SCANCODE_J;
        case RETROK_k:            return SDL_SCANCODE_K;
        case RETROK_l:            return SDL_SCANCODE_L;
        case RETROK_m:            return SDL_SCANCODE_M;
        case RETROK_n:            return SDL_SCANCODE_N;
        case RETROK_o:            return SDL_SCANCODE_O;
        case RETROK_p:            return SDL_SCANCODE_P;
        case RETROK_q:            return SDL_SCANCODE_Q;
        case RETROK_r:            return SDL_SCANCODE_R;
        case RETROK_s:            return SDL_SCANCODE_S;
        case RETROK_t:            return SDL_SCANCODE_T;
        case RETROK_u:            return SDL_SCANCODE_U;
        case RETROK_v:            return SDL_SCANCODE_V;
        case RETROK_w:            return SDL_SCANCODE_W;
        case RETROK_x:            return SDL_SCANCODE_X;
        case RETROK_y:            return SDL_SCANCODE_Y;
        case RETROK_z:            return SDL_SCANCODE_Z;
        case RETROK_DELETE:       return SDL_SCANCODE_DELETE;
        case RETROK_KP0:          return SDL_SCANCODE_KP_0;
        case RETROK_KP1:          return SDL_SCANCODE_KP_1;
        case RETROK_KP2:          return SDL_SCANCODE_KP_2;
        case RETROK_KP3:          return SDL_SCANCODE_KP_3;
        case RETROK_KP4:          return SDL_SCANCODE_KP_4;
        case RETROK_KP5:          return SDL_SCANCODE_KP_5;
        case RETROK_KP6:          return SDL_SCANCODE_KP_6;
        case RETROK_KP7:          return SDL_SCANCODE_KP_7;
        case RETROK_KP8:          return SDL_SCANCODE_KP_8;
        case RETROK_KP9:          return SDL_SCANCODE_KP_9;
        case RETROK_KP_PERIOD:    return SDL_SCANCODE_KP_PERIOD;
        case RETROK_KP_DIVIDE:    return SDL_SCANCODE_KP_DIVIDE;
        case RETROK_KP_MULTIPLY:  return SDL_SCANCODE_KP_MULTIPLY;
        case RETROK_KP_MINUS:     return SDL_SCANCODE_KP_MINUS;
        case RETROK_KP_PLUS:      return SDL_SCANCODE_KP_PLUS;
        case RETROK_KP_ENTER:     return SDL_SCANCODE_KP_ENTER;
        case RETROK_KP_EQUALS:    return SDL_SCANCODE_KP_EQUALS;
        case RETROK_UP:           return SDL_SCANCODE_UP;
        case RETROK_DOWN:         return SDL_SCANCODE_DOWN;
        case RETROK_RIGHT:        return SDL_SCANCODE_RIGHT;
        case RETROK_LEFT:         return SDL_SCANCODE_LEFT;
        case RETROK_INSERT:       return SDL_SCANCODE_INSERT;
        case RETROK_HOME:         return SDL_SCANCODE_HOME;
        case RETROK_END:          return SDL_SCANCODE_END;
        case RETROK_PAGEUP:       return SDL_SCANCODE_PAGEUP;
        case RETROK_PAGEDOWN:     return SDL_SCANCODE_PAGEDOWN;
        case RETROK_F1:           return SDL_SCANCODE_F1;
        case RETROK_F2:           return SDL_SCANCODE_F2;
        case RETROK_F3:           return SDL_SCANCODE_F3;
        case RETROK_F4:           return SDL_SCANCODE_F4;
        case RETROK_F5:           return SDL_SCANCODE_F5;
        case RETROK_F6:           return SDL_SCANCODE_F6;
        case RETROK_F7:           return SDL_SCANCODE_F7;
        case RETROK_F8:           return SDL_SCANCODE_F8;
        case RETROK_F9:           return SDL_SCANCODE_F9;
        case RETROK_F10:          return SDL_SCANCODE_F10;
        case RETROK_F11:          return SDL_SCANCODE_F11;
        case RETROK_F12:          return SDL_SCANCODE_F12;
        case RETROK_F13:          return SDL_SCANCODE_F13;
        case RETROK_F14:          return SDL_SCANCODE_F14;
        case RETROK_F15:          return SDL_SCANCODE_F15;
        case RETROK_NUMLOCK:      return SDL_SCANCODE_NUMLOCKCLEAR;
        case RETROK_CAPSLOCK:     return SDL_SCANCODE_CAPSLOCK;
        case RETROK_SCROLLOCK:    return SDL_SCANCODE_SCROLLLOCK;
        case RETROK_RSHIFT:       return SDL_SCANCODE_RSHIFT;
        case RETROK_LSHIFT:       return SDL_SCANCODE_LSHIFT;
        case RETROK_RCTRL:        return SDL_SCANCODE_RCTRL;
        case RETROK_LCTRL:        return SDL_SCANCODE_LCTRL;
        case RETROK_RALT:         return SDL_SCANCODE_RALT;
        case RETROK_LALT:         return SDL_SCANCODE_LALT;
        case RETROK_RMETA:        return SDL_SCANCODE_RGUI;
        case RETROK_LMETA:        return SDL_SCANCODE_LGUI;
        case RETROK_RSUPER:       return SDL_SCANCODE_RGUI;
        case RETROK_LSUPER:       return SDL_SCANCODE_LGUI;
        case RETROK_MENU:         return SDL_SCANCODE_MENU;
        default:                  return SDL_SCANCODE_UNKNOWN;
    }
}

/**
 * Ports a SDL physical key to a RETROK_*.
 *
 * @see SDL_SCANCODE_BACKSPACE
 * @see RETROK_BACKSPACE
 * @see SDL_Libretro_RetroKeyToScancode()
 */
static unsigned SDL_Libretro_ScancodeToRetroKey(SDL_Scancode scancode) {
    switch (scancode) {
        case SDL_SCANCODE_BACKSPACE:    return RETROK_BACKSPACE;
        case SDL_SCANCODE_TAB:          return RETROK_TAB;
        case SDL_SCANCODE_RETURN:       return RETROK_RETURN;
        case SDL_SCANCODE_ESCAPE:       return RETROK_ESCAPE;
        case SDL_SCANCODE_SPACE:        return RETROK_SPACE;
        case SDL_SCANCODE_APOSTROPHE:   return RETROK_QUOTE;
        case SDL_SCANCODE_COMMA:        return RETROK_COMMA;
        case SDL_SCANCODE_MINUS:        return RETROK_MINUS;
        case SDL_SCANCODE_PERIOD:       return RETROK_PERIOD;
        case SDL_SCANCODE_SLASH:        return RETROK_SLASH;
        case SDL_SCANCODE_0:            return RETROK_0;
        case SDL_SCANCODE_1:            return RETROK_1;
        case SDL_SCANCODE_2:            return RETROK_2;
        case SDL_SCANCODE_3:            return RETROK_3;
        case SDL_SCANCODE_4:            return RETROK_4;
        case SDL_SCANCODE_5:            return RETROK_5;
        case SDL_SCANCODE_6:            return RETROK_6;
        case SDL_SCANCODE_7:            return RETROK_7;
        case SDL_SCANCODE_8:            return RETROK_8;
        case SDL_SCANCODE_9:            return RETROK_9;
        case SDL_SCANCODE_SEMICOLON:    return RETROK_SEMICOLON;
        case SDL_SCANCODE_EQUALS:       return RETROK_EQUALS;
        case SDL_SCANCODE_LEFTBRACKET:  return RETROK_LEFTBRACKET;
        case SDL_SCANCODE_BACKSLASH:    return RETROK_BACKSLASH;
        case SDL_SCANCODE_RIGHTBRACKET: return RETROK_RIGHTBRACKET;
        case SDL_SCANCODE_GRAVE:        return RETROK_BACKQUOTE;
        case SDL_SCANCODE_A:            return RETROK_a;
        case SDL_SCANCODE_B:            return RETROK_b;
        case SDL_SCANCODE_C:            return RETROK_c;
        case SDL_SCANCODE_D:            return RETROK_d;
        case SDL_SCANCODE_E:            return RETROK_e;
        case SDL_SCANCODE_F:            return RETROK_f;
        case SDL_SCANCODE_G:            return RETROK_g;
        case SDL_SCANCODE_H:            return RETROK_h;
        case SDL_SCANCODE_I:            return RETROK_i;
        case SDL_SCANCODE_J:            return RETROK_j;
        case SDL_SCANCODE_K:            return RETROK_k;
        case SDL_SCANCODE_L:            return RETROK_l;
        case SDL_SCANCODE_M:            return RETROK_m;
        case SDL_SCANCODE_N:            return RETROK_n;
        case SDL_SCANCODE_O:            return RETROK_o;
        case SDL_SCANCODE_P:            return RETROK_p;
        case SDL_SCANCODE_Q:            return RETROK_q;
        case SDL_SCANCODE_R:            return RETROK_r;
        case SDL_SCANCODE_S:            return RETROK_s;
        case SDL_SCANCODE_T:            return RETROK_t;
        case SDL_SCANCODE_U:            return RETROK_u;
        case SDL_SCANCODE_V:            return RETROK_v;
        case SDL_SCANCODE_W:            return RETROK_w;
        case SDL_SCANCODE_X:            return RETROK_x;
        case SDL_SCANCODE_Y:            return RETROK_y;
        case SDL_SCANCODE_Z:            return RETROK_z;
        case SDL_SCANCODE_DELETE:       return RETROK_DELETE;
        case SDL_SCANCODE_KP_0:         return RETROK_KP0;
        case SDL_SCANCODE_KP_1:         return RETROK_KP1;
        case SDL_SCANCODE_KP_2:         return RETROK_KP2;
        case SDL_SCANCODE_KP_3:         return RETROK_KP3;
        case SDL_SCANCODE_KP_4:         return RETROK_KP4;
        case SDL_SCANCODE_KP_5:         return RETROK_KP5;
        case SDL_SCANCODE_KP_6:         return RETROK_KP6;
        case SDL_SCANCODE_KP_7:         return RETROK_KP7;
        case SDL_SCANCODE_KP_8:         return RETROK_KP8;
        case SDL_SCANCODE_KP_9:         return RETROK_KP9;
        case SDL_SCANCODE_KP_PERIOD:    return RETROK_KP_PERIOD;
        case SDL_SCANCODE_KP_DIVIDE:    return RETROK_KP_DIVIDE;
        case SDL_SCANCODE_KP_MULTIPLY:  return RETROK_KP_MULTIPLY;
        case SDL_SCANCODE_KP_MINUS:     return RETROK_KP_MINUS;
        case SDL_SCANCODE_KP_PLUS:      return RETROK_KP_PLUS;
        case SDL_SCANCODE_KP_ENTER:     return RETROK_KP_ENTER;
        case SDL_SCANCODE_KP_EQUALS:    return RETROK_KP_EQUALS;
        case SDL_SCANCODE_UP:           return RETROK_UP;
        case SDL_SCANCODE_DOWN:         return RETROK_DOWN;
        case SDL_SCANCODE_RIGHT:        return RETROK_RIGHT;
        case SDL_SCANCODE_LEFT:         return RETROK_LEFT;
        case SDL_SCANCODE_INSERT:       return RETROK_INSERT;
        case SDL_SCANCODE_HOME:         return RETROK_HOME;
        case SDL_SCANCODE_END:          return RETROK_END;
        case SDL_SCANCODE_PAGEUP:       return RETROK_PAGEUP;
        case SDL_SCANCODE_PAGEDOWN:     return RETROK_PAGEDOWN;
        case SDL_SCANCODE_F1:           return RETROK_F1;
        case SDL_SCANCODE_F2:           return RETROK_F2;
        case SDL_SCANCODE_F3:           return RETROK_F3;
        case SDL_SCANCODE_F4:           return RETROK_F4;
        case SDL_SCANCODE_F5:           return RETROK_F5;
        case SDL_SCANCODE_F6:           return RETROK_F6;
        case SDL_SCANCODE_F7:           return RETROK_F7;
        case SDL_SCANCODE_F8:           return RETROK_F8;
        case SDL_SCANCODE_F9:           return RETROK_F9;
        case SDL_SCANCODE_F10:          return RETROK_F10;
        case SDL_SCANCODE_F11:          return RETROK_F11;
        case SDL_SCANCODE_F12:          return RETROK_F12;
        case SDL_SCANCODE_F13:          return RETROK_F13;
        case SDL_SCANCODE_F14:          return RETROK_F14;
        case SDL_SCANCODE_F15:          return RETROK_F15;
        case SDL_SCANCODE_NUMLOCKCLEAR: return RETROK_NUMLOCK;
        case SDL_SCANCODE_CAPSLOCK:     return RETROK_CAPSLOCK;
        case SDL_SCANCODE_SCROLLLOCK:   return RETROK_SCROLLOCK;
        case SDL_SCANCODE_RSHIFT:       return RETROK_RSHIFT;
        case SDL_SCANCODE_LSHIFT:       return RETROK_LSHIFT;
        case SDL_SCANCODE_RCTRL:        return RETROK_RCTRL;
        case SDL_SCANCODE_LCTRL:        return RETROK_LCTRL;
        case SDL_SCANCODE_RALT:         return RETROK_RALT;
        case SDL_SCANCODE_LALT:         return RETROK_LALT;
        case SDL_SCANCODE_RGUI:         return RETROK_RMETA;
        case SDL_SCANCODE_LGUI:         return RETROK_LMETA;
        case SDL_SCANCODE_MENU:         return RETROK_MENU;
        default:                        return RETROK_UNKNOWN;
    }
}

/**
 * Map SDL modifier flags to the RETROKMOD_* bitmask the core expects.
 *
 * @internal
 */
static uint16_t SDL_Libretro_KeymodToRetroMod(SDL_Keymod mod) {
    uint16_t retromod = RETROKMOD_NONE;
    if (mod & SDL_KMOD_SHIFT)  retromod |= RETROKMOD_SHIFT;
    if (mod & SDL_KMOD_CTRL)   retromod |= RETROKMOD_CTRL;
    if (mod & SDL_KMOD_ALT)    retromod |= RETROKMOD_ALT;
    if (mod & SDL_KMOD_GUI)    retromod |= RETROKMOD_META;
    if (mod & SDL_KMOD_NUM)    retromod |= RETROKMOD_NUMLOCK;
    if (mod & SDL_KMOD_CAPS)   retromod |= RETROKMOD_CAPSLOCK;
    if (mod & SDL_KMOD_SCROLL) retromod |= RETROKMOD_SCROLLOCK;
    return retromod;
}

static void SDL_Libretro_InputPoll(void) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr) return;

    lr->core.inputLastMouseX = lr->core.inputMouseX;
    lr->core.inputLastMouseY = lr->core.inputMouseY;
    SDL_GetMouseState(&lr->core.inputMouseX, &lr->core.inputMouseY);
}

static bool SDL_Libretro_IsKeyDown(SDL_Libretro* lr, SDL_Scancode scancode) {
    if (scancode == SDL_SCANCODE_UNKNOWN) return false;
    const bool* state = SDL_GetKeyboardState(NULL);
    return state[scancode];
}

static int16_t SDL_Libretro_InputState(unsigned port, unsigned device, unsigned index, unsigned id) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr) return 0;

    // During a backward step's throwaway re-run, report neutral input so the
    // discarded frame doesn't react to currently-held buttons.
    if (lr->rewindActive) return 0;

    unsigned baseDevice = device & RETRO_DEVICE_MASK;

    switch (baseDevice) {
        case RETRO_DEVICE_JOYPAD: {
            if (id == RETRO_DEVICE_ID_JOYPAD_MASK) {
                int16_t mask = 0;
                for (unsigned b = 0; b <= RETRO_DEVICE_ID_JOYPAD_R3; b++) {
                    if (SDL_Libretro_InputState(port, device, index, b)) {
                        mask |= (1 << b);
                    }
                }
                return mask;
            }

            /* Virtual joypad (port 0 only) */
            if (port == 0 && id < 16 && lr->core.virtualJoypadState[id]) {
                return 1;
            }

            /* Keyboard mapping (port 0) */
            if (port == 0 && id <= RETRO_DEVICE_ID_JOYPAD_R3) {
                if (SDL_Libretro_IsKeyDown(lr, lr->keyboardPlayer1[id])) {
                    return 1;
                }
            }

            /* Gamepad */
            if (port < 16 && lr->gamepads[port]) {
                /* L2/R2 are axes, not buttons */
                if (id == RETRO_DEVICE_ID_JOYPAD_L2) {
                    Sint16 val = SDL_GetGamepadAxis(lr->gamepads[port], SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
                    return val > 8192 ? 1 : 0;
                }
                if (id == RETRO_DEVICE_ID_JOYPAD_R2) {
                    Sint16 val = SDL_GetGamepadAxis(lr->gamepads[port], SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
                    return val > 8192 ? 1 : 0;
                }

                SDL_GamepadButton btn = SDL_Libretro_RetroJoypadToGamepadButton(id);
                if (btn != SDL_GAMEPAD_BUTTON_INVALID) {
                    return SDL_GetGamepadButton(lr->gamepads[port], btn) ? 1 : 0;
                }
            }
            return 0;
        }

        case RETRO_DEVICE_ANALOG: {
            if (port < 16 && lr->gamepads[port]) {
                SDL_GamepadAxis axis = SDL_GAMEPAD_AXIS_INVALID;
                if (index == RETRO_DEVICE_INDEX_ANALOG_LEFT) {
                    axis = (id == RETRO_DEVICE_ID_ANALOG_X) ? SDL_GAMEPAD_AXIS_LEFTX : SDL_GAMEPAD_AXIS_LEFTY;
                } else if (index == RETRO_DEVICE_INDEX_ANALOG_RIGHT) {
                    axis = (id == RETRO_DEVICE_ID_ANALOG_X) ? SDL_GAMEPAD_AXIS_RIGHTX : SDL_GAMEPAD_AXIS_RIGHTY;
                }
                if (axis != SDL_GAMEPAD_AXIS_INVALID) {
                    return SDL_GetGamepadAxis(lr->gamepads[port], axis);
                }
            }
            return 0;
        }

        case RETRO_DEVICE_KEYBOARD: {
            SDL_Scancode sc = SDL_Libretro_RetroKeyToScancode(id);
            return SDL_Libretro_IsKeyDown(lr, sc) ? 1 : 0;
        }

        case RETRO_DEVICE_MOUSE: {
            switch (id) {
                case RETRO_DEVICE_ID_MOUSE_X:
                    return (int16_t)(lr->core.inputMouseX - lr->core.inputLastMouseX);
                case RETRO_DEVICE_ID_MOUSE_Y:
                    return (int16_t)(lr->core.inputMouseY - lr->core.inputLastMouseY);
                case RETRO_DEVICE_ID_MOUSE_LEFT: {
                    Uint32 state = SDL_GetMouseState(NULL, NULL);
                    return (state & SDL_BUTTON_LMASK) ? 1 : 0;
                }
                case RETRO_DEVICE_ID_MOUSE_RIGHT: {
                    Uint32 state = SDL_GetMouseState(NULL, NULL);
                    return (state & SDL_BUTTON_RMASK) ? 1 : 0;
                }
                case RETRO_DEVICE_ID_MOUSE_MIDDLE: {
                    Uint32 state = SDL_GetMouseState(NULL, NULL);
                    return (state & SDL_BUTTON_MMASK) ? 1 : 0;
                }
            }
            return 0;
        }

        case RETRO_DEVICE_POINTER: {
            float mx = lr->core.inputMouseX;
            float my = lr->core.inputMouseY;
            SDL_FRect r = lr->core.renderDstRect;
            bool inside = r.w > 0 && r.h > 0 &&
                mx >= r.x && mx < r.x + r.w &&
                my >= r.y && my < r.y + r.h;

            if (index > 0) return 0;

            switch (id) {
                case RETRO_DEVICE_ID_POINTER_X:
                    if (r.w <= 0) return 0;
                    return (int16_t)(((mx - r.x) / r.w * 2.0f - 1.0f) * 0x7FFF);
                case RETRO_DEVICE_ID_POINTER_Y:
                    if (r.h <= 0) return 0;
                    return (int16_t)(((my - r.y) / r.h * 2.0f - 1.0f) * 0x7FFF);
                case RETRO_DEVICE_ID_POINTER_PRESSED: {
                    Uint32 state = SDL_GetMouseState(NULL, NULL);
                    return (state & SDL_BUTTON_LMASK) ? 1 : 0;
                }
                case RETRO_DEVICE_ID_POINTER_COUNT: {
                    Uint32 state = SDL_GetMouseState(NULL, NULL);
                    return (state & SDL_BUTTON_LMASK) ? 1 : 0;
                }
                case RETRO_DEVICE_ID_POINTER_IS_OFFSCREEN:
                    return inside ? 0 : 1;
            }
            return 0;
        }

        case RETRO_DEVICE_LIGHTGUN: {
            if (id == RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X || id == RETRO_DEVICE_ID_LIGHTGUN_X) {
                int w = 0, h = 0;
                if (lr->core.window) SDL_GetWindowSize(lr->core.window, &w, &h);
                if (w > 0) return (int16_t)((lr->core.inputMouseX / (float)w) * 32767.0f * 2.0f - 32767.0f);
            }
            if (id == RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y || id == RETRO_DEVICE_ID_LIGHTGUN_Y) {
                int w = 0, h = 0;
                if (lr->core.window) SDL_GetWindowSize(lr->core.window, &w, &h);
                if (h > 0) return (int16_t)((lr->core.inputMouseY / (float)h) * 32767.0f * 2.0f - 32767.0f);
            }
            if (id == RETRO_DEVICE_ID_LIGHTGUN_TRIGGER) {
                Uint32 state = SDL_GetMouseState(NULL, NULL);
                return (state & SDL_BUTTON_LMASK) ? 1 : 0;
            }
            return 0;
        }

        default:
            return 0;
    }
}

void SDL_Libretro_HandleEvent(SDL_Libretro* lr, const SDL_Event* event) {
    if (!lr || !event) return;

    switch (event->type) {
        case SDL_EVENT_GAMEPAD_ADDED: {
            SDL_JoystickID jid = event->gdevice.which;
            SDL_Gamepad* gp = SDL_OpenGamepad(jid);
            if (gp) {
                for (unsigned i = 0; i < 16; i++) {
                    if (!lr->gamepads[i]) {
                        lr->gamepads[i] = gp;
                        if (i + 1 > lr->gamepadCount) lr->gamepadCount = i + 1;
                        SDL_Log("SDL_libretro: Gamepad added to port %u", i);
                        break;
                    }
                }
            }
            break;
        }
        case SDL_EVENT_GAMEPAD_REMOVED: {
            SDL_JoystickID jid = event->gdevice.which;
            for (unsigned i = 0; i < 16; i++) {
                if (lr->gamepads[i] && SDL_GetGamepadID(lr->gamepads[i]) == jid) {
                    SDL_CloseGamepad(lr->gamepads[i]);
                    lr->gamepads[i] = NULL;
                    SDL_Log("SDL_libretro: Gamepad removed from port %u", i);
                    break;
                }
            }
            break;
        }
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            if (lr->core.keyboard_event) {
                bool down = (event->type == SDL_EVENT_KEY_DOWN);
                unsigned retrokey = SDL_Libretro_ScancodeToRetroKey(event->key.scancode);
                uint16_t modmask = SDL_Libretro_KeymodToRetroMod(event->key.mod);

                // Determine the character if one is available for the key event.
                uint32_t character = 0;
                if (event->key.key > 0x20 && event->key.key < 0x110000 &&
                    !(event->key.key & SDLK_SCANCODE_MASK)) {
                    character = (uint32_t)event->key.key;
                }

                lr->core.keyboard_event(down, retrokey, character, modmask);
            }
            break;
        }
        default:
            break;
    }
}

bool SDL_Libretro_SetPortDevice(SDL_Libretro* lr, unsigned port, unsigned device) {
    if (!lr || port >= 16) return false;
    lr->core.portDeviceMap[port] = device;
    if (lr->core.loaded) {
        lr->core.symbols.retro_set_controller_port_device(port, device);
    }
    return true;
}

void SDL_Libretro_SetKeyboardMapping(SDL_Libretro* lr, int retroButton, SDL_Scancode scancode) {
    if (!lr || retroButton < 0 || retroButton > RETRO_DEVICE_ID_JOYPAD_R3) return;
    lr->keyboardPlayer1[retroButton] = scancode;
}

void SDL_Libretro_SetVirtualButton(SDL_Libretro* lr, unsigned port, int button, bool pressed) {
    if (!lr || port >= 16 || button < 0 || button >= 16) return;
    lr->core.virtualJoypadState[button] = pressed;
}

// Input Descriptors

/**
 * Retrieve the number of input descriptors provided by a Libretro core.
 *
 * @param lr Pointer to an initialized SDL_Libretro instance. Must not be NULL.
 *
 * @see SDL_Libretro_GetInputDescriptor
 */
unsigned SDL_Libretro_GetInputDescriptorCount(const SDL_Libretro* lr) {
    return (lr && lr->core.loaded) ? lr->core.inputDescriptorCount : 0;
}

/**
 * @brief Retrieve an input descriptor by index.
 *
 * This function can be used to enumerate all available input descriptors by calling it with incrementing index values starting at 0 until no descriptor is returned or an error is indicated.
 *
 * @param[in] index Zero-based index of the input descriptor to retrieve. The value should start at 0 and be incremented to enumerate successive descriptors.
 * @param[out] port Zero-based port number to query (e.g., controller port).
 * @param[out] device Device identifier within the port (if applicable).
 * @param[out] id The ID of the descriptor.
 * @param[out] description The description of the input device.
 *
 * @return 0 on success, a negative error code on failure.
 *
 * @note The contents written to @p out_desc are owned by the caller after the
 *       call returns and may be modified or freed as appropriate by the caller.
 *
 * @see retro_input_descriptor
 */
bool SDL_Libretro_GetInputDescriptor(const SDL_Libretro* lr, unsigned index, unsigned* port, unsigned* device, unsigned* id, const char** description) {
    if (!lr || !lr->core.loaded || index >= lr->core.inputDescriptorCount) return false;
    const struct retro_input_descriptor* d = &lr->core.inputDescriptors[index];
    if (port) *port = d->port;
    if (device) *device = d->device;
    if (id) *id = d->id;
    if (description) *description = d->description;
    return true;
}

// Sensors

static SDL_Sensor* SDL_Libretro_OpenSensorByType(SDL_SensorType type) {
    int count = 0;
    SDL_SensorID* sensors = SDL_GetSensors(&count);
    if (!sensors) return NULL;
    SDL_Sensor* result = NULL;
    for (int i = 0; i < count; i++) {
        if (SDL_GetSensorTypeForID(sensors[i]) == type) {
            result = SDL_OpenSensor(sensors[i]);
            break;
        }
    }
    SDL_free(sensors);
    return result;
}

static bool SDL_Libretro_SetSensorState(unsigned port, enum retro_sensor_action action, unsigned rate) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr || port >= SDL_LIBRETRO_SENSOR_PORTS) return false;
    (void)rate;

    switch (action) {
        case RETRO_SENSOR_ACCELEROMETER_ENABLE:
            if (!lr->core.sensorAccel[port]) {
                lr->core.sensorAccel[port] = SDL_Libretro_OpenSensorByType(SDL_SENSOR_ACCEL);
            }
            return lr->core.sensorAccel[port] != NULL;
        case RETRO_SENSOR_ACCELEROMETER_DISABLE:
            if (lr->core.sensorAccel[port]) {
                SDL_CloseSensor(lr->core.sensorAccel[port]);
                lr->core.sensorAccel[port] = NULL;
            }
            SDL_memset(lr->core.sensorAccelData[port], 0, sizeof(lr->core.sensorAccelData[port]));
            return true;
        case RETRO_SENSOR_GYROSCOPE_ENABLE:
            if (!lr->core.sensorGyro[port]) {
                lr->core.sensorGyro[port] = SDL_Libretro_OpenSensorByType(SDL_SENSOR_GYRO);
            }
            return lr->core.sensorGyro[port] != NULL;
        case RETRO_SENSOR_GYROSCOPE_DISABLE:
            if (lr->core.sensorGyro[port]) {
                SDL_CloseSensor(lr->core.sensorGyro[port]);
                lr->core.sensorGyro[port] = NULL;
            }
            SDL_memset(lr->core.sensorGyroData[port], 0, sizeof(lr->core.sensorGyroData[port]));
            return true;
        default:
            return false;
    }
}

static float SDL_Libretro_GetSensorInput(unsigned port, unsigned id) {
    SDL_Libretro* lr = SDL_Libretro_active;
    if (!lr || port >= SDL_LIBRETRO_SENSOR_PORTS) return 0.0f;

    if (id <= RETRO_SENSOR_ACCELEROMETER_Z && lr->core.sensorAccel[port]) {
        float data[3];
        if (SDL_GetSensorData(lr->core.sensorAccel[port], data, 3)) {
            lr->core.sensorAccelData[port][0] = data[0] / SDL_STANDARD_GRAVITY;
            lr->core.sensorAccelData[port][1] = data[1] / SDL_STANDARD_GRAVITY;
            lr->core.sensorAccelData[port][2] = data[2] / SDL_STANDARD_GRAVITY;
        }
        return lr->core.sensorAccelData[port][id - RETRO_SENSOR_ACCELEROMETER_X];
    }

    if (id >= RETRO_SENSOR_GYROSCOPE_X && id <= RETRO_SENSOR_GYROSCOPE_Z && lr->core.sensorGyro[port]) {
        float data[3];
        if (SDL_GetSensorData(lr->core.sensorGyro[port], data, 3)) {
            lr->core.sensorGyroData[port][0] = data[0];
            lr->core.sensorGyroData[port][1] = data[1];
            lr->core.sensorGyroData[port][2] = data[2];
        }
        return lr->core.sensorGyroData[port][id - RETRO_SENSOR_GYROSCOPE_X];
    }

    return 0.0f;
}

static void SDL_Libretro_CloseSensors(SDL_Libretro* lr) {
    for (unsigned i = 0; i < SDL_LIBRETRO_SENSOR_PORTS; i++) {
        if (lr->core.sensorAccel[i]) {
            SDL_CloseSensor(lr->core.sensorAccel[i]);
            lr->core.sensorAccel[i] = NULL;
        }
        if (lr->core.sensorGyro[i]) {
            SDL_CloseSensor(lr->core.sensorGyro[i]);
            lr->core.sensorGyro[i] = NULL;
        }
    }
}

#endif /* SDL_LIBRETRO_INPUT_IMPL_ONCE */
