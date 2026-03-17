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

#ifndef SHAPE2D_H
#define SHAPE2D_H

#include <stddef.h>

#pragma pack (push, 1)
struct SHAPE2D {
	unsigned short s2d_width;
	unsigned short s2d_height;
	unsigned short s2d_hotspot_x;
	unsigned short s2d_hotspot_y;
	unsigned short s2d_pos_x;
	unsigned short s2d_pos_y;
	unsigned char s2d_plane_nibbles[4];
};

struct SPRITE {
	struct SHAPE2D * sprite_bitmapptr;
	unsigned short sprite_reserved1;
	unsigned short sprite_reserved2;
	unsigned short sprite_reserved3;
	unsigned int* sprite_lineofs; /* near pointer to line offset table (CS segment) */
	unsigned short sprite_left;
	unsigned short sprite_right;
	unsigned short sprite_top;
	unsigned short sprite_height;
	unsigned short sprite_pitch;
	unsigned short sprite_reserved4;
	unsigned short sprite_width2;
	unsigned short sprite_left2;
	unsigned short sprite_widthsum;
};
#pragma pack (pop)

struct SPRITE * sprite_make_wnd(unsigned int width, unsigned int height, unsigned int);
void sprite_free_wnd(struct SPRITE * wndsprite);

void sprite_set_1_from_argptr(struct SPRITE * argsprite);

void sprite_copy_2_to_1(void);
void sprite_copy_2_to_1_clear(void);
void sprite_select_wnd_as_sprite1(void);
void sprite_select_wnd_as_sprite1_and_clear(void);

void sprite_copy_both_to_arg(struct SPRITE* argsprite);
void sprite_copy_arg_to_both(struct SPRITE* argsprite);

void sprite_clear_sprite1_color(unsigned char color);
void sprite_fill_rect(unsigned short x, unsigned short y, unsigned short width, unsigned short height, unsigned char color);
void sprite_fill_rect_clipped(unsigned short x, unsigned short y, unsigned short width, unsigned short height, unsigned char color);
void sprite_draw_rect_outline(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned short color);

unsigned short sprite_phase_add_wrap(unsigned short phase, unsigned short delta, unsigned short period);
unsigned short sprite_pick_blink_frame(unsigned short phase, unsigned short threshold,
									   unsigned short sprite_hi, unsigned short sprite_lo);
short sprite_hittest_point(unsigned short px, unsigned short py,
						   short count,
						   const unsigned short* x1_array,
						   const unsigned short* x2_array,
						   const unsigned short* y1_array,
						   const unsigned short* y2_array);
void sprite_copy_assign(struct SPRITE* dst, const struct SPRITE* src);
int sprite_compute_clear_plan(const struct SPRITE* sprite,
							  const unsigned int* lineofs,
							  unsigned int* out_ofs,
							  int* out_lines,
							  int* out_width,
							  int* out_widthdiff);
void sprite_execute_clear_plan(unsigned char* bitmap,
							   unsigned int ofs,
							   int lines,
							   int width,
							   int widthdiff,
							   unsigned char color);
int sprite_wnd_stack_reserve(unsigned char* base,
							 size_t capacity,
							 unsigned char** next,
							 unsigned short height,
							 struct SPRITE** out_sprite,
							 unsigned int** out_lineofs);
int sprite_wnd_stack_release(unsigned char* base,
							 unsigned char** next,
							 struct SPRITE* wndsprite);

void sprite_putimage(struct SHAPE2D * shape);
void sprite_putimage_and(struct SHAPE2D * shape, unsigned short a, unsigned short b);
void sprite_putimage_or(struct SHAPE2D * shape, unsigned short a, unsigned short b);
void sprite_putimage_and_alt(void * shapeptr, unsigned short x, unsigned short y);
void sprite_putimage_copy_at(void * shapeptr, unsigned short x, unsigned short y);
void sprite_putimage_and_at_shape_origin(void * shapeptr, unsigned short x, unsigned short y);
void sprite_putimage_or_at_shape_origin(void * shapeptr, unsigned short x, unsigned short y);
void sprite_putimage_and_alt2(void * shapeptr, unsigned short x, unsigned short y);
void sprite_putimage_or_alt(void * shapeptr, unsigned short x, unsigned short y);

void setup_mcgawnd1(void);
void setup_mcgawnd2(void);

struct SHAPE2D * file_get_shape2d(unsigned char * memchunk, int index);

unsigned short file_get_res_shape_count(void * memchunk);

void file_unflip_shape2d(unsigned char * memchunk, char * mempages);

void file_unflip_shape2d_pes(unsigned char * memchunk, char * mempages);

void file_load_shape2d_expand(unsigned char * memchunk, char * mempages);

unsigned short file_get_unflip_size(unsigned char * memchunk);

unsigned short file_load_shape2d_expandedsize(void * memchunk);

void file_load_shape2d_palmap_init(unsigned char * pal);
void file_load_shape2d_palmap_apply(unsigned char * memchunk, unsigned char palmap[]);

void * file_load_shape2d_esh(void * memchunk, const char* str);
void * file_load_shape2d(char* shapename, int fatal);

void * file_load_shape2d_fatal(char* shapename);
void * file_load_shape2d_nofatal(const char* shapename);

void * file_load_shape2d_res(char* resname, int fatal);
void * file_load_shape2d_res_fatal(char* resname);
void * file_load_shape2d_res_nofatal(char* resname);

int shape2d_blit_rows(unsigned char* dst,
					  size_t dst_size,
					  unsigned int destofs,
					  const unsigned char* src,
					  unsigned int width,
					  unsigned int height,
					  unsigned int pitch);

unsigned int* shape2d_lineofs_flat(unsigned int* stored_lineofs);

unsigned short shape2d_resource_shape_count(const unsigned char* memchunk);
struct SHAPE2D* shape2d_resource_get_shape(unsigned char* memchunk, int index);

/* Drawing primitives */
void putpixel_single_clipped(int color, unsigned short y, unsigned short x);
void draw_filled_lines(unsigned short color, unsigned short numlines,
                           unsigned short y, unsigned short * x2arr,
                           unsigned short * x1arr);
void draw_patterned_lines(unsigned char color, unsigned short numlines,
                              unsigned short y, unsigned short * x2arr,
                              unsigned short * x1arr);
void draw_two_color_patterned_lines(unsigned char color, unsigned short numlines,
									unsigned short y, unsigned short * x2arr,
									unsigned short * x1arr);

/* RLE sprite rendering */
void shape2d_render_bmp_as_mask(void * shapeptr);
void shape2d_draw_rle_or(struct SHAPE2D * shape);
void shape2d_draw_rle_copy(void * shapeptr);
void shape2d_draw_rle_copy_at(void * shapeptr, unsigned short x, unsigned short y);
void shape2d_draw_rle_copy_clipped(void * shapeptr);
void shape2d_draw_rle_copy_clipped_at(void * shapeptr, unsigned short x, unsigned short y);

/* Global sprite framebuffers (defined in shape2d.c) */

#ifdef SHAPE2D_IMPL
#  define _S2_
#else
#  define _S2_ extern
#endif

_S2_ struct SPRITE sprite1;

#undef _S2_

/* Shape2D file loading thunks */
void * file_load_shape2d_fatal_thunk(const char* shapename);
void * file_load_shape2d_nofatal_thunk(const char* shapename);
void * file_load_shape2d_res_nofatal_thunk(const char* resname);

/* XOR rectangle fill on sprite */
void sprite_xor_fill_rect(int x, int y, int width, int height, unsigned char color);

#endif
