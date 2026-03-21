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

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "compat_fs.h"
#define SHAPE2D_IMPL
#include "stunts.h"
#include "memmgr.h"
#include "ressources.h"
#include "shape2d.h"
#include "font.h"

/* Variables moved from data_game.c */
static unsigned char aWindowReleased[32] = { 87, 105, 110, 100, 111, 119, 32, 82, 101, 108, 101, 97, 115, 101, 100, 32, 79, 117, 116, 32, 111, 102, 32, 79, 114, 100, 101, 114, 13, 10, 0, 0 };
static unsigned char aWindowdefOutOfRowTableSpa[36] = { 119, 105, 110, 100, 111, 119, 100, 101, 102, 32, 45, 32, 79, 85, 84, 32, 79, 70, 32, 82, 79, 87, 32, 84, 65, 66, 76, 69, 32, 83, 80, 65, 67, 69, 13, 0 };
static struct SPRITE * mcgawndsprite = 0;
static unsigned char palmap[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

enum {
	/* Sprite/window defaults and limits */
	SHAPE2D_DEFAULT_SPRITE_WIDTH = 320,
	SHAPE2D_DEFAULT_SPRITE_HEIGHT = 200,
	SHAPE2D_SANITIZE_MAX_PITCH = 2048,
	SHAPE2D_SANITIZE_MAX_HEIGHT = 1024,
	SHAPE2D_DIM_LIMIT_MEDIUM = 512,
	SHAPE2D_DIM_LIMIT_LARGE = 1024,
	SHAPE2D_MCGA_WND_ALLOC_TAG = 15,

	/* Header/table offsets and strides */
	SHAPE2D_HEADER_BYTES = 16,
	SHAPE2D_RES_SIZE_FIELD_BYTES = 4,
	SHAPE2D_RES_COUNT_LO_OFFSET = 4,
	SHAPE2D_RES_COUNT_HI_OFFSET = 5,
	SHAPE2D_RES_HEADER_BYTES = 6,
	SHAPE2D_RES_OFFSET_SHIFT = 2,
	SHAPE2D_RES_DATA_SHIFT = 3,
	SHAPE2D_BYTE_SHIFT_8 = 8,
	SHAPE2D_BYTE_SHIFT_16 = 16,
	SHAPE2D_BYTE_SHIFT_24 = 24,

	/* Runtime table sizes */
	SHAPE2D_INCNUMS_COUNT = 256,
	SHAPE2D_LINEOFFSETS_COUNT = 201,

	/* Sentinel/palette values */
	SHAPE2D_TRANSPARENT_COLOR = 255,
	SHAPE2D_BYTE_MASK = 255,
	SHAPE2D_BYTE_LOW_MASK_16 = 255,
	SHAPE2D_NIBBLE_MASK = 15,
	SHAPE2D_NIBBLE_SHIFT = 4,
	SHAPE2D_RLE_MAX_SHORT = 127,
	SHAPE2D_RLE_LONG_SKIP_MARKER = 129,

	/* Pointer sanity */
	SHAPE2D_VALID_PTR_MIN = 1048576u,

	/* Internal buffer sizes */
	SHAPE2D_WND_DEFS_BYTES = 3600,
	SHAPE2D_WND_STACK_MAX = 128,

	/* Font definition byte offsets */
	SHAPE2D_FONTDEF_CURSOR_X_OFFSET = 8,
	SHAPE2D_FONTDEF_CURSOR_Y_OFFSET = 10,
	SHAPE2D_FONTDEF_PROPORTIONAL_OFFSET = 12,
	SHAPE2D_FONTDEF_HEIGHT_OFFSET = 14,
	SHAPE2D_FONTDEF_PROP1_WIDTH_OFFSET = 16,
	SHAPE2D_FONTDEF_DEFAULT_WIDTH_OFFSET = 18,
	SHAPE2D_FONTDEF_CHAR_TABLE_OFFSET = 22,

	/* Misc */
	SHAPE2D_DITHER_ROW_STRIDE_FACTOR = 12
};

static const unsigned long SHAPE2D_FIXED_ONE_16_16 = 65536UL;

static char g_shape2d_namebuf[100];
static char* g_shapeexts_fallback[] = {
	".PVS",
	".XVS",
	".PES",
	".ESH",
	".VSH",
	0
};
static struct SPRITE* g_wnd_stack_ptrs[SHAPE2D_WND_STACK_MAX];
static size_t g_wnd_stack_sizes[SHAPE2D_WND_STACK_MAX];
static size_t g_wnd_stack_count = 0;

/**
 * @brief Copy rows of pixel data into a destination buffer with pitch.
 *
 * @param dst       Destination bitmap buffer.
 * @param dst_size  Size of the destination buffer in bytes.
 * @param destofs   Byte offset into @p dst for the first row.
/** @brief Data.
 * @param success Parameter `success`.
 * @param pitch Parameter `pitch`.
 * @return Function result.
 */
int shape2d_blit_rows(unsigned char* dst,
			      size_t dst_size,
			      unsigned int destofs,
			      const unsigned char* src,
			      unsigned int width,
			      unsigned int height,
			      unsigned int pitch)
{
	unsigned int row;

	if (dst == 0 || src == 0 || width == 0 || height == 0 || pitch < width) {
		return 0;
	}

	for (row = 0; row < height; row++) {
		size_t row_start = (size_t)destofs + ((size_t)row * (size_t)pitch);
		size_t row_end = row_start + (size_t)width;
		if (row_end > dst_size) {
			return 0;
		}
		memcpy(dst + row_start, src + ((size_t)row * (size_t)width), (size_t)width);
	}

	return 1;
}

/**
 * @brief Return the flat pointer to a sprite's line-offset table.
 *
 * @param stored_lineofs  Pointer to the stored line-offset array.
 * @return The same pointer (identity; legacy indirection removed).
 */
unsigned int* shape2d_lineofs_flat(unsigned int* stored_lineofs)
{
	return stored_lineofs;
}

/**
 * @brief Reserve a window-sprite entry on the internal wnd stack.
 *
 * @param base        Base of the wnd_defs buffer.
 * @param capacity    Total capacity of the buffer in bytes.
 * @param next        In/out pointer to the next free position.
/** @brief Sprite.
 * @param success Parameter `success`.
 * @param out_lineofs Parameter `out_lineofs`.
 * @return Function result.
 */
int sprite_wnd_stack_reserve(unsigned char* base,
			     size_t capacity,
			     unsigned char** next,
			     unsigned short height,
			     struct SPRITE** out_sprite,
			     unsigned int** out_lineofs)
{
	size_t spritesize;
	unsigned char* wnd;
	unsigned char* nextwnd;

	if (base == 0 || next == 0 || out_sprite == 0 || out_lineofs == 0) {
		return 0;
	}

	spritesize = sizeof(struct SPRITE) + ((size_t)height * sizeof(unsigned int));
	wnd = *next;
	nextwnd = wnd + spritesize;

	if (wnd < base || nextwnd > (base + capacity) || g_wnd_stack_count >= SHAPE2D_WND_STACK_MAX) {
		return 0;
	}

	g_wnd_stack_ptrs[g_wnd_stack_count] = (struct SPRITE*)wnd;
	g_wnd_stack_sizes[g_wnd_stack_count] = spritesize;
	g_wnd_stack_count++;

	*next = nextwnd;
	*out_sprite = (struct SPRITE*)wnd;
	*out_lineofs = (unsigned int*)(wnd + sizeof(struct SPRITE));
	return 1;
}

/**
 * @brief Release the most recently reserved window-sprite from the wnd stack.
 *
 * @param base       Base of the wnd_defs buffer.
 * @param next       In/out pointer to the next free position.
/** @brief Release.
 * @param success Parameter `success`.
 * @param wndsprite Parameter `wndsprite`.
 * @return Function result.
 */
int sprite_wnd_stack_release(unsigned char* base,
			     unsigned char** next,
			     struct SPRITE* wndsprite)
{
	size_t spritesize;
	unsigned char* wnd;

	if (base == 0 || next == 0 || wndsprite == 0) {
		return 0;
	}
	if (g_wnd_stack_count == 0) {
		return 0;
	}
	if (wndsprite != g_wnd_stack_ptrs[g_wnd_stack_count - 1]) {
		return 0;
	}

	wnd = (unsigned char*)wndsprite;
	spritesize = g_wnd_stack_sizes[g_wnd_stack_count - 1];

	if (wnd < base || wnd + spritesize != *next) {
		return 0;
	}

	*next -= spritesize;
	g_wnd_stack_count--;
	return 1;
}


unsigned char  wnd_defs[SHAPE2D_WND_DEFS_BYTES];
char * next_wnd_def = (char *)wnd_defs;
struct SPRITE  sprite1;
struct SPRITE  sprite2;
unsigned char  incnums[SHAPE2D_INCNUMS_COUNT];

static unsigned int g_shape_lineoffsets[SHAPE2D_LINEOFFSETS_COUNT];
static unsigned char g_shape_runtime_init_done = 0;

/**
 * @brief Clamp sprite bounds to valid ranges.
 *
 * @param spr  Sprite to sanitize in place.
 */
static void shape2d_sanitize_sprite(struct SPRITE * spr)
{
	unsigned short pitch;
	unsigned short height;
	uintptr_t lineofs_ptr;
	uintptr_t bitmap_ptr;

	if (spr == 0) {
		return;
	}

	pitch = spr->sprite_pitch;
	height = spr->sprite_height;
	if (pitch == 0 || pitch > SHAPE2D_SANITIZE_MAX_PITCH) {
		pitch = SHAPE2D_DEFAULT_SPRITE_WIDTH;
	}
	if (height == 0 || height > SHAPE2D_SANITIZE_MAX_HEIGHT) {
		height = SHAPE2D_DEFAULT_SPRITE_HEIGHT;
	}

	if (spr->sprite_top >= height) {
		spr->sprite_top = 0;
	}
	if (spr->sprite_left >= pitch) {
		spr->sprite_left = 0;
	}
	if (spr->sprite_right == 0 || spr->sprite_right > pitch) {
		spr->sprite_right = pitch;
	}
	if (spr->sprite_right <= spr->sprite_left) {
		spr->sprite_left = 0;
		spr->sprite_right = pitch;
	}

	lineofs_ptr = (uintptr_t)spr->sprite_lineofs;
	if (lineofs_ptr < SHAPE2D_VALID_PTR_MIN) {
		spr->sprite_lineofs = (unsigned int*)g_shape_lineoffsets;
	}

	bitmap_ptr = (uintptr_t)spr->sprite_bitmapptr;
	if (bitmap_ptr < SHAPE2D_VALID_PTR_MIN) {
		spr->sprite_bitmapptr = (struct SHAPE2D *)video_get_framebuffer();
	}

	spr->sprite_pitch = pitch;
	spr->sprite_height = height;
	spr->sprite_width2 = spr->sprite_right;
	spr->sprite_left2 = spr->sprite_left;
	spr->sprite_widthsum = spr->sprite_right;
}

/**
 * @brief Initialise global sprite1/sprite2 structures, line-offset tables
 *        and the window definition stack. Guarded; only runs once.
 */
static void shape2d_init_runtime_data(void)
{
	int i;

	if (g_shape_runtime_init_done != 0) {
		return;
	}

	for (i = 0; i < SHAPE2D_LINEOFFSETS_COUNT; i++) {
		g_shape_lineoffsets[i] = (unsigned short)(i * SHAPE2D_DEFAULT_SPRITE_WIDTH);
	}

	for (i = 0; i < SHAPE2D_INCNUMS_COUNT; i++) {
		incnums[i] = (unsigned char)i;
	}

	sprite1.sprite_bitmapptr = (struct SHAPE2D *)video_get_framebuffer();
	sprite1.sprite_reserved1 = 0;
	sprite1.sprite_reserved2 = 0;
	sprite1.sprite_reserved3 = 0;
	sprite1.sprite_lineofs = (unsigned int*)g_shape_lineoffsets;
	sprite1.sprite_left = 0;
	sprite1.sprite_right = SHAPE2D_DEFAULT_SPRITE_WIDTH;
	sprite1.sprite_top = 0;
	sprite1.sprite_height = SHAPE2D_DEFAULT_SPRITE_HEIGHT;
	sprite1.sprite_pitch = SHAPE2D_DEFAULT_SPRITE_WIDTH;
	sprite1.sprite_reserved4 = 0;
	sprite1.sprite_width2 = SHAPE2D_DEFAULT_SPRITE_WIDTH;
	sprite1.sprite_left2 = 0;
	sprite1.sprite_widthsum = SHAPE2D_DEFAULT_SPRITE_WIDTH;
	
	sprite2 = sprite1;
	next_wnd_def = (char *)wnd_defs;
	g_wnd_stack_count = 0;
	g_shape_runtime_init_done = 1;
}

/* Pattern variables for patterned line drawing */

/**
 * @brief Allocate a SPRITE backed by a SHAPE2D bitmap buffer.
 *
 * @param width   Width in pixels.
 * @param height  Height in pixels.
 * @param unk     Unused legacy parameter.
 * @return Pointer to the new window sprite, or NULL on failure.
 */
struct SPRITE * sprite_make_wnd(unsigned int width, unsigned int height, unsigned int unk) {
	int pages;
	unsigned int i;
	struct SPRITE  * farwnd = 0;
	char * shapebuf;
	struct SHAPE2D * hdr;
	unsigned int lineofs;
	unsigned int* lineofsptr = 0;
	
	(void)unk;
	shape2d_init_runtime_data();

	pages = ((width * height + sizeof(struct SHAPE2D)) >> 4) + 1;
	shapebuf = mmgr_alloc_pages("MCGA WINDOW", pages);
	if (shapebuf == 0) {
		fatal_error("sprite_make_wnd: out of memory");
	}
	
	hdr = (struct SHAPE2D *)shapebuf;
	hdr->s2d_width = width;
	hdr->s2d_height = height;
	hdr->s2d_pos_x = 0;
	hdr->s2d_pos_y = 0;
	hdr->s2d_hotspot_x = 0;
	hdr->s2d_hotspot_y = 0;
	memset(shapebuf + sizeof(struct SHAPE2D), 0, width * height);

	if (!sprite_wnd_stack_reserve((unsigned char*)wnd_defs,
			sizeof(wnd_defs),
			(unsigned char**)&next_wnd_def,
			(unsigned short)height,
			&farwnd,
			&lineofsptr)) {
		fatal_error((const char*)aWindowdefOutOfRowTableSpa);
	}

	farwnd->sprite_bitmapptr = hdr;
	farwnd->sprite_lineofs = lineofsptr;
	farwnd->sprite_left = 0;
	farwnd->sprite_left2 = 0;
	farwnd->sprite_right = width;
	farwnd->sprite_pitch = width;	// ??
	farwnd->sprite_top = 0;
	farwnd->sprite_height = height;
	farwnd->sprite_width2 = width;
	farwnd->sprite_widthsum = width;
	
	lineofs = sizeof(struct SHAPE2D);
	for (i = 0; i < height; i++) {
		*lineofsptr = lineofs;
		lineofsptr++;
		lineofs += width;
	}

	return farwnd;
}

/**
/** @brief Sprite make wnd.
 * @param wndsprite Parameter `wndsprite`.
 * @return Function result.
 */
void sprite_free_wnd(struct SPRITE * wndsprite) {
	unsigned char* base;
	unsigned char* next;
	unsigned char* wnd;

	if (wndsprite == 0) {
		return;
	}

	if (wndsprite->sprite_bitmapptr == 0) {
		return;
	}

	base = (unsigned char*)wnd_defs;
	next = (unsigned char*)next_wnd_def;
	wnd = (unsigned char*)wndsprite;

	/* Duplicate/stale free after stack pop: ignore instead of aborting. */
	if (wnd >= next && wnd < (base + sizeof(wnd_defs))) {
		wndsprite->sprite_bitmapptr = NULL;
		return;
	}

	if (!sprite_wnd_stack_release((unsigned char*)wnd_defs,
			(unsigned char**)&next_wnd_def,
			wndsprite)) {
		fatal_error((const char*)aWindowReleased);
	}
	mmgr_release((void *)wndsprite->sprite_bitmapptr);
	wndsprite->sprite_bitmapptr = NULL;
}

/**
 * @brief Set sprite1 from an external SPRITE pointer.
 *
 * Initialises runtime data if needed, copies the argument into sprite1,
 * and sanitizes bounds.
 *
 * @param argsprite  Source sprite to copy into sprite1.
 */
void sprite_set_1_from_argptr(struct SPRITE * argsprite) {
	shape2d_init_runtime_data();
	if (argsprite == 0) {
		return;
	}
	sprite_copy_assign(&sprite1, argsprite);
	shape2d_sanitize_sprite(&sprite1);
	if (sprite1.sprite_lineofs == 0) {
		sprite1.sprite_lineofs = (unsigned int*)g_shape_lineoffsets;
	}
	if (sprite1.sprite_bitmapptr == 0) {
		sprite1.sprite_bitmapptr = (struct SHAPE2D *)video_get_framebuffer();
	}
}

/**
 * @brief Copy sprite2 into sprite1.
 */
void sprite_copy_2_to_1(void) {
	sprite_set_1_from_argptr(&sprite2);
}

/**
 * @brief Copy sprite2 into sprite1 and fill the bitmap with colour 0.
 */
void sprite_copy_2_to_1_clear(void) {
	sprite_set_1_from_argptr(&sprite2);
	sprite_clear_sprite1_color(0);
}

/**
 * @brief Set the global wndsprite as sprite1.
 */
void sprite_select_wnd_as_sprite1(void) {
	sprite_set_1_from_argptr(wndsprite);
}

/**
 * @brief Set the global wndsprite as sprite1 and clear its bitmap.
 */
void sprite_select_wnd_as_sprite1_and_clear(void) {
	sprite_set_1_from_argptr(wndsprite);
	sprite_clear_sprite1_color(0);
}

/**
 * @brief Save sprite1 and sprite2 into a two-element SPRITE array.
 *
 * @param argsprite  Array of at least 2 SPRITE structs.
 */
void sprite_copy_both_to_arg(struct SPRITE* argsprite) {
	/* Copy sprite1 and sprite2 individually to avoid ASAN issues */
	/* (ASAN adds redzones between globals, breaking contiguous access) */
	memcpy(&argsprite[0], &sprite1, sizeof(struct SPRITE));
	memcpy(&argsprite[1], &sprite2, sizeof(struct SPRITE));
}

/**
 * @brief Restore sprite1 and sprite2 from a two-element SPRITE array.
 *
 * @param argsprite  Array of 2 SPRITE structs previously saved.
 */
void sprite_copy_arg_to_both(struct SPRITE* argsprite) {
	shape2d_init_runtime_data();
	if (argsprite == 0) {
		return;
	}
	memcpy(&sprite1, &argsprite[0], sizeof(struct SPRITE));
	memcpy(&sprite2, &argsprite[1], sizeof(struct SPRITE));
	shape2d_sanitize_sprite(&sprite1);
	shape2d_sanitize_sprite(&sprite2);
}

/**
/** @brief Color.
 * @param color Parameter `color`.
 * @return Function result.
 */
void sprite_fill_rect(unsigned short x, unsigned short y, unsigned short width, unsigned short height, unsigned char color) {
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned int destofs;
	int row, col;
	int pitch_skip;
	
	if ((int)height <= 0 || (int)width <= 0) return;
	
	/* Bounds check - y must be within sprite height */
	if (y >= sprite1.sprite_height) {
		printf("[sprite_fill_rect] ERROR: y=%u out of bounds (height=%u)\n", y, sprite1.sprite_height);
		return;
	}
	if (y + height > sprite1.sprite_height) {
		height = sprite1.sprite_height - y;
	}
	if (x + width > sprite1.sprite_pitch) {
		width = sprite1.sprite_pitch - x;
	}
	if ((int)height <= 0 || (int)width <= 0) return;
	
	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	
	destofs = lineofs[y] + x;
	pitch_skip = sprite1.sprite_pitch - width;
	
	for (row = 0; row < (int)height; row++) {
		for (col = 0; col < (int)width; col++) {
			bitmapptr[destofs++] = color;
		}
		destofs += pitch_skip;
	}
}

/**
/** @brief Color.
 * @param bounds Parameter `bounds`.
 * @param color Parameter `color`.
 * @return Function result.
 */
void sprite_fill_rect_clipped(unsigned short x, unsigned short y, unsigned short width, unsigned short height, unsigned char color) {
	int clip;
	int ix = (int)x;
	int iy = (int)y;
	int iwidth = (int)width;
	int iheight = (int)height;
	
	/* Clip left */
	clip = sprite1.sprite_left - ix;
	if (clip > 0) {
		ix += clip;
		iwidth -= clip;
	}
	if (iwidth <= 0) return;
	
	/* Clip right */
	clip = (ix + iwidth) - sprite1.sprite_right;
	if (clip > 0) {
		iwidth -= clip;
	}
	if (iwidth <= 0) return;
	
	/* Clip top */
	clip = sprite1.sprite_top - iy;
	if (clip > 0) {
		iy += clip;
		iheight -= clip;
	}
	if (iheight <= 0) return;
	
	/* Clip bottom */
	clip = (iy + iheight) - sprite1.sprite_height;
	if (clip > 0) {
		iheight -= clip;
	}
	if (iheight <= 0) return;
	
	/* Call the non-clipping fill */
	sprite_fill_rect((unsigned short)ix, (unsigned short)iy, (unsigned short)iwidth, (unsigned short)iheight, color);
}

/** Ported from seg013.asm - draws a rectangle border */
void sprite_draw_rect_outline(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned short color) {
	unsigned short width = x2 - x1 + 1;
	unsigned short height = y2 - y1;
	
	if (width > 0) {
		/* Top line */
		sprite_fill_rect_clipped(x1, y1, width, 1, color);
		/* Bottom line */
		sprite_fill_rect_clipped(x1, y2, width, 1, color);
	}
	if (height > 0) {
		/* Left line */
		sprite_fill_rect_clipped(x1, y1, 1, height, color);
		/* Right line */
		sprite_fill_rect_clipped(x2, y1, 1, height, color);
	}
}

/**
 * @brief Fill the entire sprite1 bitmap with a given colour.
 *
 * @param color  Colour byte to fill.
 */
void sprite_clear_sprite1_color(unsigned char color) {
	int lines;
	int width;
	int widthdiff;
	unsigned int ofs;
	unsigned char * bitmapptr;
	unsigned int * lineofs;

	shape2d_init_runtime_data();

	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	lineofs = (unsigned int *)sprite1.sprite_lineofs;
	if (lineofs == 0) {
		lineofs = g_shape_lineoffsets;
	}

	if (!sprite_compute_clear_plan(&sprite1, lineofs, &ofs, &lines, &width, &widthdiff)) {
		return;
	}

	sprite_execute_clear_plan(bitmapptr, ofs, lines, width, widthdiff, color);
}

/**
 * sprite_putimage - Copy shape bitmap to sprite1 at shape's position
 * 
 * Copies pixels from shape to sprite1, clipping against sprite1 bounds.
 * Position is read from shape->s2d_pos_x/s2d_pos_y.
 */
void sprite_putimage(struct SHAPE2D * shape) {
	unsigned int * lineofs;
	unsigned char * destbitmapptr;
	unsigned char * srcbitmapptr;
	int shapex, shapey;
	int shape_width, shape_height;
	int draw_width, draw_height;
	int src_skip_x, src_skip_y;
	unsigned int destofs;
	int row, col;
	int dest_pitch_skip;

	/* Get shape dimensions and position */
	if (shape == 0 || sprite1.sprite_bitmapptr == 0) {
		return;
	}

	shapex = shape->s2d_pos_x;
	shapey = shape->s2d_pos_y;
	shape_width = shape->s2d_width;
	shape_height = shape->s2d_height;
	if (shape_width <= 0 || shape_height <= 0 ||
		shape_width > SHAPE2D_DIM_LIMIT_MEDIUM || shape_height > SHAPE2D_DIM_LIMIT_MEDIUM) {
		return;
	}
	if (sprite1.sprite_pitch <= 0 || sprite1.sprite_height <= 0 ||
		sprite1.sprite_pitch > SHAPE2D_DIM_LIMIT_LARGE || sprite1.sprite_height > SHAPE2D_DIM_LIMIT_LARGE) {
		return;
	}

	/* Source bitmap starts after SHAPE2D header (16 bytes) */
	srcbitmapptr = ((unsigned char *)shape) + sizeof(struct SHAPE2D);

	/* Vertical clipping */
	src_skip_y = 0;
	draw_height = shape_height;

	if (shapey < sprite1.sprite_top) {
		/* Clip top */
		src_skip_y = sprite1.sprite_top - shapey;
		draw_height -= src_skip_y;
		shapey = sprite1.sprite_top;
	}
	if (shapey + draw_height > sprite1.sprite_height) {
		/* Clip bottom */
		draw_height = sprite1.sprite_height - shapey;
	}
	if (draw_height <= 0) return;

	/* Horizontal clipping */
	src_skip_x = 0;
	draw_width = shape_width;

	if (shapex < sprite1.sprite_left) {
		/* Clip left */
		src_skip_x = sprite1.sprite_left - shapex;
		draw_width -= src_skip_x;
		shapex = sprite1.sprite_left;
	}
	if (shapex + draw_width > sprite1.sprite_right) {
		/* Clip right */
		draw_width = sprite1.sprite_right - shapex;
	}
	if (draw_width <= 0) return;

	/* Set up pointers */
	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	destbitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;

	/* Skip rows in source for top clipping */
	srcbitmapptr += src_skip_y * shape_width + src_skip_x;

	/* Calculate destination offset */
	destofs = lineofs[shapey] + shapex;
	dest_pitch_skip = sprite1.sprite_pitch - draw_width;

	/* Copy pixels */
	for (row = 0; row < draw_height; row++) {
		for (col = 0; col < draw_width; col++) {
			destbitmapptr[destofs++] = *srcbitmapptr++;
		}
		/* Skip clipped pixels on right in source */
		srcbitmapptr += shape_width - draw_width;
		/* Move to next line in destination */
		destofs += dest_pitch_skip;
	}
}

/**
/** @brief Explicit.
 * @param x Parameter `x`.
 * @param shapey Parameter `shapey`.
 * @return Function result.
 */
void sprite_putimage_at(struct SHAPE2D * shape, int shapex, int shapey) {
	unsigned int * lineofs;
	unsigned char * destbitmapptr;
	unsigned char * srcbitmapptr;
	int shape_width, shape_height;
	int draw_width, draw_height;
	int src_skip_x, src_skip_y;
	unsigned int destofs;
	int row, col;
	int dest_pitch_skip;

	if (shape == 0 || sprite1.sprite_bitmapptr == 0) return;

	shape_width  = shape->s2d_width;
	shape_height = shape->s2d_height;
	if (shape_width <= 0 || shape_height <= 0 ||
		shape_width > SHAPE2D_DIM_LIMIT_MEDIUM || shape_height > SHAPE2D_DIM_LIMIT_MEDIUM) return;
	if (sprite1.sprite_pitch <= 0 || sprite1.sprite_height <= 0 ||
		sprite1.sprite_pitch > SHAPE2D_DIM_LIMIT_LARGE || sprite1.sprite_height > SHAPE2D_DIM_LIMIT_LARGE) return;

	srcbitmapptr = ((unsigned char *)shape) + sizeof(struct SHAPE2D);

	src_skip_y  = 0;
	draw_height = shape_height;
	if (shapey < sprite1.sprite_top) {
		src_skip_y   = sprite1.sprite_top - shapey;
		draw_height -= src_skip_y;
		shapey = sprite1.sprite_top;
	}
	if (shapey + draw_height > sprite1.sprite_height)
		draw_height = sprite1.sprite_height - shapey;
	if (draw_height <= 0) return;

	src_skip_x = 0;
	draw_width = shape_width;
	if (shapex < sprite1.sprite_left) {
		src_skip_x  = sprite1.sprite_left - shapex;
		draw_width -= src_skip_x;
		shapex = sprite1.sprite_left;
	}
	if (shapex + draw_width > sprite1.sprite_right)
		draw_width = sprite1.sprite_right - shapex;
	if (draw_width <= 0) return;

	lineofs       = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	destbitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	srcbitmapptr += src_skip_y * shape_width + src_skip_x;
	destofs       = lineofs[shapey] + shapex;
	dest_pitch_skip = sprite1.sprite_pitch - draw_width;

	for (row = 0; row < draw_height; row++) {
		for (col = 0; col < draw_width; col++) {
			destbitmapptr[destofs++] = *srcbitmapptr++;
		}
		srcbitmapptr += shape_width - draw_width;
		destofs      += dest_pitch_skip;
	}
}

/**
/** @brief Sprite1.
 * @param sprite1 Parameter `sprite1`.
 * @param b Parameter `b`.
 * @return Function result.
 */
void sprite_putimage_and(struct SHAPE2D * shape, unsigned short a, unsigned short b) {
	unsigned int * lineofs;
	unsigned char * destbitmapptr;
	unsigned char * srcbitmapptr;
	int shapex, shapey;
	int shape_width, shape_height;
	int draw_width, draw_height;
	int src_skip_x, src_skip_y;
	unsigned int destofs;
	int row, col;
	int dest_pitch_skip;

	/* Position from parameters */
	if (shape == 0 || sprite1.sprite_bitmapptr == 0) {
		return;
	}

	shapex = (short)a;
	shapey = (short)b;
	shape_width = shape->s2d_width;
	shape_height = shape->s2d_height;
	if (shape_width <= 0 || shape_height <= 0 ||
		shape_width > SHAPE2D_DIM_LIMIT_MEDIUM || shape_height > SHAPE2D_DIM_LIMIT_MEDIUM) {
		return;
	}
	if (sprite1.sprite_pitch <= 0 || sprite1.sprite_height <= 0 ||
		sprite1.sprite_pitch > SHAPE2D_DIM_LIMIT_LARGE || sprite1.sprite_height > SHAPE2D_DIM_LIMIT_LARGE) {
		return;
	}

	/* Source bitmap starts after SHAPE2D header (16 bytes) */
	srcbitmapptr = ((unsigned char *)shape) + sizeof(struct SHAPE2D);

	/* Vertical clipping */
	src_skip_y = 0;
	draw_height = shape_height;

	if (shapey < sprite1.sprite_top) {
		src_skip_y = sprite1.sprite_top - shapey;
		draw_height -= src_skip_y;
		shapey = sprite1.sprite_top;
	}
	if (shapey + draw_height > sprite1.sprite_height) {
		draw_height = sprite1.sprite_height - shapey;
	}
	if (draw_height <= 0) return;

	/* Horizontal clipping */
	src_skip_x = 0;
	draw_width = shape_width;

	if (shapex < sprite1.sprite_left) {
		src_skip_x = sprite1.sprite_left - shapex;
		draw_width -= src_skip_x;
		shapex = sprite1.sprite_left;
	}
	if (shapex + draw_width > sprite1.sprite_right) {
		draw_width = sprite1.sprite_right - shapex;
	}
	if (draw_width <= 0) return;

	/* Set up pointers */
	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	destbitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;

	/* Skip rows in source for top clipping */
	srcbitmapptr += src_skip_y * shape_width + src_skip_x;

	/* Calculate destination offset */
	destofs = lineofs[shapey] + shapex;
	dest_pitch_skip = sprite1.sprite_pitch - draw_width;

	/* AND pixels */
	for (row = 0; row < draw_height; row++) {
		for (col = 0; col < draw_width; col++) {
			destbitmapptr[destofs] &= *srcbitmapptr++;
			destofs++;
		}
		srcbitmapptr += shape_width - draw_width;
		destofs += dest_pitch_skip;
	}
}

/**
/** @brief Sprite1.
 * @param sprite1 Parameter `sprite1`.
 * @param b Parameter `b`.
 * @return Function result.
 */
void sprite_putimage_or(struct SHAPE2D * shape, unsigned short a, unsigned short b) {
	unsigned int * lineofs;
	unsigned char * destbitmapptr;
	unsigned char * srcbitmapptr;
	int shapex, shapey;
	int shape_width, shape_height;
	int draw_width, draw_height;
	int src_skip_x, src_skip_y;
	unsigned int destofs;
	int row, col;
	int dest_pitch_skip;

	/* Position from parameters */
	if (shape == 0 || sprite1.sprite_bitmapptr == 0) {
		return;
	}

	shapex = (short)a;
	shapey = (short)b;
	shape_width = shape->s2d_width;
	shape_height = shape->s2d_height;
	if (shape_width <= 0 || shape_height <= 0 ||
		shape_width > SHAPE2D_DIM_LIMIT_MEDIUM || shape_height > SHAPE2D_DIM_LIMIT_MEDIUM) {
		return;
	}
	if (sprite1.sprite_pitch <= 0 || sprite1.sprite_height <= 0 ||
		sprite1.sprite_pitch > SHAPE2D_DIM_LIMIT_LARGE || sprite1.sprite_height > SHAPE2D_DIM_LIMIT_LARGE) {
		return;
	}

	/* Source bitmap starts after SHAPE2D header (16 bytes) */
	srcbitmapptr = ((unsigned char *)shape) + sizeof(struct SHAPE2D);

	/* Vertical clipping */
	src_skip_y = 0;
	draw_height = shape_height;

	if (shapey < sprite1.sprite_top) {
		src_skip_y = sprite1.sprite_top - shapey;
		draw_height -= src_skip_y;
		shapey = sprite1.sprite_top;
	}
	if (shapey + draw_height > sprite1.sprite_height) {
		draw_height = sprite1.sprite_height - shapey;
	}
	if (draw_height <= 0) return;

	/* Horizontal clipping */
	src_skip_x = 0;
	draw_width = shape_width;

	if (shapex < sprite1.sprite_left) {
		src_skip_x = sprite1.sprite_left - shapex;
		draw_width -= src_skip_x;
		shapex = sprite1.sprite_left;
	}
	if (shapex + draw_width > sprite1.sprite_right) {
		draw_width = sprite1.sprite_right - shapex;
	}
	if (draw_width <= 0) return;

	/* Set up pointers */
	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	destbitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;

	/* Skip rows in source for top clipping */
	srcbitmapptr += src_skip_y * shape_width + src_skip_x;

	/* Calculate destination offset */
	destofs = lineofs[shapey] + shapex;
	dest_pitch_skip = sprite1.sprite_pitch - draw_width;

	/* OR pixels */
	for (row = 0; row < draw_height; row++) {
		for (col = 0; col < draw_width; col++) {
			destbitmapptr[destofs] |= *srcbitmapptr++;
			destofs++;
		}
		srcbitmapptr += shape_width - draw_width;
		destofs += dest_pitch_skip;
	}
}

/**
/** @brief Explicit.
 * @param x Parameter `x`.
 * @param engineering Parameter `engineering`.
 * @param movsb Parameter `movsb`.
 * @param y Parameter `y`.
 * @return Function result.
 */
void sprite_putimage_and_alt(void * shapeptr, unsigned short x, unsigned short y) {
	struct SHAPE2D * shape = (struct SHAPE2D *)shapeptr;
	int shapex = (short)x;
	int shapey = (short)y;
	sprite_putimage_at(shape, shapex, shapey);
}

/**
/** @brief At.
 * @param x Parameter `x`.
 * @param framebuffer Parameter `framebuffer`.
 * @param ANDing Parameter `ANDing`.
 * @param y Parameter `y`.
 * @return Function result.
 */
void sprite_putimage_copy_at(void * shapeptr, unsigned short x, unsigned short y) {
	struct SHAPE2D * shape = (struct SHAPE2D *)shapeptr;
	unsigned int * lineofs;
	unsigned char * destbitmapptr;
	unsigned char * srcbitmapptr;
	int shapex, shapey;
	int shape_width, shape_height;
	int draw_width, draw_height;
	int src_skip_x, src_skip_y;
	unsigned int destofs;
	int row, col;
	int dest_pitch_skip;

	if (shape == 0 || sprite1.sprite_bitmapptr == 0) return;

	shapex = (short)x;
	shapey = (short)y;
	shape_width  = shape->s2d_width;
	shape_height = shape->s2d_height;
	if (shape_width <= 0 || shape_height <= 0 ||
		shape_width > SHAPE2D_DIM_LIMIT_MEDIUM || shape_height > SHAPE2D_DIM_LIMIT_MEDIUM) return;
	if (sprite1.sprite_pitch <= 0 || sprite1.sprite_height <= 0 ||
		sprite1.sprite_pitch > SHAPE2D_DIM_LIMIT_LARGE || sprite1.sprite_height > SHAPE2D_DIM_LIMIT_LARGE) return;

	srcbitmapptr = ((unsigned char *)shape) + sizeof(struct SHAPE2D);

	src_skip_y  = 0;
	draw_height = shape_height;
	if (shapey < sprite1.sprite_top) {
		src_skip_y   = sprite1.sprite_top - shapey;
		draw_height -= src_skip_y;
		shapey = sprite1.sprite_top;
	}
	if (shapey + draw_height > sprite1.sprite_height)
		draw_height = sprite1.sprite_height - shapey;
	if (draw_height <= 0) return;

	src_skip_x = 0;
	draw_width = shape_width;
	if (shapex < sprite1.sprite_left) {
		src_skip_x  = sprite1.sprite_left - shapex;
		draw_width -= src_skip_x;
		shapex = sprite1.sprite_left;
	}
	if (shapex + draw_width > sprite1.sprite_right)
		draw_width = sprite1.sprite_right - shapex;
	if (draw_width <= 0) return;

	lineofs      = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	destbitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	srcbitmapptr += src_skip_y * shape_width + src_skip_x;
	destofs       = lineofs[shapey] + shapex;
	dest_pitch_skip = sprite1.sprite_pitch - draw_width;

	for (row = 0; row < draw_height; row++) {
		for (col = 0; col < draw_width; col++) {
			destbitmapptr[destofs++] = *srcbitmapptr++;
		}
		srcbitmapptr += shape_width - draw_width;
		destofs      += dest_pitch_skip;
	}
}

/**
 * sprite_putimage_and_at_shape_origin - AND shape bitmap using shape-origin-relative coords
 *
 * Original ASM subtracts [si+4],[si+6] which are s2d_hotspot_x/y, NOT s2d_pos_x/y.
 */
void sprite_putimage_and_at_shape_origin(void * shapeptr, unsigned short x, unsigned short y) {
	struct SHAPE2D * shape = (struct SHAPE2D *)shapeptr;
	unsigned short draw_x = x - shape->s2d_hotspot_x;
	unsigned short draw_y = y - shape->s2d_hotspot_y;
	sprite_putimage_and(shape, draw_x, draw_y);
}

/**
 * sprite_putimage_or_at_shape_origin - OR shape bitmap using shape-origin-relative coords
 *
 * Original ASM subtracts [si+4],[si+6] which are s2d_hotspot_x/y, NOT s2d_pos_x/y.
 */
void sprite_putimage_or_at_shape_origin(void * shapeptr, unsigned short x, unsigned short y) {
	struct SHAPE2D * shape = (struct SHAPE2D *)shapeptr;
	unsigned short draw_x = x - shape->s2d_hotspot_x;
	unsigned short draw_y = y - shape->s2d_hotspot_y;
	sprite_putimage_or(shape, draw_x, draw_y);
}

/**
/** @brief Sprite putimage and at shape origin.
 * @param y Parameter `y`.
 * @return Function result.
 */
void sprite_putimage_and_alt2(void * shapeptr, unsigned short x, unsigned short y) {
	sprite_putimage_and_at_shape_origin(shapeptr, x, y);
}

/**
/** @brief Sprite putimage or at shape origin.
 * @param y Parameter `y`.
 * @return Function result.
 */
void sprite_putimage_or_alt(void * shapeptr, unsigned short x, unsigned short y) {
	sprite_putimage_or_at_shape_origin(shapeptr, x, y);
}

/**
 * @brief Set up MCGA window 1 by saving sprite2 into the mcga wnd sprite.
 */
void setup_mcgawnd1(void) {
	shape2d_init_runtime_data();
	if (!mcgawndsprite) {
		mcgawndsprite = sprite_make_wnd(SHAPE2D_DEFAULT_SPRITE_WIDTH,
			SHAPE2D_DEFAULT_SPRITE_HEIGHT,
			SHAPE2D_MCGA_WND_ALLOC_TAG);
	}

	sprite_set_1_from_argptr(&sprite2);
	sprite_putimage(mcgawndsprite->sprite_bitmapptr);
}

/**
 * @brief Set up MCGA window 2 by selecting the mcga wnd sprite as sprite1.
 */
void setup_mcgawnd2(void) {
	shape2d_init_runtime_data();
	if (!mcgawndsprite) {
		mcgawndsprite = sprite_make_wnd(SHAPE2D_DEFAULT_SPRITE_WIDTH,
			SHAPE2D_DEFAULT_SPRITE_HEIGHT,
			SHAPE2D_MCGA_WND_ALLOC_TAG);
	}
	
	sprite_set_1_from_argptr(mcgawndsprite);
}

// like locate_resource_by_index()
/**
 * @brief Return a shape at index from a resource chunk.
 *
/** @brief Shape2d resource get shape.
 * @param index Parameter `index`.
 * @return Function result.
 */
struct SHAPE2D * file_get_shape2d(unsigned char * memchunk, int index) {
	return shape2d_resource_get_shape(memchunk, index);
}

/**
 * @brief Return the number of shapes in a resource chunk.
 *
 * @param memchunk  Pointer to the loaded resource chunk.
 * @return Shape count.
 */
unsigned short file_get_res_shape_count(void * memchunk) {
	return shape2d_resource_shape_count((const unsigned char*)memchunk);
}

/**
/** @brief Transpose.
 * @param mempages Parameter `mempages`.
 * @return Function result.
 */
void file_unflip_shape2d(unsigned char * memchunk, char * mempages) {

	int shapecount, counter, width, height;
	struct SHAPE2D * memshape;
	char * membitmapptr;
	unsigned char flag;
	int i, j;

	shapecount = *(unsigned short *)&memchunk[SHAPE2D_RES_COUNT_LO_OFFSET];
	counter = 0;
	do {
		memshape = file_get_shape2d(memchunk, counter);
		membitmapptr = ((char *)memshape) + sizeof(struct SHAPE2D);

		flag = memshape->s2d_plane_nibbles[3];
		if ((flag & 240) == 0) {
			flag = memshape->s2d_plane_nibbles[2] >> SHAPE2D_NIBBLE_SHIFT;
			if (flag != 0) {
				if (flag < 4) {
					width = memshape->s2d_width;
					height = memshape->s2d_height;
					switch (flag - 1) {
						case 0:
							// regular flip
							for (j = 0; j < height; j++) { // height
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[j + i * height];
								}
							}
							break;
						case 1:
							// interlaced flip?
							for (j = 0; j < height; j += 2) { // height
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[(j / 2) + i * height];
								}
							}
							for (j = 0; (j + 1) < height; j += 2) { // height
								for (i = 0; i < width; i++) { // width
									mempages[width + i + j * width] = membitmapptr[((height + j + 1) / 2) + i * height];
								}
							}
							break;
						case 2:
							// refer to loc_32BDE in the original function
							fatal_error("unhandled flip type 2");
							break;
					}
					
					// copy flipped bits from mempages -> subres
					for (j = 0; j < height; j++) { // height
						for (i = 0; i < width; i++) { // width
							membitmapptr[i + j * width] = mempages[i + j * width];
						}
					}
				}
			}
		}
		counter++;
		shapecount--;
	} while (shapecount > 0);

}

/**
/** @brief Transpose.
 * @param mempages Parameter `mempages`.
 * @return Function result.
 */
void file_unflip_shape2d_pes(unsigned char * memchunk, char * mempages) {
	int shapecount, width, height, i, j, x, y;
	unsigned char val;
	unsigned char * membitmapptr;
	struct SHAPE2D * memshape;

	shapecount = file_get_res_shape_count(memchunk);

	for (i = 0; i < shapecount; ++i) {
		memshape = file_get_shape2d(memchunk, i);

		if (!(memshape->s2d_plane_nibbles[3] & 240)) {
			val = (memshape->s2d_plane_nibbles[2] >> SHAPE2D_NIBBLE_SHIFT) & SHAPE2D_NIBBLE_MASK;

			if (val) {
				width = memshape->s2d_width;
				height = memshape->s2d_height;
				membitmapptr = ((unsigned char *)memshape) + sizeof(struct SHAPE2D);
				
				for (j = 0; j < 4; ++j) {
					if (val & 1) {
						for (y = 0; y < height; ++y) {
							for (x = 0; x < width; ++x) {
								mempages[y * width + x] = membitmapptr[x * height + y];
							}
						}
						
						// Copy flipped data from mempages -> subres
						for (y = 0; y < height; ++y) {
							for (x = 0; x < width; ++x) {
								membitmapptr[y * width + x] = mempages[y * width + x];
							}
						}
					}
					membitmapptr += width * height;
					val >>= 1;
				}
			}
		}
	}
}

/**
 * @brief Expand compressed shape data with RLE encoding into mempages.
 *
 * @param memchunk  Pointer to the source compressed shape resource.
 * @param mempages  Destination buffer for expanded shapes.
 */
void file_load_shape2d_expand(unsigned char * memchunk, char * mempages) {
	int shapecount, length, i, j, k, l;
	unsigned char * memchunkptr, * mempagesptr, px, pat;
	uint32_t val;
	uint32_t * offsets;
	uint32_t nextoffset;
	struct SHAPE2D * srcshape, * dstshape;

	shapecount = file_get_res_shape_count(memchunk);
	
	// Skip size.
	memchunkptr = memchunk + SHAPE2D_RES_SIZE_FIELD_BYTES;
	mempagesptr = (unsigned char *)mempages + SHAPE2D_RES_SIZE_FIELD_BYTES;
	
	// Copy count and ids.
	for (i = 0; i < (shapecount * 2 + 1); ++i) {
		unsigned short * dst16 = (unsigned short *)mempagesptr;
		unsigned short * src16 = (unsigned short *)memchunkptr;
		*dst16 = *src16;
		mempagesptr = (unsigned char *)(dst16 + 1);
		memchunkptr = (unsigned char *)(src16 + 1);
	}
	
	// Store pointer to offset table.
	offsets = (uint32_t *)mempagesptr;
	nextoffset = 0;
	
	for (i = 0; i < shapecount; ++i) {
		srcshape = file_get_shape2d(memchunk, i);
		length = srcshape->s2d_width * srcshape->s2d_height;

		offsets[i] = nextoffset;
		nextoffset += sizeof(struct SHAPE2D) + length * 8;
		
		dstshape = file_get_shape2d((unsigned char *)mempages, i);
		*dstshape = *srcshape;
		
		dstshape->s2d_width *= 8;

		if (length && length <= 8000) {
			mempagesptr = (unsigned char *)dstshape + sizeof(struct SHAPE2D);
			
			val = srcshape->s2d_plane_nibbles[1] >> 4;
			val |= val << 8;

			for (j = 0; j < length * 4; ++j) {
				unsigned short * dst16 = (unsigned short *)mempagesptr;
				*dst16 = (unsigned short)val;
				mempagesptr = (unsigned char *)(dst16 + 1);
			}
			memchunkptr = (unsigned char *)srcshape + sizeof(struct SHAPE2D);
			
			for (j = 0; j < 4; ++j) {
				pat = srcshape->s2d_plane_nibbles[j] & SHAPE2D_NIBBLE_MASK;

				if (pat) {
					mempagesptr = (unsigned char *)dstshape + sizeof(struct SHAPE2D);
					for (k = 0; k < length; ++k) {
						px = *memchunkptr++;
						for (l = 0; l < 8; ++l) {
							if (px & 128) {
								*mempagesptr |= pat;
							}
							px <<= 1;
							mempagesptr++;
						}
					}
				}
				else {
					break;
				}
			}
		}
	}
	
	// Final size.
	*(uint32_t *)mempages = (uint32_t)(6 + (shapecount * 8) + nextoffset);
}

/**
/** @brief Size.
 * @param memchunk Parameter `memchunk`.
 * @return Function result.
 */
unsigned short file_get_unflip_size(unsigned char * memchunk) {
	unsigned short i, shapecount, size, maxsize;
	struct SHAPE2D * memshape;

	shapecount = file_get_res_shape_count(memchunk);
	maxsize = 0;
	
	for (i = 0; i < shapecount; i++) {
		memshape = file_get_shape2d(memchunk, i);
		if (memshape == 0) {
			break;
		}
		size = (memshape->s2d_width * memshape->s2d_height + 32) >> 4;
		maxsize = (maxsize > size) ? maxsize : size;
	}
	return maxsize;
}

/**
/** @brief Size.
 * @param memchunk Parameter `memchunk`.
 * @return Function result.
 */
unsigned short file_load_shape2d_expandedsize(void * memchunk) {
	unsigned short shapecount, i;
	long size;
	struct SHAPE2D * memshape;
	
	shapecount = file_get_res_shape_count(memchunk);

	size = (shapecount * 8) + sizeof(struct SHAPE2D);

	for (i = 0; i < shapecount; ++i) {
		memshape = file_get_shape2d(memchunk, i);
		if (memshape == 0) {
			break;
		}
		size += (memshape->s2d_width * memshape->s2d_height * 8) + sizeof(struct SHAPE2D);
	}

	return (size + sizeof(struct SHAPE2D)) >> 4;
}

/**
 * @brief Initialise the palette remap table from a 16-byte palette array.
 *
/** @brief Array.
 * @param pal Parameter `pal`.
 * @return Function result.
 */
void file_load_shape2d_palmap_init(unsigned char * pal) {
	int i;
	
	for (i = 0; i < 16; ++i) {
		palmap[i] = pal[i];
	}
}

/**
 * @brief Apply the palette remap table to all shapes in a resource chunk.
 *
 * @param memchunk  Pointer to the loaded shape resource.
/** @brief Table.
 * @param palmap Parameter `palmap`.
 * @return Function result.
 */
void file_load_shape2d_palmap_apply(unsigned char * memchunk, unsigned char palmap[]) {
	unsigned short shapecount, length, i, j;
	unsigned char * memchunkptr;
	struct SHAPE2D * memshape;
	
	shapecount = file_get_res_shape_count(memchunk);
	
	for (i = 0; i < shapecount; ++i) {
		memshape = file_get_shape2d(memchunk, i);
		length = memshape->s2d_width * memshape->s2d_height;
		
		memchunkptr = (unsigned char *)memshape + sizeof(struct SHAPE2D);
		
		for (j = 0; j < length; ++j) {
			*memchunkptr = palmap[*memchunkptr];
			memchunkptr++;
		}
	}
}

/**
 * @brief Load and expand a shape resource, applying palette remap.
 *
 * Allocates memory, expands RLE data, and applies palette mapping.
 *
 * @param memchunk  Pointer to the source compressed shape resource.
 * @param str       Resource name for memory allocation.
 * @return Pointer to the expanded shape chunk, or NULL on failure.
 */
void * file_load_shape2d_esh(void * memchunk, const char* str) {
	unsigned short expandedsize;
	void * mempages;
	void * palmapres;

	expandedsize = file_load_shape2d_expandedsize(memchunk);

	palmapres = locate_shape_nofatal(memchunk, "!MGA");
	
	if (palmapres) {
		file_load_shape2d_palmap_init(((unsigned char *)palmapres) + sizeof(struct SHAPE2D));
	}
	
	mempages = mmgr_alloc_pages(str, expandedsize);

	*(uint32_t*)mempages = (uint32_t)expandedsize * 16u;
	
	file_load_shape2d_expand(memchunk, mempages);
	mmgr_release(memchunk);
	memchunk = mmgr_normalize_ptr(mempages);
	file_load_shape2d_palmap_apply(memchunk, palmap);
	
	return memchunk;
}

/**
 * @brief Load a shape2d resource by filename, trying multiple extensions.
 *
 * Tries .PVS, .XVS, and .PES extensions. Applies un-flip and expansion
 * as needed. If @p fatal is set, aborts on failure.
 *
/** @brief File.
 * @param chunk Parameter `chunk`.
 * @param fatal Parameter `fatal`.
 * @return Function result.
 */
void * file_load_shape2d(char* shapename, int fatal) {
	char* str = g_shape2d_namebuf;
	char** extlist;
	char* strptr;
	int counter;
	int namelen;
	void * memchunk;
	void * mempages;
	int unflipsize;

	namelen = 0;
	while (namelen < (int)sizeof(g_shape2d_namebuf) - 1 && shapename[namelen] != 0) {
		str[namelen] = shapename[namelen];
		namelen++;
	}
	if (namelen >= (int)sizeof(g_shape2d_namebuf) - 1) {
		if (fatal) {
			fatal_error("unhandled - invalid shape name");
		}
		return 0;
	}
	str[namelen] = 0;
	strptr = str;
	extlist = g_shapeexts_fallback;
	
	while (*strptr != '.' && *strptr) {
		strptr++;
	}
	
	if (*strptr != 0) {
		if (fatal) {
			fatal_error("unhandled - load_2dshape has dot in the name");
		}
		return 0;
	}

	for (counter = 0; extlist[counter] != 0; counter++) {
		size_t remaining = sizeof(g_shape2d_namebuf) - (size_t)(strptr - g_shape2d_namebuf);
		snprintf(strptr, remaining, "%s", extlist[counter]);

		if (file_find(str)) {
			if (fs_strcasecmp(strptr, ".PVS") == 0) {
				memchunk = file_decomp(str, fatal);
				if (!memchunk) return NULL;
				
				unflipsize = file_get_unflip_size(memchunk);
				mempages = mmgr_alloc_pages("UNFLIP", unflipsize);
				file_unflip_shape2d(memchunk, mempages);
				mmgr_release(mempages);
				
				return memchunk;
			}
			else if (fs_strcasecmp(strptr, ".XVS") == 0) {
				return file_decomp(str, fatal);
			}
			else if (fs_strcasecmp(strptr, ".PES") == 0) {
				memchunk = file_decomp(str, fatal);
				if (!memchunk) return NULL;
				
				mempages = mmgr_alloc_pages("UNFLIP", 1000);
				file_unflip_shape2d_pes(memchunk, mempages);
				mmgr_release(mempages);
				return file_load_shape2d_esh(memchunk, str);
			}
			else if (fs_strcasecmp(strptr, ".ESH") == 0) {
				memchunk = file_load_binary(str, fatal);
				if (!memchunk) return NULL;

				return file_load_shape2d_esh(memchunk, str);
			}
			else { // .VSH
				return file_load_binary(str, fatal);
			}
		}
	}
	if (fatal) {
		fatal_error("unhandled - cannot load %s", str);
	}
	return 0;
}

/**
 * @brief Load a shape2d resource; abort on failure.
 *
 * @param shapename  Shape filename.
 * @return Pointer to the loaded shape chunk.
 */
void * file_load_shape2d_fatal(char* shapename) {
	return file_load_shape2d(shapename, 1);
}

/**
 * @brief Load a shape2d resource; return NULL on failure.
 *
 * @param shapename  Shape filename.
 * @return Pointer to the loaded shape chunk, or NULL.
 */
void * file_load_shape2d_nofatal(const char* shapename) {
	return file_load_shape2d((char*)shapename, 0);
}

/**
 * shape2d_count_matching_prefix_bytes - Count consecutive matching bytes
 * 
 * Counts how many bytes at ptr match the first byte.
 * Returns 0 if the second byte differs from first, 1 if two match, etc.
 */
unsigned short shape2d_count_matching_prefix_bytes(unsigned char * ptr, unsigned short maxlen) {
	unsigned char firstbyte = *ptr;
	unsigned short count = 0;
	
	while (count < maxlen && *ptr == firstbyte) {
		ptr++;
		count++;
	}
	return count;
}

/* External math helpers for pointer arithmetic */

/**
 * parse_shape2d - Parse and compress shape data
 * 
 * Processes shape data from memchunk to mempages using RLE encoding.
 * This is a complex function that handles the shape resource format.
 */
void parse_shape2d(void * memchunk_arg, void * mempages_arg) {
	unsigned char * memchunk = (unsigned char *)memchunk_arg;
	unsigned char * mempages = (unsigned char *)mempages_arg;
	unsigned char * chunkptr;
	unsigned char * pagesptr;
	unsigned char * pagescnt6ptr;
	unsigned char * outptr;
	unsigned char * srcptr;
	unsigned char * runptr;
	struct SHAPE2D * shape;
	unsigned short shapecount;
	unsigned short counter;
	unsigned short pixelsleft;
	unsigned short skipcount;
	unsigned short runlen;
	unsigned short i;
	
	/* Get shape count from memchunk */
	shapecount = file_get_res_shape_count(memchunk);
	
	/* Calculate pointers */
	/* pagescnt6ptr = mempages + 6 + shapecount*4 */
	pagescnt6ptr = mempages + SHAPE2D_RES_HEADER_BYTES + (shapecount << SHAPE2D_RES_OFFSET_SHIFT);
	pagesptr = mempages;
	chunkptr = memchunk;
	
	/* Copy header: first (6 + shapecount*4) bytes */
	for (counter = 0; counter < SHAPE2D_RES_HEADER_BYTES + (shapecount << SHAPE2D_RES_OFFSET_SHIFT); counter++) {
		*pagesptr++ = *chunkptr++;
	}
	
	/* outptr starts after header + shape offset table */
	/* outptr = mempages + 6 + shapecount*8 */
	outptr = mempages + SHAPE2D_RES_HEADER_BYTES + (shapecount << SHAPE2D_RES_DATA_SHIFT);
	
	/* Process each shape */
	for (counter = 0; counter < shapecount; counter++) {
		/* Get shape from source */
		shape = file_get_shape2d(memchunk, counter);
		
		/* Calculate relative offset and store in offset table */
		{
			unsigned char * baseptr = mempages + SHAPE2D_RES_HEADER_BYTES + (shapecount << SHAPE2D_RES_DATA_SHIFT);
			uint32_t diffval = (uint32_t)(outptr - baseptr);
			*(uint32_t *)pagescnt6ptr = diffval;
			pagescnt6ptr += sizeof(uint32_t);
		}
		
		/* Copy 16-byte shape header */
		srcptr = (unsigned char *)shape;
		for (i = 0; i < SHAPE2D_HEADER_BYTES; i++) {
			*outptr++ = *srcptr++;
		}
		
		/* runptr points to bitmap data after header */
		runptr = srcptr;
		skipcount = 0;
		
		/* Calculate total pixels to process */
		pixelsleft = shape->s2d_width * shape->s2d_height;
		
		/* Process bitmap with RLE */
		while (pixelsleft > 0) {
			unsigned short scanleft;
			/* Count how many consecutive bytes match */
			scanleft = (skipcount < pixelsleft) ? (unsigned short)(pixelsleft - skipcount) : 0;
			runlen = (scanleft > 0) ? shape2d_count_matching_prefix_bytes(runptr, scanleft) : 0;
			
/** @brief Skip.
 * @param matching Parameter `matching`.
 * @param scanleft Parameter `scanleft`.
 * @return Function result.
 */
			/* Check for short skip (3 or fewer matching, and more pixels left) */
			if (runlen <= 3 && scanleft > 0) {
				/* Accumulate skip count */
				runptr++;
				skipcount++;
				continue;
			}
			
			/* Emit accumulated skipped bytes first */
			if (skipcount > SHAPE2D_RLE_MAX_SHORT) {
				/* Long skip: emit 129 followed by 127 bytes */
				skipcount -= SHAPE2D_RLE_MAX_SHORT;
				pixelsleft -= SHAPE2D_RLE_MAX_SHORT;
				*outptr++ = SHAPE2D_RLE_LONG_SKIP_MARKER;
				for (i = 0; i < SHAPE2D_RLE_MAX_SHORT; i++) {
					*outptr++ = *srcptr++;
				}
			} else if (skipcount > 0) {
				/* Short skip: emit negative count, then bytes */
				*outptr++ = (unsigned char)(-(signed char)skipcount);
				pixelsleft -= skipcount;
				for (i = 0; i < skipcount; i++) {
					*outptr++ = *srcptr++;
				}
			}
			
			/* Clamp runlen to remaining pixels */
			if (runlen > pixelsleft) {
				runlen = pixelsleft;
			}
			
			/* Emit run */
			if (runlen > SHAPE2D_RLE_MAX_SHORT) {
				/* Long run: emit 127 bytes with same value */
				pixelsleft -= SHAPE2D_RLE_MAX_SHORT;
				runlen -= SHAPE2D_RLE_MAX_SHORT;
				*outptr++ = SHAPE2D_RLE_MAX_SHORT;
				*outptr++ = *runptr;
				runptr += SHAPE2D_RLE_MAX_SHORT;
			} else if (runlen > 3) {
				/* Regular run */
				pixelsleft -= runlen;
				*outptr++ = (unsigned char)runlen;
				*outptr++ = *runptr;
				runptr += runlen;
			}
			
			srcptr = runptr;
			skipcount = 0;
		}
		
		/* End of shape marker */
		*outptr++ = 0;
	}
}

/**
 * @brief Load a shape2d as a named resource with memory-manager caching.
 *
 * Returns a cached copy if already loaded. Otherwise loads, parses,
 * and stores the result under the given resource name.
 *
 * @param resname  Resource path/name.
 * @param fatal    Non-zero to abort on load failure.
 * @return Pointer to the loaded/cached shape chunk, or NULL.
 */
void * file_load_shape2d_res(char* resname, int fatal) {
	int chunksize;
	const char* shapename = mmgr_path_to_name(resname);
	void * mempages;
	void * memchunk = mmgr_get_chunk_by_name(shapename);

	if (memchunk) return memchunk;

	memchunk = file_load_shape2d((char*)shapename, fatal);
	if (!memchunk) return 0;

	chunksize = mmgr_get_chunk_size(memchunk);
	mempages = mmgr_alloc_pages(resname, chunksize);

	parse_shape2d(memchunk, mempages);

	mmgr_release(memchunk);
	return mmgr_normalize_ptr(mempages);
}

/**
 * @brief Load a shape2d resource by name; abort on failure.
 *
 * @param resname  Resource name.
 * @return Pointer to the loaded shape chunk.
 */
void * file_load_shape2d_res_fatal(char* resname) {
	return file_load_shape2d_res(resname, 1);
}

/**
 * @brief Load a shape2d resource by name; return NULL on failure.
 *
 * @param resname  Resource name.
 * @return Pointer to the loaded shape chunk, or NULL.
 */
void * file_load_shape2d_res_nofatal(char* resname) {
	return file_load_shape2d_res(resname, 0);
}

/**
/** @brief File load shape2d res nofatal.
 * @param chunk Parameter `chunk`.
 * @param resname Parameter `resname`.
 * @return Function result.
 */
void * file_load_shape2d_res_nofatal_thunk(const char* resname) {
	return file_load_shape2d_res_nofatal((char*)resname);
}

/**
/** @brief File load shape2d fatal.
 * @param shapename Parameter `shapename`.
 * @return Function result.
 */
void * file_load_shape2d_fatal_thunk(const char* shapename) {
	return file_load_shape2d_fatal((char*)shapename);
}

/**
/** @brief File load shape2d nofatal.
 * @param chunk Parameter `chunk`.
 * @param shapename Parameter `shapename`.
 * @return Function result.
 */
void * file_load_shape2d_nofatal_thunk(const char* shapename) {
	return file_load_shape2d_nofatal(shapename);
}

/**
 * putpixel_single_clipped - Draw a single pixel with bounds checking
 * 
 * Checks if (x,y) is within sprite1 bounds before drawing.
 * 
/** @brief Value.
 * @param x Parameter `x`.
 * @return Function result.
 */
void putpixel_single_clipped(int color, unsigned short y, unsigned short x) {
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned int ofs;

	/* Bounds check x */
	if ((int)x < sprite1.sprite_left) return;
	if ((int)x >= sprite1.sprite_right) return;
	
	/* Bounds check y */
	if ((int)y < sprite1.sprite_top) return;
	if ((int)y >= sprite1.sprite_height) return;

	/* Draw pixel */
	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	ofs = lineofs[y] + x;
	bitmapptr[ofs] = (unsigned char)color;
}

/**
 * draw_filled_lines - Fill horizontal lines with solid color
 * 
 * Fills a series of horizontal line segments with a solid color.
 * Uses optimized word writes when possible.
 * 
/** @brief Value.
 * @param x1arr Parameter `x1arr`.
 * @return Function result.
 */
void draw_filled_lines(unsigned short color, unsigned short numlines,
                           unsigned short y, unsigned short * x2arr,
                           unsigned short * x1arr) {
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned short colorbyte;
	int i, cx, x1, x2;
	int sx1, sx2;
	unsigned int ofs;
	int sprite_left = sprite1.sprite_left2;
	int sprite_right = sprite1.sprite_widthsum - 1;

	if (numlines == 0) return;

	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	colorbyte = (unsigned char)color;

	for (i = 0; i < numlines; i++) {
		sx1 = (int)(int16_t)(*x1arr);
		sx2 = (int)(int16_t)(*x2arr);

		if (sx2 < sx1) {
			x1arr++;
			x2arr++;
			continue;
		}
		if (sx2 < sprite_left || sx1 > sprite_right) {
			x1arr++;
			x2arr++;
			continue;
		}

		x1 = sx1;
		x2 = sx2;
		
		/* Clamp x1 and x2 to valid sprite bounds */
		if (x1 < sprite_left) x1 = sprite_left;
		if (x1 > sprite_right) x1 = sprite_right;
		if (x2 < sprite_left) x2 = sprite_left;
		if (x2 > sprite_right) x2 = sprite_right;
		
		cx = x2 - x1 + 1;
		if (cx > 0) {
			ofs = lineofs[y + i] + x1;
			/* Simple fill - memset equivalent */
			while (cx-- > 0) {
				bitmapptr[ofs++] = colorbyte;
			}
		}
		x1arr++;
		x2arr++;
	}
}

/**
 * sprite_draw_line_from_info - Draw a line using pre-computed line info
 *
 * This function draws lines using various optimized modes based on
 * the line slope and direction. The LINEINFO struct is populated
/** @brief Draw line related.
 * @param field_12 Parameter `field_12`.
 * @param line Parameter `line`.
 * @param increasing Parameter `increasing`.
 * @param diagonal Parameter `diagonal`.
 * @param increasing Parameter `increasing`.
 * @param diagonal Parameter `diagonal`.
 * @param increasing Parameter `increasing`.
 * @param Diagonal Parameter `Diagonal`.
 * @param increasing Parameter `increasing`.
 * @param Diagonal Parameter `Diagonal`.
 * @param increasing Parameter `increasing`.
 * @param diagonal Parameter `diagonal`.
 * @param decreasing Parameter `decreasing`.
 * @param diagonal Parameter `diagonal`.
 * @param increasing Parameter `increasing`.
 * @param info Parameter `info`.
 * @return Function result.
 */
void sprite_draw_line_from_info(unsigned short * info) {
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned short var_2;    /* fractional x (16-bit) */
	unsigned short var_6;    /* fractional y (16-bit) */
	unsigned char color;
	unsigned short slope;
	unsigned short count;
	unsigned short mode;
	unsigned short x_int, y_int;
	unsigned short i;
	unsigned int ofs;
	unsigned long temp;
	
	/* Read x coordinate (32-bit) and add 32768 for rounding */
	temp = ((unsigned long)info[1] << 16) | info[0];
	temp += 32768UL;
	var_2 = (unsigned short)(temp & 65535);      /* fractional part */
	x_int = (unsigned short)(temp >> 16);          /* integer part */
	
	/* Read y coordinate (32-bit) and add 32768 for rounding */
	temp = ((unsigned long)info[3] << 16) | info[2];
	temp += 32768UL;
	var_6 = (unsigned short)(temp & 65535);      /* fractional part */
	y_int = (unsigned short)(temp >> 16);          /* integer part */
	
	color = (unsigned char)info[8];    /* field_10 */
	slope = info[6];                    /* field_0C */
	count = info[7];                    /* field_0E */
	mode = info[9];                     /* field_12 */
	
	/* Get bitmap pointers */
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	
	switch (mode) {
	case 0:
	case 1:
		/* Horizontal line - fill count pixels at (x_int, y_int) */
		ofs = lineofs[y_int] + x_int;
		for (i = 0; i < count; i++) {
			bitmapptr[ofs + i] = color;
		}
		break;
		
	case 9:
		/* Single pixel at (x_int, y_int) */
		ofs = lineofs[y_int] + x_int;
		bitmapptr[ofs] = color;
		break;
		
	case 2:
		/* Vertical line, y increasing, x fixed */
		/* y_int comes from info[3] = field_06 directly */
		y_int = info[3];
		for (i = 0; i < count; i++) {
			ofs = lineofs[y_int] + x_int;
			bitmapptr[ofs] = color;
			y_int++;
		}
		break;
		
	case 3:
		/* Steep diagonal, y increasing, x decreasing */
		y_int = info[3];
		for (i = 0; i < count; i++) {
			ofs = lineofs[y_int] + x_int;
			bitmapptr[ofs] = color;
			x_int--;
			y_int++;
		}
		break;
		
	case 4:
		/* Steep diagonal, y increasing, x increasing */
		y_int = info[3];
		for (i = 0; i < count; i++) {
			ofs = lineofs[y_int] + x_int;
			bitmapptr[ofs] = color;
			x_int++;
			y_int++;
		}
		break;
		
	case 5:
		/* Diagonal, y increasing, x decreasing with slope */
		/* sub var_2, dx; sbb bx, 0 */
		y_int = info[3];
		for (i = 0; i < count; i++) {
			ofs = lineofs[y_int] + x_int;
			bitmapptr[ofs] = color;
			y_int++;
			/* Subtract slope from fractional x, borrow from integer */
			if (var_2 < slope) {
				x_int--;  /* borrow */
			}
			var_2 -= slope;
		}
		break;
		
	case 6:
		/* Diagonal, y increasing, x increasing with slope */
		/* add var_2, dx; adc bx, 0 */
		y_int = info[3];
		for (i = 0; i < count; i++) {
			ofs = lineofs[y_int] + x_int;
			bitmapptr[ofs] = color;
			y_int++;
			/* Add slope to fractional x, carry to integer */
			temp = (unsigned long)var_2 + slope;
			if (temp > 65535) {
				x_int++;  /* carry */
			}
			var_2 = (unsigned short)temp;
		}
		break;
		
	case 7:
		/* Shallow diagonal, x decreasing, y varies with slope */
		/* add var_6, dx; adc var_8, 0 */
		for (i = 0; i < count; i++) {
			ofs = lineofs[y_int] + x_int;
			bitmapptr[ofs] = color;
			x_int--;
			/* Add slope to fractional y, carry to integer */
			temp = (unsigned long)var_6 + slope;
			if (temp > 65535) {
				y_int++;  /* carry */
			}
			var_6 = (unsigned short)temp;
		}
		break;
		
	case 8:
		/* Shallow diagonal, x increasing, y varies with slope */
		/* add var_6, dx; adc var_8, 0 */
		for (i = 0; i < count; i++) {
			ofs = lineofs[y_int] + x_int;
			bitmapptr[ofs] = color;
			x_int++;
			/* Add slope to fractional y, carry to integer */
			temp = (unsigned long)var_6 + slope;
			if (temp > 65535) {
				y_int++;  /* carry */
			}
			var_6 = (unsigned short)temp;
		}
		break;
	}
}

/* Dithering lookup tables for sprite_draw_dithered_pass */
static const unsigned char dither_row_order_table[12] = {11, 5, 8, 2, 10, 4, 7, 1, 9, 3, 6, 0};
static const unsigned char dither_skip_table[4] = {1, 3, 0, 2};
static const unsigned char dither_step_table[4] = {3, 1, 4, 2};

/**
 * sprite_draw_dithered_pass - Draw dithered/interlaced sprite pattern
 *
 * This function draws pixels from a SPRITE's SHAPE2D bitmap to sprite1
 * in a dithered pattern. Used for fade-in animation effects.
 * The function processes 12 passes with different row orderings to create
 * an interlacing effect.
 *
 * Note: The ASM code appears to access the passed pointer directly as SHAPE2D,
 * but the C extern declares it as SPRITE*. We access sprite->sprite_bitmapptr
 * to match the expected behavior.
 *
/** @brief Index.
 * @param sprite Parameter `sprite`.
 * @return Function result.
 */
void sprite_draw_dithered_pass(int idx, struct SPRITE * sprite) {
	unsigned int * lineofs1;
	unsigned char * bitmapptr1;
	unsigned char * src_data;
	struct SHAPE2D * shape;
	int var_2;                   /* x offset in sprite1 */
	int var_4;                   /* line offset table base */
	int var_6;                   /* max line offset */
	int var_8;                   /* source width */
	int var_A;                   /* row stride in source (width * 12) */
	short var_E;                 /* row counter */
	int var_10;                  /* source offset within row */
	int var_12;                  /* current line offset pointer */
	int var_14 = 0;              /* saved source pointer */
	unsigned short var_16;       /* dither phase counter */
	int cx;                      /* pixel counter */
	int di;                      /* destination offset */
	int si;                      /* source offset */
	unsigned short bx;           /* dither index */
	unsigned char skip, step;
	int row_order;
	int src_height;
	int src_size;
	int dst_y;
	int dst_size;
	
	if (sprite == 0 || sprite->sprite_bitmapptr == 0) {
		return;
	}

	shape = (struct SHAPE2D *)sprite->sprite_bitmapptr;
	
	/* Get sprite1 bitmap pointers */
	bitmapptr1 = (unsigned char *)sprite1.sprite_bitmapptr;
	lineofs1 = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	
	/* Use signed positions: shape fields are stored as 16-bit signed in resources */
	var_2 = (short)shape->s2d_pos_x;
	
	/* var_4 = [si+0Ah] * 2 + sprite1.sprite_lineofs (as offset in array) */
	var_4 = (short)shape->s2d_pos_y;
	
	/* var_6 = var_4 + [si+2] = starting y + height */
	var_6 = var_4 + shape->s2d_height;
	
	/* var_8 = [si+0] = width */
	var_8 = shape->s2d_width;
	src_height = shape->s2d_height;
	if (var_8 <= 0 || src_height <= 0) {
		return;
	}
	src_size = var_8 * src_height;
	if (src_size <= 0) {
		return;
	}

	/* Destination bounds */
	if (sprite1.sprite_pitch <= 0 || sprite1.sprite_height <= 0) {
		return;
	}
	dst_size = (int)sprite1.sprite_pitch * (int)sprite1.sprite_height;
	if (dst_size <= 0) {
		return;
	}
	
	/* var_A = width * 12 (row stride between same-phase rows) */
	var_A = var_8 * SHAPE2D_DITHER_ROW_STRIDE_FACTOR;
	
	/* var_C = source data start (at offset 16 from shape) */
	src_data = ((unsigned char *)shape) + SHAPE2D_HEADER_BYTES;
	
	/* Process 12 rows in dithered order */
	for (var_E = 11; var_E >= 0; var_E--) {
		/* Get row order from lookup table */
		row_order = dither_row_order_table[var_E];
		
		/* Calculate source offset for this row */
		var_10 = var_8 * row_order;
		
		/* Starting line in destination */
		var_12 = var_4 + row_order;
		
		/* Source pointer offset for this row */
		si = var_10;
		
		/* Initialize dither phase from idx */
		var_16 = idx;
		
		/* Process each scanline (every 12th line in destination) */
		var_14 = si;
		while (var_12 < var_6) {
			dst_y = var_12;
			if (dst_y < 0 || dst_y >= (int)sprite1.sprite_height) {
				var_16++;
				var_12 += 12;
				si = var_14 + var_A;
				continue;
			}

			/* Get destination line offset from sprite1 */
			di = lineofs1[var_12] + var_2;
			
			/* Copy pixels with dithering */
			cx = var_8;
			var_14 = si;
			
			bx = var_16;
			while (cx > 0) {
				/* Get skip and step values based on dither phase */
				bx = bx & 3;
							skip = dither_skip_table[bx];
				
				if (cx <= skip) break;
				cx -= skip;
				si += skip;
				di += skip;
				
				/* Copy one pixel */
				if (si < 0 || si >= src_size) {
					break;
				}
				if (di >= 0 && di < dst_size) {
					bitmapptr1[di] = src_data[si];
				}
				
				/* Advance by step */
							step = dither_step_table[bx];
				if (cx <= step) break;
				si += step;
				di += step;
				cx -= step;
				
				bx++;
			}
			
			/* Next dither phase */
			var_16++;
			
			/* Advance to next row (12 rows ahead in destination) */
			var_12 += 12;
			si = var_14 + var_A;
		}
		
		/* Advance idx for next pass */
		idx++;
	}
}

/**
 * draw_patterned_lines - Draw lines with pattern masking
 * 
 * Draws horizontal lines where pixels are only drawn when the
/** @brief Pattern.
 * @param x1arr Parameter `x1arr`.
 * @return Function result.
 */
void draw_patterned_lines(unsigned char color, unsigned short numlines,
                              unsigned short y, unsigned short * x2arr,
                              unsigned short * x1arr) {
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned int si, ofs;
	int cx, x1, x2;
	int sx1, sx2;
	unsigned char pattern, cl;
	int sprite_left = sprite1.sprite_left2;
	int sprite_right = sprite1.sprite_widthsum - 1;

	/* If y is even, swap pattern bytes */
	if ((y & 1) == 0) {
		polygon_pattern_type = ((polygon_pattern_type >> SHAPE2D_BYTE_SHIFT_8) & SHAPE2D_BYTE_MASK) |
			((polygon_pattern_type & SHAPE2D_BYTE_MASK) << SHAPE2D_BYTE_SHIFT_8);
	}

	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;

	for (si = y; numlines > 0; numlines--, si++) {
		sx1 = (int)(int16_t)(*x1arr);
		sx2 = (int)(int16_t)(*x2arr);

		if (sx2 < sx1 || sx2 < sprite_left || sx1 > sprite_right) {
			x2arr++;
			x1arr++;
			polygon_pattern_type = ((polygon_pattern_type >> SHAPE2D_BYTE_SHIFT_8) & SHAPE2D_BYTE_MASK) |
				((polygon_pattern_type & SHAPE2D_BYTE_MASK) << SHAPE2D_BYTE_SHIFT_8);
			continue;
		}

		x1 = sx1;
		x2 = sx2;
		if (x1 < sprite_left) x1 = sprite_left;
		if (x2 > sprite_right) x2 = sprite_right;
		
		/* Get pattern byte and rotate by x1 position */
		pattern = (unsigned char)(polygon_pattern_type & SHAPE2D_BYTE_MASK);
		cl = x1 & 7;
		pattern = (pattern << cl) | (pattern >> (8 - cl)); /* ROL */
		
		cx = x2 - x1 + 1;
		if (cx > 0) {
			ofs = lineofs[si] + x1;
			while (cx-- > 0) {
				/* ROL pattern, 1 */
				unsigned char newcarry = (pattern >> 7) & 1;
				pattern = (pattern << 1) | newcarry;
				
				if (newcarry) {
					/* Bit was set - draw pixel */
					bitmapptr[ofs] = color;
				}
				ofs++;
			}
		}
		
		x2arr++;
		x1arr++;
		
		/* Swap pattern bytes for next line */
		polygon_pattern_type = ((polygon_pattern_type >> SHAPE2D_BYTE_SHIFT_8) & SHAPE2D_BYTE_MASK) |
			((polygon_pattern_type & SHAPE2D_BYTE_MASK) << SHAPE2D_BYTE_SHIFT_8);
	}
}

/**
 * draw_two_color_patterned_lines - Draw patterned lines with alternate color
 * 
 * Similar to draw_patterned_lines but uses two colors:
/** @brief Color.
 * @param x1arr Parameter `x1arr`.
 * @return Function result.
 */
void draw_two_color_patterned_lines(unsigned char color, unsigned short numlines,
									unsigned short y, unsigned short * x2arr,
									unsigned short * x1arr) {
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned int si, ofs;
	int cx, x1, x2;
	int sx1, sx2;
	unsigned char pattern, cl, altcolor;
	int sprite_left = sprite1.sprite_left2;
	int sprite_right = sprite1.sprite_widthsum - 1;

	/* If y is even, swap pattern bytes */
	if ((y & 1) == 0) {
		polygon_pattern_type = ((polygon_pattern_type >> SHAPE2D_BYTE_SHIFT_8) & SHAPE2D_BYTE_MASK) |
			((polygon_pattern_type & SHAPE2D_BYTE_MASK) << SHAPE2D_BYTE_SHIFT_8);
	}

	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	altcolor = (unsigned char)(polygon_alternate_color & SHAPE2D_BYTE_MASK);

	for (si = y; numlines > 0; numlines--, si++) {
		sx1 = (int)(int16_t)(*x1arr);
		sx2 = (int)(int16_t)(*x2arr);

		if (sx2 < sx1 || sx2 < sprite_left || sx1 > sprite_right) {
			x2arr++;
			x1arr++;
			polygon_pattern_type = ((polygon_pattern_type >> SHAPE2D_BYTE_SHIFT_8) & SHAPE2D_BYTE_MASK) |
				((polygon_pattern_type & SHAPE2D_BYTE_MASK) << SHAPE2D_BYTE_SHIFT_8);
			continue;
		}

		x1 = sx1;
		x2 = sx2;
		if (x1 < sprite_left) x1 = sprite_left;
		if (x2 > sprite_right) x2 = sprite_right;
		
		/* Get pattern byte and rotate by x1 position */
		pattern = (unsigned char)(polygon_pattern_type & SHAPE2D_BYTE_MASK);
		cl = x1 & 7;
		pattern = (pattern << cl) | (pattern >> (8 - cl)); /* ROL */
		
		cx = x2 - x1 + 1;
		if (cx > 0) {
			ofs = lineofs[si] + x1;
			while (cx-- > 0) {
				/* ROL pattern, 1 */
				unsigned char newcarry = (pattern >> 7) & 1;
				pattern = (pattern << 1) | newcarry;
				
				if (newcarry) {
					/* Bit was set - draw alternate color */
					bitmapptr[ofs] = altcolor;
				} else {
					/* Bit was clear - draw primary color */
					bitmapptr[ofs] = color;
				}
				ofs++;
			}
		}
		
		x2arr++;
		x1arr++;
		
		/* Swap pattern bytes for next line */
		polygon_pattern_type = ((polygon_pattern_type >> SHAPE2D_BYTE_SHIFT_8) & SHAPE2D_BYTE_MASK) |
			((polygon_pattern_type & SHAPE2D_BYTE_MASK) << SHAPE2D_BYTE_SHIFT_8);
	}
}

/**
 * sprite_set_1_size - Set clipping bounds for sprite1
 * 
 * Sets the left, right, top, and height fields of sprite1.
 * Used to establish the drawing clip region.
 * 
 * @param left    Left boundary
 * @param right   Right boundary
 * @param top     Top boundary
/** @brief Boundary.
 * @param height Parameter `height`.
 * @return Function result.
 */
void sprite_set_1_size(unsigned short left, unsigned short right,
                           unsigned short top, unsigned short height) {
	sprite1.sprite_left2 = left;
	sprite1.sprite_left = left;
	sprite1.sprite_widthsum = right;
	sprite1.sprite_right = right;
	sprite1.sprite_top = top;
	sprite1.sprite_height = height;
}

/**
 * sprite_blit_shape_to_sprite1 - Copy shape data to sprite1 bitmap
 * 
 * Copies pixel data from a SHAPE2D to sprite1's bitmap at
 * the position specified in the shape's s2d_pos_x/s2d_pos_y fields.
 * 
 * @param shape  Far pointer to SHAPE2D structure
 */
void sprite_blit_shape_to_sprite1(void * shapeptr) {
	struct SHAPE2D * shape = (struct SHAPE2D *)shapeptr;
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned char * srcptr;
	unsigned int destofs;
	unsigned int linestart;
	unsigned int dst_limit;
	unsigned long row_end;
	int width, height;

	if (shape == 0) {
		fatal_error("sprite_blit_shape_to_sprite1: null shape");
	}

	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	if (bitmapptr == (unsigned char *)0) {
		fatal_error("sprite_blit_shape_to_sprite1: null sprite1 bitmap");
	}
	if (sprite1.sprite_pitch == 0 || sprite1.sprite_height == 0 ||
		sprite1.sprite_pitch > SHAPE2D_DIM_LIMIT_MEDIUM || sprite1.sprite_height > SHAPE2D_DIM_LIMIT_MEDIUM) {
		fatal_error("sprite_blit_shape_to_sprite1: bad sprite1 pitch=%u h=%u",
			(unsigned)sprite1.sprite_pitch,
			(unsigned)sprite1.sprite_height);
	}
	
	width = shape->s2d_width;
	height = shape->s2d_height;
	if (width <= 0 || height <= 0) {
		fatal_error("sprite_blit_shape_to_sprite1: invalid size %u x %u", (unsigned)shape->s2d_width, (unsigned)shape->s2d_height);
	}
	if (shape->s2d_pos_x >= sprite1.sprite_pitch || shape->s2d_pos_y >= sprite1.sprite_height) {
		fatal_error("sprite_blit_shape_to_sprite1: pos out of bounds x=%u y=%u pitch=%u h=%u",
			(unsigned)shape->s2d_pos_x,
			(unsigned)shape->s2d_pos_y,
			(unsigned)sprite1.sprite_pitch,
			(unsigned)sprite1.sprite_height);
	}
	if ((unsigned long)shape->s2d_pos_x + (unsigned long)shape->s2d_width > (unsigned long)sprite1.sprite_pitch ||
		(unsigned long)shape->s2d_pos_y + (unsigned long)shape->s2d_height > (unsigned long)sprite1.sprite_height) {
		fatal_error("sprite_blit_shape_to_sprite1: rect out of bounds x=%u y=%u w=%u h=%u pitch=%u sh=%u",
			(unsigned)shape->s2d_pos_x,
			(unsigned)shape->s2d_pos_y,
			(unsigned)shape->s2d_width,
			(unsigned)shape->s2d_height,
			(unsigned)sprite1.sprite_pitch,
			(unsigned)sprite1.sprite_height);
	}
	if (shape->s2d_width > SHAPE2D_DIM_LIMIT_MEDIUM || shape->s2d_height > SHAPE2D_DIM_LIMIT_MEDIUM) {
		fatal_error("sprite_blit_shape_to_sprite1: absurd size %u x %u",
			(unsigned)shape->s2d_width,
			(unsigned)shape->s2d_height);
	}
	srcptr = (unsigned char *)shape + SHAPE2D_HEADER_BYTES; /* skip header */

	linestart = lineofs[shape->s2d_pos_y];
	destofs = linestart + shape->s2d_pos_x;
	dst_limit = lineofs[sprite1.sprite_height - 1] + sprite1.sprite_pitch;
	row_end = (unsigned long)destofs + (unsigned long)width;
	if (row_end > 65536UL) {
		fatal_error("sprite_blit_shape_to_sprite1: row overflow dst=%u w=%u", (unsigned)destofs, (unsigned)width);
	}

	if (!shape2d_blit_rows(bitmapptr,
			(size_t)dst_limit,
			destofs,
			srcptr,
			(unsigned int)width,
			(unsigned int)height,
			(unsigned int)sprite1.sprite_pitch)) {
		fatal_error("sprite_blit_shape_to_sprite1: blit rows out of bounds dst=%u w=%u h=%u pitch=%u",
			(unsigned)destofs,
			(unsigned)width,
			(unsigned)height,
			(unsigned)sprite1.sprite_pitch);
	}
}

/**
 * sprite_shape_to_1 - Copy shape data to sprite1 bitmap at x,y
 * 
 * Copies pixel data from a SHAPE2D to sprite1's bitmap.
 * Position is specified by parameters rather than shape fields.
 * 
 * @param shape  Far pointer to SHAPE2D structure
 * @param x      X position in sprite1
 * @param y      Y position in sprite1
 */
void sprite_shape_to_1(struct SHAPE2D * shape, unsigned short x, unsigned short y) {
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned char * srcptr;
	unsigned int destofs;
	int width, height, row, col;
	int draw_width, draw_height;
	int src_skip_x, src_skip_y;
	int dst_x, dst_y;
	int src_pitch_skip;

	if (shape == 0 || sprite1.sprite_bitmapptr == 0) {
		return;
	}

	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	
	width = shape->s2d_width;
	height = shape->s2d_height;
	if (width <= 0 || height <= 0 || width > SHAPE2D_DIM_LIMIT_LARGE || height > SHAPE2D_DIM_LIMIT_LARGE) {
		return;
	}
	if (sprite1.sprite_pitch <= 0 || sprite1.sprite_height <= 0) {
		return;
	}

	dst_x = (int)x;
	dst_y = (int)y;

	/* Vertical clipping */
	src_skip_y = 0;
	draw_height = height;
	if (dst_y < sprite1.sprite_top) {
		src_skip_y = sprite1.sprite_top - dst_y;
		draw_height -= src_skip_y;
		dst_y = sprite1.sprite_top;
	}
	if (dst_y + draw_height > (int)sprite1.sprite_height) {
		draw_height = (int)sprite1.sprite_height - dst_y;
	}
	if (draw_height <= 0) {
		return;
	}

	/* Horizontal clipping */
	src_skip_x = 0;
	draw_width = width;
	if (dst_x < sprite1.sprite_left) {
		src_skip_x = sprite1.sprite_left - dst_x;
		draw_width -= src_skip_x;
		dst_x = sprite1.sprite_left;
	}
	if (dst_x + draw_width > (int)sprite1.sprite_right) {
		draw_width = (int)sprite1.sprite_right - dst_x;
	}
	if (draw_width <= 0) {
		return;
	}

	srcptr = (unsigned char *)shape + SHAPE2D_HEADER_BYTES; /* skip header */
	srcptr += src_skip_y * width + src_skip_x;
	src_pitch_skip = width - draw_width;

	destofs = lineofs[dst_y] + (unsigned int)dst_x;

	for (row = 0; row < draw_height; row++) {
		for (col = 0; col < draw_width; col++) {
			bitmapptr[destofs++] = *srcptr++;
		}
		srcptr += src_pitch_skip;
		destofs += (unsigned int)(sprite1.sprite_pitch - draw_width);
	}
}

/**
 * shape2d_render_bmp_as_mask - Render RLE bitmap with AND masking
 * 
 * Decodes RLE-compressed bitmap data and ANDs pixels with sprite1's bitmap.
 * Uses the shape's s2d_pos_x/s2d_pos_y for position.
 * 
 * RLE format:
 *   byte > 0: Run length - repeat next byte 'count' times
 *   byte < 0: Copy -count bytes directly
 *   byte == 0: End of data
 * 
 * @param shapeptr  Far pointer to SHAPE2D structure
 */
void shape2d_render_bmp_as_mask(void * shapeptr) {
	struct SHAPE2D * shape = (struct SHAPE2D *)shapeptr;
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned char * srcptr;
	unsigned int lineptr_idx;
	unsigned int destofs;
	int width, pixels_left, count;
	signed char ctrl;
	unsigned char pixel;

	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	
	width = shape->s2d_width;
	srcptr = (unsigned char *)shape + SHAPE2D_HEADER_BYTES; /* skip header */

	/* Setup initial line */
	lineptr_idx = shape->s2d_pos_y;
	destofs = lineofs[lineptr_idx] + shape->s2d_pos_x;
	pixels_left = width;

	/* RLE decode loop */
	for (;;) {
		ctrl = (signed char)*srcptr++;
		
		if (ctrl == 0) {
			/* End of data */
			break;
		} else if (ctrl > 0) {
			/* Run length: repeat next byte 'ctrl' times */
			count = ctrl;
			pixel = *srcptr++;
			
			while (count > 0) {
				bitmapptr[destofs] &= pixel;
				destofs++;
				pixels_left--;
				
				if (pixels_left <= 0) {
					/* Move to next line */
					lineptr_idx++;
					destofs = lineofs[lineptr_idx] + shape->s2d_pos_x;
					pixels_left = width;
				}
				count--;
			}
		} else {
			/* Copy -ctrl bytes directly */
			count = -ctrl;
			
			while (count > 0) {
				pixel = *srcptr++;
				bitmapptr[destofs] &= pixel;
				destofs++;
				pixels_left--;
				
				if (pixels_left <= 0) {
					/* Move to next line */
					lineptr_idx++;
					destofs = lineofs[lineptr_idx] + shape->s2d_pos_x;
					pixels_left = width;
				}
				count--;
			}
		}
	}
}

/**
 * shape2d_draw_rle_or - Render RLE bitmap with OR blending
 * 
 * Decodes RLE-compressed bitmap data and ORs pixels with sprite1's bitmap.
 * Uses the shape's s2d_pos_x/s2d_pos_y for position.
 * 
 * RLE format:
 *   byte > 0: Run length - repeat next byte 'count' times
 *   byte < 0: Copy -count bytes directly
 *   byte == 0: End of data
 * 
 * @param shape  Pointer to SHAPE2D structure
 */
void shape2d_draw_rle_or(struct SHAPE2D * shape) {
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned char * srcptr;
	unsigned int lineptr_idx;
	unsigned int destofs;
	int width, pixels_left, count;
	signed char ctrl;
	unsigned char pixel;

	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	
	width = shape->s2d_width;
	srcptr = (unsigned char *)shape + SHAPE2D_HEADER_BYTES; /* skip header */

	/* Setup initial line */
	lineptr_idx = shape->s2d_pos_y;
	destofs = lineofs[lineptr_idx] + shape->s2d_pos_x;
	pixels_left = width;

	/* RLE decode loop */
	for (;;) {
		ctrl = (signed char)*srcptr++;
		
		if (ctrl == 0) {
			/* End of data */
			break;
		} else if (ctrl > 0) {
			/* Run length: repeat next byte 'ctrl' times */
			count = ctrl;
			pixel = *srcptr++;
			
			while (count > 0) {
				bitmapptr[destofs] |= pixel;
				destofs++;
				pixels_left--;
				
				if (pixels_left <= 0) {
					/* Move to next line */
					lineptr_idx++;
					destofs = lineofs[lineptr_idx] + shape->s2d_pos_x;
					pixels_left = width;
				}
				count--;
			}
		} else {
			/* Copy -ctrl bytes directly */
			count = -ctrl;
			
			while (count > 0) {
				pixel = *srcptr++;
				bitmapptr[destofs] |= pixel;
				destofs++;
				pixels_left--;
				
				if (pixels_left <= 0) {
					/* Move to next line */
					lineptr_idx++;
					destofs = lineofs[lineptr_idx] + shape->s2d_pos_x;
					pixels_left = width;
				}
				count--;
			}
		}
	}
}

/**
/** @brief Copy.
 * @param shapeptr Parameter `shapeptr`.
 * @return Function result.
 */
void shape2d_draw_rle_copy(void * shapeptr) {
	struct SHAPE2D * shape = (struct SHAPE2D *)shapeptr;
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned char * srcptr;
	unsigned int lineptr_idx;
	unsigned int destofs;
	int width, pixels_left, count;
	signed char ctrl;
	unsigned char pixel;

	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	
	width = shape->s2d_width;
	srcptr = (unsigned char *)shape + SHAPE2D_HEADER_BYTES; /* skip header */

	/* Setup initial line */
	lineptr_idx = shape->s2d_pos_y;
	destofs = lineofs[lineptr_idx] + shape->s2d_pos_x;
	pixels_left = width;

	/* RLE decode loop */
	for (;;) {
		ctrl = (signed char)*srcptr++;
		
		if (ctrl == 0) {
			/* End of data */
			break;
		} else if (ctrl > 0) {
			/* Run length: repeat next byte 'ctrl' times */
			count = ctrl;
			pixel = *srcptr++;
			
			while (count > 0) {
				bitmapptr[destofs] = pixel;
				destofs++;
				pixels_left--;
				
				if (pixels_left <= 0) {
					/* Move to next line */
					lineptr_idx++;
					destofs = lineofs[lineptr_idx] + shape->s2d_pos_x;
					pixels_left = width;
				}
				count--;
			}
		} else {
			/* Copy -ctrl bytes directly */
			count = -ctrl;
			
			while (count > 0) {
				pixel = *srcptr++;
				bitmapptr[destofs] = pixel;
				destofs++;
				pixels_left--;
				
				if (pixels_left <= 0) {
					/* Move to next line */
					lineptr_idx++;
					destofs = lineofs[lineptr_idx] + shape->s2d_pos_x;
					pixels_left = width;
				}
				count--;
			}
		}
	}
}

/**
 * shape2d_draw_rle_copy_at - Render RLE bitmap with direct copy at explicit position
 * 
 * Decodes RLE-compressed bitmap data and copies pixels to sprite1's bitmap.
 * Uses explicit x,y position parameters instead of shape fields.
 * 
 * RLE format:
 *   byte > 0: Run length - repeat next byte 'count' times
 *   byte < 0: Copy -count bytes directly
 *   byte == 0: End of data
 * 
 * @param shapeptr  Far pointer to SHAPE2D structure
 * @param x         X position in sprite1
 * @param y         Y position in sprite1
 */
void shape2d_draw_rle_copy_at(void * shapeptr, unsigned short x, unsigned short y) {
	struct SHAPE2D * shape = (struct SHAPE2D *)shapeptr;
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned char * srcptr;
	unsigned int lineptr_idx;
	unsigned int destofs;
	int width, pixels_left, count;
	signed char ctrl;
	unsigned char pixel;

	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	
	width = shape->s2d_width;
	srcptr = (unsigned char *)shape + SHAPE2D_HEADER_BYTES; /* skip header */

	/* Setup initial line with explicit position */
	lineptr_idx = y;
	destofs = lineofs[lineptr_idx] + x;
	pixels_left = width;

	/* RLE decode loop */
	for (;;) {
		ctrl = (signed char)*srcptr++;
		
		if (ctrl == 0) {
			/* End of data */
			break;
		} else if (ctrl > 0) {
			/* Run length: repeat next byte 'ctrl' times */
			count = ctrl;
			pixel = *srcptr++;
			
			while (count > 0) {
				bitmapptr[destofs] = pixel;
				destofs++;
				pixels_left--;
				
				if (pixels_left <= 0) {
					/* Move to next line */
					lineptr_idx++;
					destofs = lineofs[lineptr_idx] + x;
					pixels_left = width;
				}
				count--;
			}
		} else {
			/* Copy -ctrl bytes directly */
			count = -ctrl;
			
			while (count > 0) {
				pixel = *srcptr++;
				bitmapptr[destofs] = pixel;
				destofs++;
				pixels_left--;
				
				if (pixels_left <= 0) {
					/* Move to next line */
					lineptr_idx++;
					destofs = lineofs[lineptr_idx] + x;
					pixels_left = width;
				}
				count--;
			}
		}
	}
}

/**
 * shape2d_draw_rle_copy_clipped - Render RLE bitmap with clipping
 * 
 * Decodes RLE-compressed bitmap data and copies pixels to sprite1's bitmap.
 * Uses the shape's s2d_pos_x/s2d_pos_y for position.
 * Includes full clipping to sprite1 bounds.
 * 
 * RLE format:
 *   byte > 0: Run length - repeat next byte 'count' times
 *   byte < 0: Copy -count bytes directly
 *   byte == 0: End of data
 * 
 * @param shapeptr  Far pointer to SHAPE2D structure
 */
void shape2d_draw_rle_copy_clipped(void * shapeptr) {
	struct SHAPE2D * shape = (struct SHAPE2D *)shapeptr;
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned char * srcptr;
	int pos_x, pos_y;
	int shape_width, shape_height;
	int sprite_top_val, sprite_height_val;
	int sprite_left_val, sprite_right_val;
	int visible_rows, visible_cols;
	int skip_top_rows, skip_left_cols, skip_right_cols;
	int needs_clipping;
	unsigned int lineptr_idx;
	unsigned int destofs;
	int pixels_left, count, total_skip;
	signed char ctrl;
	unsigned char pixel;

	/* Get shape properties */
	pos_x = shape->s2d_pos_x;
	pos_y = shape->s2d_pos_y;
	shape_width = shape->s2d_width;
	shape_height = shape->s2d_height;
	srcptr = (unsigned char *)shape + SHAPE2D_HEADER_BYTES; /* skip header */

	/* Get sprite1 bounds */
	sprite_top_val = sprite1.sprite_top;
	sprite_height_val = sprite1.sprite_height;
	sprite_left_val = sprite1.sprite_left;
	sprite_right_val = sprite1.sprite_right;

	/* Get sprite1 pointers */
	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;

	needs_clipping = 0;
	skip_top_rows = 0;
	skip_left_cols = 0;
	skip_right_cols = 0;
	visible_rows = shape_height;
	visible_cols = shape_width;

	/* Vertical clipping */
	if (pos_y < sprite_top_val) {
		/* Shape starts above visible area */
		needs_clipping = 1;
		skip_top_rows = sprite_top_val - pos_y;
		visible_rows = shape_height - skip_top_rows;
		if (visible_rows <= 0) return;
		pos_y = sprite_top_val;
		
		/* Check if remaining rows extend past bottom */
		if (pos_y + visible_rows > sprite_height_val) {
			visible_rows = sprite_height_val - pos_y;
			if (visible_rows <= 0) return;
		}
	} else {
		/* Check if shape extends past bottom */
		if (pos_y + visible_rows > sprite_height_val) {
			needs_clipping = 1;
			visible_rows = sprite_height_val - pos_y;
			if (visible_rows <= 0) return;
		}
	}

	/* Horizontal clipping */
	if (pos_x < sprite_left_val) {
		/* Shape starts left of visible area */
		needs_clipping = 1;
		skip_left_cols = sprite_left_val - pos_x;
		visible_cols = shape_width - skip_left_cols;
		if (visible_cols <= 0) return;
		pos_x = sprite_left_val;
		
		/* Check right edge */
		if (pos_x + visible_cols > sprite_right_val) {
			int excess = (pos_x + visible_cols) - sprite_right_val;
			visible_cols -= excess;
			skip_right_cols = excess;
			if (visible_cols <= 0) return;
		}
	} else {
		/* Check if shape extends past right edge */
		if (pos_x + visible_cols > sprite_right_val) {
			needs_clipping = 1;
			skip_right_cols = (pos_x + visible_cols) - sprite_right_val;
			visible_cols -= skip_right_cols;
			if (visible_cols <= 0) return;
		}
	}

	/* If no clipping needed, use fast path */
	if (!needs_clipping) {
		/* Fast non-clipping path */
		lineptr_idx = pos_y;
		destofs = lineofs[lineptr_idx] + pos_x;
		pixels_left = shape_width;

		for (;;) {
			ctrl = (signed char)*srcptr++;
			
			if (ctrl == 0) break;
			
			if (ctrl > 0) {
				count = ctrl;
				pixel = *srcptr++;
				
				while (count > 0) {
					bitmapptr[destofs++] = pixel;
					pixels_left--;
					if (pixels_left <= 0) {
						lineptr_idx++;
						destofs = lineofs[lineptr_idx] + pos_x;
						pixels_left = shape_width;
					}
					count--;
				}
			} else {
				count = -ctrl;
				
				while (count > 0) {
					bitmapptr[destofs++] = *srcptr++;
					pixels_left--;
					if (pixels_left <= 0) {
						lineptr_idx++;
						destofs = lineofs[lineptr_idx] + pos_x;
						pixels_left = shape_width;
					}
					count--;
				}
			}
		}
	} else {
		/*
		 * Clipping path - unified skip/draw approach matching original ASM
			* (shape2d transparent-skip path).
		 *
		 * Combines top-row skip + left-column skip into a single initial
		 * pixel count, then alternates between drawing visible_cols pixels
		 * and skipping (shape_width - visible_cols) pixels per row.
		 * Correctly handles RLE runs that straddle skip/draw boundaries.
		 */
		int draw_width;     /* visible_cols (var_6 in ASM) */
		int row_skip;       /* shape_width - visible_cols (var_C in ASM) */
		int rows_left;      /* visible_rows (var_8 in ASM) */
		int dx;             /* pixels remaining in current phase */
		int mode;           /* 0 = skip, 1 = draw */
		int run_len;
		unsigned char fill_byte = 0;
		int is_fill;
		int n, i;

		draw_width = visible_cols;
		row_skip = shape_width - visible_cols;
		rows_left = visible_rows;

		lineptr_idx = pos_y;
		destofs = lineofs[lineptr_idx] + pos_x;

		/* Combine top row skip and left column skip (matches ASM var_A) */
		total_skip = skip_top_rows * shape_width + skip_left_cols;

		if (total_skip > 0) {
			mode = 0;  /* start in skip mode */
			dx = total_skip;
		} else {
			mode = 1;  /* start in draw mode */
			dx = draw_width;
		}

		for (;;) {
			/* Read next RLE command */
			ctrl = (signed char)*srcptr++;
			if (ctrl == 0) return;

			if (ctrl > 0) {
				run_len = ctrl;
				fill_byte = *srcptr++;
				is_fill = 1;
			} else {
				run_len = -ctrl;
				is_fill = 0;
			}

			/* Process this run, potentially across multiple mode transitions */
			while (run_len > 0) {
				n = (run_len < dx) ? run_len : dx;

				if (mode == 1) {
					/* Drawing mode - write pixels to destination */
					if (is_fill) {
						for (i = 0; i < n; i++)
							bitmapptr[destofs++] = fill_byte;
					} else {
						for (i = 0; i < n; i++)
							bitmapptr[destofs++] = *srcptr++;
					}
				} else {
					/* Skip mode - advance source pointer for literals only */
					if (!is_fill)
						srcptr += n;
				}

				run_len -= n;
				dx -= n;

				if (dx == 0) {
					if (mode == 1) {
						/* Finished drawing visible portion of row */
						rows_left--;
						if (rows_left <= 0) return;
						lineptr_idx++;
						destofs = lineofs[lineptr_idx] + pos_x;
						/* Switch to skip mode for row boundary */
						if (row_skip > 0) {
							mode = 0;
							dx = row_skip;
						} else {
							/* No horizontal skip - stay in draw mode */
							dx = draw_width;
						}
					} else {
						/* Finished skipping - switch to draw mode */
						mode = 1;
						dx = draw_width;
					}
				}
			}
		}
	}
}

/**
 * shape2d_draw_rle_copy_clipped_at - Render RLE bitmap with clipping at explicit position
 * 
 * Same as shape2d_draw_rle_copy_clipped but takes explicit x,y position parameters.
 * 
 * @param shapeptr  Far pointer to SHAPE2D structure
 * @param x         X position in sprite1
 * @param y         Y position in sprite1
 */
void shape2d_draw_rle_copy_clipped_at(void * shapeptr, unsigned short x, unsigned short y) {
	struct SHAPE2D * shape = (struct SHAPE2D *)shapeptr;
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned char * srcptr;
	int pos_x, pos_y;
	int shape_width, shape_height;
	int sprite_top_val, sprite_height_val;
	int sprite_left_val, sprite_right_val;
	int visible_rows, visible_cols;
	int skip_top_rows, skip_left_cols, skip_right_cols;
	int needs_clipping;
	unsigned int lineptr_idx;
	unsigned int destofs;
	int pixels_left, count, total_skip;
	signed char ctrl;
	unsigned char pixel;

	/* Get shape properties - use explicit x,y (sign-extend like original 16-bit code) */
	pos_x = (short)x;
	pos_y = (short)y;
	shape_width = shape->s2d_width;
	shape_height = shape->s2d_height;
	srcptr = (unsigned char *)shape + SHAPE2D_HEADER_BYTES; /* skip header */

	/* Get sprite1 bounds */
	sprite_top_val = sprite1.sprite_top;
	sprite_height_val = sprite1.sprite_height;
	sprite_left_val = sprite1.sprite_left;
	sprite_right_val = sprite1.sprite_right;

	/* Get sprite1 pointers */
	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;

	needs_clipping = 0;
	skip_top_rows = 0;
	skip_left_cols = 0;
	skip_right_cols = 0;
	visible_rows = shape_height;
	visible_cols = shape_width;

	/* Vertical clipping */
	if (pos_y < sprite_top_val) {
		/* Shape starts above visible area */
		needs_clipping = 1;
		skip_top_rows = sprite_top_val - pos_y;
		visible_rows = shape_height - skip_top_rows;
		if (visible_rows <= 0) return;
		pos_y = sprite_top_val;
		
		/* Check if remaining rows extend past bottom */
		if (pos_y + visible_rows > sprite_height_val) {
			visible_rows = sprite_height_val - pos_y;
			if (visible_rows <= 0) return;
		}
	} else {
		/* Check if shape extends past bottom */
		if (pos_y + visible_rows > sprite_height_val) {
			needs_clipping = 1;
			visible_rows = sprite_height_val - pos_y;
			if (visible_rows <= 0) return;
		}
	}

	/* Horizontal clipping */
	if (pos_x < sprite_left_val) {
		/* Shape starts left of visible area */
		needs_clipping = 1;
		skip_left_cols = sprite_left_val - pos_x;
		visible_cols = shape_width - skip_left_cols;
		if (visible_cols <= 0) return;
		pos_x = sprite_left_val;
		
		/* Check right edge */
		if (pos_x + visible_cols > sprite_right_val) {
			int excess = (pos_x + visible_cols) - sprite_right_val;
			visible_cols -= excess;
			skip_right_cols = excess;
			if (visible_cols <= 0) return;
		}
	} else {
		/* Check if shape extends past right edge */
		if (pos_x + visible_cols > sprite_right_val) {
			needs_clipping = 1;
			skip_right_cols = (pos_x + visible_cols) - sprite_right_val;
			visible_cols -= skip_right_cols;
			if (visible_cols <= 0) return;
		}
	}

	/* If no clipping needed, use fast path */
	if (!needs_clipping) {
		/* Fast non-clipping path */
		lineptr_idx = pos_y;
		destofs = lineofs[lineptr_idx] + pos_x;
		pixels_left = shape_width;

		for (;;) {
			ctrl = (signed char)*srcptr++;
			
			if (ctrl == 0) break;
			
			if (ctrl > 0) {
				count = ctrl;
				pixel = *srcptr++;
				
				while (count > 0) {
					bitmapptr[destofs++] = pixel;
					pixels_left--;
					if (pixels_left <= 0) {
						lineptr_idx++;
						destofs = lineofs[lineptr_idx] + pos_x;
						pixels_left = shape_width;
					}
					count--;
				}
			} else {
				count = -ctrl;
				
				while (count > 0) {
					bitmapptr[destofs++] = *srcptr++;
					pixels_left--;
					if (pixels_left <= 0) {
						lineptr_idx++;
						destofs = lineofs[lineptr_idx] + pos_x;
						pixels_left = shape_width;
					}
					count--;
				}
			}
		}
	} else {
		/*
		 * Clipping path - unified skip/draw approach matching original ASM
			* (shape2d transparent-skip path).
		 *
		 * The ASM combines top-row skip + left-column skip into a single
		 * initial pixel count, then alternates between drawing visible_cols
		 * pixels and skipping (shape_width - visible_cols) pixels per row.
		 * This correctly handles RLE runs that straddle skip/draw boundaries.
		 *
		 * State machine:
		 *   mode=0 (skip):  consume dx pixels without writing
		 *   mode=1 (draw):  write dx pixels to destination
		 * When dx reaches 0, transition to the other mode.
		 */
		int draw_width;     /* visible_cols (var_6 in ASM) */
		int row_skip;       /* shape_width - visible_cols (var_C in ASM) */
		int rows_left;      /* visible_rows (var_8 in ASM) */
		int dx;             /* pixels remaining in current phase */
		int mode;           /* 0 = skip, 1 = draw */
		int run_len;
		unsigned char fill_byte = 0;
		int is_fill;
		int n, i;

		draw_width = visible_cols;
		row_skip = shape_width - visible_cols;
		rows_left = visible_rows;

		lineptr_idx = pos_y;
		destofs = lineofs[lineptr_idx] + pos_x;

		/* Combine top row skip and left column skip (matches ASM var_A) */
		total_skip = skip_top_rows * shape_width + skip_left_cols;

		if (total_skip > 0) {
			mode = 0;  /* start in skip mode */
			dx = total_skip;
		} else {
			mode = 1;  /* start in draw mode */
			dx = draw_width;
		}

		for (;;) {
			/* Read next RLE command */
			ctrl = (signed char)*srcptr++;
			if (ctrl == 0) return;

			if (ctrl > 0) {
				run_len = ctrl;
				fill_byte = *srcptr++;
				is_fill = 1;
			} else {
				run_len = -ctrl;
				is_fill = 0;
			}

			/* Process this run, potentially across multiple mode transitions */
			while (run_len > 0) {
				n = (run_len < dx) ? run_len : dx;

				if (mode == 1) {
					/* Drawing mode - write pixels to destination */
					if (is_fill) {
						for (i = 0; i < n; i++)
							bitmapptr[destofs++] = fill_byte;
					} else {
						for (i = 0; i < n; i++)
							bitmapptr[destofs++] = *srcptr++;
					}
				} else {
					/* Skip mode - advance source pointer for literals only */
					if (!is_fill)
						srcptr += n;
				}

				run_len -= n;
				dx -= n;

				if (dx == 0) {
					if (mode == 1) {
						/* Finished drawing visible portion of row */
						rows_left--;
						if (rows_left <= 0) return;
						lineptr_idx++;
						destofs = lineofs[lineptr_idx] + pos_x;
						/* Switch to skip mode for row boundary */
						if (row_skip > 0) {
							mode = 0;
							dx = row_skip;
						} else {
							/* No horizontal skip - stay in draw mode */
							dx = draw_width;
						}
					} else {
						/* Finished skipping - switch to draw mode */
						mode = 1;
						dx = draw_width;
					}
				}
			}
		}
	}
}

/**
 * sprite_clear_shape - Copy sprite1 bitmap to shape
 * 
 * Copies pixel data from sprite1's bitmap to a SHAPE2D buffer.
 * Uses the shape's s2d_pos_x/s2d_pos_y for position.
 * 
 * @param shape  Far pointer to SHAPE2D structure
 */
void sprite_clear_shape(void * shapeptr) {
	struct SHAPE2D * shape = (struct SHAPE2D *)shapeptr;
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned char * destptr;
	unsigned int srcofs;
	int width, height, row, col;
	int src_x, src_y;

	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	
	width = shape->s2d_width;
	height = shape->s2d_height;
	src_x = shape->s2d_pos_x;
	src_y = shape->s2d_pos_y;

	if (width <= 0 || height <= 0 || width > SHAPE2D_DIM_LIMIT_LARGE || height > SHAPE2D_DIM_LIMIT_LARGE) {
		return;
	}
	if (sprite1.sprite_pitch <= 0 || sprite1.sprite_height <= 0) {
		return;
	}
	if (src_x < 0 || src_y < 0 || src_x + width > (int)sprite1.sprite_pitch || src_y + height > (int)sprite1.sprite_height) {
		return;
	}

	destptr = (unsigned char *)shape + SHAPE2D_HEADER_BYTES; /* skip header */

	for (row = 0; row < height; row++) {
		srcofs = lineofs[src_y + row] + src_x;
		for (col = 0; col < width; col++) {
			*destptr++ = bitmapptr[srcofs++];
		}
	}
}

/**
 * sprite_clear_shape_alt - Copy sprite1 bitmap to shape at x,y
 * 
 * Copies pixel data from sprite1's bitmap to a SHAPE2D buffer.
 * Position is specified by parameters and stored in shape fields.
 * 
 * @param shape  Far pointer to SHAPE2D structure
 * @param x      X position in sprite1
 * @param y      Y position in sprite1
 */
void sprite_clear_shape_alt(void * shapeptr, unsigned short x, unsigned short y) {
	struct SHAPE2D * shape = (struct SHAPE2D *)shapeptr;
	unsigned int * lineofs;
	unsigned char * bitmapptr;
	unsigned char * destptr;
	unsigned int srcofs;
	int width, height, row, col;
	int src_x, src_y;
	int copy_width, copy_height;
	int src_pitch;

	/* Store position in shape */
	shape->s2d_pos_x = x;
	shape->s2d_pos_y = y;

	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	
	width = shape->s2d_width;
	height = shape->s2d_height;
	if (width <= 0 || height <= 0 || width > SHAPE2D_DIM_LIMIT_LARGE || height > SHAPE2D_DIM_LIMIT_LARGE) {
		return;
	}

	src_pitch = (int)sprite1.sprite_pitch;
	if (src_pitch <= 0 || (int)sprite1.sprite_height <= 0) {
		return;
	}

	src_x = (int)x;
	src_y = (int)y;
	if (src_x >= src_pitch || src_y >= (int)sprite1.sprite_height) {
		return;
	}
	copy_width = width;
	if (src_x + copy_width > src_pitch) {
		copy_width = src_pitch - src_x;
	}
	copy_height = height;
	if (src_y + copy_height > (int)sprite1.sprite_height) {
		copy_height = (int)sprite1.sprite_height - src_y;
	}
	if (copy_width <= 0 || copy_height <= 0) {
		return;
	}

	destptr = (unsigned char *)shape + SHAPE2D_HEADER_BYTES; /* skip header */

	for (row = 0; row < copy_height; row++) {
		srcofs = lineofs[src_y + row] + src_x;
		for (col = 0; col < copy_width; col++) {
			*destptr++ = bitmapptr[srcofs++];
		}
		/* Clear remainder of destination row if clipped */
		for (; col < width; col++) {
			*destptr++ = 0;
		}
	}

	/* Clear remaining destination rows if clipped */
	for (; row < height; row++) {
		for (col = 0; col < width; col++) {
			*destptr++ = 0;
		}
	}
}

/* Icon data structure:
 * [0..1] = width
 * [2..3] = height
 * [4..5] = hotspot_x
 * [6..7] = hotspot_y  
 * [8..9] = default_x
 * [10..11] = default_y
 * [16+] = pixel data
 */

/**
/** @brief Operation.
 * @param x Parameter `x`.
 * @param y Parameter `y`.
 * @return Function result.
 */
void putpixel_iconMask(void * icon, unsigned short x, unsigned short y) {
	unsigned short * icondata = (unsigned short *)icon;
	unsigned int width, height;
	unsigned int words_per_row;
	int has_trailing_byte;
	unsigned short * srcptr;
	unsigned char * bitmapptr;
	unsigned int * lineofs;
	unsigned int destofs;
	int pitch;
	int row;
	unsigned int col;
	unsigned short * destword;
	unsigned char * destbyte;

	if (icon == 0) {
		return;
	}
	
	width = icondata[0];
	height = icondata[1];
	
	/* Get sprite1 info */
	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	pitch = sprite1.sprite_pitch;
	
	/* Calculate destination */
	destofs = lineofs[y] + x;
	
	/* Source pixels start at offset 16 (skip header) */
	srcptr = (unsigned short *)((unsigned char *)icondata + SHAPE2D_HEADER_BYTES);
	
	/* Calculate words per row and check for trailing byte */
	words_per_row = width >> 1;
	has_trailing_byte = width & 1;
	
	/* Handle three cases based on width */
	if (has_trailing_byte && words_per_row > 0) {
		/* Case 1: Odd width > 1 - loop words + 1 trailing byte per row */
		for (row = 0; row < (int)height; row++) {
			destword = (unsigned short *)(bitmapptr + destofs);
			for (col = 0; col < words_per_row; col++) {
				*destword++ &= *srcptr++;
			}
			/* Trailing byte */
			destbyte = (unsigned char *)destword;
			*destbyte &= *(unsigned char *)srcptr;
			srcptr = (unsigned short *)((unsigned char *)srcptr + 1);
			destofs += pitch;
		}
	} else if (!has_trailing_byte && words_per_row > 0) {
		/* Case 2: Even width >= 2 - loop words only */
		for (row = 0; row < (int)height; row++) {
			destword = (unsigned short *)(bitmapptr + destofs);
			for (col = 0; col < words_per_row; col++) {
				*destword++ &= *srcptr++;
			}
			destofs += pitch;
		}
	} else {
		/* Case 3: Width is 1 - single byte per row */
		for (row = 0; row < (int)height; row++) {
			destbyte = bitmapptr + destofs;
			*destbyte &= *(unsigned char *)srcptr;
			srcptr = (unsigned short *)((unsigned char *)srcptr + 1);
			destofs += pitch;
		}
	}
}

/**
/** @brief Operation.
 * @param x Parameter `x`.
 * @param y Parameter `y`.
 * @return Function result.
 */
void putpixel_iconFillings(void * icon, unsigned short x, unsigned short y) {
	unsigned short * icondata = (unsigned short *)icon;
	unsigned int width, height;
	unsigned int words_per_row;
	int has_trailing_byte;
	unsigned short * srcptr;
	unsigned char * bitmapptr;
	unsigned int * lineofs;
	unsigned int destofs;
	int pitch;
	int row;
	unsigned int col;
	unsigned short * destword;
	unsigned char * destbyte;

	if (icon == 0) {
		return;
	}
	
	width = icondata[0];
	height = icondata[1];
	
	/* Get sprite1 info */
	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	pitch = sprite1.sprite_pitch;
	
	/* Calculate destination */
	destofs = lineofs[y] + x;
	
	/* Source pixels start at offset 16 (skip header) */
	srcptr = (unsigned short *)((unsigned char *)icondata + SHAPE2D_HEADER_BYTES);
	
	/* Calculate words per row and check for trailing byte */
	words_per_row = width >> 1;
	has_trailing_byte = width & 1;
	
	/* Handle three cases based on width */
	if (has_trailing_byte && words_per_row > 0) {
		/* Case 1: Odd width > 1 - loop words + 1 trailing byte per row */
		for (row = 0; row < (int)height; row++) {
			destword = (unsigned short *)(bitmapptr + destofs);
			for (col = 0; col < words_per_row; col++) {
				*destword++ |= *srcptr++;
			}
			/* Trailing byte */
			destbyte = (unsigned char *)destword;
			*destbyte |= *(unsigned char *)srcptr;
			srcptr = (unsigned short *)((unsigned char *)srcptr + 1);
			destofs += pitch;
		}
	} else if (!has_trailing_byte && words_per_row > 0) {
		/* Case 2: Even width >= 2 - loop words only */
		for (row = 0; row < (int)height; row++) {
			destword = (unsigned short *)(bitmapptr + destofs);
			for (col = 0; col < words_per_row; col++) {
				*destword++ |= *srcptr++;
			}
			destofs += pitch;
		}
	} else {
		/* Case 3: Width is 1 - single byte per row */
		for (row = 0; row < (int)height; row++) {
			destbyte = bitmapptr + destofs;
			*destbyte |= *(unsigned char *)srcptr;
			srcptr = (unsigned short *)((unsigned char *)srcptr + 1);
			destofs += pitch;
		}
	}
}

/**
 * sprite_putimage_transparent - Render sprite with transparency
 * 
 * Parameters:
 *   shape -  pointer to SHAPE2D structure
 *   x, y - position to render at
 * 
 * Renders the sprite using transparency where pixel value 255 is transparent.
 * Includes full clipping to sprite1 bounds.
 */
void sprite_putimage_transparent(void * shape, unsigned short x, unsigned short y) {
	struct SHAPE2D * shapeptr = (struct SHAPE2D *)shape;
	unsigned char * srcptr;
	unsigned char * bitmapptr;
	unsigned int * lineofs;
	unsigned int destofs;
	int pitch;
	int width, height;
	int rows_visible;
	int cols_visible;
	int clip_top, clip_bottom;
	int src_skip_left, src_skip_per_row;
	int sprite_top, sprite_height, sprite_left, sprite_right;
	int pos_x, pos_y;
	int row, col;
	unsigned char pixel;
	
	/* Get shape dimensions */
	width = shapeptr->s2d_width;
	height = shapeptr->s2d_height;
	pos_x = x;
	pos_y = y;
	
	/* Get sprite1 bounds */
	sprite_top = sprite1.sprite_top;
	sprite_height = sprite1.sprite_height;
	sprite_left = sprite1.sprite_left;
	sprite_right = sprite1.sprite_right;
	pitch = sprite1.sprite_pitch;
	
	/* Get sprite1 pointers */
	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	
	/* Source data starts at offset 16 (after SHAPE2D header) */
	srcptr = (unsigned char *)shapeptr + SHAPE2D_HEADER_BYTES;
	
	/* Vertical clipping */
	rows_visible = height;
	clip_top = 0;
	clip_bottom = 0;
	
	if (pos_y < sprite_top) {
		/* Shape starts above visible area */
		clip_top = sprite_top - pos_y;
		rows_visible -= clip_top;
		if (rows_visible <= 0) return;
		srcptr += clip_top * width;
		pos_y = sprite_top;
	}
	
	if (pos_y + rows_visible > sprite_height) {
		/* Shape extends below visible area */
		clip_bottom = (pos_y + rows_visible) - sprite_height;
		rows_visible -= clip_bottom;
		if (rows_visible <= 0) return;
	}
	
	/* Horizontal clipping */
	cols_visible = width;
	src_skip_left = 0;
	src_skip_per_row = 0;
	
	if (pos_x < sprite_left) {
		/* Shape starts left of visible area */
		src_skip_left = sprite_left - pos_x;
		cols_visible -= src_skip_left;
		if (cols_visible <= 0) return;
		srcptr += src_skip_left;
		pos_x = sprite_left;
	}
	
	if (pos_x + cols_visible > sprite_right) {
		/* Shape extends past right edge */
		int clip_right_amount = (pos_x + cols_visible) - sprite_right;
		cols_visible -= clip_right_amount;
		if (cols_visible <= 0) return;
		src_skip_per_row = clip_right_amount;
	}
	
	/* Add left skip to per-row skip (total non-visible pixels per source row) */
	src_skip_per_row += src_skip_left;
	
	/* Calculate initial destination offset */
	destofs = lineofs[pos_y] + pos_x;
	
/** @brief Transparency.
 * @param row Parameter `row`.
 * @return Function result.
 */
	/* Render with transparency (255 is transparent) */
	for (row = 0; row < rows_visible; row++) {
		unsigned char * destrow = bitmapptr + destofs;
		for (col = 0; col < cols_visible; col++) {
			pixel = *srcptr++;
			if (pixel != SHAPE2D_TRANSPARENT_COLOR) {
				destrow[col] = pixel;
			}
		}
		srcptr += src_skip_per_row;
		destofs += pitch;
	}
}

/**
 * shape_op_explosion - Render scaled sprite with transparency
 * 
 * Parameters:
/** @brief Factor.
 * @param x Parameter `x`.
 * @param y Parameter `y`.
 * @return Function result.
 */
void shape_op_explosion(int scale, void * shp, int x, int y) {
	struct SHAPE2D * shape = (struct SHAPE2D *)shp;
	unsigned char * srcdata;
	unsigned char * bitmapptr;
	unsigned int * lineofs;
	int pitch;
	int pos_x, pos_y;
	int scaled_width, scaled_height;
	int src_width;
	unsigned int scale_step;
	unsigned int frac_x, frac_y;
	int col;
	long temp;
	int dest_ofs;
	int skip_adjust;
	unsigned char * srcptr;
	unsigned char * srcrow;
	int rows_left;
	int cols_per_row;
	int pitch_adjust;
	
	/* Scale factor must be at least 2 */
	if (scale < 2) return;
	
	/* Calculate scaled dimensions (scale >> 8 applied to result) */
	temp = (long)shape->s2d_height * scale;
	scaled_height = (int)(temp >> 8);
	if (scaled_height == 0) return;
	
	temp = (long)shape->s2d_width * scale;
	scaled_width = (int)(temp >> 8);
	if (scaled_width == 0) return;
	
	src_width = shape->s2d_width;
	
	/* Calculate position adjusted by scaled hotspot */
	temp = (long)shape->s2d_hotspot_x * scale;
	pos_x = x - (int)(temp >> 8);
	
	temp = (long)shape->s2d_hotspot_y * scale;
	pos_y = y - (int)(temp >> 8);
	
	/* Get sprite info */
	bitmapptr = (unsigned char *)sprite1.sprite_bitmapptr;
	lineofs = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	pitch = sprite1.sprite_pitch;
	
	/* Calculate inverse scale step (16-bit fixed point) */
	/* scale_step = 65536 / scale; */
	scale_step = SHAPE2D_FIXED_ONE_16_16 / (unsigned int)scale;
	
	/* Initialize source pointer to shape data */
	srcdata = (unsigned char *)shape + SHAPE2D_HEADER_BYTES;
	
	/* Handle initial half-pixel offset for rounding */
	frac_x = 0;
	frac_y = 0;
	skip_adjust = (scale_step >> 1) >> 8;  /* Integer part of half step */
	if (skip_adjust > 0) {
		int i;
		srcdata += skip_adjust;
		for (i = 0; i < skip_adjust; i++) {
			srcdata += src_width;
		}
	}
	
	/* Clip horizontally */
	cols_per_row = scaled_width;
	if (pos_x < sprite1.sprite_left2) {
		int clip_cols = sprite1.sprite_left2 - pos_x;
		cols_per_row = scaled_width - clip_cols;
		if (cols_per_row <= 0) return;
		
		/* Advance source by fractional amount */
		temp = (long)clip_cols * scale_step;
		frac_x = (unsigned int)temp & SHAPE2D_BYTE_LOW_MASK_16;
		srcdata += (int)(temp >> 8);
		
		pos_x = sprite1.sprite_left2;
	}
	
	if (pos_x + cols_per_row > sprite1.sprite_widthsum) {
		int clip_cols = (pos_x + cols_per_row) - sprite1.sprite_widthsum;
		cols_per_row -= clip_cols;
		if (cols_per_row <= 0) return;
	}
	
	pitch_adjust = pitch - cols_per_row;
	(void)pitch_adjust;
	
	/* Clip vertically */
	rows_left = scaled_height;
	if (pos_y < sprite1.sprite_top) {
		int clip_rows = sprite1.sprite_top - pos_y;
		rows_left = scaled_height - clip_rows;
		if (rows_left <= 0) return;
		
		/* Advance source rows by fractional amount */
		temp = (long)clip_rows * scale_step;
		frac_y = (unsigned int)temp & SHAPE2D_BYTE_LOW_MASK_16;
		skip_adjust = (int)(temp >> 8);
		srcdata += (long)skip_adjust * src_width;
		
		pos_y = sprite1.sprite_top;
	}
	
	if (pos_y + rows_left > sprite1.sprite_height) {
		int clip_rows = (pos_y + rows_left) - sprite1.sprite_height;
		rows_left -= clip_rows;
		if (rows_left <= 0) return;
	}
	
	srcrow = srcdata;
	
	/* Calculate initial destination offset */
	dest_ofs = lineofs[pos_y] + pos_x;
	
	/* Render loop */
	while (rows_left > 0) {
		unsigned char * destptr = bitmapptr + dest_ofs;
		unsigned int bx = frac_x;
		srcptr = srcrow;
		
		/* Render one row */
		for (col = 0; col < cols_per_row; col++) {
			unsigned char pixel = *srcptr;
			if (pixel != SHAPE2D_TRANSPARENT_COLOR) {
				*destptr = pixel;
			}
			destptr++;
			
			/* Advance source X with fixed-point */
			bx += scale_step;
			srcptr += (bx >> 8);  /* Add integer part */
			bx &= SHAPE2D_BYTE_LOW_MASK_16;         /* Keep only fractional part */
		}
		
		rows_left--;
		if (rows_left <= 0) break;
		
		/* Move to next destination row */
		dest_ofs += pitch;
		
		/* Advance source Y with fixed-point */
		frac_y += scale_step;
		if ((frac_y >> 8) != 0) {
			int skip_rows = frac_y >> 8;
			while (skip_rows > 0) {
				srcrow += src_width;
				skip_rows--;
			}
			frac_y &= SHAPE2D_BYTE_LOW_MASK_16;  /* Keep only fractional part */
		}
	}
}

/*
 * sprite_copy_rect_2_to_1 - Copy rectangle from sprite2 to sprite1
 * Ported from original ASM
 * 
 * Parameters:
 *   left - x position
 *   top - y position  
 *   width - width of rectangle to copy
 *   height - height of rectangle to copy
 *   xofs - x offset for destination wrapping
 */

/**
 * @brief Copy a rectangular region from sprite2 to sprite1 with x-offset wrapping.
 *
 * @param left    X position in the source.
 * @param top     Y position in the source.
 * @param width   Width of the rectangle.
 * @param height  Height of the rectangle.
/** @brief Destination.
 * @param xofs Parameter `xofs`.
 * @return Function result.
 */
void sprite_copy_rect_2_to_1(int left, int top, int width, int height, int xofs) {
	unsigned char * es_ptr;
	unsigned char * ds_ptr;
	unsigned short * lineofs1;
	unsigned short * lineofs2;
	int var_2;  /* adjusted x dest */
	int var_4;  /* sprite2 line table idx * 2 */
	int var_6;  /* sprite1 line table idx * 2 */
	int dx;     /* height counter */
	int cx;     /* width */
	int quotient, remainder;
	int si_ofs, di_ofs;
	
	es_ptr = (unsigned char *)sprite1.sprite_bitmapptr;
	ds_ptr = (unsigned char *)sprite2.sprite_bitmapptr;
	
	lineofs1 = (unsigned short *)shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	lineofs2 = (unsigned short *)shape2d_lineofs_flat((unsigned int*)sprite2.sprite_lineofs);
	
	var_2 = left;
	var_4 = top;  /* row index for sprite2 */
	var_6 = top;  /* row index for sprite1 */
	
	/* Handle x wrapping calculation:
	 * quotient = (xofs + left) / sprite1.sprite_width2
	 * remainder = (xofs + left) % sprite1.sprite_width2
	 * var_6 (row) gets adjusted by quotient
	 * var_2 (x offset) gets adjusted by remainder - left
	 */
	{
		int val = xofs + left;
		int w2 = (int)sprite1.sprite_width2;
		if (w2 != 0) {
			quotient = val / w2;
			remainder = val % w2;
			/* Handle negative remainder from signed division */
			if (val < 0 && remainder != 0) {
				quotient--;
				remainder += w2;
			}
			var_6 = top + quotient;
			var_2 = left + (remainder - left);
		}
	}
	
	dx = height;
	while (dx > 0) {
		cx = width;
		si_ofs = lineofs2[var_4] + left;
		di_ofs = lineofs1[var_6] + var_2;
		
		/* Copy row */
		memcpy(es_ptr + di_ofs, ds_ptr + si_ofs, cx);
		
		var_4++;
		var_6++;
		dx--;
	}
}

/*
 * sprite_xor_fill_rect - XOR fill rectangle on sprite1 with clipping
 * Ported from original ASM
 * 
 * Parameters:
 *   x - x position
 *   y - y position  
 *   width - width of rectangle
 *   height - height of rectangle
 *   color - color byte to XOR
 */
/** @brief Sprite xor fill rect.
 * @param x Parameter `x`.
 * @param y Parameter `y`.
 * @param width Parameter `width`.
 * @param height Parameter `height`.
 * @param color Parameter `color`.
 */

void sprite_xor_fill_rect(int x, int y, int width, int height, unsigned char color) {
	unsigned char * es_ptr;
	unsigned short * lineofs1;
	int ax;
	
	/* Clip left edge */
	ax = sprite1.sprite_left - x;
	if (ax > 0) {
		x = sprite1.sprite_left;
		width -= ax;
		if (width <= 0) return;
	}
	
	/* Clip right edge */
	ax = x + width - sprite1.sprite_right;
	if (ax > 0) {
		width -= ax;
		if (width <= 0) return;
	}
	
	/* Clip top edge */
	ax = sprite1.sprite_top - y;
	if (ax > 0) {
		height -= ax;
		if (height <= 0) return;
		y = sprite1.sprite_top;
	}
	
	/* Clip bottom edge */
	ax = y + height - sprite1.sprite_height;
	if (ax > 0) {
		height -= ax;
		if (height <= 0) return;
	}
	
	/* Final bounds check */
	if (width <= 0 || height <= 0) return;
	
	es_ptr = (unsigned char *)sprite1.sprite_bitmapptr;
	lineofs1 = (unsigned short *)shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	
	{
		int row;
		
		for (row = 0; row < height; row++) {
			int di = lineofs1[y + row] + x;
			int col;
			
			/* XOR each pixel in the row */
			for (col = 0; col < width; col++) {
				es_ptr[di + col] ^= color;
			}
		}
	}
}

/*
/** @brief Lookup.
 * @param pixel Parameter `pixel`.
 * @param shape Parameter `shape`.
 * @return Function result.
 */
/** @brief Sprite draw shape with palmap.
 * @param shape Parameter `shape`.
 */

void sprite_draw_shape_with_palmap(void * shape) {
	unsigned char * es_ptr;
	struct SHAPE2D * shp;
	unsigned char * pixels;
	unsigned int * lineofs1;
	int width, height, xpos, ypos;
	int row, col;
	int draw_width, draw_height;
	int src_skip_x, src_skip_y;
	unsigned int di;
	unsigned int src_ofs;
	int src_pitch_skip;
	
	if (shape == 0 || sprite1.sprite_bitmapptr == 0) {
		return;
	}

	shp = (struct SHAPE2D *)shape;
	
	width = shp->s2d_width;
	height = shp->s2d_height;
	xpos = (short)shp->s2d_pos_x;
	ypos = (short)shp->s2d_pos_y;
	if (width <= 0 || height <= 0) {
		return;
	}
	if (sprite1.sprite_pitch <= 0 || sprite1.sprite_height <= 0) {
		return;
	}
	
	es_ptr = (unsigned char *)sprite1.sprite_bitmapptr;
	lineofs1 = shape2d_lineofs_flat((unsigned int*)sprite1.sprite_lineofs);
	
	pixels = ((unsigned char *)shape) + SHAPE2D_HEADER_BYTES;

	/* Clip top */
	src_skip_y = 0;
	draw_height = height;
	if (ypos < sprite1.sprite_top) {
		src_skip_y = sprite1.sprite_top - ypos;
		draw_height -= src_skip_y;
		ypos = sprite1.sprite_top;
	}
	if (ypos + draw_height > (int)sprite1.sprite_height) {
		draw_height = (int)sprite1.sprite_height - ypos;
	}
	if (draw_height <= 0) {
		return;
	}

	/* Clip left */
	src_skip_x = 0;
	draw_width = width;
	if (xpos < sprite1.sprite_left) {
		src_skip_x = sprite1.sprite_left - xpos;
		draw_width -= src_skip_x;
		xpos = sprite1.sprite_left;
	}
	if (xpos + draw_width > sprite1.sprite_right) {
		draw_width = sprite1.sprite_right - xpos;
	}
	if (draw_width <= 0) {
		return;
	}

	src_ofs = (unsigned int)(src_skip_y * width + src_skip_x);
	src_pitch_skip = width - draw_width;
	
	for (row = 0; row < draw_height; row++) {
		di = lineofs1[ypos + row] + xpos;
		
		for (col = 0; col < draw_width; col++) {
			unsigned char srcpixel = pixels[src_ofs++];
			unsigned char mapped = incnums[srcpixel];
			
			if (mapped != SHAPE2D_TRANSPARENT_COLOR) {
				es_ptr[di] = mapped;
			}
			di++;
		}

		src_ofs += src_pitch_skip;
	}
}

/*
 * sprite_draw_text_opaque - Render text string to sprite1 using font data
 * Ported from original ASM
 * 
 * Parameters:
 *   text - null-terminated string to render
 *   x - x position
 *   y - y position
 */
/* Font data structure in fontdefseg:
 *   [8] = cursor x
/** @brief Offset.
 * @param fontdef Parameter `fontdef`.
 * @param y Parameter `y`.
 * @return Function result.
 */

/** sprite_draw_text_opaque in ASM is its own opaque renderer proc: it paints BOTH fg pixels
 * (bit=1 → al=fontdef[0]) and bg pixels (bit=0 → ah=fontdef[2]), giving a
 * solid character-cell highlight. font_draw_text is transparent (skips bit=0).
 * This matches the split in seg012.asm: sprite_draw_text_opaque proc vs font_draw_text proc. */
void sprite_draw_text_opaque(char* text, int x, int y) {
	if (text == 0) {
		return;
	}
	unsigned char * fontdef = g_fontdef_ptr;
	unsigned char * vram = (unsigned char *)sprite1.sprite_bitmapptr;
	unsigned int * lineofs = sprite1.sprite_lineofs;
	if (fontdef == 0) {
		return;
	}
	unsigned char fg_color = fontdef[0];
	unsigned char bg_color = fontdef[2]; /* fontdef[1] as unsigned short* = byte offset 2 */
	unsigned char proportional = fontdef[SHAPE2D_FONTDEF_PROPORTIONAL_OFFSET];
	unsigned short prop1_width = *(unsigned short *)(fontdef + SHAPE2D_FONTDEF_PROP1_WIDTH_OFFSET);
	unsigned short font_height = *(unsigned short *)(fontdef + SHAPE2D_FONTDEF_HEIGHT_OFFSET);
	unsigned short default_charwidth = *(unsigned short *)(fontdef + SHAPE2D_FONTDEF_DEFAULT_WIDTH_OFFSET);
	unsigned short * char_table = (unsigned short *)(fontdef + SHAPE2D_FONTDEF_CHAR_TABLE_OFFSET);
	int has_width_byte = (proportional == 2) || (proportional == 1 && prop1_width == 0);
	if (font_height == 0) {
		font_height = fontdef_line_height;
		if (font_height == 0) font_height = 8;
	}
	if (default_charwidth == 0) default_charwidth = 8;
	
	*(unsigned short *)(fontdef + SHAPE2D_FONTDEF_CURSOR_X_OFFSET) = (unsigned short)x;
	*(unsigned short *)(fontdef + SHAPE2D_FONTDEF_CURSOR_Y_OFFSET) = (unsigned short)y;
	
	while (*text) {
		unsigned char ch = (unsigned char)*text++;
		unsigned short char_ptr = char_table[ch];
		if (char_ptr == 0) {
			if (ch == 13 || ch == 10) {
				*(unsigned short *)(fontdef + 10) += (unsigned short)(font_height + 2);
			}
			continue;
		}
		unsigned short char_x = *(unsigned short *)(fontdef + 8);
		unsigned short char_pixel_width;
		if (has_width_byte) {
			char_pixel_width = fontdef[char_ptr];
			char_ptr++;
		} else if (proportional == 1 && prop1_width != 0) {
			char_pixel_width = prop1_width;
		} else {
			char_pixel_width = default_charwidth;
		}
		unsigned short char_width_bytes = (char_pixel_width + 7) >> 3;
		unsigned short char_y = *(unsigned short *)(fontdef + 10);
		unsigned char * src = fontdef + char_ptr;
		for (unsigned short i = 0; i < font_height; i++) {
			unsigned short di = lineofs[char_y + i] + char_x;
			for (unsigned short j = 0; j < char_width_bytes; j++) {
				unsigned char byte = *src++;
				for (unsigned short k = 0; k < 8; k++) {
					/* Opaque render: paint both fg (bit=1) and bg (bit=0) */
					vram[di] = (byte & 128) ? fg_color : bg_color;
					byte <<= 1;
					di++;
				}
			}
		}
		*(unsigned short *)(fontdef + 8) += char_pixel_width;
	}
}

/* ============================================================
 * Inline units formerly in shape2d_anim.c / shape2d_hit.c /
 * shape2d_res.c / shape2d_sprite_plan.c
 * All of these are pure functions with no VGA or global state.
 * ============================================================ */

/** @brief Helpers.
 * @param period Parameter `period`.
 * @return Function result.
 */
/* ---- animation phase helpers (ex shape2d_anim.c) ---- */

/**
 * @brief Advance an animation phase counter with wrap-around.
 */
unsigned short sprite_phase_add_wrap(unsigned short phase,
                                     unsigned short delta,
                                     unsigned short period)
{
    unsigned int value;

    if (period == 0) {
        return phase;
    }

    value = (unsigned int)phase + (unsigned int)delta;
    while (value > period) {
        value -= period;
    }

    return (unsigned short)value;
}

/**
 * @brief Select between two sprite frames based on a blink threshold.
 */
unsigned short sprite_pick_blink_frame(unsigned short phase,
                                       unsigned short threshold,
                                       unsigned short sprite_hi,
                                       unsigned short sprite_lo)
{
    if (phase > threshold) {
        return sprite_hi;
    }
    return sprite_lo;
}

/** @brief Point.
 * @param px Parameter `px`.
 * @param y2_array Parameter `y2_array`.
 * @return Function result.
 */
/* ---- point-in-rectangle hit-testing (ex shape2d_hit.c) ---- */

/**
 * @brief Test whether point (px,py) falls inside any of the @p count rectangles.
 */
short sprite_hittest_point(unsigned short px, unsigned short py,
                           short count,
                           const unsigned short* x1_array,
                           const unsigned short* x2_array,
                           const unsigned short* y1_array,
                           const unsigned short* y2_array)
{
    short i;

    if (count <= 0 || x1_array == 0 || x2_array == 0 ||
        y1_array == 0 || y2_array == 0) {
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (x1_array[i] > px) continue;
        if (x2_array[i] < px) continue;
        if (y1_array[i] > py) continue;
        if (y2_array[i] < py) continue;
        return i;
    }

    return -1;
}

/** @brief Interrogation.
 * @param memchunk Parameter `memchunk`.
 * @return Function result.
 */
/* ---- resource-chunk interrogation (ex shape2d_res.c) ---- */

unsigned short shape2d_resource_shape_count(const unsigned char* memchunk)
{
    if (memchunk == 0) {
        return 0;
    }
    return (unsigned short)(
        (unsigned short)memchunk[SHAPE2D_RES_COUNT_LO_OFFSET] |
        ((unsigned short)memchunk[SHAPE2D_RES_COUNT_HI_OFFSET] << SHAPE2D_BYTE_SHIFT_8));
}

/** @brief Shape2d resource get shape.
 * @param memchunk Parameter `memchunk`.
 * @param index Parameter `index`.
 * @return Function result.
 */
struct SHAPE2D* shape2d_resource_get_shape(unsigned char* memchunk, int index)
{
    unsigned short shapecount;
    uint32_t offsetofs;
    uint32_t dataofs;
    uint32_t chunkofs;

    if (memchunk == 0 || index < 0) {
        return 0;
    }

    shapecount = shape2d_resource_shape_count(memchunk);
    if ((unsigned int)index >= (unsigned int)shapecount) {
        return 0;
    }

    offsetofs = ((uint32_t)(unsigned int)index     << SHAPE2D_RES_OFFSET_SHIFT) +
                ((uint32_t)shapecount              << SHAPE2D_RES_OFFSET_SHIFT) +
                SHAPE2D_RES_HEADER_BYTES;

    dataofs   = ((uint32_t)shapecount << SHAPE2D_RES_DATA_SHIFT) + SHAPE2D_RES_HEADER_BYTES;

    chunkofs = ((uint32_t)memchunk[offsetofs + 0u])                                     |
               ((uint32_t)memchunk[offsetofs + 1u] << SHAPE2D_BYTE_SHIFT_8)  |
               ((uint32_t)memchunk[offsetofs + 2u] << SHAPE2D_BYTE_SHIFT_16) |
               ((uint32_t)memchunk[offsetofs + 3u] << SHAPE2D_BYTE_SHIFT_24);

    return (struct SHAPE2D*)(memchunk + dataofs + chunkofs);
}

/** @brief Copy.
 * @param src Parameter `src`.
 * @return Function result.
 */
/* ---- sprite clear-plan and struct copy (ex shape2d_sprite_plan.c) ---- */

void sprite_copy_assign(struct SPRITE* dst, const struct SPRITE* src)
{
    if (dst == 0 || src == 0) {
        return;
    }
    memcpy(dst, src, sizeof(struct SPRITE));
}

/** @brief Sprite compute clear plan.
 * @param sprite Parameter `sprite`.
 * @param lineofs Parameter `lineofs`.
 * @param out_ofs Parameter `out_ofs`.
 * @param out_lines Parameter `out_lines`.
 * @param out_width Parameter `out_width`.
 * @param out_widthdiff Parameter `out_widthdiff`.
 * @return Function result.
 */
int sprite_compute_clear_plan(const struct SPRITE* sprite,
                              const unsigned int* lineofs,
                              unsigned int* out_ofs,
                              int* out_lines,
                              int* out_width,
                              int* out_widthdiff)
{
    int top;
    int left;
    int right;
    int pitch;
    int lines;
    int width;

    if (sprite == 0 || lineofs == 0 || out_ofs == 0 ||
        out_lines == 0 || out_width == 0 || out_widthdiff == 0) {
        return 0;
    }

    top   = (int)sprite->sprite_top;
    left  = (int)sprite->sprite_left;
    right = (int)sprite->sprite_right;
    pitch = (int)sprite->sprite_pitch;

    lines = (int)sprite->sprite_height - top;
    if (lines <= 0) {
        return 0;
    }

    width = right - left;
    if (width <= 0) {
        return 0;
    }

    *out_ofs       = lineofs[top] + (unsigned int)left;
    *out_lines     = lines;
    *out_width     = width;
    *out_widthdiff = pitch - width;
    return 1;
}

/** @brief Sprite execute clear plan.
 * @param bitmap Parameter `bitmap`.
 * @param ofs Parameter `ofs`.
 * @param lines Parameter `lines`.
 * @param width Parameter `width`.
 * @param widthdiff Parameter `widthdiff`.
 * @param color Parameter `color`.
 */
void sprite_execute_clear_plan(unsigned char* bitmap,
                               unsigned int ofs,
                               int lines,
                               int width,
                               int widthdiff,
                               unsigned char color)
{
    int i;
    int j;

    if (bitmap == 0 || lines <= 0 || width <= 0) {
        return;
    }

    for (i = 0; i < lines; i++) {
        for (j = 0; j < width; j++) {
            bitmap[ofs++] = color;
        }
        ofs += (unsigned int)widthdiff;
    }
}

