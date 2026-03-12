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

/* mouse.c — Mouse management extracted from stunts.c */
#include <stdio.h>
#include <time.h>
#define MOUSE_IMPL
#include "stunts.h"
#include "keyboard.h"
#include "shape2d.h"
#include "timer.h"
#include "mouse.h"

/* Variables moved from data_game.c (private to this translation unit) */
static unsigned short game_paused_timer_counter = 0;
static unsigned short mouse_button_state_register = 0;
static unsigned short mouse_driver_enabled = 0;
static unsigned char mousehorscale = 0;
static short physics_update_accumulator = 0;


unsigned short mouse_butstate = 0;
unsigned short mouse_xpos = 0;
unsigned short mouse_ypos = 0;
char mouse_isdirty = 0;
char mouse_motion_state_flag = 0;
char mouse_motion_detected_flag = 0;
char kbormouse = 0;

static void mouse_write_state(unsigned short* buttons, unsigned short* x, unsigned short* y)
{
	if (buttons) {
		*buttons = mouse_butstate;
	}
	if (x) {
		*x = mouse_xpos;
	}
	if (y) {
		*y = mouse_ypos;
	}
}

/* --- mouse_get_state --- */

/**
 * mouse_get_state - Poll mouse driver for buttons and position
 *
 * Mirrors original INT 33h AX=3 call. Applies horizontal scaling
 * using mousehorscale (shift right by mousehorscale bits) to match
 * the ASM behaviour.
 */
void mouse_get_state(unsigned short* buttons, unsigned short* x, unsigned short* y)
{
	if (mouse_driver_enabled == 0) {
		mouse_butstate = 0;
		mouse_write_state(buttons, x, y);
		return;
	}
	kb_sdl_get_mouse_state(&mouse_butstate, &mouse_xpos, &mouse_ypos);
	mouse_write_state(buttons, x, y);
}

/* --- mouse_set_pixratio --- */

/**
 * mouse_set_pixratio - Set mouse mickey-to-pixel ratio
 *
 * INT 33h function 0Fh - set horizontal and vertical mickey-to-pixel ratios.
 * Both parameters are the number of mickeys per 8 pixels.
 * @hpix: Horizontal mickeys per 8 pixels
 * @vpix: Vertical mickeys per 8 pixels
 */
void mouse_set_pixratio(unsigned short hpix, unsigned short vpix)
{
	(void)hpix;
	(void)vpix;
}

/* --- mouse_init --- */

/**
 * mouse_init - Initialize mouse driver
 *
 * Attempts to enable PS/2 pointing device (INT 15h C201), then resets
 * the mouse driver (INT 33h 0). If successful, sets up mouse limits
 * and pixel ratio.
 * @width: Screen width for mouse limits (e.g. 320)
 * @height: Screen height for mouse limits (e.g. 200)
 * Returns: Non-zero if mouse driver present, 0 otherwise
 */
unsigned short  mouse_init(unsigned short width, unsigned short height)
{
	unsigned short result;
	unsigned short buttons;

	/* Host build: simulate successful mouse init. */
	result = 1;
	buttons = 2;
	mouse_button_state_register = buttons;

	if (result != 0) {
		/* Set mousehorscale based on width */
		if (width == 320) {
			mousehorscale = 1;
		} else {
			mousehorscale = 0;
		}

		/* Set mouse limits (0 to width-1, 0 to height-1) */
		mouse_set_minmax(0, 0, width - 1, height - 1);
		kb_sdl_set_mouse_position(width / 2, height / 2);

		/* Set pixel ratio to 16:16 */
		mouse_set_pixratio(16, 16);

		mouse_driver_enabled = 65535;
	}

	return result;
}

/* --- mouse_set_minmax --- */

/**
 * mouse_set_minmax - Set mouse cursor movement limits
 *
 * INT 33h function 7 sets horizontal limits, function 8 sets vertical limits.
 * Horizontal values are scaled by mousehorscale.
 * @hmin: Minimum horizontal position
 * @vmin: Minimum vertical position
 * @hmax: Maximum horizontal position
 * @vmax: Maximum vertical position
 */
void mouse_set_minmax(int hmin, int vmin, int hmax, int vmax)
{
	(void)mousehorscale;
	kb_sdl_set_mouse_limits((unsigned short)hmin, (unsigned short)vmin, (unsigned short)hmax, (unsigned short)vmax);
}

/* --- mouse_set_position --- */

/**
 * mouse_set_position - Set mouse cursor position
 *
 * INT 33h function 4 - Set mouse cursor position.
 * Horizontal position is scaled by mousehorscale.
 * @x: Horizontal position
 * @y: Vertical position
 */
void mouse_set_position(int x, int y)
{
	physics_update_accumulator = x;
	game_paused_timer_counter = y;
	kb_sdl_set_mouse_position((unsigned short)x, (unsigned short)y);
	(void)mousehorscale;
}

/* --- mouse_draw_transparent_check --- */

// Draw mouse cursor transparent (show cursor)
/** @brief Draw the mouse cursor transparently when redraw conditions are met. */
void mouse_draw_transparent_check(void) {
	mouse_motion_detected_flag = 1;
	if (kbormouse == 0) return;
	if (mouse_isdirty != 0) return;
	mouse_draw_transparent();
}

/* --- mouse_draw_opaque_check --- */

// Draw mouse cursor opaque (hide/restore background)  
/** @brief Restore the background under the mouse cursor when needed. */
void mouse_draw_opaque_check(void) {
	mouse_motion_detected_flag = 0;
	if (mouse_isdirty == 0) return;
	mouse_draw_opaque();
}

/* --- mouse_multi_hittest --- */

// Check if mouse is inside any of an array of rectangles
// Returns index of hit rectangle, or -1 if none
/** @brief Return the first rectangle hit by the current mouse cursor position. */
short mouse_multi_hittest(short count, unsigned short* x1_array, unsigned short* x2_array, unsigned short* y1_array, unsigned short* y2_array) {
	if (kbormouse == 0) {
		return -1;
	}

	return sprite_hittest_point(mouse_xpos, mouse_ypos, count,
		(const unsigned short*)x1_array,
		(const unsigned short*)x2_array,
		(const unsigned short*)y1_array,
		(const unsigned short*)y2_array);
}

/* --- mouse_draw_opaque --- */

// Draw mouse cursor opaque (solid)
/** @brief Draw the mouse sprite opaquely at the current cursor coordinates. */
void mouse_draw_opaque(void) {
	// Save both sprite buffers (sprite_copy_both_to_arg copies sprite1 and sprite2)
	static struct SPRITE local_sprites[2];
	if (mouseunkspriteptr == 0) {
		mouse_isdirty = 0;
		return;
	}
	sprite_copy_both_to_arg(local_sprites);
	sprite_copy_2_to_1();
	sprite_putimage(mouseunkspriteptr->sprite_bitmapptr);
	sprite_copy_arg_to_both(local_sprites);
	mouse_isdirty = 0;
}

/* --- mouse_draw_transparent --- */

// Draw mouse cursor with transparency
/** @brief Draw the mouse sprite using mask/overlay transparency. */
void mouse_draw_transparent(void) {
	// Save both sprite buffers (sprite_copy_both_to_arg copies sprite1 and sprite2)
	static struct SPRITE local_sprites[2];
	
	if (smouspriteptr == 0 || mmouspriteptr == 0 || mouseunkspriteptr == 0) {
		return;
	}
	
	sprite_copy_both_to_arg(local_sprites);
	sprite_copy_2_to_1();
	sprite_clear_shape_alt(mouseunkspriteptr->sprite_bitmapptr, mouse_xpos, mouse_ypos);
	sprite_putimage_and(mmouspriteptr->sprite_bitmapptr, mouse_xpos, mouse_ypos);
	sprite_putimage_or(smouspriteptr->sprite_bitmapptr, mouse_xpos, mouse_ypos);
	sprite_copy_arg_to_both(local_sprites);
	mouse_isdirty = 1;
}

/* --- mouse_update_menu_blink --- */

// Timer-based sprite update for menu highlighting
// Returns delta time since last call
unsigned int mouse_update_menu_blink(
	unsigned char selected,
	unsigned short* x1_arr,
	unsigned short* x2_arr,
	unsigned short* y1_arr,
	unsigned short* y2_arr,
	unsigned short sprite_hi,
	unsigned short sprite_lo) {
	
	unsigned short delta;
	unsigned short sprite_idx;
	unsigned short idx;
	
	/* Use a private counter instead of timer_get_delta_alt() so that the
	 * blink phase advances correctly even when the outer ui_screen_run_modal
	 * loop has already called timer_get_delta_alt() this frame (which would
	 * reset last_timer_callback_counter and make a second call return 0). */
	{
		static unsigned long blink_prev_counter = 0;
		unsigned long curr = timer_get_counter();
		unsigned long diff = curr - blink_prev_counter;
		blink_prev_counter = curr;
		/* Cap to period to avoid excessive looping in sprite_phase_add_wrap. */
		delta = (diff > 300UL) ? 300U : (unsigned short)diff;
	}
	if (delta == 0) {
		struct timespec ts_yield;
		ts_yield.tv_sec  = 0;
		ts_yield.tv_nsec = GAME_YIELD_NS;
		nanosleep(&ts_yield, NULL);
	}
	/* timer_get_counter() units: g_timer_counter_units_per_tick=5 per 100Hz tick.
	 * At 60fps delta≈5-10 units/frame.  Period=300, threshold=150 → 0.5s cycle. */
	distance_accumulator_counter = sprite_phase_add_wrap(distance_accumulator_counter, delta, 300);
	sprite_idx = sprite_pick_blink_frame(distance_accumulator_counter, 150, sprite_hi, sprite_lo);

	if (x1_arr == 0 || x2_arr == 0 || y1_arr == 0 || y2_arr == 0) {
		video_refresh();
		return delta;
	}

	/* Always redraw the outline so callers that restore the base sprite each
	 * frame (sprite_copy_2_to_1_2 before this call) don't lose it between
	 * blink state changes.  Always call video_refresh so the loop paces at
	 * GAME_DISPLAY_HZ regardless of whether the blink colour toggled. */
	if (game_timer_milliseconds != sprite_idx) {
		game_timer_milliseconds = sprite_idx;
	}
	mouse_draw_opaque_check();
	idx = selected;
	sprite_draw_rect_outline(x1_arr[idx], y1_arr[idx], x2_arr[idx], y2_arr[idx], game_timer_milliseconds);
	mouse_draw_transparent_check();
	video_refresh();

	return delta;
}

