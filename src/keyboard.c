/*
 * Copyright (c) 2026 Stunts Engine Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "keyboard.h"
#include "framebuffer.h"
#include "stunts.h"
#include <SDL3/SDL.h>
#include <string.h>

enum {
    KB_KEY_STATE_COUNT = 90,
    KB_EVENT_QUEUE_SIZE = 128,
    KB_EVENT_QUEUE_MASK = KB_EVENT_QUEUE_SIZE - 1,
    KB_CALLBACK_DIRECT_BINDING_COUNT = 128,
    KB_CALLBACK_EXTENDED_BINDING_COUNT = 134,
    KB_KEYCODE_SCANCODE_SHIFT = 8,
    KB_KEYCODE_LOW_BYTE_MASK = 255,
    KB_ASCII_CR = 13,
    KB_ASCII_TAB = 9,
    KB_ASCII_BS = 8,
    KB_ASCII_ESC = 27,
    KB_KEYCODE_UP = 0x4800,
    KB_KEYCODE_DOWN = 0x5000,
    KB_KEYCODE_LEFT = 0x4B00,
    KB_KEYCODE_RIGHT = 0x4D00,
    KB_SHIFTFLAG_NUMLOCK_BIT = 32,
    KB_SHIFTFLAG_NUMLOCK_CLEAR_MASK = 223,
    KB_CONTROLLER_AXIS_DEADZONE = 12000,
    KB_CALLBACK_SLOT_COUNT = 64,
    KB_CALLBACK_INDEX_BASE = 1,
    KB_CALLBACK_DIRECT_KEY_MASK = 127,
    KB_CALLBACK_EXTENDED_SCANCODE_MAX = 132,
    KB_JOYSTICK_NEUTRAL_POSITION = 80,
    KB_JOYSTICK_SCALED_CENTER = 31,
    KB_JOYSTICK_SCALE_SHIFT = 8
};

typedef struct {
    unsigned short key_state_index;
    unsigned short flags;
} KeyboardDigitalBinding;

typedef struct {
    unsigned short entries[KB_EVENT_QUEUE_SIZE];
    unsigned short head;
    unsigned short tail;
} KeyboardEventQueue;

typedef struct {
    void (*slots[KB_CALLBACK_SLOT_COUNT])(void);
    unsigned char direct_key_bindings[KB_CALLBACK_DIRECT_BINDING_COUNT];
    unsigned char extended_key_bindings[KB_CALLBACK_EXTENDED_BINDING_COUNT];
    bool dispatch_in_progress;
} KeyboardCallbackRegistry;

typedef struct {
    unsigned short neutral_x;
    unsigned short raw_x;
    unsigned short x_scale;
} JoystickCalibrationState;

typedef struct {
    bool initialized;
    KeyboardEventQueue queue;
    SDL_Gamepad *gamepad;
} InputBackendState;

unsigned short newjoyflags = 0;
unsigned short mouse_oldbut = 0;
unsigned short joyinputcode = 0;
unsigned short mousebutinputcode = 0;
unsigned const char g_ascii_props[]
    = { 32,  32,  32,  32,  32, 32, 32, 32,  32,  40,  40,  40,  40,  40,  32,  32,  32,  32,
        32,  32,  32,  32,  32, 32, 32, 32,  32,  32,  32,  32,  32,  32,  72,  16,  16,  16,
        16,  16,  16,  16,  16, 16, 16, 16,  16,  16,  16,  16,  132, 132, 132, 132, 132, 132,
        132, 132, 132, 132, 16, 16, 16, 16,  16,  16,  16,  129, 129, 129, 129, 129, 129, 1,
        1,   1,   1,   1,   1,  1,  1,  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
        1,   16,  16,  16,  16, 16, 16, 130, 130, 130, 130, 130, 130, 2,   2,   2,   2,   2,
        2,   2,   2,   2,   2,  2,  2,  2,   2,   2,   2,   2,   2,   2,   2,   16,  16,  16,
        16,  32,  0,   0,   0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0 };
bool game_startup_flag = false;
unsigned char kbinput[KB_KEY_STATE_COUNT] = { 0 };
unsigned short kbjoyflags = 0;
char mouse_button_press_state[2] = { 0, 0 };

static const KeyboardDigitalBinding g_keyboard_navigation_bindings[] = {
    { 57, KB_INPUT_FLAG_PRIMARY_ACTION },
    { 28, KB_INPUT_FLAG_SECONDARY_ACTION },
    { 71, KB_INPUT_FLAG_LEFT | KB_INPUT_FLAG_UP },
    { 72, KB_INPUT_FLAG_UP },
    { 73, KB_INPUT_FLAG_UP | KB_INPUT_FLAG_RIGHT },
    { 77, KB_INPUT_FLAG_RIGHT },
    { 81, KB_INPUT_FLAG_RIGHT | KB_INPUT_FLAG_DOWN },
    { 80, KB_INPUT_FLAG_DOWN },
    { 79, KB_INPUT_FLAG_LEFT | KB_INPUT_FLAG_DOWN },
    { 75, KB_INPUT_FLAG_LEFT }
};

static const unsigned char g_joystick_direction_lookup[16]
    = { 0, 1, 5, 0, 3, 2, 4, 3, 7, 8, 6, 7, 0, 1, 5, 0 };

static KeyboardCallbackRegistry g_callback_registry = { { 0 }, { 0 }, { 0 }, false };
static JoystickCalibrationState g_joystick_calibration = { KB_JOYSTICK_NEUTRAL_POSITION, 0, 0 };
static InputBackendState g_input_backend = { false, { { 0 }, 0, 0 }, 0 };
static unsigned short g_last_read_or_joy_flags = 0;

/* Forward declarations */
unsigned short kb_parse_key(unsigned short key);

/** @brief Translate an SDL scancode to the engine key-state index.
 */
static unsigned char
kb_translate_scancode(SDL_Scancode sc) {
    switch (sc) {
    case SDL_SCANCODE_ESCAPE:
        return 1;
    case SDL_SCANCODE_1:
        return 2;
    case SDL_SCANCODE_2:
        return 3;
    case SDL_SCANCODE_3:
        return 4;
    case SDL_SCANCODE_4:
        return 5;
    case SDL_SCANCODE_5:
        return 6;
    case SDL_SCANCODE_6:
        return 7;
    case SDL_SCANCODE_7:
        return 8;
    case SDL_SCANCODE_8:
        return 9;
    case SDL_SCANCODE_9:
        return 10;
    case SDL_SCANCODE_0:
        return 11;
    case SDL_SCANCODE_MINUS:
        return 12;
    case SDL_SCANCODE_EQUALS:
        return 13;
    case SDL_SCANCODE_BACKSPACE:
        return 14;
    case SDL_SCANCODE_TAB:
        return 15;
    case SDL_SCANCODE_Q:
        return 16;
    case SDL_SCANCODE_W:
        return 17;
    case SDL_SCANCODE_E:
        return 18;
    case SDL_SCANCODE_R:
        return 19;
    case SDL_SCANCODE_T:
        return 20;
    case SDL_SCANCODE_Y:
        return 21;
    case SDL_SCANCODE_U:
        return 22;
    case SDL_SCANCODE_I:
        return 23;
    case SDL_SCANCODE_O:
        return 24;
    case SDL_SCANCODE_P:
        return 25;
    case SDL_SCANCODE_LEFTBRACKET:
        return 26;
    case SDL_SCANCODE_RIGHTBRACKET:
        return 27;
    case SDL_SCANCODE_RETURN:
        return 28;
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL:
        return 29;
    case SDL_SCANCODE_A:
        return 30;
    case SDL_SCANCODE_S:
        return 31;
    case SDL_SCANCODE_D:
        return 32;
    case SDL_SCANCODE_F:
        return 33;
    case SDL_SCANCODE_G:
        return 34;
    case SDL_SCANCODE_H:
        return 35;
    case SDL_SCANCODE_J:
        return 36;
    case SDL_SCANCODE_K:
        return 37;
    case SDL_SCANCODE_L:
        return 38;
    case SDL_SCANCODE_SEMICOLON:
        return 39;
    case SDL_SCANCODE_APOSTROPHE:
        return 40;
    case SDL_SCANCODE_GRAVE:
        return 41;
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
        return 42;
    case SDL_SCANCODE_BACKSLASH:
        return 43;
    case SDL_SCANCODE_Z:
        return 44;
    case SDL_SCANCODE_X:
        return 45;
    case SDL_SCANCODE_C:
        return 46;
    case SDL_SCANCODE_V:
        return 47;
    case SDL_SCANCODE_B:
        return 48;
    case SDL_SCANCODE_N:
        return 49;
    case SDL_SCANCODE_M:
        return 50;
    case SDL_SCANCODE_COMMA:
        return 51;
    case SDL_SCANCODE_PERIOD:
        return 52;
    case SDL_SCANCODE_SLASH:
        return 53;
    case SDL_SCANCODE_SPACE:
        return 57;
    case SDL_SCANCODE_F1:
        return 59;
    case SDL_SCANCODE_F2:
        return 60;
    case SDL_SCANCODE_F3:
        return 61;
    case SDL_SCANCODE_F4:
        return 62;
    case SDL_SCANCODE_F5:
        return 63;
    case SDL_SCANCODE_F6:
        return 64;
    case SDL_SCANCODE_F7:
        return 65;
    case SDL_SCANCODE_F8:
        return 66;
    case SDL_SCANCODE_F9:
        return 67;
    case SDL_SCANCODE_F10:
        return 68;
    case SDL_SCANCODE_HOME:
        return 71;
    case SDL_SCANCODE_UP:
        return 72;
    case SDL_SCANCODE_PAGEUP:
        return 73;
    case SDL_SCANCODE_LEFT:
        return 75;
    case SDL_SCANCODE_RIGHT:
        return 77;
    case SDL_SCANCODE_END:
        return 79;
    case SDL_SCANCODE_DOWN:
        return 80;
    case SDL_SCANCODE_PAGEDOWN:
        return 81;
    case SDL_SCANCODE_INSERT:
        return 82;
    case SDL_SCANCODE_DELETE:
        return 83;
    default:
        return 0;
    }
}

/** @brief Translate an SDL key press into the engine's ASCII byte.
 */
static unsigned char
kb_translate_ascii(SDL_Keycode key, Uint16 mod) {
    if (key >= SDLK_A && key <= SDLK_Z) {
        unsigned char ch = (unsigned char)('a' + (key - SDLK_A));
        if ((mod & SDL_KMOD_SHIFT) != 0 || (mod & SDL_KMOD_CAPS) != 0) {
            ch = (unsigned char)(ch - ('a' - 'A'));
        }
        return ch;
    }
    if (key >= SDLK_0 && key <= SDLK_9) {
        return (unsigned char)('0' + (key - SDLK_0));
    }
    if (key == SDLK_SPACE)
        return ' ';
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
        return KB_ASCII_CR;
    if (key == SDLK_TAB)
        return KB_ASCII_TAB;
    if (key == SDLK_BACKSPACE)
        return KB_ASCII_BS;
    if (key == SDLK_ESCAPE)
        return KB_ASCII_ESC;
    return 0;
}

static unsigned short
kb_compose_keycode(unsigned char key_state_index, unsigned char ascii) {
    unsigned short keycode = (unsigned short)key_state_index << KB_KEYCODE_SCANCODE_SHIFT;
    if (ascii != 0) {
        keycode = (unsigned short)(keycode | ascii);
    }
    return keycode;
}

unsigned short
kb_input_flags_to_keycode(unsigned short input_flags) {
    if ((input_flags & KB_INPUT_FLAG_SECONDARY_ACTION) != 0) {
        return KB_ASCII_CR;
    }
    if ((input_flags & KB_INPUT_FLAG_PRIMARY_ACTION) != 0) {
        return ' ';
    }
    if ((input_flags & KB_INPUT_FLAG_UP) != 0) {
        return KB_KEYCODE_UP;
    }
    if ((input_flags & KB_INPUT_FLAG_DOWN) != 0) {
        return KB_KEYCODE_DOWN;
    }
    if ((input_flags & KB_INPUT_FLAG_LEFT) != 0) {
        return KB_KEYCODE_LEFT;
    }
    if ((input_flags & KB_INPUT_FLAG_RIGHT) != 0) {
        return KB_KEYCODE_RIGHT;
    }
    return 0;
}

/** @brief Push a key event into the buffered input queue.
 */
static void
kb_queue_push(unsigned short key) {
    unsigned short next = (unsigned short)((g_input_backend.queue.tail + 1u) & KB_EVENT_QUEUE_MASK);
    if (next == g_input_backend.queue.head) {
        return;
    }
    g_input_backend.queue.entries[g_input_backend.queue.tail] = key;
    g_input_backend.queue.tail = next;
}

/** @brief Pop the next buffered input keycode.
 */
static unsigned short
kb_queue_pop(void) {
    unsigned short key;
    if (g_input_backend.queue.head == g_input_backend.queue.tail) {
        return 0;
    }
    key = g_input_backend.queue.entries[g_input_backend.queue.head];
    g_input_backend.queue.head
        = (unsigned short)((g_input_backend.queue.head + 1u) & KB_EVENT_QUEUE_MASK);
    return key;
}

/** @brief Requeue a keycode for later processing.
 */
void
kb_requeue_key(unsigned short key) {
    kb_queue_push(key);
}

static int
kb_has_pending_input(void) {
    return (g_input_backend.queue.head != g_input_backend.queue.tail) ? 1 : 0;
}

static bool
kb_handle_host_shortcut(const SDL_KeyboardEvent *event) {
    SDL_Keycode key;
    SDL_Keymod modifiers;

    if (event->repeat != 0) {
        return false;
    }

    key = event->key;
    modifiers = event->mod;
    if ((key == SDLK_RETURN || key == SDLK_RETURN2) && (modifiers & SDL_KMOD_ALT) != 0) {
        video_toggle_fullscreen();
        return true;
    }
    if (key == SDLK_PLUS || key == SDLK_EQUALS || key == SDLK_KP_PLUS) {
        video_scale_up();
        return true;
    }
    if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
        video_scale_down();
        return true;
    }
    return false;
}

/** @brief Poll the active SDL input sources and update buffered state.
 */
void
kb_poll_input(void) {
    SDL_Event ev;
    if (!g_input_backend.initialized) {
        return;
    }

    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_EVENT_QUIT) {
            call_exitlist2();
            return;
        }

        if (ev.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            call_exitlist2();
            return;
        }

        if (ev.type == SDL_EVENT_KEY_DOWN || ev.type == SDL_EVENT_KEY_UP) {
            unsigned char key_state_index = kb_translate_scancode(ev.key.scancode);
            if (ev.type == SDL_EVENT_KEY_DOWN && kb_handle_host_shortcut(&ev.key)) {
                continue;
            }
            if (key_state_index != 0 && key_state_index < KB_KEY_STATE_COUNT) {
                unsigned char ascii = kb_translate_ascii(ev.key.key, ev.key.mod);
                kbinput[key_state_index] = (ev.type == SDL_EVENT_KEY_DOWN) ? 1u : 0u;
                if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.repeat == 0) {
                    kb_queue_push(kb_compose_keycode(key_state_index, ascii));
                }
            }
            continue;
        }

        if (ev.type == SDL_EVENT_GAMEPAD_ADDED && g_input_backend.gamepad == 0) {
            if (SDL_IsGamepad(ev.gdevice.which)) {
                g_input_backend.gamepad = SDL_OpenGamepad(ev.gdevice.which);
            }
            continue;
        }

        if (ev.type == SDL_EVENT_GAMEPAD_REMOVED && g_input_backend.gamepad != 0) {
            SDL_JoystickID jid = SDL_GetGamepadID(g_input_backend.gamepad);
            if (jid == ev.gdevice.which) {
                SDL_CloseGamepad(g_input_backend.gamepad);
                g_input_backend.gamepad = 0;
            }
            continue;
        }
    }

}

/** @brief Initialize keyboard input state and SDL input backends.
 */
void
kb_init_interrupt(void) {
    memset(kbinput, 0, sizeof(kbinput));
    g_input_backend.queue.head = 0;
    g_input_backend.queue.tail = 0;
    g_input_backend.initialized = true;
    g_last_read_or_joy_flags = 0;
    if ((SDL_WasInit(SDL_INIT_GAMEPAD) & SDL_INIT_GAMEPAD) == 0) {
        (void)SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    }
    {
        int gamepad_count = 0;
        SDL_JoystickID *gamepads = SDL_GetGamepads(&gamepad_count);
        if (gamepads != 0 && gamepad_count > 0) {
            g_input_backend.gamepad = SDL_OpenGamepad(gamepads[0]);
        }
        SDL_free(gamepads);
    }
    add_exit_handler(kb_exit_handler);
}

/** @brief Shutdown keyboard input handling and release SDL resources.
 */
void
kb_exit_handler(void) {

    if (g_input_backend.gamepad != 0) {
        SDL_CloseGamepad(g_input_backend.gamepad);
        g_input_backend.gamepad = 0;
    }
    g_input_backend.initialized = false;
}

/** @brief Return pressed-state information for a DOS scancode.
 */
int
kb_get_key_state(int key) {

    if (key < 0 || (unsigned int)key >= KB_KEY_STATE_COUNT) {
        return 0;
    }
    return kbinput[key];
}

/** @brief Invoke the installed read-char callback, if present.
 */
int
kb_call_readchar_callback(void) {

    // the orginal code uses a (hard-coded, non-changing) callback for
    // reading chars.. we just call kb_read_char() directly:
    return kb_read_char();
}

/** @brief Read one key code from SDL queue or DOS callback fallback.
 */
int
kb_read_char(void) {

    unsigned short key;
    kb_poll_input();
    key = kb_queue_pop();
    if (key == 0) {
        return 0;
    }
    kb_parse_key(key);
    return (int)(key & KB_KEYCODE_LOW_BYTE_MASK);
}

/** @brief Check whether keyboard input is currently available.
 */
int
kb_checking(void) {

    kb_poll_input();
    return kb_has_pending_input();
}

// Ported from seg018.asm - manipulate keyboard shift flags in BIOS Data Area
// 0040:0017 = keyboard shift flags byte, bit 5 = NumLock active state
static unsigned char kb_shift_flags_shadow = 0;

/** @brief Force the NumLock bit on in the keyboard shift-state shadow.
 */
void
kb_shift_checking1(void) {

    kb_shift_flags_shadow |= KB_SHIFTFLAG_NUMLOCK_BIT; // Set bit 5 (NumLock)
    kb_checking();
}

/** @brief Clear the NumLock bit in the keyboard shift-state shadow.
 */
void
kb_shift_checking2(void) {

    kb_shift_flags_shadow &= KB_SHIFTFLAG_NUMLOCK_CLEAR_MASK; // Clear bit 5 (NumLock)
    kb_checking();
}

/** @brief Return a non-zero value when a key event is pending.
 */
int
kb_check(void) {

    kb_poll_input();
    return kb_has_pending_input();
}

/* Joystick state used by kb_read_key_or_joy */

/**
 * kb_read_key_or_joy - Read a key; fall back to joystick (nopsub_304B6)
 *
 * Returns BIOS keycode in AX (AL=ASCII, AH=scancode) or 0 if no new input.
 * Keyboard has priority; joystick is debounced to avoid spamming repeats.
 */
unsigned short
kb_read_key_or_joy(void) {
    unsigned short keycode;
    unsigned short current_flags;
    unsigned short newly_pressed_flags;

    keycode = (unsigned short)kb_read_char();
    if (keycode != 0) {
        return keycode;
    }

    current_flags = (unsigned short)get_joy_flags();
    newly_pressed_flags = (unsigned short)(current_flags & (unsigned short)~g_last_read_or_joy_flags);
    g_last_read_or_joy_flags = current_flags;
    return kb_input_flags_to_keycode(newly_pressed_flags);
}

/**
 * joystick_init_calibration - Initialize joystick calibration
 * 
 * Sets joystick enabled flag and default calibration values.
 */
void
joystick_init_calibration(void) {
    joystick_assigned_flags = true;
    g_joystick_calibration.neutral_x = KB_JOYSTICK_NEUTRAL_POSITION;
    g_joystick_calibration.raw_x = 0;
    g_joystick_calibration.x_scale = 0;
}

/**
 * joystick_direction_lookup - Joystick direction lookup
 * 
 * Parameters:
 */
short
joystick_direction_lookup(unsigned short joy_flags) {
    return (short)g_joystick_direction_lookup[joy_flags & 15];
}

/**
 * joystick_get_scaled_x - Calculate joystick X axis position
 * 
 * Returns scaled joystick X position based on calibration.
 */
short
joystick_get_scaled_x(void) {
    short scaled_x;
    unsigned long result;

    scaled_x = (short)g_joystick_calibration.raw_x - g_joystick_calibration.neutral_x;
    if (scaled_x < 0)
        scaled_x = 0;

    result = (unsigned long)scaled_x * g_joystick_calibration.x_scale;
    scaled_x = (short)(result >> KB_JOYSTICK_SCALE_SHIFT);
    return (short)scaled_x - KB_JOYSTICK_SCALED_CENTER;
}

/* Keyboard scan code and input globals */

/**
 * get_kb_or_joy_flags - Get keyboard or joystick input flags
 * 
 * Checks keyboard input array against scancode bindings and returns
 * a bitmask of active inputs. If no keyboard input, checks joystick.
 * 
 * Bit flags:
 */
short
get_kb_or_joy_flags(void) {
    size_t binding_index;
    short flags = 0;

    for (binding_index = 0;
         binding_index < sizeof(g_keyboard_navigation_bindings) / sizeof(g_keyboard_navigation_bindings[0]);
         ++binding_index) {
        const KeyboardDigitalBinding *binding = &g_keyboard_navigation_bindings[binding_index];
        if (kbinput[binding->key_state_index] != 0) {
            flags |= (short)binding->flags;
        }
    }

    /* If no keyboard input, check joystick */
    if (flags == 0) {
        flags = get_joy_flags();
    }

    return flags;
}

/**
 * kb_get_char - Read a key from BIOS and update key state
 *
 * Returns 0 if no key is available; otherwise returns AX from BIOS
 */
int
kb_get_char(void) {
    unsigned short key_ax;

    kb_poll_input();
    key_ax = kb_queue_pop();
    if (key_ax == 0) {
        return 0;
    }

    kb_parse_key(key_ax);
    return key_ax;
}

/**
 * get_joy_flags - Poll joystick port and compute calibrated flags
 *
 * Returns a bitmask of joystick state using the original calibration logic:
 */
short
get_joy_flags(void) {
    unsigned short joy = 0;
    const bool *state;

    kb_poll_input();

    state = SDL_GetKeyboardState(0);
    if (state[SDL_SCANCODE_UP])
        joy |= KB_INPUT_FLAG_UP;
    if (state[SDL_SCANCODE_DOWN])
        joy |= KB_INPUT_FLAG_DOWN;
    if (state[SDL_SCANCODE_RIGHT])
        joy |= KB_INPUT_FLAG_RIGHT;
    if (state[SDL_SCANCODE_LEFT])
        joy |= KB_INPUT_FLAG_LEFT;
    if (state[SDL_SCANCODE_RETURN] || state[SDL_SCANCODE_SPACE])
        joy |= KB_INPUT_FLAG_PRIMARY_ACTION;
    if (state[SDL_SCANCODE_RCTRL] || state[SDL_SCANCODE_LCTRL])
        joy |= KB_INPUT_FLAG_SECONDARY_ACTION;

    if (g_input_backend.gamepad != 0) {
        Sint16 lx = SDL_GetGamepadAxis(g_input_backend.gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        Sint16 ly = SDL_GetGamepadAxis(g_input_backend.gamepad, SDL_GAMEPAD_AXIS_LEFTY);
        g_joystick_calibration.raw_x = (unsigned short)(lx + 32768);
        if (lx < -KB_CONTROLLER_AXIS_DEADZONE)
            joy |= KB_INPUT_FLAG_LEFT;
        if (lx > KB_CONTROLLER_AXIS_DEADZONE)
            joy |= KB_INPUT_FLAG_RIGHT;
        if (ly < -KB_CONTROLLER_AXIS_DEADZONE)
            joy |= KB_INPUT_FLAG_UP;
        if (ly > KB_CONTROLLER_AXIS_DEADZONE)
            joy |= KB_INPUT_FLAG_DOWN;
        if (SDL_GetGamepadButton(g_input_backend.gamepad, SDL_GAMEPAD_BUTTON_SOUTH))
            joy |= KB_INPUT_FLAG_PRIMARY_ACTION;
        if (SDL_GetGamepadButton(g_input_backend.gamepad, SDL_GAMEPAD_BUTTON_EAST))
            joy |= KB_INPUT_FLAG_SECONDARY_ACTION;
    }

    return (short)joy;
}

/**
 * kb_set_callback_flag - Internal helper to set callback flag for a key
 * 
 * Sets the callback index for the given key scancode.
 */
static void
kb_set_callback_flag(unsigned short key, unsigned char slot_index) {
    unsigned short scancode;

    if ((key & KB_KEYCODE_LOW_BYTE_MASK) != 0) {
        scancode = key & KB_CALLBACK_DIRECT_KEY_MASK;
        g_callback_registry.direct_key_bindings[scancode] = slot_index;
    }
    else {
        scancode = (key >> KB_KEYCODE_SCANCODE_SHIFT) & KB_KEYCODE_LOW_BYTE_MASK;
        if (scancode > KB_CALLBACK_EXTENDED_SCANCODE_MAX) {
            scancode = KB_CALLBACK_EXTENDED_SCANCODE_MAX;
        }
        g_callback_registry.extended_key_bindings[scancode] = slot_index;
    }
}

/**
 * kb_parse_key - Parse keyboard input and call registered callback
 * 
 * Returns 0 if callback was called, or the key code if no callback.
 */
unsigned short
kb_parse_key(unsigned short key) {
    unsigned short scancode;
    unsigned char slot_index;
    int callback_idx;

    if (g_callback_registry.dispatch_in_progress) {
        return key;
    }
    g_callback_registry.dispatch_in_progress = true;

    if ((key & KB_KEYCODE_LOW_BYTE_MASK) != 0) {
        scancode = key & KB_CALLBACK_DIRECT_KEY_MASK;
        slot_index = g_callback_registry.direct_key_bindings[scancode];
    }
    else {
        scancode = (key >> KB_KEYCODE_SCANCODE_SHIFT) & KB_KEYCODE_LOW_BYTE_MASK;
        if (scancode >= KB_CALLBACK_EXTENDED_SCANCODE_MAX) {
            scancode = KB_CALLBACK_EXTENDED_SCANCODE_MAX;
        }
        slot_index = g_callback_registry.extended_key_bindings[scancode];
    }

    callback_idx = (int)slot_index - KB_CALLBACK_INDEX_BASE;

    if (callback_idx >= 0) {
        g_callback_registry.slots[callback_idx]();
        g_callback_registry.dispatch_in_progress = false;
        return 0;
    }

    g_callback_registry.dispatch_in_progress = false;
    return key;
}

/**
 * kb_reg_callback - Register a callback function for a key
 * 
 * Registers a  function pointer to be called when the given key is pressed.
 * The callbacks array has 64 slots. Returns slot index used.
 */
void
kb_reg_callback(unsigned short key, void (*callback)(void)) {
    int i;

    for (i = 0; i < KB_CALLBACK_SLOT_COUNT; i++) {
        if (g_callback_registry.slots[i] == callback) {
            kb_set_callback_flag(key, (unsigned char)(i + KB_CALLBACK_INDEX_BASE));
            return;
        }
        if (g_callback_registry.slots[i] == 0) {
            g_callback_registry.slots[i] = callback;
            kb_set_callback_flag(key, (unsigned char)(i + KB_CALLBACK_INDEX_BASE));
            return;
        }
    }
}
