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
#include "data_game.h"
#include "framebuffer.h"
#include "stunts.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

/* Variables moved from data_game.c (private to this translation unit) */
static void (*callbacks[64])(void) = { 0 };
static unsigned char callbackflags[128]  = { 0 };
static unsigned char callbackflags2[134] = { 0 };
static unsigned short camera_rotation_state = 0;
static unsigned char collision_debug_state[16] = { 0, 1, 5, 0, 3, 2, 4, 3, 7, 8, 6, 7, 0, 1, 5, 0 };
static unsigned char in_kb_parse_key = 0;
static unsigned short joyflag1 = 0;
static unsigned char joyinput = 0;
static unsigned short kblastinput = 0;
static unsigned char kbscancodes[10] = { 57, 28, 71, 72, 73, 77, 81, 80, 79, 75 };
static unsigned short render_depth_sort = 0;
static unsigned short rendering_viewport_offset = 80;
static unsigned short screen_scroll_values = 80;
static unsigned short viewport_transform_matrix = 0;


typedef void (* voidinterruptfunctype)(void);
typedef void (* voidfunctype)(void);

/* Forward declarations */
unsigned short  kb_parse_key(unsigned short key);

voidinterruptfunctype old_kb_int9_handler;
voidinterruptfunctype old_kb_int16_handler;


// these data are local to keyboard.c, but kept in their original slots for convencience:

enum {
	KB_SDL_DEFAULT_MOUSE_MAX_X = 319,
	KB_SDL_DEFAULT_MOUSE_MAX_Y = 199,
	KB_SDL_QUEUE_SIZE = 128,
	KB_SDL_QUEUE_MASK = KB_SDL_QUEUE_SIZE - 1,
	KB_KEYCODE_SCANCODE_SHIFT = 8,
	KB_ASCII_CR = 13,
	KB_ASCII_TAB = 9,
	KB_ASCII_BS = 8,
	KB_ASCII_ESC = 27,
	KB_MOUSE_BUTTON_LEFT = 1,
	KB_MOUSE_BUTTON_RIGHT = 2,
	KB_MOUSE_BUTTON_MIDDLE = 4,
	KB_KBD_DATA_PORT = 96,
	KB_KBD_CTRL_PORT = 97,
	KB_KBD_CTRL_ACK_TOGGLE = 128,
	KB_PIC1_COMMAND_PORT = 32,
	KB_PIC_EOI = 32,
	KB_KEYMAP_LAST_VALID_SCANCODE = 89,
	KB_KEYMAP_INVALID_SCANCODE = 0,
	KB_KEY_RELEASE_BIT = 128,
	KB_EXTENDED_ASCII_LIMIT = 133,
	KB_EXTENDED_ASCII_MASK = 127,
	KB_BUFFER_ENTRY_SIZE = 2,
	KB_INPUTMOD_ALT = 56,
	KB_INPUTMOD_CTRL = 29,
	KB_INPUTMOD_LSHIFT = 42,
	KB_INPUTMOD_RSHIFT = 54,
	KB_INPUTMOD_CAPS = 58,
	KB_SHIFTFLAG_NUMLOCK_BIT = 32,
	KB_SHIFTFLAG_NUMLOCK_CLEAR_MASK = 223,
	KB_JOY_FLAG_UP = 1,
	KB_JOY_FLAG_DOWN = 2,
	KB_JOY_FLAG_RIGHT = 4,
	KB_JOY_FLAG_LEFT = 8,
	KB_JOY_FLAG_BUTTON1 = 16,
	KB_JOY_FLAG_BUTTON2 = 32,
	KB_JOY_BUTTON_MASK = 48,
	KB_JOY_ASSIGNED_BIT = 1,
	KB_CONTROLLER_AXIS_DEADZONE = 12000,
	KB_CALLBACK_SLOT_COUNT = 64,
	KB_CALLBACK_INDEX_BASE = 1,
	KB_CALLBACK_SCANCODE_MASK = 127,
	KB_CALLBACK_KEY_LOBYTE_MASK = 255,
	KB_CALLBACK_EXT_SCANCODE_MAX = 132
};

static unsigned char kb_sdl_inited = 0;
static unsigned char kb_sdl_quit = 0;
static unsigned short kb_sdl_mouse_x = 0;
static unsigned short kb_sdl_mouse_y = 0;
static unsigned short kb_sdl_mouse_buttons = 0;
static unsigned short kb_sdl_mouse_min_x = 0;
static unsigned short kb_sdl_mouse_min_y = 0;
static unsigned short kb_sdl_mouse_max_x = KB_SDL_DEFAULT_MOUSE_MAX_X;
static unsigned short kb_sdl_mouse_max_y = KB_SDL_DEFAULT_MOUSE_MAX_Y;
static unsigned short kb_sdl_queue[KB_SDL_QUEUE_SIZE];
static unsigned short kb_sdl_queue_head = 0;
static unsigned short kb_sdl_queue_tail = 0;
static SDL_GameController* kb_sdl_controller = 0;

#define KBINPUT_SIZE 90u

// --- DOS compatibility stubs (removed) ---
#define PTR_SEG(x) 0
#define PTR_OFF(x) 0
static inline void disable(void) {}
/** @brief Enable.
 * @return Function result.
 */
static inline void enable(void) {}
typedef int REGS; // Dummy type

// --- Joystick stub globals for portability ---

/** @brief Kb sdl scancode.
 * @param sc Parameter `sc`.
 * @return Function result.
 */
static unsigned char kb_sdl_scancode(SDL_Scancode sc)
{
	switch (sc) {
	case SDL_SCANCODE_ESCAPE: return 1;
	case SDL_SCANCODE_1: return 2;
	case SDL_SCANCODE_2: return 3;
	case SDL_SCANCODE_3: return 4;
	case SDL_SCANCODE_4: return 5;
	case SDL_SCANCODE_5: return 6;
	case SDL_SCANCODE_6: return 7;
	case SDL_SCANCODE_7: return 8;
	case SDL_SCANCODE_8: return 9;
	case SDL_SCANCODE_9: return 10;
	case SDL_SCANCODE_0: return 11;
	case SDL_SCANCODE_MINUS: return 12;
	case SDL_SCANCODE_EQUALS: return 13;
	case SDL_SCANCODE_BACKSPACE: return 14;
	case SDL_SCANCODE_TAB: return 15;
	case SDL_SCANCODE_Q: return 16;
	case SDL_SCANCODE_W: return 17;
	case SDL_SCANCODE_E: return 18;
	case SDL_SCANCODE_R: return 19;
	case SDL_SCANCODE_T: return 20;
	case SDL_SCANCODE_Y: return 21;
	case SDL_SCANCODE_U: return 22;
	case SDL_SCANCODE_I: return 23;
	case SDL_SCANCODE_O: return 24;
	case SDL_SCANCODE_P: return 25;
	case SDL_SCANCODE_LEFTBRACKET: return 26;
	case SDL_SCANCODE_RIGHTBRACKET: return 27;
	case SDL_SCANCODE_RETURN: return 28;
	case SDL_SCANCODE_LCTRL:
	case SDL_SCANCODE_RCTRL: return 29;
	case SDL_SCANCODE_A: return 30;
	case SDL_SCANCODE_S: return 31;
	case SDL_SCANCODE_D: return 32;
	case SDL_SCANCODE_F: return 33;
	case SDL_SCANCODE_G: return 34;
	case SDL_SCANCODE_H: return 35;
	case SDL_SCANCODE_J: return 36;
	case SDL_SCANCODE_K: return 37;
	case SDL_SCANCODE_L: return 38;
	case SDL_SCANCODE_SEMICOLON: return 39;
	case SDL_SCANCODE_APOSTROPHE: return 40;
	case SDL_SCANCODE_GRAVE: return 41;
	case SDL_SCANCODE_LSHIFT:
	case SDL_SCANCODE_RSHIFT: return 42;
	case SDL_SCANCODE_BACKSLASH: return 43;
	case SDL_SCANCODE_Z: return 44;
	case SDL_SCANCODE_X: return 45;
	case SDL_SCANCODE_C: return 46;
	case SDL_SCANCODE_V: return 47;
	case SDL_SCANCODE_B: return 48;
	case SDL_SCANCODE_N: return 49;
	case SDL_SCANCODE_M: return 50;
	case SDL_SCANCODE_COMMA: return 51;
	case SDL_SCANCODE_PERIOD: return 52;
	case SDL_SCANCODE_SLASH: return 53;
	case SDL_SCANCODE_SPACE: return 57;
	case SDL_SCANCODE_F1: return 59;
	case SDL_SCANCODE_F2: return 60;
	case SDL_SCANCODE_F3: return 61;
	case SDL_SCANCODE_F4: return 62;
	case SDL_SCANCODE_F5: return 63;
	case SDL_SCANCODE_F6: return 64;
	case SDL_SCANCODE_F7: return 65;
	case SDL_SCANCODE_F8: return 66;
	case SDL_SCANCODE_F9: return 67;
	case SDL_SCANCODE_F10: return 68;
	case SDL_SCANCODE_HOME: return 71;
	case SDL_SCANCODE_UP: return 72;
	case SDL_SCANCODE_PAGEUP: return 73;
	case SDL_SCANCODE_LEFT: return 75;
	case SDL_SCANCODE_RIGHT: return 77;
	case SDL_SCANCODE_END: return 79;
	case SDL_SCANCODE_DOWN: return 80;
	case SDL_SCANCODE_PAGEDOWN: return 81;
	case SDL_SCANCODE_INSERT: return 82;
	case SDL_SCANCODE_DELETE: return 83;
	default: return 0;
	}
}

/** @brief Kb sdl ascii.
 * @param key Parameter `key`.
 * @param mod Parameter `mod`.
 * @return Function result.
 */
static unsigned char kb_sdl_ascii(SDL_Keycode key, Uint16 mod)
{
	if (key >= SDLK_a && key <= SDLK_z) {
		unsigned char ch = (unsigned char)('a' + (key - SDLK_a));
		if ((mod & KMOD_SHIFT) != 0 || (mod & KMOD_CAPS) != 0) {
			ch = (unsigned char)(ch - ('a' - 'A'));
		}
		return ch;
	}
	if (key >= SDLK_0 && key <= SDLK_9) {
		return (unsigned char)('0' + (key - SDLK_0));
	}
	if (key == SDLK_SPACE) return ' ';
	if (key == SDLK_RETURN || key == SDLK_KP_ENTER) return KB_ASCII_CR;
	if (key == SDLK_TAB) return KB_ASCII_TAB;
	if (key == SDLK_BACKSPACE) return KB_ASCII_BS;
	if (key == SDLK_ESCAPE) return KB_ASCII_ESC;
	return 0;
}

/** @brief Kb sdl queue push.
 * @param key Parameter `key`.
 * @return Function result.
 */
static void kb_sdl_queue_push(unsigned short key)
{
	unsigned short next = (unsigned short)((kb_sdl_queue_tail + 1u) & KB_SDL_QUEUE_MASK);
	if (next == kb_sdl_queue_head) {
		return;
	}
	kb_sdl_queue[kb_sdl_queue_tail] = key;
	kb_sdl_queue_tail = next;
}

/** @brief Kb sdl queue pop.
 * @return Function result.
 */
static unsigned short kb_sdl_queue_pop(void)
{
	unsigned short key;
	if (kb_sdl_queue_head == kb_sdl_queue_tail) {
		return 0;
	}
	key = kb_sdl_queue[kb_sdl_queue_head];
	kb_sdl_queue_head = (unsigned short)((kb_sdl_queue_head + 1u) & KB_SDL_QUEUE_MASK);
	return key;
}

/** @brief Kb sdl requeue key.
 * @param key Parameter `key`.
 */
void kb_sdl_requeue_key(unsigned short key)
{
	kb_sdl_queue_push(key);
}

/** @brief Kb has pending input.
 * @return Function result.
 */
static int kb_has_pending_input(void)
{
	return (kb_sdl_queue_head != kb_sdl_queue_tail) ? 1 : 0;
}

/** @brief Kb sdl update mouse from system.
 * @return Function result.
 */
static void kb_sdl_update_mouse_from_system(void)
{
	int sx, sy;
	int win_w = 0;
	int win_h = 0;
	SDL_Window* mouse_win;
	int range_x;
	int range_y;
	unsigned int mstate = SDL_GetMouseState(&sx, &sy);

	/* Convert from actual window coordinates to current game mouse range. */
	mouse_win = SDL_GetMouseFocus();
	if (mouse_win != 0) {
		SDL_GetWindowSize(mouse_win, &win_w, &win_h);
	}

	range_x = (int)kb_sdl_mouse_max_x - (int)kb_sdl_mouse_min_x;
	range_y = (int)kb_sdl_mouse_max_y - (int)kb_sdl_mouse_min_y;
	if (win_w > 1 && range_x > 0) {
		sx = (int)kb_sdl_mouse_min_x + (sx * range_x) / (win_w - 1);
	}
	if (win_h > 1 && range_y > 0) {
		sy = (int)kb_sdl_mouse_min_y + (sy * range_y) / (win_h - 1);
	}

	if (sx < (int)kb_sdl_mouse_min_x) sx = kb_sdl_mouse_min_x;
	if (sy < (int)kb_sdl_mouse_min_y) sy = kb_sdl_mouse_min_y;
	if (sx > (int)kb_sdl_mouse_max_x) sx = kb_sdl_mouse_max_x;
	if (sy > (int)kb_sdl_mouse_max_y) sy = kb_sdl_mouse_max_y;

	kb_sdl_mouse_x = (unsigned short)sx;
	kb_sdl_mouse_y = (unsigned short)sy;
	kb_sdl_mouse_buttons = 0;
	if ((mstate & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0u) kb_sdl_mouse_buttons |= KB_MOUSE_BUTTON_LEFT;
	if ((mstate & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0u) kb_sdl_mouse_buttons |= KB_MOUSE_BUTTON_RIGHT;
	if ((mstate & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0u) kb_sdl_mouse_buttons |= KB_MOUSE_BUTTON_MIDDLE;
}

/** @brief Kb poll sdl input.
 */
void kb_poll_sdl_input(void)
{
	SDL_Event ev;
	if (kb_sdl_inited == 0) {
		return;
	}

	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_QUIT) {
			kb_sdl_quit = 1;
			call_exitlist2();
			return;
		}

		if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_CLOSE) {
			kb_sdl_quit = 1;
			call_exitlist2();
			return;
		}

		if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
			/* Host-only keys: intercept before the game sees them */
			if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
				SDL_Keycode sym = ev.key.keysym.sym;
				SDL_Keymod mod = ev.key.keysym.mod;
				if ((sym == SDLK_RETURN || sym == SDLK_RETURN2) && (mod & KMOD_ALT)) {
					video_toggle_fullscreen();
					continue;
				}
				if (sym == SDLK_PLUS || sym == SDLK_EQUALS || sym == SDLK_KP_PLUS) {
					video_scale_up();
					continue;
				}
				if (sym == SDLK_MINUS || sym == SDLK_KP_MINUS) {
					video_scale_down();
					continue;
				}
			}
			unsigned char dos_sc = kb_sdl_scancode(ev.key.keysym.scancode);
			if (dos_sc != 0 && dos_sc < KBINPUT_SIZE) {
				kbinput[dos_sc] = (ev.type == SDL_KEYDOWN) ? 1u : 0u;
				kblastinput = dos_sc;
				if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
					unsigned char ascii = kb_sdl_ascii(ev.key.keysym.sym, ev.key.keysym.mod);
					unsigned short key = (unsigned short)(((unsigned short)dos_sc << KB_KEYCODE_SCANCODE_SHIFT) | ascii);
					if (ascii == 0) {
						key = (unsigned short)(dos_sc << KB_KEYCODE_SCANCODE_SHIFT);
					}
					kb_sdl_queue_push(key);
				}
			}
			continue;
		}

		if (ev.type == SDL_CONTROLLERDEVICEADDED && kb_sdl_controller == 0) {
			if (SDL_IsGameController(ev.cdevice.which)) {
				kb_sdl_controller = SDL_GameControllerOpen(ev.cdevice.which);
			}
			continue;
		}

		if (ev.type == SDL_CONTROLLERDEVICEREMOVED && kb_sdl_controller != 0) {
			SDL_JoystickID jid = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(kb_sdl_controller));
			if (jid == ev.cdevice.which) {
				SDL_GameControllerClose(kb_sdl_controller);
				kb_sdl_controller = 0;
			}
			continue;
		}
	}

	kb_sdl_update_mouse_from_system();
}

/** @brief Kb sdl set mouse limits.
 * @param min_x Parameter `min_x`.
 * @param min_y Parameter `min_y`.
 * @param max_x Parameter `max_x`.
 * @param max_y Parameter `max_y`.
 */
void kb_sdl_set_mouse_limits(unsigned short min_x, unsigned short min_y, unsigned short max_x, unsigned short max_y)
{
	kb_sdl_mouse_min_x = min_x;
	kb_sdl_mouse_min_y = min_y;
	kb_sdl_mouse_max_x = max_x;
	kb_sdl_mouse_max_y = max_y;
	if (kb_sdl_mouse_x < kb_sdl_mouse_min_x) kb_sdl_mouse_x = kb_sdl_mouse_min_x;
	if (kb_sdl_mouse_y < kb_sdl_mouse_min_y) kb_sdl_mouse_y = kb_sdl_mouse_min_y;
	if (kb_sdl_mouse_x > kb_sdl_mouse_max_x) kb_sdl_mouse_x = kb_sdl_mouse_max_x;
	if (kb_sdl_mouse_y > kb_sdl_mouse_max_y) kb_sdl_mouse_y = kb_sdl_mouse_max_y;
}

/** @brief Kb sdl set mouse position.
 * @param x Parameter `x`.
 * @param y Parameter `y`.
 */
void kb_sdl_set_mouse_position(unsigned short x, unsigned short y)
{
	kb_sdl_mouse_x = x;
	kb_sdl_mouse_y = y;
	kb_sdl_set_mouse_limits(kb_sdl_mouse_min_x, kb_sdl_mouse_min_y, kb_sdl_mouse_max_x, kb_sdl_mouse_max_y);
}

/** @brief Kb sdl get mouse state.
 * @param buttons Parameter `buttons`.
 * @param x Parameter `x`.
 * @param y Parameter `y`.
 */
void kb_sdl_get_mouse_state(unsigned short* buttons, unsigned short* x, unsigned short* y)
{
	kb_poll_sdl_input();
	if (buttons) *buttons = kb_sdl_mouse_buttons;
	if (x) *x = kb_sdl_mouse_x;
	if (y) *y = kb_sdl_mouse_y;
}

/** @brief Initialize keyboard input state and SDL input backends.
 */
void kb_init_interrupt(void) {
	memset(kbinput, 0, KBINPUT_SIZE);
	kb_sdl_queue_head = 0;
	kb_sdl_queue_tail = 0;
	kb_sdl_quit = 0;
	kb_sdl_inited = 1;
	if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) == 0) {
		SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
	}
	if (SDL_NumJoysticks() > 0 && SDL_IsGameController(0)) {
		kb_sdl_controller = SDL_GameControllerOpen(0);
	}
	add_exit_handler(kb_exit_handler);
}

/** @brief Shutdown keyboard input handling and release SDL resources.
 */
void kb_exit_handler(void) {

	if (kb_sdl_controller != 0) {
		SDL_GameControllerClose(kb_sdl_controller);
		kb_sdl_controller = 0;
	}
	kb_sdl_inited = 0;
}

/** @brief Return pressed-state information for a DOS scancode.
 * @param key Parameter `key`.
 * @return Function result.
 */
int kb_get_key_state(int key) {

	if (key < 0 || (unsigned int)key >= KBINPUT_SIZE) {
		return 0;
	}
	return kbinput[key];
}

/** @brief Invoke the installed read-char callback, if present.
 * @return Function result.
 */
int kb_call_readchar_callback(void) {

	// the orginal code uses a (hard-coded, non-changing) callback for
	// reading chars.. we just call kb_read_char() directly:
	return kb_read_char();
}

/** @brief Read one key code from SDL queue or DOS callback fallback.
 * @return Function result.
 */
int kb_read_char(void) {

	unsigned short key;
	kb_poll_sdl_input();
	key = kb_sdl_queue_pop();
	if (key == 0) {
		return 0;
	}
	kb_parse_key(key);
	return (int)(key & KB_CALLBACK_KEY_LOBYTE_MASK);
}

/** @brief Check whether keyboard input is currently available.
 * @return Function result.
 */
int kb_checking(void) {

	kb_poll_sdl_input();
	return kb_has_pending_input();
}

// Ported from seg018.asm - manipulate keyboard shift flags in BIOS Data Area
// 0040:0017 = keyboard shift flags byte, bit 5 = NumLock active state
static unsigned char kb_shift_flags_shadow = 0;

/** @brief Force the NumLock bit on in the keyboard shift-state shadow.
 */
void kb_shift_checking1(void) {

	kb_shift_flags_shadow |= KB_SHIFTFLAG_NUMLOCK_BIT;  // Set bit 5 (NumLock)
	kb_checking();
}

/** @brief Clear the NumLock bit in the keyboard shift-state shadow.
 */
void kb_shift_checking2(void) {

	kb_shift_flags_shadow &= KB_SHIFTFLAG_NUMLOCK_CLEAR_MASK;  // Clear bit 5 (NumLock)
	kb_checking();
}

/** @brief Return a non-zero value when a key event is pending.
 * @return Function result.
 */
int kb_check(void) {

	kb_poll_sdl_input();
	return kb_has_pending_input();
}

/* Joystick state used by kb_read_key_or_joy */

/**
 * kb_read_key_or_joy - Read a key; fall back to joystick (nopsub_304B6)
 *
 * Returns BIOS keycode in AX (AL=ASCII, AH=scancode) or 0 if no new input.
 * Keyboard has priority; joystick is debounced to avoid spamming repeats.
 */
unsigned short  kb_read_key_or_joy(void) {
    // DOS BIOS keyboard and joystick read removed for portability
    // Implement SDL-based key reading if needed
    return 0;
}

/**
 * joystick_init_calibration - Initialize joystick calibration
 * 
 * Sets joystick enabled flag and default calibration values.
 */
void joystick_init_calibration(void) {
	joystick_assigned_flags = 1;
	screen_scroll_values = 80;
	camera_rotation_state = 0;
	rendering_viewport_offset = 80;
	render_depth_sort = 0;
}

/**
 * joystick_direction_lookup - Joystick direction lookup
 * 
 * Parameters:
/** @brief Flags.
 * @param joy_flags Parameter `joy_flags`.
 * @return Function result.
 */
short joystick_direction_lookup(unsigned short joy_flags) {
	return (short)collision_debug_state[joy_flags & 15];
}

/**
 * joystick_get_scaled_x - Calculate joystick X axis position
 * 
 * Returns scaled joystick X position based on calibration.
/** @brief Joystick get scaled x.
 * @return Function result.
 */
short joystick_get_scaled_x(void) {
	short ax;
	unsigned long result;
	
	ax = joyflag1 - screen_scroll_values;
	if (ax < 0) ax = 0;
	
	result = (unsigned long)ax * viewport_transform_matrix;
	ax = (short)(result >> 8);
	return ax - 31;
}

/* Keyboard scan code and input globals */

/**
 * get_kb_or_joy_flags - Get keyboard or joystick input flags
 * 
 * Checks keyboard input array against scancode bindings and returns
 * a bitmask of active inputs. If no keyboard input, checks joystick.
 * 
 * Bit flags:
/** @brief Get kb or joy flags.
 * @return Function result.
 */
short get_kb_or_joy_flags(void) {
	short flags = 0;
	
	/* Check each keyboard scancode against input array */
	if (kbinput[kbscancodes[0]]) flags |= 16;
	if (kbinput[kbscancodes[1]]) flags |= 32;
	if (kbinput[kbscancodes[2]]) flags |= 9;
	if (kbinput[kbscancodes[3]]) flags |= 1;
	if (kbinput[kbscancodes[4]]) flags |= 5;
	if (kbinput[kbscancodes[5]]) flags |= 4;
	if (kbinput[kbscancodes[6]]) flags |= 6;
	if (kbinput[kbscancodes[7]]) flags |= 2;
	if (kbinput[kbscancodes[8]]) flags |= 10;
	if (kbinput[kbscancodes[9]]) flags |= 8;
	
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
/** @brief Kb get char.
 * @return Function result.
 */
int kb_get_char(void) {
	unsigned short key_ax;

	kb_poll_sdl_input();
	key_ax = kb_sdl_queue_pop();
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
/** @brief Port.
 * @param get_joy_flags Parameter `get_joy_flags`.
 * @return Function result.
 */
short get_joy_flags(void) {
	unsigned short joy = 0;
	const Uint8* state;

	kb_poll_sdl_input();

	state = SDL_GetKeyboardState(0);
	if (state[SDL_SCANCODE_UP]) joy |= 1;
	if (state[SDL_SCANCODE_DOWN]) joy |= 2;
	if (state[SDL_SCANCODE_RIGHT]) joy |= 4;
	if (state[SDL_SCANCODE_LEFT]) joy |= 8;
	if (state[SDL_SCANCODE_RETURN] || state[SDL_SCANCODE_SPACE]) joy |= 16;
	if (state[SDL_SCANCODE_RCTRL] || state[SDL_SCANCODE_LCTRL]) joy |= 32;

	if (kb_sdl_controller != 0) {
		Sint16 lx = SDL_GameControllerGetAxis(kb_sdl_controller, SDL_CONTROLLER_AXIS_LEFTX);
		Sint16 ly = SDL_GameControllerGetAxis(kb_sdl_controller, SDL_CONTROLLER_AXIS_LEFTY);
		if (lx < -KB_CONTROLLER_AXIS_DEADZONE) joy |= KB_JOY_FLAG_LEFT;
		if (lx > KB_CONTROLLER_AXIS_DEADZONE) joy |= KB_JOY_FLAG_RIGHT;
		if (ly < -KB_CONTROLLER_AXIS_DEADZONE) joy |= KB_JOY_FLAG_UP;
		if (ly > KB_CONTROLLER_AXIS_DEADZONE) joy |= KB_JOY_FLAG_DOWN;
		if (SDL_GameControllerGetButton(kb_sdl_controller, SDL_CONTROLLER_BUTTON_A)) joy |= KB_JOY_FLAG_BUTTON1;
		if (SDL_GameControllerGetButton(kb_sdl_controller, SDL_CONTROLLER_BUTTON_B)) joy |= KB_JOY_FLAG_BUTTON2;
	}

	joyinput = (unsigned char)joy;
	return (short)joy;
}

/**
 * kb_set_callback_flag - Internal helper to set callback flag for a key
 * 
 * Sets the callback index for the given key scancode.
/** @brief Callbackflags.
 * @param zero Parameter `zero`.
 * @param slot_index Parameter `slot_index`.
 * @return Function result.
 */
static void kb_set_callback_flag(unsigned short key, unsigned char slot_index) {
	unsigned short scancode;
	
	if ((key & KB_CALLBACK_KEY_LOBYTE_MASK) != 0) {
		/* Normal scancode - use low byte, mask to 0-127 */
		scancode = key & KB_CALLBACK_SCANCODE_MASK;
		callbackflags[scancode] = slot_index;
	} else {
		/* Extended scancode - use high byte */
		scancode = (key >> KB_KEYCODE_SCANCODE_SHIFT) & KB_CALLBACK_KEY_LOBYTE_MASK;
		if (scancode > KB_CALLBACK_EXT_SCANCODE_MAX) {
			scancode = KB_CALLBACK_EXT_SCANCODE_MAX;
		}
		callbackflags2[scancode] = slot_index;
	}
}

/**
 * kb_parse_key - Parse keyboard input and call registered callback
 * 
 * Returns 0 if callback was called, or the key code if no callback.
 */
unsigned short  kb_parse_key(unsigned short key) {
	unsigned short scancode;
	unsigned char slot_index;
	int callback_idx;
	
	/* Reentrancy guard */
	disable();
	if (in_kb_parse_key != 0) {
		enable();
		return key;
	}
	in_kb_parse_key = 1;
	enable();
	
	/* Get callback slot index from the appropriate flags array */
	if ((key & KB_CALLBACK_KEY_LOBYTE_MASK) != 0) {
		/* Normal scancode */
		scancode = key & KB_CALLBACK_SCANCODE_MASK;
		slot_index = callbackflags[scancode];
	} else {
		/* Extended scancode */
		scancode = (key >> KB_KEYCODE_SCANCODE_SHIFT) & KB_CALLBACK_KEY_LOBYTE_MASK;
		if (scancode >= KB_CALLBACK_EXT_SCANCODE_MAX) {
			scancode = KB_CALLBACK_EXT_SCANCODE_MAX;
		}
		slot_index = callbackflags2[scancode];
	}

	/* slot_index is 1-based, 0 means no callback */
	callback_idx = (int)slot_index - KB_CALLBACK_INDEX_BASE;

	if (callback_idx >= 0) {
		/* Call the callback */
		callbacks[callback_idx]();
		in_kb_parse_key = 0;
		return 0;
	}

	/* No callback - return key code */
	in_kb_parse_key = 0;
	return key;
}

/**
 * kb_reg_callback - Register a callback function for a key
 * 
 * Registers a  function pointer to be called when the given key is pressed.
 * The callbacks array has 64 slots. Returns slot index used.
 */
void kb_reg_callback(unsigned short key, void (*callback)(void)) {
	int i;
	const unsigned short cb_ofs = PTR_OFF(callback);
	const unsigned short cb_seg = PTR_SEG(callback);
	
	/* Search for existing entry or empty slot */
	for (i = 0; i < KB_CALLBACK_SLOT_COUNT; i++) {
		if (PTR_OFF(callbacks[i]) == cb_ofs && PTR_SEG(callbacks[i]) == cb_seg) {
			/* Already registered - just set the flag */
			kb_set_callback_flag(key, (unsigned char)(i + KB_CALLBACK_INDEX_BASE));
			return;
		}
		if (PTR_SEG(callbacks[i]) == 0) {
			/* Empty slot - register new callback */
			callbacks[i] = callback;
			kb_set_callback_flag(key, (unsigned char)(i + KB_CALLBACK_INDEX_BASE));
			return;
		}
	}
	/* No free slots - silently fail (matches original ASM behavior) */
}
