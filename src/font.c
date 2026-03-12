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

/* font.c — Font rendering extracted from stunts.c */
#define FONT_IMPL
#include "stunts.h"
#include "shape2d.h"
#include "font.h"

/* --- g_fontdef_ptr --- */
unsigned char* g_fontdef_ptr = 0;

/* --- font_set_fontdef2 --- */

// Set font definition from a  pointer to font data
// Calls set_fontdefseg to set the segment, then reads fontdef_line_height from offset 14
/** @brief Handle font operation set fontdef2.
 * @param data Parameter `data`.
 */

void font_set_fontdef2(void * data) {
	set_fontdefseg(data);
	fontdef_line_height = *((unsigned short *)((char *)data + 14));
}

/* --- font_set_fontdef --- */

// Set font definition using the default fontdefptr
/** @brief Select the default font definition buffer and refresh cached metrics.
 */

void font_set_fontdef(void) {
	font_set_fontdef2(fontdefptr);
}

/* --- set_fontdefseg --- */

/*
 * Exit handler functions
 */

/* Far pointer type for exit handler functions */
// Set the font definition pointer directly (no segment arithmetic needed)
/** @brief Set the active font definition buffer.
 * @param data Pointer to the loaded font definition structure.
 */

void set_fontdefseg(void * data) {
	g_fontdef_ptr = (unsigned char*)data;
}

/* --- font_set_colors --- */

// Set font foreground and background colors
// fg_color goes to offset 0, bg_color goes to offset 2 in fontdef structure
/** @brief Handle font operation set colors.
 * @param fg_color Parameter `fg_color`.
 * @param bg_color Parameter `bg_color`.
 */

void font_set_colors(int fg_color, unsigned short bg_color) {
	unsigned short * fontdef = (unsigned short *)g_fontdef_ptr;
	if (fontdef == 0) {
		return;
	}
	fontdef[0] = (unsigned char)fg_color;  // Offset 0 - foreground color as byte extended to word
	fontdef[1] = (unsigned char)bg_color; // Offset 2 - background color as byte extended to word
}

/* --- font_op --- */

// Calculate width of text, limited by maxChars (0 means 0 width)
// Mirrors the old ASM font_op with a character count limit.
/** @brief Compute rendered text width, optionally limited to `maxChars`.
 * @param text Input string to measure.
 * @param maxChars Maximum number of characters to include in the measurement.
 * @return Total pixel width for the measured substring.
 */
unsigned short font_op(char* text, unsigned short maxChars) {
	unsigned char * fontdef = g_fontdef_ptr;
	unsigned short width = 0;
	if (fontdef == 0) {
		return 0;
	}
	unsigned char proportional = fontdef[12];
	unsigned short prop1_width = *(unsigned short *)(fontdef + 16);
	unsigned short default_width = *(unsigned short *)(fontdef + 18);
	unsigned short * char_table = (unsigned short *)(fontdef + 22);
	unsigned short fixed_width = default_width;
	int has_width_byte = 0;
	unsigned short remaining = maxChars;
	if (default_width == 0) {
		default_width = 8;
	}

	if (proportional == 2 || (proportional == 1 && prop1_width == 0)) {
		has_width_byte = 1;
	} else if (proportional == 1 && prop1_width != 0) {
		fixed_width = prop1_width;
	}

	if (remaining == 0) return 0;

	while (*text && remaining--) {
		unsigned char ch = *text++;
		unsigned short char_ptr = char_table[ch];
		unsigned short char_width;

		if (char_ptr == 0) continue; // Character not in font

		if (has_width_byte) {
			char_width = fontdef[char_ptr];
		} else {
			char_width = fixed_width;
		}
		width += char_width;
	}
	return width;
}

/* --- font_op2 --- */

// Calculate total width of text string using current font
// Returns the sum of character widths for the given string
/** @brief Compute rendered text width for the full null-terminated string.
 * @param text Input string to measure.
 * @return Total pixel width for the full string.
 */
unsigned short font_get_text_width(char* text) {
	unsigned char * fontdef = g_fontdef_ptr;
	unsigned short width = 0;
	if (fontdef == 0) {
		return 0;
	}
	unsigned char proportional = fontdef[12];
	unsigned short default_width = *(unsigned short *)(fontdef + 18);
	unsigned short * char_table = (unsigned short *)(fontdef + 22);
	unsigned short prop1_width = *(unsigned short *)(fontdef + 16);
	unsigned short fixed_width = default_width;
	int has_width_byte = 0;
	if (default_width == 0) {
		default_width = 8;
	}

	if (proportional == 2 || (proportional == 1 && prop1_width == 0)) {
		has_width_byte = 1;
	} else if (proportional == 1 && prop1_width != 0) {
		fixed_width = prop1_width;
	}
	
	while (*text) {
		unsigned char ch = *text++;
		unsigned short char_ptr = char_table[ch];
		unsigned short char_width;
		
		if (char_ptr == 0) continue; // Character not in font
		
		if (has_width_byte) {
			char_width = fontdef[char_ptr];
		} else {
			char_width = fixed_width;
		}
		width += char_width;
	}
	return width;
}

/* --- font_draw_text --- */

// Draw text string at specified position using current font
/** @brief Render text onto the current sprite buffer at `(x, y)`.
 * @param str Null-terminated string to draw.
 * @param x Left pixel position where rendering starts.
 * @param y Top pixel position where rendering starts.
 */
void font_draw_text(char* str, unsigned short x, unsigned short y) {
	unsigned char * fontdef = g_fontdef_ptr;
	unsigned char * vram = (unsigned char *)sprite1.sprite_bitmapptr;
	unsigned int * lineofs = sprite1.sprite_lineofs;
	if (fontdef == 0) {
		return;
	}
	unsigned char fg_color = fontdef[0];
	unsigned char proportional = fontdef[12];
	unsigned short font_height = *(unsigned short *)(fontdef + 14);
	unsigned short prop1_width = *(unsigned short *)(fontdef + 16);
	unsigned short default_charwidth = *(unsigned short *)(fontdef + 18);
	unsigned short * char_table = (unsigned short *)(fontdef + 22);
	unsigned short font_x_position_base = *(unsigned short *)(fontdef + 8); // x position (from offset 8)
	int has_width_byte = 0;
		if (font_height == 0) {
			font_height = fontdef_line_height;
			if (font_height == 0) {
				font_height = 8;
			}
		}
		if (default_charwidth == 0) {
			default_charwidth = 8;
		}

		if (proportional == 2 || (proportional == 1 && prop1_width == 0)) {
			has_width_byte = 1;
		}
	
	// Set initial position
	*(unsigned short *)(fontdef + 8) = x;
	*(unsigned short *)(fontdef + 10) = y;
	
	while (1) {
		unsigned char ch = *str;
		unsigned short char_ptr;
		unsigned short char_x, char_y;
		unsigned short char_width_bytes;
		unsigned short char_height;
		unsigned short char_pixel_width;
		unsigned short bx;
		unsigned char * src;
		unsigned short i, j, k;
		
		if (ch == 0) break;
		
		str++;
		char_ptr = char_table[ch];
		
		if (char_ptr == 0) {
			// Handle CR/LF
			if (ch == 13 || ch == 10) {
				*(unsigned short *)(fontdef + 8) = font_x_position_base;
				*(unsigned short *)(fontdef + 10) += (unsigned short)(fontdef_line_height + 2);
			}
			continue;
		}
		
		char_x = *(unsigned short *)(fontdef + 8);
		
		/* proportional==2 and some proportional==1 fonts use width byte at start of glyph
		 * proportional==1 with non-zero offset 16 uses fixed width (e.g. LED font)
		 * proportional==0 means fixed width from offset 18 (default_charwidth)
		 */
		if (has_width_byte) {
			char_pixel_width = fontdef[char_ptr];
			char_ptr++;
		} else if (proportional == 1 && prop1_width != 0) {
			char_pixel_width = prop1_width;
		} else {
			char_pixel_width = default_charwidth;
		}
		char_width_bytes = (char_pixel_width + 7) >> 3;
		
		char_height = font_height;
		char_y = *(unsigned short *)(fontdef + 10);
		src = fontdef + char_ptr;
		bx = char_y << 1;
		(void)bx;
		
		for (i = 0; i < char_height; i++) {
			unsigned short di = lineofs[char_y + i] + char_x;
			
			for (j = 0; j < char_width_bytes; j++) {
				unsigned char byte = *src++;
				for (k = 0; k < 8; k++) {
					if (byte & 128) {
						vram[di] = fg_color;
					}
					byte <<= 1;
					di++;
				}
			}
		}
		
		*(unsigned short *)(fontdef + 8) += char_pixel_width;
	}
}

/* --- font_op2_alt --- */

// Calculate centered x position for text
// Returns (320 - font_get_text_width(text)) / 2
/** @brief Handle font operation get centered x.
 * @param text Parameter `text`.
 * @return Function result.
 */

int font_get_centered_x(char* text) {
	int width = font_get_text_width(text);
	int result = 320 - width;  // 320 - width
	// Arithmetic right shift to divide by 2 (handling negative properly)
	if (result < 0) {
		result = (result - 1) >> 1;
	} else {
		result = result >> 1;
	}
	return result;
}

/* Font globals */
unsigned char font_x_position_base[2] = { 0, 0 };
unsigned short fontdefseg = 0;
void * fontnptr = 0;
void * fontdefptr = 0;
unsigned short fontdef_line_height = 0;
void * fontledresptr = 0;
