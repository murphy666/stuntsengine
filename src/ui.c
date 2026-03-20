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

/*
 * ui.c - User Interface Functions
 *
 * This module contains UI functions ported from seg008.asm.
 * 
 * The UI system provides:
 * - Modal window management with state save/restore
 * - Dialog box rendering and interaction
 * - Input handling (keyboard, mouse, joystick)
 * - File selection dialogs
 * - Button rendering and mouse tracking
 *
 * Functions ported from seg008.asm:
 *   ui_window_push_modal - Window creation with sprite allocation
 *   ui_window_pop_modal - Window cleanup and restoration
 *   (More functions to be added)
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include "stunts.h"
#include "shape2d.h"
#include "shape3d.h"
#include "keyboard.h"
#include "ressources.h"
#include "game_timing.h"
#include "timer.h"
#include "font.h"
#include "ui_keys.h"
#include "ui_widgets.h"

/* Variables moved from data_game.c */
static void * sprite_ptrs[10] = { 0 };
static unsigned short window_x_positions[10] = { 0 };
static unsigned short window_y_positions[10] = { 0 };


/* file-local data (moved from data_global.c) */
static unsigned short dialog_text_color = 0;
static unsigned short input_framecount = 0;
static unsigned short input_framecount2 = 0;
static unsigned short input_framecount3 = 0;
static unsigned short input_framecounter = 0;
static unsigned short joyflags = 0;
static unsigned short mouse_oldx = 0;
static unsigned short mouse_oldy = 0;

#define WINDOW_STACK_MAX 10

static unsigned char window_stack_depth = 0;

static unsigned char g_window_stack_save[WINDOW_STACK_MAX][sizeof(struct SPRITE) * 2];

/** @brief Window stack get save ptr.
 * @param idx Parameter `idx`.
 * @return Function result.
 */
static char * window_stack_get_save_ptr(unsigned short idx)
{
	if (idx >= WINDOW_STACK_MAX) {
		return NULL;
	}

	/* Use window_save_buf only when the sprite size matches the original 16-bit layout. */
	if (window_save_buf != NULL && (sizeof(struct SPRITE) * 2u) <= 60u) {
		return window_save_buf + (idx * 60u);
	}

	return (char *)g_window_stack_save[idx];
}

/* Forward declarations for seg008 wrappers */

/* Forward declarations to fix Borland missing-prototype warnings */

/*--------------------------------------------------------------
 * Static variables for read_line text input state
 * These were game_mode_tracker-menu_state_data in seg032.asm
 /*--------------------------------------------------------------*/
static unsigned short rl_cursor_height;   /* game_mode_tracker - cursor height (1 or 8) */
static unsigned short rl_x_pos;           /* read_line_x_pos - x position */
static unsigned short rl_y_pos;           /* read_line_y_pos - y position */
static unsigned short rl_cursor_visible;  /* read_line_cursor_visible - cursor visible flag */
static char* rl_buffer_ptr;               /* off_42A1E - buffer pointer */
static unsigned short rl_max_width;       /* read_line_max_width_px - max width in pixels */
static unsigned short rl_cursor_pos;      /* menu_state_data - cursor position in buffer */

/*--------------------------------------------------------------
 * read_line_helper2 - Render text in input field
 *
 * Draws the current text buffer content and clears any 
 * remaining space up to the maximum width.
 /*--------------------------------------------------------------*/
/** @brief Read line helper2.
 * @return Function result.
 */
static void read_line_helper2(void)
{
	unsigned short text_len;
	unsigned short text_width;
	unsigned short remaining_width;
	unsigned char * fontdata;
	
	/* Truncate text if it exceeds max width */
	if (rl_max_width != 0) {
		while (1) {
			text_len = strlen(rl_buffer_ptr);
			if (text_len == 0) break;
			
			text_width = font_get_text_width(rl_buffer_ptr);
			if (text_width <= rl_max_width) break;
			
			/* Remove last character */
			rl_buffer_ptr[text_len - 1] = 0;
		}
	}
	
	/* Clamp cursor position to text length */
	text_len = strlen(rl_buffer_ptr);
	if (rl_cursor_pos > text_len) {
		rl_cursor_pos = text_len;
	}
	
	/* Draw the text */
	sprite_draw_text_opaque(rl_buffer_ptr, rl_x_pos, rl_y_pos);
	
	/* Clear remaining space if max_width is set */
	if (rl_max_width != 0) {
		text_width = font_get_text_width(rl_buffer_ptr);
		remaining_width = rl_max_width - text_width;
		
		if (remaining_width > 0) {
			fontdata = (unsigned char *)fontdefptr;
			if (fontdata == 0) {
				return;
			}
			sprite_fill_rect_clipped(
				rl_x_pos + text_width,
				rl_y_pos,
				remaining_width,
				*(unsigned short *)(fontdata + 18),  /* font height at offset 18 */
				fontdata[2]   /* background color at offset 2 */
			);
		}
	}
}

/*--------------------------------------------------------------
 * read_line_helper - Draw/update cursor
 *
 * XOR draws the cursor at the current position if cursor is
/** @brief Visible.
 * @param read_line_helper Parameter `read_line_helper`.
 * @return Function result.
 */
 /*--------------------------------------------------------------*/
static void read_line_helper(void)
{
	unsigned short char_width;
	unsigned short x_offset;
	unsigned short y_offset;
	unsigned char * fontdata;
	
	if (rl_cursor_visible == 0) {
		return;
	}
	
	/* Clamp cursor position */
	if (rl_cursor_pos > strlen(rl_buffer_ptr)) {
		rl_cursor_pos = strlen(rl_buffer_ptr);
	}
	
	/* Get width of single character at cursor position */
	char_width = font_op(rl_buffer_ptr + rl_cursor_pos, 1);
	if (char_width == 0) {
		/* Use space character width as fallback (21450 = address of " " string) */
		char_width = font_get_text_width(" ");
	}
	
	/* Calculate x position (sum of character widths up to cursor) */
	x_offset = font_op(rl_buffer_ptr, rl_cursor_pos);
	x_offset += rl_x_pos;
	
	/* Calculate y position (baseline + font height - cursor height) */
	fontdata = (unsigned char *)fontdefptr;
	if (fontdata == 0) {
		return;
	}
	y_offset = rl_y_pos + *(unsigned short *)(fontdata + 18) - rl_cursor_height;
	
	/* XOR draw the cursor rectangle */
	sprite_xor_fill_rect(x_offset, y_offset, char_width, rl_cursor_height, fontdata[0]);
}

/*--------------------------------------------------------------
 * read_line - Interactive text input
 *
 * Displays an interactive text input field with cursor, supporting:
/** @brief Keys.
 * @param x Parameter `x`.
 * @param timeout Parameter `timeout`.
 * @return Function result.
 */
 /*--------------------------------------------------------------*/
unsigned short  read_line(char* buffer, unsigned char flags, unsigned short max_chars, 
                            unsigned short width, unsigned short height, void * fontptr,
                            unsigned short x, unsigned short y, void * callback_ptr, unsigned short timeout)
{
	unsigned short key_code;
	unsigned short first_key;
	unsigned short insert_mode;
	unsigned short prev_cursor_visible;
	unsigned short text_len;
	unsigned short i;
	void ( *callback)(void) = (void ( *)(void))callback_ptr;
	(void)fontptr;
	(void)height;
		// ...existing code...
	/* Save current screen */
	sprite_copy_2_to_1();
	
	/* Initialize state */
	rl_x_pos = x;
	rl_y_pos = y;
	rl_buffer_ptr = buffer;
	rl_max_width = width;
	
	/* Null-terminate at max_chars position */
	buffer[max_chars] = 0;
	
	/* Clear buffer if flag 1 is set */
	if (flags & 1) {
		buffer[0] = 0;
	}
	
	/* Reset or preserve cursor position based on flag 2 */
	if (flags & 2) {
		rl_cursor_pos = 0;
	} else {
		rl_cursor_pos = strlen(buffer);
	}
	
	/* Pad buffer with spaces up to max_chars */
	text_len = strlen(buffer);
	while (text_len < max_chars) {
		buffer[text_len++] = ' ';
	}
	buffer[max_chars] = 0;
	
	/* Render initial text */
	read_line_helper2();
	
	/* Initialize cursor */
	rl_cursor_height = 1;
	rl_cursor_visible = 1;
	insert_mode = 0;
	read_line_helper();
	
	/* Set up timeout if specified */
	timer_copy_counter((unsigned long)timeout);
	set_add_value(4, 0);
	
	first_key = 1;
	
	/* Main input loop */
	while (1) {
		key_code = 0;
		
		/* Call user callback every iteration if provided */
		if (callback != 0) {
			callback();
		}
		
		/* Poll for keyboard input */
		key_code = kb_call_readchar_callback();
		if (key_code == 0) {
			/* No key: check cursor blink deadline */
			if (timer_deadline_reached() == 0) {
				/* Deadline not yet reached: keep polling */
				continue;
			}
			/* Deadline reached: blink cursor and refresh screen */
			set_add_value(4, 0);
			prev_cursor_visible = rl_cursor_visible;
			rl_cursor_visible = 1;
			read_line_helper();
			if (prev_cursor_visible == 0) {
				rl_cursor_visible = 1;
			} else {
				rl_cursor_visible = 0;
			}
			video_refresh();
			/* Check overall timeout */
			if ((timeout != 0) && timer_compare_dx()) {
				read_line_helper();
				return key_code;
			}
			continue;
		}
		
		/* Reset timeout on keypress */
		timer_copy_counter((unsigned long)timeout);
		
		/* Check for exit keys */
		if (key_code == UI_KEY_ENTER || key_code == UI_KEY_ESCAPE) {  /* Enter or Escape */
			read_line_helper();
			return key_code;
		}
		if (key_code == UI_KEY_UP) {  /* Up arrow */
			read_line_helper();
			return key_code;
		}
		if (key_code == UI_KEY_DOWN && !(flags & 8)) {  /* Down arrow, unless flag 8 */
			read_line_helper();
			return key_code;
		}
		if (key_code == UI_KEY_TAB && !(flags & 16)) {  /* Tab, unless flag 16 */
			read_line_helper();
			return key_code;
		}
		
		/* Handle cursor movement */
		if (key_code == UI_KEY_RIGHT) {  /* Right arrow */
			read_line_helper();
			if (rl_cursor_pos < max_chars) {
				rl_cursor_pos++;
			}
			read_line_helper();
			first_key = 0;
			continue;
		}
		
		if (key_code == UI_KEY_LEFT) {  /* Left arrow */
			read_line_helper();
			if (rl_cursor_pos > 0) {
				rl_cursor_pos--;
			}
			read_line_helper();
			first_key = 0;
			continue;
		}
		
		if (key_code == UI_KEY_HOME) {  /* Home */
			read_line_helper();
			rl_cursor_pos = 0;
			read_line_helper();
			first_key = 0;
			continue;
		}
		
		if (key_code == UI_KEY_END) {  /* End */
			read_line_helper();
			rl_cursor_pos = strlen(buffer);
			read_line_helper();
			first_key = 0;
			continue;
		}
		
		/* Insert mode toggle */
		if (key_code == UI_KEY_INSERT) {  /* Insert */
			read_line_helper();
			if (insert_mode == 0) {
				insert_mode = 1;
				rl_cursor_height = 8;
			} else {
				insert_mode = 0;
				rl_cursor_height = 1;
			}
			read_line_helper();
			first_key = 0;
			continue;
		}
		
		/* Delete character at cursor */
		if (key_code == UI_KEY_DELETE) {  /* Delete */
			if (rl_cursor_pos >= max_chars || buffer[rl_cursor_pos] == 0) {
				first_key = 0;
				continue;
			}
			read_line_helper();
			/* Shift characters left */
			for (i = rl_cursor_pos; i < max_chars - 1; i++) {
				buffer[i] = buffer[i + 1];
			}
			buffer[max_chars - 1] = ' ';
			read_line_helper2();
			read_line_helper();
			first_key = 0;
			continue;
		}
		
		/* Backspace */
		if (key_code == UI_KEY_BACKSPACE) {
			if (rl_cursor_pos == 0) {
				first_key = 0;
				continue;
			}
			read_line_helper();
			rl_cursor_pos--;
			/* Shift characters left */
			for (i = rl_cursor_pos; i < max_chars - 1; i++) {
				buffer[i] = buffer[i + 1];
			}
			buffer[max_chars - 1] = ' ';
			read_line_helper2();
			read_line_helper();
			first_key = 0;
			continue;
		}
		
/** @brief Input.
 * @param key_code Parameter `key_code`.
 * @return Function result.
 */
		/* Printable character input (32-122) */
		if (key_code < 32 || key_code > 122) {
			first_key = 0;
			continue;
		}
		
		if (rl_cursor_pos >= max_chars) {
			first_key = 0;
			continue;
		}
		
		read_line_helper();
		
		/* On first printable key, clear buffer unless flag 4 is set */
		if (first_key && !(flags & 4)) {
			rl_cursor_pos = 0;
			for (i = 0; i < max_chars; i++) {
				buffer[i] = ' ';
			}
		}
		
		/* Ensure null terminator after current position */
		if (buffer[rl_cursor_pos] == 0) {
			buffer[rl_cursor_pos + 1] = 0;
		}
		
		/* Insert mode: shift characters right */
		if (insert_mode) {
			for (i = max_chars - 2; i > rl_cursor_pos; i--) {
				buffer[i] = buffer[i - 1];
			}
		}
		
		/* Insert the character */
		buffer[rl_cursor_pos] = (char)key_code;
		if (rl_cursor_pos < max_chars) {
			rl_cursor_pos++;
		}
		
		read_line_helper2();
		read_line_helper();
		first_key = 0;
	}
}

/*--------------------------------------------------------------
 * ui_window_push_modal - Create modal window with state save
 *
 * Creates a new modal window, saving the current screen state
 * to a sprite buffer. Returns 1 on success, 0 on failure.
 *
 * Parameters:
 *   x1, y1 - Top-left corner coordinates
 *   x2, y2 - Bottom-right corner coordinates
 *
 * Returns:
 *   1 if window created successfully
 *   0 if insufficient memory
 /*--------------------------------------------------------------*/
/** @brief Ui window push modal.
 * @param x1 Parameter `x1`.
 * @param y1 Parameter `y1`.
 * @param x2 Parameter `x2`.
 * @param y2 Parameter `y2`.
 * @return Function result.
 */
unsigned char  ui_window_push_modal(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2)
{
	struct SPRITE saved_sprites[2];
	unsigned long required_mem;
	unsigned long avail_mem;
	struct SPRITE * new_sprite;
	unsigned short idx;
	char * save_ptr;
	unsigned short width;
	unsigned short height;
	unsigned int sprite_bytes;
	
	/* Calculate required memory */
	width = (unsigned short)(x2 - x1);
	height = (unsigned short)(y2 - y1);
	required_mem = (unsigned long)video_flag1_is1 * video_flag4_is1 * width * height + 18;
	avail_mem = mmgr_get_res_ofs_diff_scaled();
	
	if (avail_mem < required_mem) {
		fprintf(stderr,
			"ui_window_push_modal: insufficient mem avail=%lu required=%lu w=%u h=%u\n",
			avail_mem, required_mem, width, height);
		return 0;
	}
	if (window_stack_depth >= 10) {
		fprintf(stderr, "ui_window_push_modal: window stack overflow depth=%u\n", window_stack_depth);
		return 0;
	}
	
	mouse_draw_opaque_check();
	
	/* Create window sprite */
	new_sprite = sprite_make_wnd(width, height, 15);
	if (new_sprite == NULL) {
		fprintf(stderr, "ui_window_push_modal: sprite_make_wnd failed w=%u h=%u\n", width, height);
		return 0;
	}
	
	idx = window_stack_depth;
	sprite_ptrs[idx] = new_sprite;
	window_x_positions[idx] = x1;
	window_y_positions[idx] = y1;
	
	/* Save current sprite states */
	sprite_copy_both_to_arg(saved_sprites);

	/* Save to window stack buffer */
	sprite_bytes = (unsigned int)(sizeof(struct SPRITE));
	save_ptr = window_stack_get_save_ptr(idx);
	if (save_ptr == NULL) {
		fprintf(stderr, "ui_window_push_modal: save buffer unavailable idx=%u\n", idx);
		return 0;
	}
	memcpy(save_ptr, &saved_sprites[0], sizeof(struct SPRITE));
	memcpy(save_ptr + sprite_bytes, &saved_sprites[1], sizeof(struct SPRITE));
	
	/* Copy and clear the new window area */
	sprite_copy_2_to_1();
	sprite_clear_shape_alt(new_sprite->sprite_bitmapptr, x1, y1);
	
	window_stack_depth++;
	return 1;
}

/*--------------------------------------------------------------
 * ui_window_pop_modal - Window cleanup and restoration
 *
 * Closes the topmost modal window and restores the previous
 * screen state from the saved sprite buffer.
 /*--------------------------------------------------------------*/
/** @brief Ui window pop modal.
 */
void ui_window_pop_modal(void)
{
	struct SPRITE saved_sprites[2];
	unsigned short idx;
	char * save_ptr;
	struct SPRITE * sprite_to_free;
	unsigned int sprite_bytes;
	
	if (window_stack_depth == 0) {
		return;
	}
	
	window_stack_depth--;
	idx = window_stack_depth;

	mouse_draw_opaque_check();

	/* Restore sprite to screen */
	sprite_to_free = sprite_ptrs[idx];
	sprite_shape_to_1(sprite_to_free->sprite_bitmapptr, window_x_positions[idx], window_y_positions[idx]);
	
	/* Restore saved sprite states */
	sprite_bytes = (unsigned int)(sizeof(struct SPRITE));
	save_ptr = window_stack_get_save_ptr(idx);
	if (save_ptr == NULL) {
		fprintf(stderr, "ui_window_pop_modal: save buffer unavailable idx=%u\n", idx);
		return;
	}
	memcpy(&saved_sprites[0], save_ptr, sizeof(struct SPRITE));
	memcpy(&saved_sprites[1], save_ptr + sprite_bytes, sizeof(struct SPRITE));
	
	sprite_copy_arg_to_both(saved_sprites);
	
	/* Free the window sprite */
	sprite_free_wnd(sprite_to_free);
	
	mouse_draw_transparent_check();
}

/*--------------------------------------------------------------
 * call_read_line - Interactive text input wrapper
 *
 * Displays an interactive text input line using the read_line
 * function, then trims trailing spaces from the result.
 *
 * Parameters:
 *   buffer - Buffer to store input text
 *   max_chars - Maximum number of characters
 *   dialog_x, dialog_y - Position coordinates
 *   fontptr - Pointer to font data
 *
 * Returns:
/** @brief Read line.
 * @param Enter Parameter `Enter`.
 * @param Escape Parameter `Escape`.
 * @param fontptr Parameter `fontptr`.
 * @return Function result.
 */
 /*--------------------------------------------------------------*/
unsigned short  call_read_line(char* buffer, unsigned short max_chars, 
                                   unsigned short dialog_x, unsigned short dialog_y,
                                   void * fontptr)
{
	unsigned short result;
	unsigned short text_len, i;
	
	mouse_draw_opaque_check();
	
	/* Call the full-featured read_line function */
	result = read_line(buffer, 2, max_chars, 
	                   (max_chars * 9) + 9, max_chars, 
	                   fontptr, 
	                   dialog_x, dialog_y, 
	                   NULL, 0);
	
	mouse_draw_transparent_check();
	
	/* Trim trailing spaces */
	text_len = strlen(buffer);
	for (i = text_len - 1; i > 0 && buffer[i] == ' '; i--) {
		buffer[i] = 0;
	}
	
	return result;
}

/*--------------------------------------------------------------
 * draw_button - Draw a 3D button with text
 *
 * Draws a button with 3D shading effects and centered text.
 * Text can contain multiple lines separated by ']' characters.
 *
 * Parameters:
/** @brief Text.
 * @param x Parameter `x`.
 * @param width Parameter `width`.
 * @param textcolor Parameter `textcolor`.
 * @return Function result.
 */
 /*--------------------------------------------------------------*/
void draw_button(char * buttontext, unsigned short x, unsigned short y, 
                     unsigned short width, unsigned short height,
                     unsigned short topcolor, unsigned short bottomcolor, 
                     unsigned short fillcolor, unsigned short textcolor)
{
	unsigned short right_edge, bottom_edge;
	unsigned short text_width;
	unsigned short line_count;
	unsigned short y_offset, text_idx;
	unsigned short x_center;
	char c;
	char line_text[90];
	unsigned short line_len, line_idx;
	
	right_edge = x + width;
	bottom_edge = y + height;
	
	/* Fill button background */
	sprite_fill_rect(x, y, width, height, (unsigned char)fillcolor);
	
	/* Draw top/left highlight edges */
	preRender_line(x, y, right_edge, y, topcolor);
	preRender_line(x + 1, y + 1, right_edge - 1, y + 1, topcolor);
	preRender_line(x + 2, y + 2, right_edge - 2, y + 2, topcolor);
	
	preRender_line(x, y, x, bottom_edge, topcolor);
	preRender_line(x + 1, y + 1, x + 1, bottom_edge - 1, topcolor);
	preRender_line(x + 2, y + 2, x + 2, bottom_edge - 2, topcolor);
	
	/* Draw bottom/right shadow edges */
	preRender_line(x, bottom_edge, right_edge, bottom_edge, bottomcolor);
	preRender_line(x + 1, bottom_edge - 1, right_edge - 1, bottom_edge - 1, bottomcolor);
	preRender_line(x + 2, bottom_edge - 2, right_edge - 2, bottom_edge - 2, bottomcolor);
	
	preRender_line(right_edge, y, right_edge, bottom_edge, bottomcolor);
	preRender_line(right_edge - 1, y + 1, right_edge - 1, bottom_edge - 1, bottomcolor);
	preRender_line(right_edge - 2, y + 2, right_edge - 2, bottom_edge - 2, bottomcolor);
	
	/* Draw text if provided */
	if (buttontext == 0) {
		return;
	}
	
	font_set_colors(0, textcolor);
	
	/* Copy text and parse it */
	copy_string(resID_byte1, buttontext);
	
	/* Count lines (separated by ']') */
	line_count = 1;
	text_idx = 0;
	while (resID_byte1[text_idx] != 0) {
		if (resID_byte1[text_idx] == ']') {
			line_count++;
		}
		text_idx++;
	}
	
	/* Calculate vertical centering */
	y_offset = (height - (line_count * 8)) / 2 + 1;
	
	/* Draw each line */
	line_idx = 0;
	line_len = 0;
	text_idx = 0;
	
	while (text_idx <= strlen(resID_byte1)) {
		c = resID_byte1[text_idx];
		
		if (c == ']' || c == 0) {
			/* End of line - draw it */
			line_text[line_len] = 0;
			
			text_width = font_get_text_width(line_text);
			x_center = x + (width - text_width) / 2;
			
			font_draw_text(line_text, x_center, y + y_offset + (line_idx * 8));
			
			line_idx++;
			line_len = 0;
		} else {
			line_text[line_len++] = c;
		}
		text_idx++;
	}
}

/*--------------------------------------------------------------
 * mouse_track_op - Scrollbar/slider tracking with mouse
 *
 * Handles mouse interaction for horizontal or vertical sliders.
 * Mode 0: Just draw the slider at current value
 * Mode 1: Interactive tracking with mouse dragging
 *
 * Parameters:
 *   mode - 0=draw only, 1=interactive
/** @brief Bounds.
 * @param y1 Parameter `y1`.
 * @param max_value Parameter `max_value`.
 * @return Function result.
 */
 /*--------------------------------------------------------------*/
unsigned short  mouse_track_op(unsigned short mode, unsigned short x1, unsigned short x2,
                                   unsigned short y1, unsigned short y2, unsigned short value,
                                   unsigned short range_offset, unsigned short max_value)
{
	unsigned short is_vertical;
	unsigned short slider_size;
	unsigned short scaled_divisor;
	unsigned short slider_pos_start, slider_pos_end, slider_handle_size;
	unsigned short mouse_pos = 0, mouse_delta = 0, new_pos = 0, prev_pos = 0;
	unsigned short temp_value;
	
/** @brief Vertical.
 * @param y2 Parameter `y2`.
 * @return Function result.
 */
	/* Determine orientation: horizontal (x2>y2) or vertical (y2>=x2) */
	if (x2 > y2) {
		is_vertical = 0;
		slider_size = x2;
	} else {
		is_vertical = 1;
		slider_size = y2;
	}
	
	/* Calculate scaled divisor for position calculations */
	scaled_divisor = max_value * 4;
	
	/* Calculate slider handle start and end positions */
	slider_pos_start = (unsigned short)(((unsigned long)(slider_size - 1) * value * 4) / scaled_divisor);
	slider_pos_end = (unsigned short)(((unsigned long)(slider_size - 1) * (value + range_offset) * 4) / scaled_divisor);
	slider_handle_size = slider_pos_end - slider_pos_start;
	
	/* Mode 0: Just draw the slider */
	if (mode == 0) {
		/* Draw the full slider bar */
		sprite_fill_rect(x1, y1, x2, y2, 0);
		
		/* Draw the handle */
		if (is_vertical == 0) {
			sprite_fill_rect(x1 + slider_pos_start, y1, slider_handle_size, y2, (unsigned char)dialog_fnt_colour);
		} else {
			sprite_fill_rect(x1, y1 + slider_pos_start, x2, slider_handle_size, (unsigned char)dialog_fnt_colour);
		}
		
		return value;
	}
	
	/* Mode 1: Interactive tracking */
	mouse_pos = is_vertical ? (mouse_ypos - y1) : (mouse_xpos - x1);
	
	/* Check if click is outside the current handle position */
	if (mouse_pos < slider_pos_start || mouse_pos > slider_pos_end) {
		/* Wait for mouse button release */
		while (mouse_butstate & 3) {
			input_checking(timer_get_delta_alt());
		}
		
		/* Adjust value based on click position */
		if (mouse_pos < slider_pos_start) {
			if (value > 0) value--;
		} else {
			if (value < max_value - 1) value++;
		}
	} else {
		/* Mouse clicked on handle - start dragging */
		temp_value = 65535;
		prev_pos = slider_pos_start;
		
		while (mouse_butstate & 3) {
			input_checking(timer_get_delta_alt());
			
			/* Get current mouse position */
			mouse_delta = is_vertical ? (mouse_ypos - y1) : (mouse_xpos - x1);
			
			/* Calculate new slider position */
			new_pos = mouse_delta - mouse_pos + slider_pos_start;
			
			/* Clamp to valid range */
			if ((short)new_pos < 0) {
				new_pos = 0;
			} else if (new_pos + slider_handle_size > slider_size - 1) {
				new_pos = slider_size - slider_handle_size - 1;
			}
			
			/* Redraw if position changed */
			if (new_pos != prev_pos) {
				prev_pos = new_pos;
				
				mouse_draw_opaque_check();
				
				/* Redraw full slider bar */
				sprite_fill_rect(x1, y1, x2, y2, 0);
				
				/* Draw handle at new position */
				if (is_vertical == 0) {
					sprite_fill_rect(x1 + new_pos, y1, slider_handle_size, y2, (unsigned char)dialog_fnt_colour);
				} else {
					sprite_fill_rect(x1, y1 + new_pos, x2, slider_handle_size, (unsigned char)dialog_fnt_colour);
				}
				
				mouse_draw_transparent_check();
			}
		}
		
		/* Calculate final value from slider position */
		if (temp_value == 65535) {
			value = (unsigned short)((((slider_size / max_value / 2) + new_pos) * max_value) / slider_size);
		}
	}
	
	/* Recalculate final handle position */
	slider_pos_start = (unsigned short)(((unsigned long)(slider_size - 1) * value * 4) / scaled_divisor);
	slider_pos_end = (unsigned short)(((unsigned long)(slider_size - 1) * (value + range_offset) * 4) / scaled_divisor);
	slider_handle_size = slider_pos_end - slider_pos_start;
	
	/* Redraw with final position */
	mouse_draw_opaque_check();
	sprite_fill_rect(x1, y1, x2, y2, 0);
	
	if (is_vertical == 0) {
		sprite_fill_rect(x1 + slider_pos_start, y1, slider_handle_size, y2, (unsigned char)dialog_fnt_colour);
	} else {
		sprite_fill_rect(x1, y1 + slider_pos_start, x2, slider_handle_size, (unsigned char)dialog_fnt_colour);
	}
	
	mouse_draw_transparent_check();
	
	return value;
}

/*--------------------------------------------------------------
 * input_checking - Main input handler with keyboard, mouse, and joystick
 /*--------------------------------------------------------------*/
/** @brief Input checking.
 * @param delta Parameter `delta`.
 * @return Function result.
 */

unsigned short  input_checking(unsigned short delta) {
	unsigned short key_code;
	unsigned short joy_flags_new;
	unsigned short joy_changed;
	static unsigned long s_inputcheck_prev_counter = 0;
	enum {
		INPUT_REPEAT_INTERVAL = UI_INPUT_REPEAT_MS
	};

	/* Menu pacing is now handled by video_refresh() calls in each menu
	 * loop iteration.  The old ui_wait_menu_vsync() gate here caused a
	 * double-wait (separate counter from video_present_frame) when g_is_busy=1,
	 * halving the effective frame rate in dialogs.  Removed. */

	/* Some call sites pass delta=1 from tight loops; normalize these calls to
	 * actual timer ticks so menu/key repeat cannot run at CPU speed. */
	if (delta <= 1u) {
		unsigned long curr_counter = timer_get_counter();
		unsigned long inferred_delta;

		if (s_inputcheck_prev_counter == 0 || curr_counter < s_inputcheck_prev_counter) {
			inferred_delta = (unsigned long)delta;
		} else {
			inferred_delta = curr_counter - s_inputcheck_prev_counter;
		}

		s_inputcheck_prev_counter = curr_counter;
		if (inferred_delta > 65535UL) {
			inferred_delta = 65535UL;
		}
		delta = (unsigned short)inferred_delta;
	}

	/* Menu timing is now directly synced to the shared 20 Hz menu loop. */
	
	/* Update frame counters */
	input_framecount += delta;
	if (input_framecount > UI_FRAMECOUNTER_WRAP) {
		input_framecount -= UI_FRAMECOUNTER_SUBTRACT;
		input_framecount2 -= UI_FRAMECOUNTER_SUBTRACT;
		input_framecount3 -= UI_FRAMECOUNTER_SUBTRACT;
	}
	
	/* Check keyboard input */
	key_code = kb_get_char();
	if (key_code != 0) {
		kbormouse = 0;
	}
	
	/* Check joystick */
	joy_flags_new = get_joy_flags();
	kbjoyflags = get_kb_or_joy_flags();
	
	/* Handle joystick state changes */
	if (joyflags != joy_flags_new) {
		joy_changed = joyflags ^ joy_flags_new;
		joy_changed &= joy_flags_new; /* New flags that were just set */
		newjoyflags = joy_changed;
		joyflags = joy_flags_new;
		
		/* Map joystick buttons to key codes */
		if (newjoyflags & 32) {
			joyinputcode = UI_KEY_ENTER;
		} else if (newjoyflags & 16) {
			joyinputcode = UI_KEY_SPACE;
		} else if (newjoyflags & 1) {
			joyinputcode = UI_KEY_UP;
		} else if (newjoyflags & 2) {
			joyinputcode = UI_KEY_DOWN;
		} else if (newjoyflags & 8) {
			joyinputcode = UI_KEY_LEFT;
		} else if (newjoyflags & 4) {
			joyinputcode = UI_KEY_RIGHT;
		}
		
		if (joyinputcode != 0) {
			input_framecount3 = input_framecount;
			kbormouse = 0;
		}
	} else if (joy_flags_new != 0) {
		/* Auto-repeat for held joystick */
		if (input_framecount3 + INPUT_REPEAT_INTERVAL < input_framecount) {
			/* Same logic as initial joystick press for auto-repeat */
			if (newjoyflags & 32) {
				joyinputcode = UI_KEY_ENTER;
			} else if (newjoyflags & 16) {
				joyinputcode = UI_KEY_SPACE;
			} else if (newjoyflags & 1) {
				joyinputcode = UI_KEY_UP;
			} else if (newjoyflags & 2) {
				joyinputcode = UI_KEY_DOWN;
			} else if (newjoyflags & 8) {
				joyinputcode = UI_KEY_LEFT;
			} else if (newjoyflags & 4) {
				joyinputcode = UI_KEY_RIGHT;
			}
			
			if (joyinputcode != 0) {
				input_framecount3 = input_framecount;
				kbormouse = 0;
			}
		}
	}
	
	/* Check mouse state */
	mouse_get_state(&mouse_butstate, &mouse_xpos, &mouse_ypos);
	
	/* Mouse movement detection */
	if (mouse_xpos != mouse_oldx || mouse_ypos != mouse_oldy || mouse_butstate != mouse_oldbut) {
		mouse_oldx = mouse_xpos;
		mouse_oldy = mouse_ypos;
		kbormouse = 1;
		input_framecounter = 0;
		
		/* Redraw mouse if needed */
		if (mouse_motion_detected_flag != 0) {
			if (mouse_isdirty != 0) {
				mouse_draw_opaque();
			}
			mouse_draw_transparent();
			video_refresh();
		}
	} else {
		/* Mouse timeout for hiding.
		 * Original DOS threshold: 500 raw 100Hz ticks = 5 seconds.
/** @brief Units.
 * @param tick Parameter `tick`.
 * @param kbormouse Parameter `kbormouse`.
 * @return Function result.
 */
		if (kbormouse != 0) {
			input_framecounter += delta;
			if (input_framecounter > UI_MOUSE_AUTOHIDE_MS) {
				input_framecounter = 0;
				kbormouse = 0;
				if (mouse_isdirty != 0) {
					mouse_draw_opaque();
					video_refresh();
				}
			}
		}
	}
	
	/* Handle mouse button changes */
	if (kbormouse != 0) {
		if (mouse_butstate != mouse_oldbut) {
			mouse_oldbut = mouse_butstate;

/** @brief Press.
 * @param mouse_butstate Parameter `mouse_butstate`.
 * @return Function result.
 */
			/* Register click on button PRESS (matching original game behavior) */
			if (mouse_butstate & 1) {
				mousebutinputcode = 32; /* Left button = Space */
			} else if (mouse_butstate & 2) {
				mousebutinputcode = 13; /* Right button = Enter */
			}

			if (mousebutinputcode != 0) {
				input_framecount2 = input_framecount;
			}
			input_framecounter = 0;
		} else if (mouse_butstate != 0) {
/** @brief Down.
 * @param input_framecount Parameter `input_framecount`.
 * @return Function result.
 */
			/* Auto-repeat if button held down (matching original) */
			if (input_framecount2 + INPUT_REPEAT_INTERVAL < input_framecount) {
				if (mouse_butstate & 1) {
					mousebutinputcode = 32; /* Space */
				} else if (mouse_butstate & 2) {
					mousebutinputcode = 13; /* Enter */
				}

				if (mousebutinputcode != 0) {
					input_framecount2 = input_framecount;
				}
			}
		}
		
		/* Set button flags for kbjoyflags */
		if (mouse_butstate != 0) {
			if (mouse_butstate & 1) {
				kbjoyflags |= 32;
			} else if (mouse_butstate & 2) {
				kbjoyflags |= 16;
			}
		}
	}

	/* Return input code priority: keyboard > joystick > mouse button */
	if (key_code != 0) {
		return key_code;
	}

	if (joyinputcode != 0) {
		key_code = joyinputcode;
		joyinputcode = 0;
		return key_code;
	}

	if (mousebutinputcode != 0 && kbormouse != 0) {
		key_code = mousebutinputcode;
		mousebutinputcode = 0;
		return key_code;
	}
	if (kbormouse == 0) {
		mousebutinputcode = 0;
	}
	
	return 0;
}
/*--------------------------------------------------------------
 * do_savefile_dialog - Save file dialog with filename and extension input
 *
/** @brief Layout.
 * @param template_ptr Parameter `template_ptr`.
 * @return Function result.
 */
 /*--------------------------------------------------------------*/
/** @brief Perform savefile dialog.
 * @param filename Parameter `filename`.
 * @param extension Parameter `extension`.
 * @param template_ptr Parameter `template_ptr`.
 * @return Function result.
 */

short  do_savefile_dialog(char * filename, char * extension, void * template_ptr) {
	void * dialog_text_ptr;
	unsigned short dialog_result;
	unsigned short coords[6];
	unsigned short key_code;
	unsigned short i;
	char result;
	
	/* Show "sav" dialog — fills coords[0..5] */
	dialog_text_ptr = locate_text_res(mainresptr, "sav");
	dialog_result = ui_dialog_layout(dialog_text_ptr, 0, &coords[0]);
	
	if ((short)dialog_result < 0) {
		return 0;
	}
	
	result = 0;
	
	/* Setup font and draw the extension template label (e.g. ".TRK") */
	font_set_colors(dialog_fnt_colour, dialog_text_color);
	copy_string(resID_byte1, template_ptr);
	sprite_draw_text_opaque(resID_byte1, coords[0], coords[1]);
	
	font_set_colors(dialog_fnt_colour, dialog_text_color);
	
	/* Draw filename (directory path) and extension (track/replay name) */
	sprite_draw_text_opaque(filename, coords[2], coords[3]);
	sprite_draw_text_opaque(extension, coords[4], coords[5]);
	
	mouse_draw_transparent_check();
	video_refresh();  /* Present the dialog before waiting for input */
	
/** @brief Extension.
 * @param name Parameter `name`.
 * @param while Parameter `while`.
 * @return Function result.
 */
	/* Input loop — first read extension (track name), then filename (dir) */
	while (1) {
		/* Get extension input (8 chars) at its display position */
		key_code = call_read_line(extension, 8, coords[4], coords[5], template_ptr);
		
		/* Replace spaces with underscores in extension */
		for (i = 0; extension[i] != 0; i++) {
			if (extension[i] == ' ') {
				extension[i] = '_';
			}
		}
		
		/* Check for ESC or Enter */
		if (key_code == 27) {
			break;
		}
		if (key_code == 13) {
			result = 1;
			break;
		}
		
		/* Get filename (directory path) input (18 chars) at its display position */
		key_code = call_read_line(filename, 18, coords[2], coords[3], template_ptr);
		
		if (key_code == 27) {
			break;
		}
	}
	
	/* Close window */
	ui_window_pop_modal();
	
	return result;
}

/*--------------------------------------------------------------
 * do_joy_restext - Joystick configuration dialog
 * 
 * Displays joystick calibration interface where user assigns
 * joystick directions/buttons to game functions.
 /*--------------------------------------------------------------*/
/** @brief Perform joy restext.
 */

void do_joy_restext(void) {
	unsigned short coords[22];  /* Legacy 16-bit stack words mapped into 22 dialog coordinates */
	unsigned char joy_assigned[9];  /* 9 bytes, one assignment flag per joystick function */
	unsigned short temp_width, temp_height;
	short current_joy, prev_joy;
	unsigned short i, ax;
	unsigned short keycode;
	void * textres_ptr;
	unsigned short dialog_result;
	
	input_push_status();
	game_startup_flag = 1;
	audio_disable_flag2();
	
	/* Show "joy" dialog */
	textres_ptr = locate_text_res(mainresptr, "joy");
	dialog_result = ui_dialog_layout(textres_ptr, 0, &coords[0]);
	
	if ((short)dialog_result <= 0) {
		joystick_assigned_flags = 0;
		goto cleanup;
	}
	
	/* Clear assigned flags */
	for (i = 0; i < 9; i++) {
		joy_assigned[i] = 0;
	}
	
	joystick_assigned_flags = 1;
	mouse_draw_opaque_check();
	
	/* coords mapping from asm (index-based):
	   coords[0..9] provide the anchor positions used for divider lines and slot layout.
	   The four fill calls below mirror the original geometry equations from seg009.
	*/
	
	/* Draw horizontal/vertical divider lines */
	temp_width = coords[9] - coords[3] - 8;
	sprite_fill_rect((unsigned short)(coords[2] - 4), coords[3], 1, temp_width, (unsigned char)dialogarg2);
	sprite_fill_rect((unsigned short)(coords[4] - 4), coords[5], 1, temp_width, (unsigned char)dialogarg2);
	
	temp_height = coords[6] - coords[0];
	sprite_fill_rect(coords[0], (unsigned short)(coords[7] - 4), temp_height, 1, (unsigned char)dialogarg2);
	sprite_fill_rect(coords[0], (unsigned short)(coords[8] - 4), temp_height, 1, (unsigned char)dialogarg2);
	
	/* Setup button positions - coords[10..21] are reused as derived slot anchors
	   for the 9 joystick function slots. */
	/* Row group 1 uses coords[2] as X anchor. */
	coords[10] = coords[2];
	coords[11] = coords[2];
	coords[12] = coords[2];
	/* Row group 2 uses coords[4] as X anchor. */
	coords[13] = coords[4];
	coords[14] = coords[4];
	coords[15] = coords[4];
	/* Column anchors use coords[0]. */
	coords[16] = coords[0];
	coords[17] = coords[0];
	coords[18] = coords[0];
	
	/* Derived button cell size from primary anchors. */
	temp_width = coords[2] - coords[0] - 8;  /* button width */
	temp_height = coords[7] - coords[1] - 8;  /* button height */
	
	prev_joy = -1;
	joystick_init_calibration();
	
	/* Main input loop */
	while (1) {
		keycode = kb_read_char();
		if (keycode != 0) {
			/* Key pressed - check if all assigned */
			for (i = 0; i < 9; i++) {
				joystick_assigned_flags &= joy_assigned[i];
			}
			break;
		}
		
		/* Check joystick */
		ax = get_joy_flags();
		if (ax & 48) {
			/* Joystick buttons pressed - check if all assigned */
			for (i = 0; i < 9; i++) {
				joystick_assigned_flags &= joy_assigned[i];
			}
			break;
		}
		
		current_joy = joystick_direction_lookup(ax);
		if (current_joy == prev_joy) {
			continue;
		}
		
		/* Highlight changed - redraw all slots unhighlighted */
		for (i = 0; i < 9; i++) {
			/* Clear slot with background color */
			/* Position calculation would need proper slot arrays */
		}
		
		/* Highlight current slot */
		if (current_joy >= 0 && current_joy < 9) {
			joy_assigned[current_joy] = 1;
		}
		prev_joy = current_joy;
	}
	
	/* Close dialog */
	ui_window_pop_modal();
	
	if (joystick_assigned_flags == 0) {
		/* Show "jox" error dialog - joystick not fully configured */
		textres_ptr = locate_text_res(mainresptr, "jox");
		ui_dialog_info_restext(textres_ptr);
	}
	
cleanup:
	kb_check();
	mouse_motion_state_flag = 0;
	audio_enable_flag2();
	game_startup_flag = 0;
	input_pop_status();
}

/*--------------------------------------------------------------
 * show_dialog - Main dialog display and input handler
 *
 * Displays a text-based dialog with optional button selection.
 * Text format uses special markers:
 *   [ ] - Button text delimiters
 *   { } - Section separators (4 pixel spacing)
 *   @ - Disabled button marker / coordinate output marker
 *
 * Dialog types:
 *   0 - Display only, return immediately with 0
/** @brief Count.
 * @param x Parameter `x`.
 * @param index Parameter `index`.
 * @param cancel Parameter `cancel`.
 * @param default_button Parameter `default_button`.
 * @return Function result.
 */
 /*--------------------------------------------------------------*/
short  show_dialog(unsigned short dialog_type, unsigned short create_window,
                       void * text_res_ptr, 
                       unsigned short dialog_x, unsigned short dialog_y,
                       unsigned short border_color, unsigned short * coords_array,
                       unsigned char default_button)
{
	/* Local variables matching assembly stack layout */
	unsigned char current_char;           /* Current character */
	unsigned short line_height;          /* Line height (fontdef_line_height + 2) */
	unsigned char selected_button;           /* Selected button */
	unsigned short lowered_input_key;          /* Temp key lowercase */
	char * text_ptr;               /* Text pointer */
	unsigned short second_button_hotkey = 0;          /* Button 2 first char lowercase */
	unsigned char button_char_count;           /* Character count in button */
	unsigned short first_button_hotkey = 0;          /* Button 1 first char lowercase */
	unsigned short text_y_offset;          /* Current Y offset / dialog height */
	unsigned char hit_button_index;           /* Hit test result */
	unsigned short text_width_tmp;          /* Text width temp */
	unsigned char prev_selected_button;           /* Previous button selection */
	unsigned short button_y_start[10];      /* Button Y start positions */
	unsigned short loop_index;          /* Loop counter */
	unsigned short dialog_content_width;          /* Max line width / dialog width */
	char button_text_buf[80];                /* Button text temp buffer */
	unsigned short input_code;          /* Input code */
	unsigned char button_count;           /* Number of buttons */
	char * button_text_ptrs[10];              /* Button text pointers */
	unsigned short button_y_end[10];       /* Button Y end positions */
	unsigned short button_x_end[10];       /* Button X end positions */
	unsigned char coord_write_index;            /* Button coordinate index */
	unsigned short saved_text_ofs_unused;           /* Saved text pointer offset */
	unsigned short saved_text_seg_unused;           /* Saved text pointer segment */
	unsigned char button_text_lengths[20];        /* Button text lengths */
	char dialog_loop_active;                     /* Loop control flag */
	unsigned short line_char_index;           /* Character index in line buffer */
	char line_buf[128];                /* Line buffer */
	unsigned short dialog_x1;           /* Dialog x1 */
	unsigned short dialog_x2;           /* Dialog x2 */
	unsigned short dialog_y1;           /* Dialog y1 */
	unsigned short dialog_y2;           /* Dialog y2 */
	unsigned short button_x_start[10];       /* Button X start positions */

	char c;

	(void)saved_text_ofs_unused;
	(void)saved_text_seg_unused;

	/* Initialize */
	line_height = fontdef_line_height + 2;
	text_y_offset = 0;
	dialog_content_width = 32;  /* Minimum width 32 */
	
	mouse_draw_opaque_check();
	
	text_ptr = (char *)text_res_ptr;
	line_char_index = 0;
	
	/* First pass: measure dialog size */
	while (1) {
		current_char = *text_ptr;
		if (current_char == 0) break;
		
		if (current_char == ']') {
			/* End of line - measure width */
			line_buf[line_char_index] = 0;
			text_width_tmp = font_get_text_width(line_buf);
			if (text_width_tmp > dialog_content_width) {
				dialog_content_width = text_width_tmp;
			}
			line_char_index = 0;
			text_y_offset += line_height;
			   saved_text_ofs_unused = 0;
			   saved_text_seg_unused = 0;
		} else if (*text_ptr == '}') {
			/* Section separator */
			line_buf[line_char_index] = 0;
			text_width_tmp = font_get_text_width(line_buf);
			if (text_width_tmp > dialog_content_width) {
				dialog_content_width = text_width_tmp;
			}
			line_char_index = 0;
			text_y_offset += 4;
			   saved_text_ofs_unused = 0;
			   saved_text_seg_unused = 0;
		} else {
			/* Normal character */
			if (line_char_index < (sizeof(line_buf) - 1)) {
				line_buf[line_char_index++] = current_char;
			}
		}
		text_ptr++;
	}
	
	/* Align dialog width to 8 pixels, add padding */
	dialog_content_width = (dialog_content_width + 24) & 248;
	
	/* Calculate dialog position if auto-centering */
	if (dialog_x == UI_DIALOG_AUTO_POS) {
		dialog_x = ((320 - dialog_content_width) / 2) & 248;
	}
	if (dialog_y == UI_DIALOG_AUTO_POS) {
		dialog_y = (200 - text_y_offset) / 2;
	}
	
	/* Calculate dialog bounds */
	dialog_x1 = dialog_x;
	dialog_x2 = dialog_x + dialog_content_width;
	dialog_y1 = dialog_y - 8;
	dialog_y2 = dialog_y + text_y_offset + 8;

	dialog_x += 8;
	dialog_content_width -= 16;
	
	/* Create modal window if requested */
	if (create_window != 0) {
		if (!ui_window_push_modal(dialog_x1, dialog_y1, dialog_x2, dialog_y2)) {
			fprintf(stderr, "show_dialog: ui_window_push_modal failed\n");
			return -1;
		}
	}
	
	/* Setup sprite buffers */
	sprite_copy_2_to_1();
	sprite_set_1_size(dialog_x1, dialog_x2, dialog_y1, dialog_y2);
	sprite_clear_sprite1_color(0);
	
	/* Draw border */
	sprite_draw_rect_outline(dialog_x - 4, dialog_y - 4,
	              dialog_x + dialog_content_width + 4, dialog_y + text_y_offset + 4,
	              border_color);
	
	/* Set font colors — fontdef[0] is text color, so first arg must be dialog_fnt_colour */
	font_set_colors(dialog_fnt_colour, 0);
	dialog_text_color = 0;
	font_set_colors(dialog_fnt_colour, 0);
	
	/* Second pass: render text and find buttons */
	line_char_index = 0;
	coord_write_index = 0;
	text_ptr = (char *)text_res_ptr;
	text_y_offset = 1;
	
	while (1) {
		current_char = *text_ptr;
		if (current_char == 0 || current_char == '[') break;
		
		if (current_char == ']') {
			/* End of text line - draw it */
			line_buf[line_char_index] = 0;
			sprite_draw_text_opaque(line_buf, dialog_x, dialog_y + text_y_offset);
			line_char_index = 0;
			text_y_offset += line_height;
			   saved_text_ofs_unused = 0;
			   saved_text_seg_unused = 0;
		} else if (*text_ptr == '}') {
			/* Section separator */
			line_buf[line_char_index] = 0;
			sprite_draw_text_opaque(line_buf, dialog_x, dialog_y + text_y_offset);
			line_char_index = 0;
			text_y_offset += 4;
			   saved_text_ofs_unused = 0;
			   saved_text_seg_unused = 0;
		} else if (*text_ptr == '@') {
			/* Coordinate marker for type 3 dialogs */
			if (dialog_type == 3) {
				line_buf[line_char_index] = 0;
				text_width_tmp = font_get_text_width(line_buf);
				if (coords_array != 0) {
					coords_array[coord_write_index] = text_width_tmp + dialog_x;
					coords_array[coord_write_index + 1] = dialog_y + text_y_offset;
				}
				coord_write_index += 2;
			}
			if (line_char_index < (sizeof(line_buf) - 1)) {
				line_buf[line_char_index++] = ' ';
			}
		} else {
			if (line_char_index < (sizeof(line_buf) - 1)) {
				line_buf[line_char_index++] = current_char;
			}
		}
		text_ptr++;
	}

	/* Third pass: parse button definitions */
	button_count = 0;

	/* Skip any separators before the first button */
	while (*text_ptr == '}' || *text_ptr == ' ' || *text_ptr == '\r' || *text_ptr == '\n') {
		text_ptr++;
	}
	
	while (*text_ptr == '[' && button_count < 10) {
		text_ptr++;  /* Skip '[' */
		
		/* Save button text pointer */
		button_text_ptrs[button_count] = text_ptr;
		
		line_buf[line_char_index] = 0;
		text_width_tmp = font_get_text_width(line_buf);
		button_x_start[button_count] = text_width_tmp + dialog_x;
		
		/* Store Y positions */
		button_y_start[button_count] = dialog_y + text_y_offset;
		button_y_end[button_count] = dialog_y + text_y_offset + line_height;
		
		button_char_count = 0;
		if (line_char_index < (sizeof(line_buf) - 1)) {
			line_buf[line_char_index++] = ' ';
		}
		text_width_tmp = 0;
		
		/* Parse button text */
		while (1) {
			current_char = *text_ptr;
			if (current_char == 0 || current_char == '[') break;
			
			if (current_char == ']') {
				/* End of button on this line */
				line_buf[line_char_index] = 0;
				text_width_tmp = font_get_text_width(line_buf);
				line_char_index = 0;
				text_y_offset += line_height;
				text_ptr++;
				break;
			} else if (*text_ptr == '}') {
				/* Section separator within button text */
				line_buf[line_char_index] = 0;
				text_width_tmp = font_get_text_width(line_buf);
				line_char_index = 0;
				text_y_offset += 3;
			} else {
				/* Character in button text */
				if (line_char_index < (sizeof(line_buf) - 1)) {
					line_buf[line_char_index++] = *text_ptr;
				}
				if (button_char_count < 255) {
					button_char_count++;
				}
			}
			text_ptr++;
		}
		
		/* Store button text length */
		button_text_lengths[button_count] = button_char_count;
		line_buf[line_char_index] = 0;
		
		/* If no width calculated yet, measure now */
		if (text_width_tmp == 0) {
			text_width_tmp = font_get_text_width(line_buf);
		}
		
		/* Calculate button X end position */
		button_x_end[button_count] = button_x_start[button_count] + text_width_tmp;
		
		button_count++;

		/* Skip separators between buttons */
		while (*text_ptr == '}' || *text_ptr == ' ' || *text_ptr == '\r' || *text_ptr == '\n') {
			text_ptr++;
		}
	}

	/* Adjust button widths if all buttons have same X start */
	if (button_count > 2) {
		if (button_x_start[0] == button_x_start[1] && button_x_start[1] == button_x_start[2]) {
			for (loop_index = 0; loop_index < button_count; loop_index++) {
				button_x_end[loop_index] = button_x_start[loop_index] + dialog_content_width;
			}
		}
	}
	
	mouse_draw_transparent_check();
	
	selected_button = 1;
	
	/* Handle dialog type */
	switch (dialog_type) {
		case 0:
			/* Display only - return immediately */
			return 0;
			
		case 1:
			/* Wait for any key */
			while (1) {
				input_code = input_checking(timer_get_delta_alt());
				if (input_code != 0) break;
			}
			if (UI_IS_CANCEL(input_code)) {
				selected_button = 0;
			}
			check_input();
			break;
			
		case 3:
			/* Return button count */
			return coord_write_index / 2;
			
		case 4:
			/* Timed display */
			timer_wait_ticks(8L);
			break;
			
		case 2:
		default:
			/* Interactive button selection */
			selected_button = default_button;
			prev_selected_button = 255;
			timer_get_delta_alt();
			mouse_draw_opaque_check();

			if (button_count == 0) {
				selected_button = 255;
				break;
			}

			if (selected_button >= button_count) {
				selected_button = 0;
			}

			if (button_count == 2) {
				loop_index = 0;
				do {
					c = button_text_ptrs[0][loop_index];
					first_button_hotkey = (unsigned char)c;
					loop_index++;
				} while (first_button_hotkey == ' ');
				if (g_ascii_props[first_button_hotkey] & 1) {
					first_button_hotkey += 32;
				}

				loop_index = 0;
				do {
					c = button_text_ptrs[1][loop_index];
					second_button_hotkey = (unsigned char)c;
					loop_index++;
				} while (second_button_hotkey == ' ');
				if (g_ascii_props[second_button_hotkey] & 1) {
					second_button_hotkey += 32;
				}
			}

			if (coords_array != 0 && coords_array[selected_button * 2] == 0) {
				for (loop_index = 0; loop_index < button_count; loop_index++) {
					if (coords_array[loop_index * 2] != 0) {
						selected_button = (unsigned char)loop_index;
						break;
					}
				}
			}

			dialog_loop_active = 1;
			while (dialog_loop_active) {
				if (prev_selected_button != selected_button) {
					mouse_draw_opaque_check();

					for (loop_index = 0; loop_index < button_count; loop_index++) {
						if (selected_button == loop_index) {
							font_set_colors(dialog_text_color, dialog_fnt_colour);
						} else {
							font_set_colors(dialog_fnt_colour, dialog_text_color);
						}

						if (coords_array != 0 && coords_array[loop_index * 2] == 0) {
							font_set_colors(performGraphColor, dialog_text_color);
						}

						text_ptr = button_text_ptrs[loop_index];
						for (line_char_index = 0; line_char_index < button_text_lengths[loop_index] && line_char_index < (sizeof(button_text_buf) - 1); line_char_index++) {
							button_text_buf[line_char_index] = text_ptr[line_char_index];
						}
						button_text_buf[line_char_index] = 0;
						sprite_draw_text_opaque(button_text_buf, button_x_start[loop_index], button_y_start[loop_index]);
					}

					mouse_draw_transparent_check();
					video_refresh();

					prev_selected_button = selected_button;
				}

				input_code = input_checking(timer_get_delta_alt());

				hit_button_index = (unsigned char)mouse_multi_hittest(button_count, button_x_start, button_x_end, button_y_start, button_y_end);
				if (hit_button_index != 255) {
					if (coords_array == 0 || coords_array[hit_button_index * 2] != 0) {
						if (kbormouse != 0 && selected_button != hit_button_index) {
							selected_button = hit_button_index;
						}
						if (kbormouse != 0 && UI_IS_CONFIRM(input_code)) {
							dialog_loop_active = 0;
							continue;
						}
					}
				}

				if (UI_IS_CANCEL(input_code)) {
					selected_button = 255;
					dialog_loop_active = 0;
					continue;
				}

				if (UI_IS_CONFIRM(input_code)) {
					dialog_loop_active = 0;
					continue;
				}

				if (input_code == UI_KEY_LEFT || input_code == UI_KEY_UP) {
					unsigned short tries = button_count;
					do {
						if (selected_button == 0) {
							selected_button = button_count - 1;
						} else {
							selected_button--;
						}
						if (coords_array == 0 || coords_array[selected_button * 2] != 0) {
							break;
						}
						tries--;
					} while (tries != 0);
					continue;
				}

				if (input_code == UI_KEY_RIGHT || input_code == UI_KEY_DOWN) {
					unsigned short tries = button_count;
					do {
						selected_button++;
						if (selected_button >= button_count) {
							selected_button = 0;
						}
						if (coords_array == 0 || coords_array[selected_button * 2] != 0) {
							break;
						}
						tries--;
					} while (tries != 0);
					continue;
				}

				if (button_count == 2 && input_code != 0 && input_code < 256) {
					lowered_input_key = input_code;
					if (g_ascii_props[lowered_input_key] & 1) {
						lowered_input_key += 32;
					}
					if (lowered_input_key == first_button_hotkey) {
						selected_button = 0;
						dialog_loop_active = 0;
					} else if (lowered_input_key == second_button_hotkey) {
						selected_button = 1;
						dialog_loop_active = 0;
					}
				}
			}
			break;
	}

	if (create_window) {
		ui_window_pop_modal();
	}
	return selected_button;
}

/*--------------------------------------------------------------
 * do_fileselect_dialog - File selection browser dialog
 *
 * Displays a file browser dialog allowing the user to select
 * a file matching the given extension pattern. Files are displayed
 * in an alphabetically sorted scrollable list.
 *
 * Parameters:
/** @brief Buffer.
 * @param search Parameter `search`.
 * @param buffer Parameter `buffer`.
 * @param selected Parameter `selected`.
 * @param textresptr Parameter `textresptr`.
 * @return Function result.
 */
 /*--------------------------------------------------------------*/
unsigned short  do_fileselect_dialog(char* pathbuf, char* defaultName,
                                          const char* ext, void * textresptr)
{
	/* Forward declaration for DOS-style file path parsing function from fileio.c */
	extern void parse_filepath_separators_dosptr(const char* src_path, void* dst_path_buffer);
	
	/* Local variables matching assembly - 1820 (1820) bytes stack */
	unsigned short separator_y;          /* Temp calculation */
	unsigned short filename_text_width;          /* Text width temp */
	unsigned char prev_scroll_offset;           /* Previous scroll position */
	unsigned char selected_file_index;           /* Current selection index */
	unsigned char file_count;           /* Number of files found */
	unsigned short dialog_coords[22];      /* Button coordinates from show_dialog (accessed up to index 20) */
	unsigned short list_x;          /* List X position (di) */
	unsigned short filename_row_y;          /* Filename display Y */
	unsigned short separator_x_base;          /* Separator line Y */
	unsigned char row_hit_or_index;           /* Loop counter / hit test result */
	unsigned short row_y_start[10];      /* Row Y start positions (0-9 buttons) */
	unsigned short row_y_end[10];      /* Row Y end positions */
	unsigned short row_x_end[10];      /* Row X end positions */
	unsigned short row_x_start[10];       /* Row X start positions */
	char file_names[128][13];           /* File list (128 files max, 13 chars each = 1664 bytes) */
	unsigned char dialog_accept;           /* Return value (0=cancel, 1=selected) */
	unsigned char search_char;           /* Key character for search */
	unsigned char scroll_offset;           /* Scroll offset */
	unsigned char selection_done;             /* Done flag */
	unsigned char saved_busy_flag;      /* Saved g_is_busy */
	
	char * textres_ptr;
	unsigned short i, si;
	char c;

	unsigned short list_x_end;
	const char* find_result_path;
	unsigned short compare_index;
	unsigned short scroll_up_label_y;
	unsigned short scroll_down_label_y;
	unsigned char prev_selected_index;
	unsigned short nav_input_code;

	(void)textresptr;
	
	/* Show initial "Load" dialog to get button layout */
	/* This is a type 3 dialog that returns number of buttons and their coords */
	textres_ptr = (char *)locate_text_res(mainresptr, "loa");
	si = ui_dialog_layout(textres_ptr, 0, dialog_coords);
	
	if ((short)si < 0) {
		return 0;
	}
	
	/* Save and set busy flag */
	saved_busy_flag = g_is_busy;
	g_is_busy = 1;
	
	/* Get button layout from dialog_coords array (coords stored as X,Y pairs by show_dialog type=3):
	   dialog_coords[0] = X of '@'#0  (extension label X)
	   dialog_coords[1] = Y of '@'#0  (extension label Y)  -- also named legacy_word_710h in orig ASM
	   dialog_coords[2] = X of '@'#1  -- also named list_x in orig ASM
	   dialog_coords[3] = Y of '@'#1  -- also named filename_row_y in orig ASM
	   dialog_coords[4] = X of '@'#2  -- also named separator_x_base in orig ASM
	   dialog_coords[5] = Y of '@'#2  -- also named legacy_word_708h in orig ASM
	   ASM: separator_y = legacy_word_708h + 4  (one add, not two)
	*/
	list_x = dialog_coords[2];   /* X of file-list column */
	filename_row_y = dialog_coords[3];   /* Y of filename display row */
	separator_x_base = dialog_coords[4];   /* X base for separator line */
	/* separator_y = dialog_coords[5] + 4  (ASM: mov ax,[legacy_word_708h]; add ax,4 -- single step) */
	separator_y = dialog_coords[5] + 4;
	
	/* Draw horizontal separator line.
	   ASM push order (cdecl right-to-left): dialogarg2, separator_y, separator_x_base+171, separator_y, separator_x_base-4
	   => preRender_line(x1=separator_x_base-4, y1=separator_y, x2=separator_x_base+171, y2=separator_y, color) */
	preRender_line(separator_x_base - 4, separator_y, separator_x_base + 171, separator_y, dialogarg2);
	
	/* Set font color and display current filename (ASM: font_set_colors(dialog_fnt_colour, dialog_text_color)) */
	font_set_colors(dialog_fnt_colour, dialog_text_color);
	copy_string(resID_byte1, (void *)ext);
	/* ASM: push legacy_word_710h (=Y=dialog_coords[1]), push dialog_coords (=X=dialog_coords[0]), push text
	   => sprite_draw_text_opaque(text, X=dialog_coords[0], Y=dialog_coords[1]) */
	sprite_draw_text_opaque(resID_byte1, dialog_coords[0], dialog_coords[1]);
	
	/* Setup button Y positions for the 10 visible slots (0=up, 1-8=files, 9=down) */
	list_x_end = list_x + 162;  /* X end for file list */
	for (row_hit_or_index = 0; row_hit_or_index < 10; row_hit_or_index++) {
		row_x_start[row_hit_or_index] = list_x;
		row_x_end[row_hit_or_index] = list_x_end;
		
		if (row_hit_or_index == 9) {
			/* Last button (scroll down) */
			row_y_start[row_hit_or_index] = row_y_start[row_hit_or_index - 1] + 10;
		} else {
			row_y_start[row_hit_or_index] = dialog_coords[row_hit_or_index * 2 + 3];
		}
		row_y_end[row_hit_or_index] = row_y_start[row_hit_or_index] + 10;
	}
	
	/* Display current path/filename (ASM: font_set_colors(dialog_fnt_colour, dialog_text_color)) */
	font_set_colors(dialog_fnt_colour, dialog_text_color);
	sprite_draw_text_opaque(pathbuf, list_x, filename_row_y);
	
file_search_loop:
	mouse_draw_transparent_check();
	file_count = 0;
	
	/* Search for files matching pattern */
	find_result_path = file_combine_and_find(pathbuf, "*", (char*)ext);
	if (find_result_path == 0) {
		nullsub_1();
		
// ...existing code...
		/* No files found or need to re-enter filename */
		font_set_colors(dialog_fnt_colour, dialog_text_color);
		
		/* Call read_line to edit the filename */
		si = call_read_line(pathbuf, 18, list_x, filename_row_y, mainresptr);
		
		if (si == UI_KEY_ESCAPE) {  /* ESC pressed */
			dialog_accept = 0;
			goto cleanup;
		}
		goto file_search_loop;
	}
	
	/* Parse first file found into file list */
	parse_filepath_separators_dosptr(find_result_path, resID_byte1);
	for (si = 0; si < file_count; si++) {
		if (strcasecmp(&file_names[si][0], resID_byte1) == 0) {
			break;
		}
	}
	if (si == file_count && file_count < 128) {
		size_t name_len = strnlen(resID_byte1, sizeof(file_names[file_count]) - 1);
		memcpy(&file_names[file_count][0], resID_byte1, name_len);
		file_names[file_count][name_len] = '\0';
		file_count++;
	}
	
	/* Continue finding more files */
	while (1) {
		find_result_path = file_find_next_alt();
		if (find_result_path == 0) break;
		
		parse_filepath_separators_dosptr(find_result_path, resID_byte1);
		for (si = 0; si < file_count; si++) {
			if (strcasecmp(&file_names[si][0], resID_byte1) == 0) {
				break;
			}
		}
		if (si == file_count) {
			if (file_count >= 128) break;  /* Max 128 files */
			size_t name_len = strnlen(resID_byte1, sizeof(file_names[file_count]) - 1);
			memcpy(&file_names[file_count][0], resID_byte1, name_len);
			file_names[file_count][name_len] = '\0';
			file_count++;
		}
	}
	
	nullsub_1();  /* Close file search */
	
	/* Sort file list alphabetically using bubble sort */
	if (file_count > 1) {
		for (si = 0; si < file_count - 1; si++) {
			for (compare_index = si + 1; compare_index < file_count; compare_index++) {
				if (strcmp(&file_names[compare_index][0], &file_names[si][0]) < 0) {
					/* Swap entries */
					char tmp[13];
					memcpy(tmp,                          &file_names[si][0],            13);
					memcpy(&file_names[si][0],           &file_names[compare_index][0],  13);
					memcpy(&file_names[compare_index][0], tmp,                            13);
				}
			}
		}
	}
	
	/* Draw scroll buttons if more than 7 files */
	if (file_count > 7) {
		/* Draw Up button */
		textres_ptr = (char *)locate_text_res(mainresptr, "lsu");
		copy_string(resID_byte1, textres_ptr);
		scroll_up_label_y = row_y_start[1];  /* ASM alias: scroll_up_label_y overlaps row slot 1 */
		si = (unsigned short)font_get_centered_x(resID_byte1);
		sprite_draw_text_opaque(resID_byte1, si, scroll_up_label_y);
		
		/* Draw Down button */
		textres_ptr = (char *)locate_text_res(mainresptr, "lsd");
		copy_string(resID_byte1, textres_ptr);
		scroll_down_label_y = row_y_start[9] - 1;  /* Save Y position */
		si = (unsigned short)font_get_centered_x(resID_byte1);
		sprite_draw_text_opaque(resID_byte1, si, scroll_down_label_y);
	}
	
	/* Initialize selection state */
	selected_file_index = 0;
	scroll_offset = 0;
	prev_selected_index = 255;
	prev_scroll_offset = 255;
	/* Consume the click/press that opened the parent menu item so the
	   file dialog doesn't immediately confirm on first frame. */
	check_input();
	mousebutinputcode = 0;
	joyinputcode = 0;
	timer_get_delta_alt();
	selection_done = 0;
	
	/* Main selection loop */
	while (selection_done == 0) {
		/* Redraw file list if selection or scroll changed */
		if (prev_selected_index != selected_file_index || prev_scroll_offset != scroll_offset) {
			prev_selected_index = selected_file_index;
			prev_scroll_offset = scroll_offset;
			
			mouse_draw_opaque_check();
			
/** @brief Files.
 * @param si Parameter `si`.
 * @return Function result.
 */
			/* Draw visible files (up to 7) */
			for (si = 0; si < 7; si++) {
				i = scroll_offset + si;
				
/** @brief Item.
 * @param black Parameter `black`.
 * @param black Parameter `black`.
 * @param white Parameter `white`.
 * @param selected_file_index Parameter `selected_file_index`.
 * @return Function result.
 */
/* Set highlight color for selected item (ASM parity).
					   Selected:   fontdef[0]=dialog_text_color(black), fontdef[1]=dialog_fnt_colour(white)
					              → font_draw_text paints glyph pixels black, cell BG pixels white.
					   Unselected: fontdef[0]=dialog_fnt_colour(white), fontdef[1]=dialog_text_color(black)
					              → white glyphs on black cell background. */
					if (i == selected_file_index) {
						font_set_colors(dialog_text_color, dialog_fnt_colour);
					} else {
						font_set_colors(dialog_fnt_colour, dialog_text_color);
					}
				
				/* Draw filename or blank if past end of list */
				if (i < file_count) {
					memcpy(resID_byte1, &file_names[i][0], sizeof(file_names[i]));
				} else {
					memcpy(resID_byte1, "        ", 9);  /* 8 spaces + NUL */
				}
				
				sprite_draw_text_opaque(resID_byte1, list_x, row_y_start[si + 2]);
				
				/* Clear area to right of filename (use matching row background color) */
				filename_text_width = font_get_text_width(resID_byte1);
				sprite_fill_rect(list_x + filename_text_width, row_y_start[si + 2], 
				            list_x_end - list_x - filename_text_width, 8,
				            (i == selected_file_index) ? dialog_fnt_colour : dialog_text_color);
			}
			
			mouse_draw_transparent_check();
		}
		
		/* Get input */
		nav_input_code = input_checking(timer_get_delta_alt());
		
		/* Check mouse hit test on buttons */
		row_hit_or_index = (unsigned char)mouse_multi_hittest(10, row_x_start, row_x_end, row_y_start, row_y_end);
		{
			unsigned short mouse_release_click = 0;
			if (kbormouse != 0 && UI_IS_CONFIRM(nav_input_code)) {
				mouse_release_click = 1;
			}
		
		if (row_hit_or_index != 255) {
			if (row_hit_or_index == 0) {
				/* Scroll up button */
				if (mouse_release_click != 0) {
					selected_file_index = 0;
					scroll_offset = 255;  /* Will be fixed up below */
					nav_input_code = 0;
				}
			} else if (row_hit_or_index == 1) {
				/* Scroll up one */
				if (mouse_release_click != 0) {
					if (selected_file_index + scroll_offset > 0) {
						selected_file_index--;
					}
					if (selected_file_index < scroll_offset) {
						scroll_offset = selected_file_index;
					}
					nav_input_code = 0;
				}
			} else if (row_hit_or_index == 9) {
				/* Scroll down button */
				if (mouse_release_click != 0) {
					if (selected_file_index < file_count - 1) {
						selected_file_index++;
					}
					nav_input_code = 0;
				}
			} else {
/** @brief Release.
 * @param mouse_release_click Parameter `mouse_release_click`.
 * @return Function result.
 */
				/* File row selected on mouse-button release (not hover). */
				if (mouse_release_click != 0) {
					i = scroll_offset + row_hit_or_index - 2;
					if (i < file_count) {
						selected_file_index = (unsigned char)i;
					}
				}
			}
		}
		}
		
		/* Handle keyboard input */
		if (UI_IS_CONFIRM(nav_input_code)) {
			/* Enter or Space - select file */
			selection_done = 1;
		} else if (UI_IS_CANCEL(nav_input_code)) {
			/* Escape - cancel */
			selection_done = 255;
		} else if (nav_input_code == UI_KEY_UP) {
			/* Up arrow */
			selected_file_index--;
		} else if (nav_input_code == UI_KEY_DOWN) {
			/* Down arrow */
			if (selected_file_index < file_count - 1) {
				selected_file_index++;
			}
		} else if (nav_input_code != 0) {
			/* Letter key - jump to file starting with that letter */
			c = (char)nav_input_code;
			if (g_ascii_props[(unsigned char)c] & 1) {
				c += 32;  /* Convert to lowercase */
			}
			
			if ((g_ascii_props[(unsigned char)c] & 1) || (g_ascii_props[(unsigned char)c] & 2)) {
				/* Valid letter - search for matching file */
				search_char = c;
				for (row_hit_or_index = 0; row_hit_or_index < file_count; row_hit_or_index++) {
					c = file_names[row_hit_or_index][0];
					if (g_ascii_props[(unsigned char)c] & 1) {
						c += 32;
					}
					if (c == search_char) {
						selected_file_index = row_hit_or_index;
						break;
					}
				}
			}
		}
		
		/* Adjust scroll to keep selection visible */
		if (selected_file_index < scroll_offset) {
			scroll_offset = selected_file_index;
		}
		while (scroll_offset + 6 < selected_file_index) {
			scroll_offset++;
		}
		
		/* Clamp scroll */
		if ((signed char)scroll_offset < 0) {
			// goto input_filename_loop; // Removed: label not defined, unreachable
		}
	}
	
	if (selection_done == 255) {
		/* Cancelled */
		dialog_accept = 0;
	} else {
		/* Copy selected filename into defaultName (9-byte game_trackname field).
		   Intentional truncation: file names can be up to 12 chars. */
		memcpy(defaultName, &file_names[selected_file_index][0], 8);
		defaultName[8] = '\0';
		dialog_accept = 1;
	}
	
cleanup:
	ui_window_pop_modal();
	g_is_busy = saved_busy_flag;
	
	return dialog_accept;
}

/*--------------------------------------------------------------
 * do_mrl_textres - MRL Configuration Menu
 *
 * Displays the machine rating/configuration dialog.
 * Allows user to configure:
/** @brief Flags.
 * @param do_mrl_textres Parameter `do_mrl_textres`.
 * @return Function result.
 */
 /*--------------------------------------------------------------*/
void do_mrl_textres(void)
{
	unsigned short saved_framespersec2;
	char marker_flags[9];
	char text_buf[514];		/* 514 bytes for text */
	char selected;
	int i, j;
	
	input_push_status();
	game_startup_flag = 1;
	audio_disable_flag2();
	
	saved_framespersec2 = framespersec2;
	selected = 0;
	
	do {
		/* Get MRL text resource */
		void * textres = locate_text_res(mainresptr, "mrl");
		copy_string(text_buf, textres);
		
		/* Initialize marker flags */
		for (i = 0; i < 9; i++) {
			marker_flags[i] = 0;
		}
		
		/* Set flags based on current settings */
		marker_flags[timertestflag2] = 1;
		marker_flags[timertestflag + 5] = 1;
		
		if (framespersec2 == 10) {
			marker_flags[7] = 1;
		} else {
			marker_flags[8] = 1;
		}
		
		/* Find '[' markers in text and add '*' for selected items */
		j = 0;
		i = 0;
		while (i < 9) {
			while (text_buf[j] != '[') {
				j++;
			}
			if (marker_flags[i] != 0) {
				text_buf[j + 1] = '*';
			}
			j++;
			i++;
		}
		
		/* Show dialog */
		selected = (char)ui_dialog_show_restext(UI_DIALOG_CONFIRM, 1, (void *)text_buf, UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS,
			performGraphColor, 0, (signed char)selected);
		
		if (selected == (char)255) {
			break;	/* Cancelled */
		}
		
		if (selected == 5) {
			timertestflag = 0;
		} else if (selected == 6) {
			timertestflag = 1;
		} else if (selected == 7) {
			framespersec2 = 10;
		} else if (selected == 8) {
			framespersec2 = 30;
		} else if (selected == 9) {
			break;	/* Exit option */
		} else {
			timertestflag2 = selected;
		}
	} while (1);
	
	/* If framespersec changed, show confirmation */
	if (saved_framespersec2 != framespersec2) {
		void * textres = locate_text_res(mainresptr, "mrs");
		ui_dialog_info_restext(textres);
	}

	game_startup_flag = 0;
	audio_enable_flag2();
	input_pop_status();
}

/*--------------------------------------------------------------
 * ensure_file_exists - Ensure a required file exists
 *
 * Checks if a file exists using file_find. If not found,
 * prompts the user to insert the correct disk and retries.
 *
 * The disk text resource ID is "id{file_idx}" (e.g., "id2", "id3", "id4").
 * This corresponds to (aOpp1_+5)[file_idx*2] in the original ASM.
 *
 * Parameters:
 *   file_idx - Index into findfilenames array (typically 2, 3, or 4)
 /*--------------------------------------------------------------*/
static const char aId4[] = "id4";
static const char aSetup_exe[] = "setup.exe";
static const char aSdtitl_[] = "sdtitl.*";
static const char aTedit_[] = "tedit.*";
static const char aOpp1_[] = "opp1.*\0\xDE\x33\xE2\x33\xE6\x33\0";

/** @brief Ensure file exists.
 * @param file_idx Parameter `file_idx`.
 */
void ensure_file_exists(int file_idx)
{
	char disktext_id[4];
	const char* query;
	const char* find_table[5];

	find_table[0] = aId4;
	find_table[1] = aSetup_exe;
	find_table[2] = aSdtitl_;
	find_table[3] = aTedit_;
	find_table[4] = aOpp1_;
	
	/* Build disk text resource ID: "id2", "id3", "id4", etc. */
	disktext_id[0] = 'i';
	disktext_id[1] = 'd';
	disktext_id[2] = '0' + file_idx;
	disktext_id[3] = '\0';
	
	while (1) {
		if (file_idx < 0 || file_idx > 4) {
			query = aOpp1_;
		} else {
			query = find_table[file_idx];
		}
		/* Check if file exists */
		if (file_find(query) != 0) {
			return;	/* File found */
		}
		
		/* File not found - show insert disk dialog */
		{
			void * textres = locate_text_res(mainresptr, disktext_id);
			ui_dialog_info_restext(textres);
		}
		
		mouse_draw_opaque_check();
		kbormouse = 0;
	}
}

