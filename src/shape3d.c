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
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "stunts.h"
#include "ressources.h"
#include "memmgr.h"
#include "shape3d.h"
#include "shape2d.h"
/* Variables moved from data_game.c */
static void (*spritefunc)(unsigned short, unsigned short, unsigned short, unsigned short*, unsigned short*) = 0;
static void (*imagefunc)(unsigned short, unsigned short, unsigned short, unsigned short, unsigned short) = 0;
static void * game1ptr = 0;
static void * game2ptr = 0;
static unsigned short polygon_info_head = 0;
static unsigned polygon_op_error_code = 0;
static unsigned char* polyinfoptr = 0;
static int* polyinfoptrs[4097] = { 0 };
static unsigned select_rect_scale_preview = 0;
static unsigned char shape3d_vector_direction_bucket = 0;
static unsigned short zorder_current_index = 0;

/* Variables moved from data_game.c (private to this translation unit) */
static short * car2resptr = 0;
static struct VECTOR carshapevec2 = { 0 };
static unsigned char cos80_2[4] = { 0, 0, 0, 0 };
static char * curshapeptr = 0;
static struct MATRIX mat_y0 = { 0 };
static struct MATRIX mat_y100 = { 0 };
static struct MATRIX mat_y200 = { 0 };
static struct MATRIX mat_y300 = { 0 };
static struct VECTOR oppcarshapevec2 = { 0 };
static unsigned char palette_brightness_level = 7;
static unsigned short polyinfonumpolys = 0;
static unsigned int polyinfoptrnext = 0;
static unsigned char projectiondata1[2] = { 0, 0 };
static unsigned char projectiondata2[2] = { 0, 0 };
static unsigned char projectiondata3[2] = { 160, 0 };
static unsigned char projectiondata4[2] = { 0, 0 };
static unsigned char projectiondata6[2] = { 100, 0 };
static unsigned char projectiondata7[2] = { 0, 0 };
static unsigned char sin80_2[580] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char transshapenumvertscopy = 0;
static unsigned short zorder_next_link = 0;
static int zorder_shape_list[4097] = { 0 };
static unsigned short zorder_tail_counter = 0;


/* file-local data (moved from data_global.c) */
static char aStxxx[7] = "stxxx";   /* "st" + 4-char car ID + NUL */
static unsigned char car_wheel_vertex_data[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static char track_object_shape_names[116 * 5] = {
    /* 00 barn */ 'b','a','r','n',0,
    /* 01 zbrn */ 'z','b','r','n',0,
    /* 02 brid */ 'b','r','i','d',0,
    /* 03 zbri */ 'z','b','r','i',0,
    /* 04 btur */ 'b','t','u','r',0,
    /* 05 zbtu */ 'z','b','t','u',0,
    /* 06 chi1 */ 'c','h','i','1',0,
    /* 07 zch1 */ 'z','c','h','1',0,
    /* 08 chi2 */ 'c','h','i','2',0,
    /* 09 zch2 */ 'z','c','h','2',0,
    /* 10 elrd */ 'e','l','r','d',0,
    /* 11 zelr */ 'z','e','l','r',0,
    /* 12 fini */ 'f','i','n','i',0,
    /* 13 zfin */ 'z','f','i','n',0,
    /* 14 gass */ 'g','a','s','s',0,
    /* 15 zgas */ 'z','g','a','s',0,
    /* 16 lban */ 'l','b','a','n',0,
    /* 17 zlba */ 'z','l','b','a',0,
    /* 18 loop */ 'l','o','o','p',0,
    /* 19 zloo */ 'z','l','o','o',0,
    /* 20 offi */ 'o','f','f','i',0,
    /* 21 zoff */ 'z','o','f','f',0,
    /* 22 pipe */ 'p','i','p','e',0,
    /* 23 zpip */ 'z','p','i','p',0,
    /* 24 ramp */ 'r','a','m','p',0,
    /* 25 zram */ 'z','r','a','m',0,
    /* 26 rban */ 'r','b','a','n',0,
    /* 27 zrba */ 'z','r','b','a',0,
    /* 28 rdup */ 'r','d','u','p',0,
    /* 29 zrdu */ 'z','r','d','u',0,
    /* 30 road */ 'r','o','a','d',0,
    /* 31 zroa */ 'z','r','o','a',0,
    /* 32 stur */ 's','t','u','r',0,
    /* 33 zstu */ 'z','s','t','u',0,
    /* 34 tenn */ 't','e','n','n',0,
    /* 35 zten */ 'z','t','e','n',0,
    /* 36 tunn */ 't','u','n','n',0,
    /* 37 ztun */ 'z','t','u','n',0,
    /* 38 turn */ 't','u','r','n',0,
    /* 39 ztur */ 'z','t','u','r',0,
    /* 40 goui */ 'g','o','u','i',0,
    /* 41 gouo */ 'g','o','u','o',0,
    /* 42 goup */ 'g','o','u','p',0,
    /* 43 high */ 'h','i','g','h',0,
    /* 44 lakc */ 'l','a','k','c',0,
    /* 45 lake */ 'l','a','k','e',0,
    /* 46 cld1 */ 'c','l','d','1',0,
    /* 47 cld2 */ 'c','l','d','2',0,
    /* 48 cld3 */ 'c','l','d','3',0,
    /* 49 sigl */ 's','i','g','l',0,
    /* 50 sigr */ 's','i','g','r',0,
    /* 51 tree */ 't','r','e','e',0,
    /* 52 inte */ 'i','n','t','e',0,
    /* 53 zint */ 'z','i','n','t',0,
    /* 54 offl */ 'o','f','f','l',0,
    /* 55 zofl */ 'z','o','f','l',0,
    /* 56 offr */ 'o','f','f','r',0,
    /* 57 zofr */ 'z','o','f','r',0,
    /* 58 palm */ 'p','a','l','m',0,
    /* 59 zpal */ 'z','p','a','l',0,
    /* 60 bank */ 'b','a','n','k',0,
    /* 61 zban */ 'z','b','a','n',0,
    /* 62 sofl */ 's','o','f','l',0,
    /* 63 zsol */ 'z','s','o','l',0,
    /* 64 sofr */ 's','o','f','r',0,
    /* 65 zsor */ 'z','s','o','r',0,
    /* 66 sram */ 's','r','a','m',0,
    /* 67 zsra */ 'z','s','r','a',0,
    /* 68 selr */ 's','e','l','r',0,
    /* 69 zser */ 'z','s','e','r',0,
    /* 70 elsp */ 'e','l','s','p',0,
    /* 71 zesp */ 'z','e','s','p',0,
    /* 72 cact */ 'c','a','c','t',0,
    /* 73 cact */ 'c','a','c','t',0,
    /* 74 spip */ 's','p','i','p',0,
    /* 75 zspi */ 'z','s','p','i',0,
    /* 76 sest */ 's','e','s','t',0,
    /* 77 zses */ 'z','s','e','s',0,
    /* 78 wroa */ 'w','r','o','a',0,
    /* 79 zwro */ 'z','w','r','o',0,
    /* 80 barr */ 'b','a','r','r',0,
    /* 81 zbar */ 'z','b','a','r',0,
    /* 82 lco0 */ 'l','c','o','0',0,
    /* 83 zlco */ 'z','l','c','o',0,
    /* 84 rco0 */ 'r','c','o','0',0,
    /* 85 zrco */ 'z','r','c','o',0,
    /* 86 gwro */ 'g','w','r','o',0,
    /* 87 zgwr */ 'z','g','w','r',0,
    /* 88 lco1 */ 'l','c','o','1',0,
    /* 89 rco1 */ 'r','c','o','1',0,
    /* 90 loo1 */ 'l','o','o','1',0,
    /* 91 hig1 */ 'h','i','g','1',0,
    /* 92 hig2 */ 'h','i','g','2',0,
    /* 93 hig3 */ 'h','i','g','3',0,
    /* 94 wind */ 'w','i','n','d',0,
    /* 95 zwin */ 'z','w','i','n',0,
    /* 96 boat */ 'b','o','a','t',0,
    /* 97 zboa */ 'z','b','o','a',0,
    /* 98 rest */ 'r','e','s','t',0,
    /* 99 zres */ 'z','r','e','s',0,
    /*100 hpip */ 'h','p','i','p',0,
    /*101 zhpi */ 'z','h','p','i',0,
    /*102 vcor */ 'v','c','o','r',0,
    /*103 zvco */ 'z','v','c','o',0,
    /*104 tun2 */ 't','u','n','2',0,
    /*105 pip2 */ 'p','i','p','2',0,
    /*106 fenc */ 'f','e','n','c',0,
    /*107 zfen */ 'z','f','e','n',0,
    /*108 cfen */ 'c','f','e','n',0,
    /*109 zcfe */ 'z','c','f','e',0,
    /*110 flag */ 'f','l','a','g',0,
    /*111 truk */ 't','r','u','k',0,
    /*112 exp0 */ 'e','x','p','0',0,
    /*113 exp1 */ 'e','x','p','1',0,
    /*114 exp2 */ 'e','x','p','2',0,
    /*115 exp3 */ 'e','x','p','3',0,
};

static uint32_t invpow2tbl[32] = {
    2147483648u, 1073741824u, 536870912u, 268435456u,
    134217728u, 67108864u, 33554432u, 16777216u,
    8388608u, 4194304u, 2097152u, 1048576u,
    524288u, 262144u, 131072u, 65536u,
    32768u, 16384u, 8192u, 4096u,
    2048u, 1024u, 512u, 256u,
    128u, 64u, 32u, 16u,
    8u, 4u, 2u, 1u
};
static unsigned char primidxcounttab[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 2, 6, 3, 0, 0 };
static unsigned char primtypetab[16] = { 0, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 4, 0, 0 };

static unsigned char* s_polyinfo_base = 0;

/* ================================================================
 * STRUCT DEFINITIONS NEEDED BY FORWARD DECLARATIONS
 * ================================================================ */

/* Used by draw_line_related and sprite_draw_line_from_info.
 * Total size is 28 (28) bytes. */
struct LINEINFO {
	unsigned short reserved0;         /* offset 0 */
	unsigned short current_x;         /* offset 2 */
	unsigned short x_fraction;        /* offset 4 */
	unsigned short y_start;           /* offset 6 */
	unsigned short x_fraction_seed;   /* offset 8 */
	unsigned short y_end;             /* offset 10 */
	unsigned short x_step_fixed;      /* offset 12 */
	unsigned short pixel_count;       /* offset 14 */
	unsigned short line_color;        /* offset 16 */
	unsigned short draw_mode;         /* offset 18 */
	unsigned short clip_top_rows;     /* offset 20 */
	unsigned short reserved11;        /* offset 22 */
	unsigned short clip_bottom_rows;  /* offset 24 */
	unsigned short reserved13;        /* offset 26 */
};

/* ================================================================
 * EXTERN DECLARATIONS
 * ================================================================ */

/* --- projection data (reinterpreted as uint16_t via pointer macros) --- */

#define projectiondata1_raw  projectiondata1
#define projectiondata2_raw  projectiondata2
#define projectiondata3_raw  projectiondata3
#define projectiondata4_raw  projectiondata4
#define projectiondata5_raw  projectiondata5
#define projectiondata6_raw  projectiondata6
#define projectiondata7_raw  projectiondata7
#define projectiondata8_raw  projectiondata8
#define projectiondata9_raw  projectiondata9
#define projectiondata10_raw projectiondata10

#define projectiondata1  (*(uint16_t*)(void*)projectiondata1_raw)
#define projectiondata2  (*(uint16_t*)(void*)projectiondata2_raw)
#define projectiondata3  (*(uint16_t*)(void*)projectiondata3_raw)
#define projectiondata4  (*(uint16_t*)(void*)projectiondata4_raw)
#define projectiondata5  (*(uint16_t*)(void*)projectiondata5_raw)
#define projectiondata6  (*(uint16_t*)(void*)projectiondata6_raw)
#define projectiondata7  (*(uint16_t*)(void*)projectiondata7_raw)
#define projectiondata8  (*(uint16_t*)(void*)projectiondata8_raw)
#define projectiondata9  (*(uint16_t*)(void*)projectiondata9_raw)
#define projectiondata10 (*(uint16_t*)(void*)projectiondata10_raw)

/* --- rendering state --- */

/* --- sprite / draw functions --- */

/* --- sphere / track objects --- */
/* off_3F3C8: sphere LUT offset table (40 little-endian shorts from data_global.c) — private to shape3d */
static unsigned short off_3F3C8[40] = {
    15528, 15528, 15529, 15531, 15534, 15538, 15543, 15549,
    15556, 15564, 15573, 15583, 15594, 15606, 15619, 15633,
    15648, 15664, 15681, 15699, 15718, 15738, 15759, 15781,
    15804, 15828, 15853, 15879, 15906, 15934, 15963, 15993,
    16024, 16056, 16089, 16123, 16158, 16194, 16231, 16269
};

/* --- car shape data --- */
/* carshapevecs2/3/4 are contiguous within carshapevecs at offsets +6/+12/+18 */
#define carshapevecs2 (carshapevecs + 6)
#define carshapevecs3 (carshapevecs + 12)
#define carshapevecs4 (carshapevecs + 18)
/* oppcarshapevecs2/3/4 are contiguous within oppcarshapevecs at offsets +6/+12/+18 */
#define oppcarshapevecs2 (oppcarshapevecs + 6)
#define oppcarshapevecs3 (oppcarshapevecs + 12)
#define oppcarshapevecs4 (oppcarshapevecs + 18)

/* ================================================================
 * FORWARD DECLARATIONS OF INTERNAL FUNCTIONS
 * ================================================================ */
void preRender_sphere(int x, int y, int radius, int color);
void preRender_default(unsigned fill_color, unsigned vertex_line_count, unsigned *vertex_lines);
void preRender_default_alt(unsigned fill_color, unsigned vertex_line_count, unsigned *vertex_lines);
/**
 * @brief Render a polygon with a single-colour dither pattern.
 *
 * @param pattern_type       Pattern type selector.
 * @param fill_color         Fill colour index.
 * @param vertex_line_count  Number of vertex pairs.
 * @param vertex_lines       Vertex coordinate array.
 */
void preRender_patterned(unsigned pattern_type, unsigned fill_color, unsigned vertex_line_count, struct POINT2D *vertex_lines);
/**
 * @brief Render a polygon with a two-colour dither pattern (variant 1).
 *
 * @param pattern            Pattern type selector.
 * @param alt_color          Alternate colour index.
 * @param fill_color         Primary fill colour index.
 * @param vertex_line_count  Number of vertex pairs.
 * @param vertex_lines       Vertex coordinate array.
 */
void preRender_two_color_pattern_flag1(unsigned pattern, unsigned alt_color, unsigned fill_color, unsigned vertex_line_count, struct POINT2D *vertex_lines);
void preRender_wheel(int *input_data, int output_data, unsigned sidewall_color, unsigned rim_color, unsigned tread_color);
void preRender_default_impl(unsigned fill_color, unsigned vertex_line_count, int *vertex_lines, unsigned variant_flag);
void generate_poly_edges(int16_t *scanline_bounds, int *regsi, int mode);
void accumulate_scanline_bounds(int *regsi, unsigned unused_flag, unsigned apply_border_clip, int16_t *scanline_bounds);
void shape3d_update_car_wheel_vertices(struct VECTOR *wheel_vertices, int wheel_rotation_angle, short *wheel_compression_src, short *wheel_state_cache, struct VECTOR *wheel_vertex_offsets, struct VECTOR *wheel_center_points);
void shape3d_update_car_wheel_vertices_legacy(struct VECTOR *wheel_vertices, int wheel_rotation_angle, short *wheel_compression_src, short *wheel_state_cache, struct VECTOR *wheel_vertex_offsets, struct VECTOR *wheel_center_points);

#define SHAPE3D_TOTAL_SHAPES 130
#define SHAPE3D_TRACK_SHAPE_COUNT 116
#define SHAPE3D_NAME_LEN 5
#define SHAPE3D_RES_OFS_MIN_REQUIRED 65000

#define SHAPE3D_HEADER_SIZE_BYTES 4
#define SHAPE3D_VERTEX_SIZE_BYTES 6
#define SHAPE3D_CULL_ENTRY_SIZE_BYTES 4
#define SHAPE3D_PRIMITIVE_SIZE_BYTES 8

#define POLYINFO_MAX_POLYS 4096
#define POLYINFO_HEAD_INDEX POLYINFO_MAX_POLYS
#define POLYINFO_BUFFER_SIZE 131072u
#define POLYINFO_TRANSFORM_LIMIT (POLYINFO_BUFFER_SIZE - 46u)

#define ZORDER_LINK_END (-1)


#define VERTEX_CACHE_COUNT 256
#define VERTEX_FLAG_COUNT 256

#define ROTATION_ZERO_ARG 0
#define NEAR_PLANE_Z 12
#define VIEW_FORWARD_VECTOR_Z 4096
#define RECT_CLIP_FULL_MASK 15

#define VERTEX_DEPTH_FLAG_VISIBLE 0
#define VERTEX_DEPTH_FLAG_BEHIND_NEAR 1
#define VERTEX_DEPTH_FLAG_UNCACHED 255

#define TRANSFORM_FLAG_SKIP_VIEW_CULL 2
#define TRANSFORM_FLAG_UPDATE_RECT 8
#define TRANSFORM_FLAG_FORCE_UNSORTED 1

#define PRIMITIVE_TYPE_POLYGON 0
#define PRIMITIVE_TYPE_LINE 1
#define PRIMITIVE_TYPE_SPHERE 2
#define PRIMITIVE_TYPE_WHEEL 3
#define PRIMITIVE_TYPE_PIXEL 5

#define BACKLIGHTS_PAINT_ID 45

#define POLYINFO_ENTRY_HEADER_SIZE 6u
#define POLYINFO_LINE_ENTRY_SIZE 14u
#define POLYINFO_LINE_DEPTH_OFFSET 0u
#define POLYINFO_LINE_MATERIAL_OFFSET 2u
#define POLYINFO_LINE_VERTEX_COUNT_OFFSET 3u
#define POLYINFO_LINE_PRIMITIVE_TYPE_OFFSET 4u
#define POLYINFO_LINE_X0_OFFSET 6u
#define POLYINFO_LINE_Y0_OFFSET 8u
#define POLYINFO_LINE_X1_OFFSET 10u
#define POLYINFO_LINE_Y1_OFFSET 12u

#define POLY_DEPTH_MAX_SIGNED 32767u
#define POLYLIST_WALK_LIMIT 512u

#define PROJECTION_ANGLE_SHIFT 11
#define PROJECTION_ANGLE_SCALE_DIV 360
#define PROJECTION_ANGLE_HALF_SHIFT 1
#define PROJECTION_ANGLE_MASK 1023

#define SELECT_CLIP_VEC_Z 10000

#define SINCOS80_ANGLE 128
#define MAT_Y_ROT_100 256
#define MAT_Y_ROT_200 512
#define MAT_Y_ROT_300 768

#define WHEEL_VERTEX_CONTROL_ABS_MAX 2000
#define WHEEL_INTERP_Q14 9472
#define WHEEL_SCALE_OUTER_Q15 11585
#define WHEEL_SCALE_INNER_Q15 14654
#define WHEEL_POINTS_PER_RING 6
#define WHEEL_POINTS_BOTH_RINGS 12
#define WHEEL_COUNT 4
#define WHEEL_COMPRESSION_SHIFT 6

#define SPHERE_VERTEX_BUFFER_LINES 32
#define WHEEL_RIM_OUTLINE_LINECOUNT 18
#define WHEEL_TREAD_LINECOUNT 16
#define SPHERE_LUT_DSEG_OFFSET 259016
#define SPHERE_LARGE_RADIUS_MIN 40

#define WHEEL_RING_POINTS 16
#define WHEEL_SIDE_QUAD_COUNT 15

#define DRAW_HEALTH_EXTREME_COORD_LIMIT 4096

#define DRAW_STRICT_LOG_LIMIT 200u

#define INTERP_TABLE_COUNT 50
#define CAR_SHAPE_MIN_VERTS 24

#define MATERIAL_TYPE_INVALID_START 129
#define RENDER_COORD_ABS_LIMIT 32000

#define SCANLINE_HEIGHT 480

#define FIXED_SHIFT_16 16
#define FIXED_HALF_16 32768UL
#define FIXED_FRAC_MASK_16 65535UL
#define FIXED_ONE_16 65536

#define EDGE_STATE_WORD_COUNT 14

#define POLYLIST_PTR_SANITY_MIN ((uintptr_t)1048576u)

/*
 * Edge-state rows originate from 16-bit DOS intermediates. Keep the row-start
 * calculation in signed-16-bit domain before widening to host int.
 */
static inline int edge_state_start_row_from_count(const int* edge_state, int count)
{
    int start_row = (int)(int16_t)edge_state[3] - count;
    if ((int16_t)edge_state[2] < 0) {
        start_row++;
    }
    return start_row;
}

static inline unsigned short material_table_word_at(const unsigned char* table, unsigned index)
{
    unsigned offset = index * 2u;
    return (unsigned short)((unsigned short)table[offset] |
                            (unsigned short)((unsigned short)table[offset + 1u] << 8));
}

/**
 * @brief Return the depth key of a polygon at the given index.
 *
 * @param poly_index Index into the polyinfo pointer array.
 * @return 16-bit depth key used by z-order insertion.
 */
static unsigned short polyinfo_depth_at(int poly_index)
{
    return ((unsigned short*)(void*)polyinfoptrs[poly_index])[0];
}

char is_positive_winding_2d(struct POINT2D* pts);
/**
 * @brief Project a 3-D radius to screen pixels at a given depth.
 *
 * @param radius_3d  Radius in world units.
 * @param depth_z    Depth (z) value; must be positive.
 * @return Projected radius in pixels, or 0 if behind camera.
 */
unsigned project_radius_by_depth(unsigned radius_3d, int depth_z);
unsigned polyinfo_insert_sorted_by_depth(unsigned depth_key, unsigned search_mode);

/** @brief Load base 3-D shape resources and initialize track-object shapes.
 * @return 0 on success, 1 on memory/resource failure.
 */
int shape3d_load_all(void) {
    int i;
    unsigned long mmgrofsdiff;
    char* shapename;

    game1ptr = 0;
    game2ptr = 0;

    mmgrofsdiff = mmgr_get_res_ofs_diff_scaled();
    if (mmgrofsdiff < SHAPE3D_RES_OFS_MIN_REQUIRED) {
        return 1;
    }

    game1ptr = file_load_3dres("game1");
    game2ptr = file_load_3dres("game2");

    for (i = 0; i < SHAPE3D_TRACK_SHAPE_COUNT; i++) {
        shapename = &track_object_shape_names[i * SHAPE3D_NAME_LEN];
        curshapeptr = locate_shape_nofatal(game1ptr, shapename);
        if (curshapeptr == 0) {
            curshapeptr = locate_shape_fatal(game2ptr, shapename);
        }
        shape3d_init_shape(curshapeptr, &game3dshapes[i]);
    }

    return 0;
}

/**
 * @brief Free all loaded 3-D shape resources.
 */
void shape3d_free_all(void) {
    if (game1ptr != 0) {
        mmgr_free(game1ptr);
    }
    if (game2ptr != 0) {
        mmgr_free(game2ptr);
    }
}

/**
 * @brief Initialise a SHAPE3D descriptor from a raw resource pointer.
 *
 * Parses the SHAPE3DHEADER and sets vertex, cull, and primitive pointers.
 *
 * @param shapeptr   Pointer to the raw shape data.
 * @param gameshape  Output SHAPE3D structure.
 */
void shape3d_init_shape(char* shapeptr, struct SHAPE3D* gameshape) {
    struct SHAPE3DHEADER* hdr = (struct SHAPE3DHEADER*)shapeptr;

    gameshape->shape3d_numverts = hdr->header_numverts;
    gameshape->shape3d_numprimitives = hdr->header_numprimitives;
    gameshape->shape3d_numpaints = hdr->header_numpaints;
    gameshape->shape3d_verts = (struct VECTOR*)(shapeptr + SHAPE3D_HEADER_SIZE_BYTES);
    gameshape->shape3d_cull1 = shapeptr + hdr->header_numverts * SHAPE3D_VERTEX_SIZE_BYTES + SHAPE3D_HEADER_SIZE_BYTES;
    gameshape->shape3d_cull2 = shapeptr + hdr->header_numprimitives * SHAPE3D_CULL_ENTRY_SIZE_BYTES + hdr->header_numverts * SHAPE3D_VERTEX_SIZE_BYTES + SHAPE3D_HEADER_SIZE_BYTES;
    gameshape->shape3d_primitives = shapeptr + hdr->header_numprimitives * SHAPE3D_PRIMITIVE_SIZE_BYTES + hdr->header_numverts * SHAPE3D_VERTEX_SIZE_BYTES + SHAPE3D_HEADER_SIZE_BYTES;
}

/**
 * @brief Append a polygon to the z-order list without sorting.
 *
 * @param index  Polygon index.
 */
static void __attribute__((unused)) polyinfo_append_unsorted(unsigned index) {
    unsigned cursor;

    if (index >= POLYINFO_MAX_POLYS) {
        return;
    }

    if (polyinfonumpolys == 0) {
        zorder_shape_list[POLYINFO_HEAD_INDEX] = index;
        zorder_shape_list[index] = ZORDER_LINK_END;
        return;
    }

    cursor = POLYINFO_HEAD_INDEX;
    while (zorder_shape_list[cursor] >= 0) {
        cursor = zorder_shape_list[cursor];
        if (cursor >= POLYINFO_MAX_POLYS) {
            break;
        }
    }

    if (cursor < POLYINFO_MAX_POLYS) {
        zorder_shape_list[cursor] = index;
        zorder_shape_list[index] = ZORDER_LINK_END;
    }
}

/**
 * @brief Render a pre-transformed 3-D shape into the polygon info buffer.
 *
 * Performs back-face culling, near-plane clipping, projection, and
 * depth-sorted insertion for each primitive of the shape.
 *
 * @param transformed_shape  Transformed shape with position, rotation, and clip data.
 * @return Number of accepted primitives, or 0 if shape is fully culled.
 */
unsigned shape3d_render_transformed(struct TRANSFORMEDSHAPE3D* transformed_shape) {
    uint32_t* primitive_cull_table;
    uint32_t* paint_cull_table;
    unsigned char vertex_depth_flags[VERTEX_FLAG_COUNT];
    struct MATRIX* rotation_matrix_ptr;
    struct MATRIX inverse_model_view_matrix;
    struct MATRIX model_view_matrix;
    struct VECTOR shape_position_view;
    struct VECTOR scratch_vector_a;
    struct VECTOR scratch_vector_b;
    struct VECTOR scratch_vector_c;
    uint32_t direction_cull_mask_primary;
    uint32_t paint_cull_mask;
    unsigned accepted_primitive_count, all_vertices_behind_near_plane, has_near_plane_vertex;
    unsigned char rect_clip_mask, primitive_render_type;
    struct VECTOR cached_view_vertices[VERTEX_CACHE_COUNT];
    unsigned primitive_flags, primitive_file_type, primitive_accept_count, polygon_vertex_counter, current_vertex_index, previous_vertex_index, unused_primitive_counter, projected_radius;
    int polygon_vertex_x, polygon_vertex_y;
    struct POINT2D* polyinfo_point_write_ptr;
    long depth_sum = 0;
    struct POINT2D clipped_intersection_point, rect_expand_point;
    struct POINT2D cached_projected_points[VERTEX_CACHE_COUNT];
    struct POINT2D** poly_point_slot_ptr;
    struct POINT2D* polygon_points_ptr;

    unsigned i;
    unsigned temp, temp0, temp1;
    unsigned reject_prim_culltable = 0;
    unsigned reject_prim_all_behind = 0;
    unsigned reject_prim_rect_stage1 = 0;
    unsigned reject_prim_rect_stage2 = 0;
    unsigned reject_prim_winding = 0;
    unsigned reject_prim_numverts_zero = 0;

    unsigned transshapenumverts;
    unsigned char* transshapeprimitives;
    struct VECTOR* transshapeverts;
    unsigned char transshapenumpaints;
    unsigned char transshapematerial;
    unsigned char transshapeflags;
    struct RECTANGLE* transshaperectptr = 0;
    unsigned char* transshapeprimptr;
    unsigned char* transshapeprimindexptr;
    unsigned char* transshapepolyinfo;
    struct POINT2D* transshapepolyinfopts;
    unsigned char transprimitivepaintjob;
    struct POINT2D* polyvertpointptrtab[VERTEX_FLAG_COUNT];

    (void)unused_primitive_counter;
    if (transformed_shape == 0 || transformed_shape->shapeptr == 0) {
        return 1;
    }
    if (polygon_op_error_code != 0) {
        return 1;
    }
    if (s_polyinfo_base == 0) {
        return 1;
    }

    transshapenumverts = transformed_shape->shapeptr->shape3d_numverts;
    transshapeprimitives = (unsigned char*)transformed_shape->shapeptr->shape3d_primitives;
    transshapeverts = transformed_shape->shapeptr->shape3d_verts;
    transshapenumpaints = (unsigned char)transformed_shape->shapeptr->shape3d_numpaints;
    primitive_cull_table = (uint32_t*)transformed_shape->shapeptr->shape3d_cull1;
    paint_cull_table = (uint32_t*)transformed_shape->shapeptr->shape3d_cull2;
    transshapematerial = transformed_shape->material;
    if (transshapematerial >= transshapenumpaints) {
        transshapematerial = 0;
    }
    transshapeflags = transformed_shape->ts_flags;

    if ((transshapeflags & TRANSFORM_FLAG_UPDATE_RECT) != 0) {
        transshaperectptr = transformed_shape->rectptr;
    }

    memset(cached_view_vertices, 0, sizeof(cached_view_vertices));
    memset(cached_projected_points, 0, sizeof(cached_projected_points));
    memset(vertex_depth_flags, VERTEX_DEPTH_FLAG_UNCACHED, sizeof(vertex_depth_flags));

    for (i = 0; i < VERTEX_CACHE_COUNT; i++) {
        polyvertpointptrtab[i] = &cached_projected_points[i];
    }

    if ((transshapeflags & TRANSFORM_FLAG_SKIP_VIEW_CULL) == 0) {
        rotation_matrix_ptr = mat_rot_zxy(transformed_shape->rotvec.x,
                                    transformed_shape->rotvec.y,
                                    transformed_shape->rotvec.z,
                                    ROTATION_ZERO_ARG);
        mat_mul_vector(&transformed_shape->pos, &mat_temp, &shape_position_view);
        mat_multiply(rotation_matrix_ptr, &mat_temp, &model_view_matrix);
        mat_invert(&model_view_matrix, &inverse_model_view_matrix);
        scratch_vector_a.x = 0;
        scratch_vector_a.y = 0;
        scratch_vector_a.z = VIEW_FORWARD_VECTOR_Z;
        mat_mul_vector(&scratch_vector_a, &inverse_model_view_matrix, &scratch_vector_b);
        if ((scratch_vector_b.y <= 0 || transformed_shape->pos.y >= 0) &&
            ((transformed_shape->shape_visibility_threshold * 2) <= abs(shape_position_view.x) ||
             (transformed_shape->shape_visibility_threshold * 2) <= abs(shape_position_view.z))) {
            shape3d_vector_direction_bucket = vector_direction_bucket32(&scratch_vector_b);
            direction_cull_mask_primary = invpow2tbl[shape3d_vector_direction_bucket];
            paint_cull_mask = invpow2tbl[shape3d_vector_direction_bucket];
        } else {
            direction_cull_mask_primary = (uint32_t)-1;
            paint_cull_mask = 0;
        }
    } else {
        rotation_matrix_ptr = mat_rot_zxy(transformed_shape->rotvec.x,
                                    transformed_shape->rotvec.y,
                                    transformed_shape->rotvec.z,
                                    ROTATION_ZERO_ARG);
        mat_multiply(rotation_matrix_ptr, &mat_temp, &model_view_matrix);
        shape_position_view = transformed_shape->pos;
        direction_cull_mask_primary = (uint32_t)-1;
        paint_cull_mask = 0;
    }

    zorder_next_link = polygon_info_head;
    zorder_current_index = polygon_info_head;
    zorder_tail_counter = 0;
    accepted_primitive_count = 0;

    if (transshapenumverts <= 8) {
        transshapenumvertscopy = (unsigned char)transshapenumverts;
    } else {
        transshapenumvertscopy = 8;
    }

    if (transshapenumvertscopy > 4 && transshapeverts[0].y == transshapeverts[4].y) {
        transshapenumvertscopy = 4;
    }

    goto label_init_vertex_visibility_scan;

label_init_vertex_visibility_scan:
    rect_clip_mask = RECT_CLIP_FULL_MASK;
    all_vertices_behind_near_plane = 1;
    has_near_plane_vertex = 0;
    i = 0;
    goto label_vertex_scan_loop;

label_vertex_scan_increment:
    i++;
label_vertex_scan_loop:
    if (transshapenumvertscopy > i) goto label_transform_and_project_vertex;
    if ((all_vertices_behind_near_plane != 0 || transformed_shape->shape_visibility_threshold < abs(shape_position_view.x))
        && (transshapeflags & TRANSFORM_FLAG_SKIP_VIEW_CULL) == 0) {
        return -1;
    }
    goto label_begin_primitive_processing;

label_transform_and_project_vertex:
    polyvertpointptrtab[i] = &cached_projected_points[i];
    scratch_vector_a = transshapeverts[i];
    if (select_rect_scale_preview != 0) {
        scratch_vector_a.x /= 2;
        scratch_vector_a.y /= 2;
        scratch_vector_a.z /= 2;
    }
    mat_mul_vector(&scratch_vector_a, &model_view_matrix, &scratch_vector_b);
    scratch_vector_b.x += shape_position_view.x;
    scratch_vector_b.y += shape_position_view.y;
    scratch_vector_b.z += shape_position_view.z;
    cached_view_vertices[i] = scratch_vector_b;
    if (scratch_vector_b.z < NEAR_PLANE_Z) {
        vertex_depth_flags[i] = VERTEX_DEPTH_FLAG_BEHIND_NEAR;
        has_near_plane_vertex = 1;
        goto label_vertex_scan_increment;
    }
    all_vertices_behind_near_plane = 0;
    vertex_depth_flags[i] = VERTEX_DEPTH_FLAG_VISIBLE;
    vector_to_point(&scratch_vector_b, polyvertpointptrtab[i]);
    if (rect_clip_mask != 0) {
        unsigned char rect_mask = rect_compare_point(polyvertpointptrtab[i]);
        rect_clip_mask &= rect_mask;
    }
    if (rect_clip_mask != 0) {
        goto label_vertex_scan_increment;
    }
    goto label_begin_primitive_processing;

label_begin_primitive_processing:
    transshapeprimitives = (unsigned char*)transformed_shape->shapeptr->shape3d_primitives;

label_primitive_loop_next:
    transshapeprimptr = transshapeprimitives + primidxcounttab[transshapeprimitives[0]] +
                        transshapenumpaints + 2;
    primitive_flags = transshapeprimitives[1];
    primitive_accept_count = 0;
    if ((primitive_cull_table[0] & direction_cull_mask_primary) != 0) {
        goto label_decode_primitive_header;
    }
    reject_prim_culltable++;
    goto label_finish_current_primitive;

label_decode_primitive_header:
    primitive_file_type = transshapeprimitives[0];
    transshapenumvertscopy = primidxcounttab[primitive_file_type];
    primitive_render_type = primtypetab[primitive_file_type];

    transshapepolyinfo = s_polyinfo_base + polyinfoptrnext;
    polyinfoptrs[polyinfonumpolys] = (int*)transshapepolyinfo;

    transprimitivepaintjob = transshapeprimitives[2 + transshapematerial];
    transshapeprimitives += 2 + transshapenumpaints;

    rect_clip_mask = RECT_CLIP_FULL_MASK;
    all_vertices_behind_near_plane = 1;
    has_near_plane_vertex = 0;
    transshapeprimindexptr = transshapeprimitives;
    polygon_vertex_counter = 0;
    goto label_polygon_vertex_loop;

label_polygon_vertex_visible:
    all_vertices_behind_near_plane = 0;
label_polygon_vertex_rect_clip_test:
    if (rect_clip_mask != 0) {
        rect_clip_mask &= rect_compare_point(polyvertpointptrtab[polygon_vertex_counter]);
    }
label_polygon_vertex_advance:
    polygon_vertex_counter++;
label_polygon_vertex_loop:
    if (polygon_vertex_counter >= transshapenumvertscopy) goto label_dispatch_primitive_render_path;

    temp = transshapeprimindexptr[0];
    transshapeprimindexptr++;
    polyvertpointptrtab[polygon_vertex_counter] = &cached_projected_points[temp];
    if (vertex_depth_flags[temp] == VERTEX_DEPTH_FLAG_UNCACHED) {
        goto label_project_uncached_vertex;
    }
    if (vertex_depth_flags[temp] == VERTEX_DEPTH_FLAG_VISIBLE) {
        goto label_polygon_vertex_visible;
    }
    if (vertex_depth_flags[temp] == VERTEX_DEPTH_FLAG_BEHIND_NEAR) {
        has_near_plane_vertex = 1;
        goto label_polygon_vertex_advance;
    }
    goto label_polygon_vertex_advance;

label_project_uncached_vertex:
    scratch_vector_a = transshapeverts[temp];
    if (select_rect_scale_preview != 0) {
        scratch_vector_a.x /= 2;
        scratch_vector_a.y /= 2;
        scratch_vector_a.z /= 2;
    }
    mat_mul_vector(&scratch_vector_a, &model_view_matrix, &scratch_vector_b);
    scratch_vector_b.x += shape_position_view.x;
    scratch_vector_b.y += shape_position_view.y;
    scratch_vector_b.z += shape_position_view.z;
    cached_view_vertices[temp] = scratch_vector_b;

    if (scratch_vector_b.z >= NEAR_PLANE_Z) {
        all_vertices_behind_near_plane = 0;
        vertex_depth_flags[temp] = VERTEX_DEPTH_FLAG_VISIBLE;
        vector_to_point(&scratch_vector_b, polyvertpointptrtab[polygon_vertex_counter]);
        goto label_polygon_vertex_rect_clip_test;
    }
    vertex_depth_flags[temp] = VERTEX_DEPTH_FLAG_BEHIND_NEAR;
    has_near_plane_vertex = 1;
    goto label_polygon_vertex_advance;

label_dispatch_primitive_render_path:
    if (all_vertices_behind_near_plane != 0) {
        reject_prim_all_behind++;
        goto label_finish_current_primitive;
    }
    if (rect_clip_mask != 0 && has_near_plane_vertex == 0) {
        reject_prim_rect_stage1++;
        goto label_finish_current_primitive;
    }
    if (primitive_render_type == PRIMITIVE_TYPE_POLYGON) goto _primtype_poly;
    if (primitive_render_type == PRIMITIVE_TYPE_LINE) goto _primtype_line;
    if (primitive_render_type == PRIMITIVE_TYPE_SPHERE) goto _primtype_sphere;
    if (primitive_render_type == PRIMITIVE_TYPE_WHEEL) goto _primtype_wheel;
    if (primitive_render_type == PRIMITIVE_TYPE_PIXEL) goto label_handle_unsupported_primitive_type5;
    goto label_finish_current_primitive;

_primtype_poly:
    polyinfo_point_write_ptr = (struct POINT2D*)(transshapepolyinfo + POLYINFO_ENTRY_HEADER_SIZE);
    transshapeprimindexptr = transshapeprimitives;

    depth_sum = 0;
    rect_clip_mask = RECT_CLIP_FULL_MASK;

    if (has_near_plane_vertex != 0) goto label_clip_polygon_against_near_plane;
    i = 0;
    goto label_copy_polygon_vertex_loop;
label_copy_polygon_vertex_advance:
    i++;
label_copy_polygon_vertex_loop:
    if (transshapenumvertscopy <= i) goto label_prepare_primitive_submission;
    current_vertex_index = transshapeprimindexptr[0];
    transshapeprimindexptr++;
    depth_sum += cached_view_vertices[current_vertex_index].z;
    poly_point_slot_ptr = &polyvertpointptrtab[i];
    *polyinfo_point_write_ptr = **poly_point_slot_ptr;
    if (rect_clip_mask != 0) {
        rect_clip_mask &= rect_compare_point(*poly_point_slot_ptr);
    }
    polyinfo_point_write_ptr++;
    goto label_copy_polygon_vertex_advance;

label_clip_polygon_against_near_plane:
    polygon_vertex_counter = 0;
    previous_vertex_index = transshapeprimitives[transshapenumvertscopy - 1];
    i = 0;
    goto label_clip_edge_loop;

label_clip_edge_step_start:
    if (vertex_depth_flags[previous_vertex_index] != 0) goto label_clip_edge_advance_indices;

    vector_lerp_at_z(&cached_view_vertices[previous_vertex_index], &cached_view_vertices[current_vertex_index], &scratch_vector_a, NEAR_PLANE_Z);
    vector_to_point(&scratch_vector_a, &clipped_intersection_point);

    if (rect_clip_mask != 0) {
        rect_clip_mask &= rect_compare_point(&clipped_intersection_point);
    }

    *polyinfo_point_write_ptr = clipped_intersection_point;

label_clip_emit_vertex_advance:
    polyinfo_point_write_ptr++;
    polygon_vertex_counter++;

label_clip_edge_advance_indices:
    previous_vertex_index = current_vertex_index;
    i++;
label_clip_edge_loop:
    if (transshapenumvertscopy <= i) goto label_finalize_polygon_vertex_count;
    current_vertex_index = transshapeprimindexptr[0];
    transshapeprimindexptr++;

    depth_sum += cached_view_vertices[current_vertex_index].z;

    if (vertex_depth_flags[current_vertex_index] != 0) goto label_clip_edge_step_start;

    if (vertex_depth_flags[previous_vertex_index] == 0) goto label_clip_emit_current_vertex;

    vector_lerp_at_z(&cached_view_vertices[current_vertex_index], &cached_view_vertices[previous_vertex_index], &scratch_vector_a, NEAR_PLANE_Z);
    vector_to_point(&scratch_vector_a, &clipped_intersection_point);

    if (rect_clip_mask != 0) {
        rect_clip_mask &= rect_compare_point(&clipped_intersection_point);
    }

    *polyinfo_point_write_ptr = clipped_intersection_point;
    polyinfo_point_write_ptr++;
    polygon_vertex_counter++;

label_clip_emit_current_vertex:
    *polyinfo_point_write_ptr = *polyvertpointptrtab[i];
    if (rect_clip_mask != 0) {
        rect_clip_mask &= rect_compare_point(polyvertpointptrtab[i]);
    }
    goto label_clip_emit_vertex_advance;

label_finalize_polygon_vertex_count:
    transshapenumvertscopy = (unsigned char)polygon_vertex_counter;

label_prepare_primitive_submission:
    if (transshapenumvertscopy == 0) {
        reject_prim_numverts_zero++;
        goto label_finish_current_primitive;
    }
    if (rect_clip_mask != 0) {
        reject_prim_rect_stage2++;
        goto label_finish_current_primitive;
    }
    if ((primitive_flags & 1) != 0) goto label_mark_primitive_accepted;
    if ((paint_cull_mask & *paint_cull_table) != 0) goto label_mark_primitive_accepted;

    if ((transshapeflags & TRANSFORM_FLAG_TERRAIN_DOUBLE_SIDED) == 0) {
        if (is_positive_winding_2d((struct POINT2D*)(transshapepolyinfo + POLYINFO_ENTRY_HEADER_SIZE)) == 0) {
            reject_prim_winding++;
            goto label_optional_rect_update_begin;
        }
    }

label_mark_primitive_accepted:
    primitive_accept_count++;

label_optional_rect_update_begin:
    if (primitive_accept_count == 0) goto label_finish_current_primitive;
    if ((transshapeflags & TRANSFORM_FLAG_UPDATE_RECT) == 0) goto label_finish_current_primitive;

    polygon_points_ptr = (struct POINT2D*)(transshapepolyinfo + POLYINFO_ENTRY_HEADER_SIZE);
    polygon_vertex_counter = 0;
    goto label_rect_update_loop_check;

label_rect_update_vertex_loop:
    polygon_vertex_x = polygon_points_ptr->px;
    polygon_vertex_y = polygon_points_ptr->py;
    polygon_points_ptr++;

    if (polygon_vertex_x < transshaperectptr->left) {
        transshaperectptr->left = polygon_vertex_x;
    }
    if (transshaperectptr->right < polygon_vertex_x + 1) {
        transshaperectptr->right = polygon_vertex_x + 1;
    }
    if (transshaperectptr->top > polygon_vertex_y) {
        transshaperectptr->top = polygon_vertex_y;
    }
    if (transshaperectptr->bottom < polygon_vertex_y + 1) {
        transshaperectptr->bottom = polygon_vertex_y + 1;
    }

    polygon_vertex_counter++;

label_rect_update_loop_check:
    if (polygon_vertex_counter < transshapenumvertscopy) goto label_rect_update_vertex_loop;

label_finish_current_primitive:
    transshapeprimitives = transshapeprimptr;
    primitive_cull_table++;
    paint_cull_table++;
    if (primitive_accept_count != 0) goto label_store_polyinfo_entry;
    if ((primitive_flags & 2) != 0) goto label_primitive_loop_exit_or_continue;
label_skip_hidden_primitive_chain:
    if ((transshapeprimitives[1] & 2) == 0) goto label_primitive_loop_exit_or_continue;
    transshapeprimitives += primidxcounttab[transshapeprimitives[0]] + transshapenumpaints + 2;
    primitive_cull_table++;
    paint_cull_table++;
    goto label_skip_hidden_primitive_chain;

_primtype_line:
    temp0 = transshapeprimitives[0];
    temp1 = transshapeprimitives[1];
    if (vertex_depth_flags[temp0] + vertex_depth_flags[temp1] == 2) {
        goto label_finish_current_primitive;
    }
    if (vertex_depth_flags[temp0] == 0) {
        goto label_prepare_line_or_sphere_clip;
    }
    vector_lerp_at_z(&cached_view_vertices[temp1], &cached_view_vertices[temp0], &scratch_vector_a, NEAR_PLANE_Z);
    temp = temp0;
    goto label_store_line_clip_point;

label_prepare_line_or_sphere_clip:
    if (vertex_depth_flags[temp1] == 0) goto label_emit_line_primitive;
    vector_lerp_at_z(&cached_view_vertices[temp0], &cached_view_vertices[temp1], &scratch_vector_a, NEAR_PLANE_Z);
    temp = temp1;

label_store_line_clip_point:
    vector_to_point(&scratch_vector_a, &cached_projected_points[temp]);

label_emit_line_primitive:
    depth_sum = cached_view_vertices[temp0].z + cached_view_vertices[temp1].z;
    transshapepolyinfopts = (struct POINT2D*)(transshapepolyinfo + POLYINFO_ENTRY_HEADER_SIZE);
    transshapepolyinfopts[0] = *polyvertpointptrtab[0];
    transshapepolyinfopts[1] = *polyvertpointptrtab[1];

    if ((transshapeflags & TRANSFORM_FLAG_UPDATE_RECT) == 0) goto label_emit_line_polyinfo_basic;
    rect_adjust_from_point(polyvertpointptrtab[0], transshaperectptr);
    rect_adjust_from_point(polyvertpointptrtab[1], transshaperectptr);

label_emit_line_polyinfo_basic:
    transshapenumvertscopy = 2;

label_emit_line_done:
    primitive_accept_count++;
    goto label_finish_current_primitive;

_primtype_wheel:
    if (has_near_plane_vertex != 0) goto label_finish_current_primitive;

    transshapepolyinfopts = (struct POINT2D*)(transshapepolyinfo + POLYINFO_ENTRY_HEADER_SIZE);
    transshapepolyinfopts[0] = *polyvertpointptrtab[0];
    transshapepolyinfopts[1] = *polyvertpointptrtab[1];
    transshapepolyinfopts[2] = *polyvertpointptrtab[2];
    transshapepolyinfopts[3] = *polyvertpointptrtab[3];

    /* Viewport bounds check for wheel control points.
       On DOS (16-bit int), extreme projections wrapped naturally.
       On 32-bit, they stay large and blow up polarRadius2D → huge wheel.
       Reject the wheel if any control point is far outside the screen. */
    {
        int _whi;
        for (_whi = 0; _whi < 4; _whi++) {
            int _wax = transshapepolyinfopts[_whi].px;
            int _way = transshapepolyinfopts[_whi].py;
            if (_wax < 0) _wax = -_wax;
            if (_way < 0) _way = -_way;
            if (_wax > WHEEL_VERTEX_CONTROL_ABS_MAX || _way > WHEEL_VERTEX_CONTROL_ABS_MAX)
                goto label_finish_current_primitive;
        }
    }

    if (is_positive_winding_2d(transshapepolyinfopts) != 0) goto label_emit_sphere_depth_seed;

    transshapepolyinfopts[0] = *polyvertpointptrtab[3];
    transshapepolyinfopts[1] = *polyvertpointptrtab[4];
    transshapepolyinfopts[2] = *polyvertpointptrtab[5];
    transshapepolyinfopts[3] = *polyvertpointptrtab[0];

    depth_sum = cached_view_vertices[transshapeprimitives[3]].z << 2;
    goto label_emit_sphere_radius_compute;

label_emit_sphere_depth_seed:
    depth_sum = cached_view_vertices[transshapeprimitives[0]].z << 2;

label_emit_sphere_radius_compute:
    transshapepolyinfopts = (struct POINT2D*)(transshapepolyinfo + POLYINFO_ENTRY_HEADER_SIZE);
    temp = polarRadius2D(transshapepolyinfopts[0].px - transshapepolyinfopts[1].px,
                         transshapepolyinfopts[0].py - transshapepolyinfopts[1].py);
    temp1 = polarRadius2D(transshapepolyinfopts[0].px - transshapepolyinfopts[2].px,
                          transshapepolyinfopts[0].py - transshapepolyinfopts[2].py);

    if (temp1 > temp) temp = temp1;

    if ((transshapeflags & TRANSFORM_FLAG_UPDATE_RECT) == 0) goto label_emit_sphere_polyinfo;

    rect_expand_point.px = transshapepolyinfopts[0].px - (int)temp - 1;
    rect_expand_point.py = transshapepolyinfopts[0].py - (int)temp - 1;
    rect_adjust_from_point(&rect_expand_point, transshaperectptr);

    rect_expand_point.px = transshapepolyinfopts[0].px + (int)temp + 1;
    rect_expand_point.py = transshapepolyinfopts[0].py + (int)temp + 1;
    rect_adjust_from_point(&rect_expand_point, transshaperectptr);

    rect_expand_point.px = transshapepolyinfopts[3].px - (int)temp - 1;
    rect_expand_point.py = transshapepolyinfopts[3].py - (int)temp - 1;
    rect_adjust_from_point(&rect_expand_point, transshaperectptr);

    rect_expand_point.px = transshapepolyinfopts[3].px + (int)temp + 1;
    rect_expand_point.py = transshapepolyinfopts[3].py + (int)temp + 1;
    rect_adjust_from_point(&rect_expand_point, transshaperectptr);

label_emit_sphere_polyinfo:
    transshapenumvertscopy = 4;
    primitive_accept_count = 1;
    goto label_finish_current_primitive;

_primtype_sphere:
    temp0 = transshapeprimitives[0];
    temp1 = transshapeprimitives[1];
    depth_sum = cached_view_vertices[temp0].z + cached_view_vertices[temp1].z;
    if (vertex_depth_flags[temp0] + vertex_depth_flags[temp1] != 0) goto label_finish_current_primitive;

    transshapepolyinfopts = (struct POINT2D*)(transshapepolyinfo + POLYINFO_ENTRY_HEADER_SIZE);
    transshapepolyinfopts[0] = *polyvertpointptrtab[0];
    scratch_vector_b = cached_view_vertices[temp0];
    scratch_vector_c = cached_view_vertices[temp1];

    scratch_vector_a.x = scratch_vector_b.x - scratch_vector_c.x;
    scratch_vector_a.y = scratch_vector_b.y - scratch_vector_c.y;
    scratch_vector_a.z = scratch_vector_b.z - scratch_vector_c.z;
    projected_radius = project_radius_by_depth(polarRadius3D(&scratch_vector_a), scratch_vector_b.z);
    transshapepolyinfopts[1].px = projected_radius;
    if ((transshapeflags & TRANSFORM_FLAG_UPDATE_RECT) == 0) goto label_emit_line_polyinfo_basic;

    rect_expand_point.py = polyvertpointptrtab[0]->py - (int)projected_radius;
    rect_expand_point.px = polyvertpointptrtab[0]->px - (int)projected_radius;
    rect_adjust_from_point(&rect_expand_point, transshaperectptr);

    rect_expand_point.py = polyvertpointptrtab[0]->py + (int)projected_radius;
    rect_expand_point.px = polyvertpointptrtab[0]->px + (int)projected_radius;
    rect_adjust_from_point(&rect_expand_point, transshaperectptr);
    goto label_emit_line_polyinfo_basic;

label_handle_unsupported_primitive_type5:
    fatal_error("unhandled primitive type 5");
    goto label_emit_line_done;

label_store_polyinfo_entry:
    accepted_primitive_count++;
    transshapepolyinfo[3] = transshapenumvertscopy;
    transshapepolyinfo[POLYINFO_LINE_PRIMITIVE_TYPE_OFFSET] = primitive_render_type;
    if (transprimitivepaintjob == BACKLIGHTS_PAINT_ID) {
        transshapepolyinfo[2] = backlights_paint_override;
    } else {
        transshapepolyinfo[2] = transprimitivepaintjob;
    }

    if (transshapenumvertscopy == 1) {
        temp0 = (unsigned)depth_sum;
    } else if (transshapenumvertscopy == 2) {
        temp0 = (unsigned)(depth_sum >> 1);
    } else if (transshapenumvertscopy == 4) {
        temp0 = (unsigned)(depth_sum >> 2);
    } else if (transshapenumvertscopy == 8) {
        temp0 = (unsigned)(depth_sum >> 3);
    } else {
        temp0 = (unsigned)(depth_sum / transshapenumvertscopy);
    }

    if (temp0 > POLY_DEPTH_MAX_SIGNED) {
        temp0 = POLY_DEPTH_MAX_SIGNED;
    }

    if (has_near_plane_vertex != 0) {
        if ((short)temp0 < NEAR_PLANE_Z) {
            temp0 = NEAR_PLANE_Z;
        }
    } else if ((short)temp0 < 0) {
        goto label_finish_current_primitive;
    }

    ((unsigned short*)transshapepolyinfo)[0] = (unsigned short)temp0;

    if ((transshapeflags & TRANSFORM_FLAG_FORCE_UNSORTED) != 0 || (primitive_flags & 2) != 0) {
        temp = 0;
    } else {
        temp = 1;
    }

    polygon_op_error_code = polyinfo_insert_sorted_by_depth(temp0, temp);
    if (polygon_op_error_code != 0) {
        return 1;
    }

label_primitive_loop_exit_or_continue:
    if (transshapeprimitives[0] != 0) goto label_primitive_loop_next;
    if (accepted_primitive_count != 0) return 0;
    return -1;
}

/** @brief Check whether three projected points form a positive winding.
 * @param pts Pointer to at least three 2-D points.
 * @return 1 for positive winding, 0 otherwise.
 */
char is_positive_winding_2d(struct POINT2D * pts) {
	long dx0, dy0, dx1, dy1;
	long temp;

	dx0 = (long)pts[0].px - pts[1].px;
	dx1 = (long)pts[2].px - pts[1].px;
	
	if (dx0 == 0 && dx1 == 0) return 0;
		
	dy0 = (long)pts[0].py - pts[1].py;
	dy1 = (long)pts[2].py - pts[1].py;

	if (dy0 == 0 && dy1 == 0) return 0;
    temp = (dx1 * dy0) - (dx0 * dy1);
	return temp <= 0 ? 0 : 1;
}

/**
 * @brief Apply projection parameters from viewport half-widths.
 *
 * @param i3  Horizontal viewport half-size.
 * @param i4  Vertical viewport half-size.
 */
static void projection_apply(unsigned short i3, unsigned short i4) {
    projectiondata3 = i3 >> 1;
    projectiondata5 = projectiondata3 + projectiondata4;
    projectiondata6 = i4 >> 1;
    projectiondata8 = projectiondata6 + projectiondata7;
    projectiondata9 = (long)cos_fast(projectiondata1) * projectiondata3 / sin_fast(projectiondata1);

    if (projectiondata2 != 0) {
        projectiondata10 = (long)cos_fast(projectiondata2) * projectiondata6 / sin_fast(projectiondata2);
    } else {
        projectiondata10 = projectiondata9 - (projectiondata9 >> 3) - (projectiondata9 >> 4);
        projectiondata2 = polarAngle(projectiondata10, projectiondata6);
    }
}

/** @brief Project a world-space radius to screen-space using current projection scale.
 * @param radius_3d Radius in world units.
 * @param depth_z Positive depth value.
 * @return Projected radius in pixels, or 0 when depth is non-positive.
 */
unsigned project_radius_by_depth(unsigned radius_3d, int depth_z) {
    unsigned long numer;

    if (depth_z <= 0) {
        return 0;
    }

    numer = (unsigned long)projectiondata9 * (unsigned long)radius_3d;
    return (unsigned)(numer / (unsigned long)depth_z);
}

/**
 * @brief Insert a polygon into the depth-sorted z-order linked list.
 *
 * @param depth_key_input  Depth value used as the sort key.
 * @param search_mode      0 = restart from current index, 1 = continue from last position.
 * @return 0 on success, 1 if the polygon buffer is full.
 */
unsigned polyinfo_insert_sorted_by_depth(unsigned depth_key_input, unsigned search_mode) {
	int regdi, regsi, regax;
    unsigned short depth_key;
    unsigned short depth_cur;

    depth_key = (unsigned short)depth_key_input;

    if (polyinfonumpolys >= POLYINFO_MAX_POLYS) {
        return 1;
    }
    if (polyinfoptrnext > POLYINFO_TRANSFORM_LIMIT) {
        return 1;
    }
    if (zorder_current_index > POLYINFO_HEAD_INDEX) {
        zorder_current_index = POLYINFO_HEAD_INDEX;
    }

    //return polyinfo_insert_sorted_by_depth_(depth_key_input, search_mode);

    if (search_mode == 0) {
		regdi = zorder_shape_list[zorder_current_index];
        if (regdi >= POLYINFO_MAX_POLYS) {
            regdi = -1;
        }
	} else {
		zorder_current_index = zorder_next_link;
        if (zorder_current_index > POLYINFO_HEAD_INDEX) {
            zorder_current_index = POLYINFO_HEAD_INDEX;
        }
		regdi = zorder_shape_list[zorder_next_link];
		regsi = zorder_tail_counter;

		while (regdi >= 0) {
            if (regdi >= POLYINFO_MAX_POLYS || polyinfoptrs[regdi] == 0) {
                regdi = -1;
                break;
            }
			regax = regsi;
			regsi--;
			if (regax == 0) break;
            depth_cur = polyinfo_depth_at(regdi);
            if (depth_cur < depth_key) break;
			zorder_current_index = regdi;
			regdi = zorder_shape_list[regdi];
		}
	}

    zorder_shape_list[polyinfonumpolys] = regdi;
    zorder_shape_list[zorder_current_index] = polyinfonumpolys;
	zorder_tail_counter++;
	if (regdi < 0) {
		polygon_info_head = polyinfonumpolys;
	}
	zorder_current_index = zorder_shape_list[zorder_current_index];
	polyinfonumpolys++;
    polyinfoptrnext += (unsigned)(transshapenumvertscopy * (unsigned)sizeof(struct POINT2D)) + POLYINFO_ENTRY_HEADER_SIZE;
    if (polyinfonumpolys == POLYINFO_MAX_POLYS) return 1;
    if (polyinfoptrnext <= POLYINFO_TRANSFORM_LIMIT) return 0;
	return 1;
}

/**
 * @brief Set the 3-D projection from game-space angle and viewport parameters.
 *
 * @param i1 Horizontal FOV-like angle parameter.
 * @param i2 Vertical FOV-like angle parameter.
 * @param i3 Viewport width parameter.
 * @param i4 Viewport height parameter.
 */
void set_projection(int i1, int i2, int i3, int i4) {
	
    projectiondata1 = (((long)i1 << PROJECTION_ANGLE_SHIFT) / PROJECTION_ANGLE_SCALE_DIV) >> PROJECTION_ANGLE_HALF_SHIFT;
    projectiondata2 = (((long)i2 << PROJECTION_ANGLE_SHIFT) / PROJECTION_ANGLE_SCALE_DIV) >> PROJECTION_ANGLE_HALF_SHIFT;
    projection_apply((unsigned short)i3, (unsigned short)i4);
	
}

/** Set projection from raw angles (nopsub_322DF). */
void set_projection_raw(unsigned short ang1, unsigned short ang2, unsigned short i3, unsigned short i4) {
    projectiondata1 = ang1;
    projectiondata2 = ang2;
    projection_apply(i3, i4);
}

/** @brief Legacy compatibility wrapper for set_projection_raw(). */
void nopsub_322DF(unsigned short ang1, unsigned short ang2, unsigned short i3, unsigned short i4) {
	set_projection_raw(ang1, ang2, i3, i4);
}

/** @brief Set global palette brightness level used by 3-D renderer. */
void set_byte_4032C(unsigned short val) {
	palette_brightness_level = (unsigned char)val;
}

/** @brief Update projection center offsets on X/Y axes. */
void set_projection_offsets(unsigned short arg0, unsigned short arg2) {
	projectiondata4 = arg0;
	projectiondata5 = projectiondata3 + arg0;
	projectiondata7 = arg2;
	projectiondata8 = projectiondata6 + arg2;
}

/** @brief Store a 32-bit signed value into a raw byte buffer. */
static void write_i32_to_buffer(unsigned char* dst, int32_t value)
{
    memcpy(dst, &value, sizeof(value));
}

/**
 * @brief Set up the clipping rectangle and rotation matrix for rendering.
 *
 * Builds the rotation matrix, resets polygon info, and computes
 * a viewing direction angle.
 *
 * @param angZ               Z rotation angle.
 * @param angX               X rotation angle.
 * @param angY               Y rotation angle.
 * @param cliprect           Clipping rectangle to apply.
 * @param use_scaled_preview Non-zero to use scaled preview mode.
 * @return View direction angle in engine angle units.
 */
unsigned select_cliprect_rotate(int angZ, int angX, int angY, struct RECTANGLE* cliprect, int use_scaled_preview) {
	struct MATRIX* matptr;
	struct VECTOR vec, vec2;

	//return select_cliprect_rotate_(angX, angY, angZ, cliprect, use_scaled_preview);
	
	mat_temp = *mat_rot_zxy(angZ, angX, angY, 1);
	polyinfo_reset();
	select_rect_rc = *cliprect;
	select_rect_scale_preview = use_scaled_preview;
    matptr = mat_rot_zxy(-angZ, -angX, -angY, ROTATION_ZERO_ARG);
    vec.z = SELECT_CLIP_VEC_Z;
	vec.y = 0;
	vec.x = 0;
	mat_mul_vector(&vec, matptr, &vec2);
    return polarAngle(vec2.x, vec2.z) & PROJECTION_ANGLE_MASK;
}

/**
 * @brief Reset the polygon info buffer for a new frame.
 */
void polyinfo_reset(void) {
	polyinfonumpolys = 0;
	polyinfoptrnext = 0;
	polygon_op_error_code = 0;
    zorder_shape_list[POLYINFO_HEAD_INDEX] = ZORDER_LINK_END;
    polygon_info_head = POLYINFO_HEAD_INDEX;
}

/**
 * @brief Precompute sin/cos values at the fixed 80-degree angle.
 */
void calc_sincos80(void) {
    write_i32_to_buffer(sin80, (int32_t)sin_fast(SINCOS80_ANGLE));
    write_i32_to_buffer(cos80, (int32_t)cos_fast(SINCOS80_ANGLE));
    write_i32_to_buffer(sin80_2, (int32_t)sin_fast(SINCOS80_ANGLE));
    write_i32_to_buffer(cos80_2, (int32_t)cos_fast(SINCOS80_ANGLE));
}

/**
 * @brief Atexit handler: free the polygon info buffer.
 */
static void free_polyinfo_atexit(void) {
	if (s_polyinfo_base != NULL) {
		mmgr_free((char*)s_polyinfo_base);
		s_polyinfo_base = NULL;
		polyinfoptr = NULL;
	}
}

/**
 * @brief Allocate and initialise the polygon info buffer and z-order tables.
 */
void init_polyinfo(void) {
    polyinfoptr = mmgr_alloc_resbytes("polyinfo", POLYINFO_BUFFER_SIZE);
    s_polyinfo_base = polyinfoptr;
	atexit(free_polyinfo_atexit);
	
    mat_rot_y(&mat_y0, 0);
    mat_rot_y(&mat_y100, MAT_Y_ROT_100);
    mat_rot_y(&mat_y200, MAT_Y_ROT_200);
    mat_rot_y(&mat_y300, MAT_Y_ROT_300);
	calc_sincos80();
}

enum { TRACKOBJECT_RAW_SIZE = 14 };

/**
 * @brief Return a pointer to a track-object table entry by index.
 *
 * @param table  Base of the track-object table.
 * @param index  Entry index.
 * @return Pointer to the entry.
 */
static inline const unsigned char* trkobj_entry(const unsigned char* table, unsigned index)
{
    return table + index * TRACKOBJECT_RAW_SIZE;
}

/**
 * @brief Extract the overlay byte from a track-object entry.
 *
 * @param obj  Pointer to the track-object entry.
 * @return Overlay value.
 */
static inline unsigned char trkobj_overlay(const unsigned char* obj)
{
    return obj[8];
}

/** Ported from seg020.asm */
void draw_sphere_from_vertex_buffer(const unsigned short* control_points_ptr, unsigned fill_color) {
	unsigned vertbuf[64];  /* 128 bytes = 64 words */
    build_sphere_vertex_buffer(control_points_ptr, vertbuf);
    preRender_default_alt(fill_color, SPHERE_VERTEX_BUFFER_LINES, vertbuf);
}

/**
 * @brief Render a filled sphere approximation at screen coordinates.
 * @param center_x Sphere center X.
 * @param center_y Sphere center Y.
 * @param radius Sphere radius.
 * @param color Fill color index.
 */
void preRender_sphere(int center_x, int center_y, int radius, int color) {
	/* Large arrays for left/right edge coordinates (stack: ~2KB) */
	unsigned short left_edge_x[493];  /* x1 array (left edges) */
	unsigned short right_edge_x[493];  /* x2 array (right edges) */
	
    /* Local variables for helper function call */
    unsigned short helper_coords[6];
	
	int adjusted_radius;
	int half;
	int bx_val;
	int left_clip, right_clip;
	int y_start, total_lines;
	unsigned char * table_ptr;
    uintptr_t dseg_base;
	int si_idx, di_idx, bx_ofs;
	int half_width;
	int x_left, x_right;
	int clip_skip;
	int y_top_clip;
	
	/* Calculate adjusted radius: radius - radius/4 + radius/16 */
	adjusted_radius = radius;
	{
		int ax = radius >> 2;
		adjusted_radius -= ax;
		ax >>= 2;
		adjusted_radius += ax;
	}
	
	/* If adjusted radius <= 0, nothing to draw */
	if (adjusted_radius <= 0) {
		return;
	}
	
	half = adjusted_radius >> 1;
	
	/* If half == 0, just draw single pixel */
	if (half == 0) {
		putpixel_single_clipped(color, center_y, center_x);
		return;
	}
	
	bx_val = adjusted_radius - half;
	
	/* Cache sprite1 clip bounds */
	left_clip = sprite1.sprite_left2;
	right_clip = sprite1.sprite_widthsum - 1;
	
	/* Clipping checks */
	/* Check if top of sphere is below viewport */
	if (center_y - half >= (int)sprite1.sprite_height) {
		return;
	}
	y_start = center_y - half;
	
	/* Check if bottom of sphere is above viewport */
	if (center_y + bx_val <= (int)sprite1.sprite_top) {
		return;
	}
	
	/* Horizontal clipping check */
	{
		int dx = bx_val + (bx_val >> 2);
		if (center_x - dx > right_clip) {
			return;
		}
		if (center_x + dx < left_clip) {
			return;
		}
	}
	
    /* If sphere is large (>= SPHERE_LARGE_RADIUS_MIN), use helper function */
    if (bx_val >= SPHERE_LARGE_RADIUS_MIN) {
		/* Build helper coordinate struct */
        helper_coords[0] = center_x;           /* x */
		helper_coords[1] = center_x;           /* wheel_vertices_base_ptr = x */
        helper_coords[2] = center_x + (radius >> 1);  /* x + radius/2 */
        helper_coords[3] = center_y;           /* y */
        helper_coords[4] = center_y;           /* y */
        helper_coords[5] = center_y + (adjusted_radius >> 1);  /* y + adj/2 */
        draw_sphere_from_vertex_buffer(helper_coords, color);
		return;
	}
	
	/* Use lookup table for small spheres */
    /* Convert DSEG offset table to flat pointer */
    dseg_base = (uintptr_t)&off_3F3C8[0] - (uintptr_t)SPHERE_LUT_DSEG_OFFSET;
    table_ptr = (unsigned char *)(dseg_base + (uintptr_t)off_3F3C8[bx_val]);
	total_lines = adjusted_radius;
	bx_ofs = (adjusted_radius - 1) * 2;   /* offset for mirror entries */
	si_idx = 0;
	di_idx = 0;
	
	/* Build edge arrays from lookup table */
	while (bx_ofs >= 0) {
		half_width = *table_ptr++;
		
		/* Calculate left edge */
		x_left = center_x - half_width;
		if (x_left > right_clip) {
			/* Line is entirely off right edge - skip it */
			y_start++;
			total_lines -= 2;
			bx_ofs -= 4;
			continue;
		}
		if (x_left < left_clip) {
			x_left = left_clip;
		}
		left_edge_x[si_idx] = x_left;
		left_edge_x[si_idx + (bx_ofs >> 1)] = x_left;
		
		/* Calculate right edge */
		x_right = center_x + half_width;
		if (x_right < left_clip) {
			/* Line is entirely off left edge - should not happen after above check */
			y_start++;
			total_lines -= 2;
			bx_ofs -= 4;
			continue;
		}
		if (x_right > right_clip) {
			x_right = right_clip;
		}
		right_edge_x[di_idx] = x_right;
		right_edge_x[di_idx + (bx_ofs >> 1)] = x_right;
		
		si_idx++;
		di_idx++;
		bx_ofs -= 4;
	}
	
	/* Apply vertical clipping */
	clip_skip = 0;
	y_top_clip = sprite1.sprite_top - y_start;
	if (y_top_clip > 0) {
		total_lines -= y_top_clip;
		clip_skip = y_top_clip * 2;  /* bytes to skip in arrays */
		y_start = sprite1.sprite_top;
	}
	
	/* Bottom clip */
	{
		int y_bottom_over = y_start + total_lines - sprite1.sprite_height;
		if (y_bottom_over > 0) {
			total_lines -= y_bottom_over;
		}
	}
	
	/* Draw the filled lines */
	draw_filled_lines(color, total_lines, y_start,
	                  (unsigned short *)&right_edge_x[clip_skip >> 1],
	                  (unsigned short *)&left_edge_x[clip_skip >> 1]);
}

/**
 * @brief Build wheel ring vertices from three control points.
 * @param input_data Packed input control point coordinates.
 * @param output_data Output ring vertex/interpolation data.
 */
void build_wheel_ring_vertices(int* input_data, int* output_data) {
	int si, di;
    int half_delta_x1, half_delta_y1, neg_delta_x1, neg_delta_y1;
	int i;
	
	si = input_data[0];  /* x1 */
	
	/* Store deltas at output indices 0,1,8,9 */
	output_data[0] = input_data[2] - si;       /* delta_x1 = x2 - x1 */
	output_data[1] = input_data[3] - input_data[1]; /* delta_y1 = y2 - y1 */
	output_data[8] = input_data[4] - si;       /* delta_x2 = x3 - x1 */
	output_data[9] = input_data[5] - input_data[1]; /* delta_y2 = y3 - y1 */
	
	/* Calculate scaled coordinates */
    /* output_data[4,5] at WHEEL_SCALE_OUTER_Q15 scale */
    output_data[4] = multiply_and_scale(output_data[8] + output_data[0], WHEEL_SCALE_OUTER_Q15);
    output_data[5] = multiply_and_scale(output_data[1] + output_data[9], WHEEL_SCALE_OUTER_Q15);
	
    /* output_data[2,3] at WHEEL_SCALE_INNER_Q15 scale with half delta */
    output_data[2] = multiply_and_scale(output_data[0] + (output_data[8] >> 1), WHEEL_SCALE_INNER_Q15);
	di = output_data[9] >> 1;
	output_data[3] = multiply_and_scale(output_data[1] + di, WHEEL_SCALE_INNER_Q15);
	
    /* output_data[6,7] at WHEEL_SCALE_INNER_Q15 scale with half base */
    half_delta_x1 = output_data[0] >> 1;
    output_data[6] = multiply_and_scale(output_data[8] + half_delta_x1, WHEEL_SCALE_INNER_Q15);
    half_delta_y1 = output_data[1] >> 1;
    output_data[7] = multiply_and_scale(output_data[9] + half_delta_y1, WHEEL_SCALE_INNER_Q15);
	
    /* output_data[12,13] at WHEEL_SCALE_OUTER_Q15 scale with negated base */
    neg_delta_x1 = -output_data[0];
    output_data[12] = multiply_and_scale(output_data[8] + neg_delta_x1, WHEEL_SCALE_OUTER_Q15);
    neg_delta_y1 = -output_data[1];
    output_data[13] = multiply_and_scale(output_data[9] + neg_delta_y1, WHEEL_SCALE_OUTER_Q15);
	
    /* output_data[14,15] at WHEEL_SCALE_INNER_Q15 scale */
    output_data[14] = multiply_and_scale(neg_delta_x1 + (output_data[8] >> 1), WHEEL_SCALE_INNER_Q15);
    output_data[15] = multiply_and_scale(neg_delta_y1 + di, WHEEL_SCALE_INNER_Q15);
	
	/* output_data[10,11] at WHEEL_SCALE_INNER_Q15 scale */
    output_data[10] = multiply_and_scale(output_data[8] - half_delta_x1, WHEEL_SCALE_INNER_Q15);
    output_data[11] = multiply_and_scale(output_data[9] - half_delta_y1, WHEEL_SCALE_INNER_Q15);
	
	/* Now fill remaining entries 16-31 by offsetting with base coords */
	for (i = 0; i < 8; i++) {
		int idx = i * 2;
		di = input_data[0];  /* x base */
		output_data[16 + idx] = di - output_data[idx];
		output_data[16 + idx + 1] = input_data[1] - output_data[idx + 1];
		output_data[idx] += di;
		output_data[idx + 1] += input_data[1];
	}
}

/** Ported from seg019.asm */
void build_interpolated_wheel_rings(int* input_data, int* output_data, int interpolation_factor) {
    int start_x, start_y, interp_x12, interp_y12, interp_x13, interp_y13;
    int interp[6];
	
	start_x = input_data[0];
	start_y = input_data[1];
	
    interp_x12 = multiply_and_scale((short)interpolation_factor, (short)(input_data[2] - start_x)) + start_x;
    interp_y12 = multiply_and_scale((short)interpolation_factor, (short)(input_data[3] - input_data[1])) + input_data[1];
    interp_x13 = multiply_and_scale((short)interpolation_factor, (short)(input_data[4] - start_x)) + start_x;
    interp_y13 = multiply_and_scale((short)interpolation_factor, (short)(input_data[5] - input_data[1])) + input_data[1];
	
    build_wheel_ring_vertices(input_data, output_data);
    interp[0] = start_x;
    interp[1] = start_y;
    interp[2] = interp_x12;
    interp[3] = interp_y12;
    interp[4] = interp_x13;
    interp[5] = interp_y13;
    build_wheel_ring_vertices(interp, output_data + 32);
}

/** Ported from seg023.asm */
void build_wheel_shell_vertices(int* input_data, int* output_data, int interpolation_factor) {
    int shell_offset_x, shell_offset_y;
    int* src;
    int* dest;
	int i;
	
    build_interpolated_wheel_rings(input_data, output_data, interpolation_factor);
	
	/* input_data[6] is at offset 12, input_data[7] at 14 */
    shell_offset_x = input_data[6] - input_data[0];
    shell_offset_y = input_data[7] - input_data[1];
	
	/* Copy 16 POINT2D entries from output_data to output_data+128 with offset applied */
    src = output_data;
    dest = output_data + 64;
	
    for (i = 0; i < WHEEL_RING_POINTS; i++) {
        dest[0] = src[0] + shell_offset_x;  /* x */
        dest[1] = src[1] + shell_offset_y;  /* y */
		src += 2;  /* 4 bytes */
		dest += 2;
	}
}

/* Forward declaration for preRender_wheel */
void draw_wheel_quad(unsigned fill_color, unsigned vertex_line_count, struct POINT2D vertex_lines[]);

/** Ported from seg022.asm */
void preRender_wheel(int* input_data, int output_data, unsigned sidewall_color, unsigned rim_color, unsigned tread_color) {
	/* Three regions: outer ring (outer_ring_points), inner ring (inner_ring_points), back-face copy (back_face_points) */
	/* Each region: 16 POINT2D = 32 ints. Original ASM: 64 bytes each, C port: 128 bytes each */
	unsigned wheelBuf[96];    /* 384 bytes = 3 x 32 ints */
	struct POINT2D outBuf[18]; /* outline_buffer_region region - 72 bytes for 18 points */
	struct POINT2D quadVerts[4];
	int i, minIdx, minY, curIdx;
	struct POINT2D* wheel0;
	struct POINT2D* wheel1;
	struct POINT2D* outPtr;
	struct POINT2D* outEnd;
	
	/* Call helper to fill both wheel point arrays */
    build_wheel_shell_vertices(input_data, (int*)wheelBuf, output_data);
	
	wheel0 = (struct POINT2D*)wheelBuf;        /* 16 outer ring points */
	wheel1 = (struct POINT2D*)(wheelBuf + 64); /* 16 back-face copy points */
	
	/* Draw 15 quads connecting the two wheels */
    for (i = 0; i < WHEEL_SIDE_QUAD_COUNT; i++) {
		quadVerts[0] = wheel0[i];
		quadVerts[1] = wheel0[i+1];
		quadVerts[2] = wheel1[i+1];
		quadVerts[3] = wheel1[i];
        draw_wheel_quad(sidewall_color, 4, quadVerts);
	}
	
	/* Draw wrap-around quad (i=15 to i=0) */
	quadVerts[0] = wheel0[15];
	quadVerts[1] = wheel0[0];
	quadVerts[2] = wheel1[0];
	quadVerts[3] = wheel1[15];
    draw_wheel_quad(sidewall_color, 4, quadVerts);
	
	/* Find point with minimum Y in wheel0 */
	minY = wheel0[0].py;
	minIdx = 0;
    for (i = 1; i < WHEEL_RING_POINTS; i++) {
		if (wheel0[i].py < minY) {
			minY = wheel0[i].py;
			minIdx = i;
		}
	}
	
	/* Build first half outline: walk forward from minIdx, collecting 9 points from each wheel */
	/* Original ASM uses outer ring (outer_ring_points) and inner ring (inner_ring_points) for the cap outline */
	struct POINT2D* innerRing = (struct POINT2D*)(wheelBuf + 32);
	outPtr = outBuf;
	outEnd = outBuf + 17;  /* Last entry at index 17 */
	curIdx = minIdx;
	for (i = 0; i <= 8; i++) {
		*outPtr = wheel0[curIdx];
		*outEnd = innerRing[curIdx];
		outPtr++;
		outEnd--;
		curIdx++;
        if (curIdx >= WHEEL_RING_POINTS) curIdx = 0;
	}
    preRender_default_alt(rim_color, WHEEL_RIM_OUTLINE_LINECOUNT, (unsigned*)outBuf);
	
	/* Build second half outline: walk backward from minIdx */
	outPtr = outBuf;
	outEnd = outBuf + 17;
	curIdx = minIdx;
	for (i = 0; i < 9; i++) {
		*outPtr = wheel0[curIdx];
		*outEnd = innerRing[curIdx];
		outPtr++;
		outEnd--;
		curIdx--;
        if (curIdx < 0) curIdx = WHEEL_SIDE_QUAD_COUNT;
	}
    preRender_default_alt(rim_color, WHEEL_RIM_OUTLINE_LINECOUNT, (unsigned*)outBuf);
	
	/* Draw inner ring (tread) - original ASM uses inner_ring_points (inner ring) */
    preRender_default_alt(tread_color, WHEEL_TREAD_LINECOUNT, (unsigned*)innerRing);
}

struct DRAW_HEALTH_STATS {
    unsigned drawn_total;
    unsigned degenerate;
    unsigned fully_clipped;
    unsigned extreme_coords;
    unsigned near_depth;
    unsigned invalid_link;
    unsigned order_break;
    unsigned cycle_suspect;
};

/**
 * @brief Record diagnostic statistics for a single polygon during draw-health checks.
 *
 * @param stats   Accumulator for draw-health counters.
 * @param points  Interleaved x/y point array.
 * @param count   Number of points.
 * @param depth   Polygon depth key.
 */
static void draw_health_note_poly(struct DRAW_HEALTH_STATS* stats, const int* points, int count, unsigned short depth)
{
    int i;
    int minx, maxx, miny, maxy;

    if (count < 2) {
        stats->degenerate++;
        return;
    }

    minx = maxx = points[0];
    miny = maxy = points[1];
    for (i = 1; i < count; ++i) {
        int x = points[i * 2];
        int y = points[i * 2 + 1];
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
    }

    if (maxx < sprite1.sprite_left2 ||
        minx >= sprite1.sprite_widthsum ||
        maxy < sprite1.sprite_top ||
        miny >= sprite1.sprite_height) {
        stats->fully_clipped++;
    }

    if (minx < -DRAW_HEALTH_EXTREME_COORD_LIMIT ||
        maxx > DRAW_HEALTH_EXTREME_COORD_LIMIT ||
        miny < -DRAW_HEALTH_EXTREME_COORD_LIMIT ||
        maxy > DRAW_HEALTH_EXTREME_COORD_LIMIT) {
        stats->extreme_coords++;
    }

    if (depth < NEAR_PLANE_Z) {
        stats->near_depth++;
    }
}

/**
 * @brief Validate the z-order linked list for cycles and ordering errors.
 *
 * @param stats  Accumulator for draw-health counters.
 */
static void draw_health_validate_links(struct DRAW_HEALTH_STATS* stats)
{
    int node = zorder_shape_list[POLYINFO_HEAD_INDEX];
    unsigned walked = 0;
    unsigned short prev_depth = USHRT_MAX;

    while (node >= 0) {
        unsigned short depth;
        if (node >= POLYINFO_MAX_POLYS || polyinfoptrs[node] == 0) {
            stats->invalid_link++;
            break;
        }

        depth = polyinfo_depth_at(node);
        if (walked != 0 && prev_depth < depth) {
            stats->order_break++;
        }
        prev_depth = depth;

        node = zorder_shape_list[node];
        walked++;
        if (walked > (unsigned)polyinfonumpolys + 1u) {
            stats->cycle_suspect++;
            break;
        }
    }
}

/**
 * @brief Walk the sorted polygon list and dispatch rendering for each polygon.
 *
 * Iterates over all accepted polygons, reads material/pattern info,
 * and calls the appropriate preRender function.
 */
void get_a_poly_info(void) {
	unsigned mattype;
	int matcolor;
	int maxcount;
	int matpattern;
	int pattype2;
	int regdi;
	unsigned char * polyinfoptr;
	int * pdata;
	unsigned counter;
	int j;
    int polygon_points_xy[256];
    const unsigned char* clrlist;
    const unsigned char* clrlist2;
    const unsigned char* patlist;
    const unsigned char* patlist2;
    static int draw_health_enabled = -1;
    static int draw_health_every = 30;
    static int draw_strict_enabled = -1;
    static int draw_strict_every = 60;
    static int draw_strict_max_depth = 4095;
    static int draw_strict_max_abs_coord = 8192;
    static unsigned draw_strict_drop_total = 0;
    static unsigned draw_strict_log_count = 0;

    struct DRAW_HEALTH_STATS draw_health;
    if (draw_health_enabled == -1) {
        draw_health_enabled = 0;
    }
    if (draw_strict_enabled == -1) {
        draw_strict_enabled = 0;
    }

    memset(&draw_health, 0, sizeof(draw_health));
    if (draw_health_enabled) {
        draw_health_validate_links(&draw_health);
    }

    regdi = POLYINFO_HEAD_INDEX;
	for (counter = 0; counter < polyinfonumpolys; counter++) {
		regdi = zorder_shape_list[regdi];
        if (regdi < 0 || regdi >= POLYINFO_MAX_POLYS) {
            break;
        }
        if (polyinfoptrs[regdi] == 0) {
            break;
        }
		polyinfoptr = (unsigned char *)polyinfoptrs[regdi];
        if (draw_strict_enabled) {
            unsigned short depth_check = *(unsigned short*)polyinfoptr;
            if ((int)depth_check <= 0 || (int)depth_check > draw_strict_max_depth) {
                draw_strict_drop_total++;
                if (draw_strict_log_count < DRAW_STRICT_LOG_LIMIT) {
                    draw_strict_log_count++;
                }
                continue;
            }
        }
		mattype = polyinfoptr[2];
		if (mattype >= MATERIAL_TYPE_INVALID_START) {
            mattype = 0;
        }
		clrlist = material_color_list;
        if (material_clrlist_ptr_cpy != 0 && (uintptr_t)material_clrlist_ptr_cpy >= POLYLIST_PTR_SANITY_MIN) {
			clrlist = material_clrlist_ptr_cpy;
		}
		matcolor = (int)clrlist[(unsigned)mattype * 2u];

		switch ((signed char)polyinfoptr[4]) {
		case 0: /* polygon */
            {
                int drop_poly = 0;
			maxcount = (signed char)polyinfoptr[3];
            if (maxcount <= 1 || maxcount > (int)(sizeof(polygon_points_xy) / (sizeof(polygon_points_xy[0]) * 2))) {
                break;
            }
            pdata = (int *)(polyinfoptr + POLYINFO_ENTRY_HEADER_SIZE);
			for (j = 0; j < maxcount; j++) {
				polygon_points_xy[j * 2]     = pdata[j * 2];
				polygon_points_xy[j * 2 + 1] = pdata[j * 2 + 1];
                {
                    int ax = polygon_points_xy[j * 2] < 0 ? -polygon_points_xy[j * 2] : polygon_points_xy[j * 2];
                    int ay = polygon_points_xy[j * 2 + 1] < 0 ? -polygon_points_xy[j * 2 + 1] : polygon_points_xy[j * 2 + 1];
                    if (ax > RENDER_COORD_ABS_LIMIT || ay > RENDER_COORD_ABS_LIMIT) {
                        drop_poly = 1;
                    }
                }
			}
            if (drop_poly) {
                break;
            }

            patlist = material_pattern_list;
            if (material_patlist_ptr_cpy != 0 && (uintptr_t)material_patlist_ptr_cpy >= POLYLIST_PTR_SANITY_MIN) {
                patlist = material_patlist_ptr_cpy;
            }
            matpattern = (int)patlist[(unsigned)mattype * 2u];
			pattype2 = 0;
			if (matpattern == 1 || matpattern == 2) {
                patlist2 = material_pattern2_list;
                if (material_patlist2_ptr_cpy != 0 && (uintptr_t)material_patlist2_ptr_cpy >= POLYLIST_PTR_SANITY_MIN) {
                    patlist2 = material_patlist2_ptr_cpy;
                }
                pattype2 = (int)material_table_word_at(patlist2, (unsigned)mattype);
            }
			if (matpattern == 0) {
                preRender_default(matcolor, maxcount, (unsigned int*)polygon_points_xy);
            } else if (matpattern == 1) {
                if (pattype2 != 0) {
                    preRender_patterned(pattype2, matcolor, maxcount, (struct POINT2D*)polygon_points_xy);
                } else {
                    /* Fallback: render as solid when pattern requested but pattern2 is 0 */
                    preRender_default(matcolor, maxcount, (unsigned int*)polygon_points_xy);
                }
            } else if (matpattern == 2) {
                clrlist2 = material_color_list;
                if (material_clrlist2_ptr_cpy != 0 && (uintptr_t)material_clrlist2_ptr_cpy >= POLYLIST_PTR_SANITY_MIN) {
                    clrlist2 = material_clrlist2_ptr_cpy;
                }
                preRender_two_color_pattern_flag1(pattype2,
                    clrlist2[(unsigned)mattype * 2u],
                    matcolor, maxcount, (struct POINT2D*)polygon_points_xy);
			}
            if (draw_health_enabled) {
                draw_health_note_poly(&draw_health, polygon_points_xy, maxcount, *(unsigned short*)polyinfoptr);
                draw_health.drawn_total++;
            }
			break;
            }
		case 1: /* line */
            {
                int x0 = *(unsigned *)(polyinfoptr + POLYINFO_LINE_X0_OFFSET);
                int y0 = *(unsigned *)(polyinfoptr + POLYINFO_LINE_Y0_OFFSET);
                int x1 = *(unsigned *)(polyinfoptr + POLYINFO_LINE_X1_OFFSET);
                int y1 = *(unsigned *)(polyinfoptr + POLYINFO_LINE_Y1_OFFSET);
                int a0 = x0 < 0 ? -x0 : x0;
                int b0 = y0 < 0 ? -y0 : y0;
                int a1 = x1 < 0 ? -x1 : x1;
                int b1 = y1 < 0 ? -y1 : y1;
                if (a0 > RENDER_COORD_ABS_LIMIT || b0 > RENDER_COORD_ABS_LIMIT ||
                    a1 > RENDER_COORD_ABS_LIMIT || b1 > RENDER_COORD_ABS_LIMIT) {
                    continue;
                }
            }
            if (draw_health_enabled) {
                int line_pts[4];
                line_pts[0] = *(unsigned *)(polyinfoptr + POLYINFO_LINE_X0_OFFSET);
                line_pts[1] = *(unsigned *)(polyinfoptr + POLYINFO_LINE_Y0_OFFSET);
                line_pts[2] = *(unsigned *)(polyinfoptr + POLYINFO_LINE_X1_OFFSET);
                line_pts[3] = *(unsigned *)(polyinfoptr + POLYINFO_LINE_Y1_OFFSET);
                draw_health_note_poly(&draw_health, line_pts, 2, *(unsigned short*)polyinfoptr);
                draw_health.drawn_total++;
            }
            preRender_line(
                *(unsigned *)(polyinfoptr + POLYINFO_LINE_X0_OFFSET),
                *(unsigned *)(polyinfoptr + POLYINFO_LINE_Y0_OFFSET),
                *(unsigned *)(polyinfoptr + POLYINFO_LINE_X1_OFFSET),
                *(unsigned *)(polyinfoptr + POLYINFO_LINE_Y1_OFFSET),
				matcolor);
			break;
		case 2: /* sphere */
            {
                int sx = *(unsigned *)(polyinfoptr + POLYINFO_LINE_X0_OFFSET);
                int sy = *(unsigned *)(polyinfoptr + POLYINFO_LINE_Y0_OFFSET);
                int sr = *(unsigned *)(polyinfoptr + POLYINFO_LINE_X1_OFFSET);
                int ax = sx < 0 ? -sx : sx;
                int ay = sy < 0 ? -sy : sy;
                if (ax > RENDER_COORD_ABS_LIMIT || ay > RENDER_COORD_ABS_LIMIT || sr > RENDER_COORD_ABS_LIMIT) {
                    continue;
                }
            }
            if (draw_health_enabled) {
                draw_health.drawn_total++;
                if (*(unsigned short*)polyinfoptr < NEAR_PLANE_Z) {
                    draw_health.near_depth++;
                }
            }
			preRender_sphere(
                *(unsigned *)(polyinfoptr + POLYINFO_LINE_X0_OFFSET),
                *(unsigned *)(polyinfoptr + POLYINFO_LINE_Y0_OFFSET),
                *(unsigned *)(polyinfoptr + POLYINFO_LINE_X1_OFFSET),
				matcolor);
			break;
		case 3: { /* wheel */
            const struct POINT2D* wheel_pts;
            int wheel_interp;
			int drop_wheel = 0;
            clrlist = material_color_list;
            if (material_clrlist_ptr_cpy != 0 && (uintptr_t)material_clrlist_ptr_cpy >= POLYLIST_PTR_SANITY_MIN) {
                clrlist = material_clrlist_ptr_cpy;
            }
            wheel_pts = (const struct POINT2D*)(polyinfoptr + POLYINFO_ENTRY_HEADER_SIZE);
			for (j = 0; j < 4; j++) {
                polygon_points_xy[j * 2]     = wheel_pts[j].px;
                polygon_points_xy[j * 2 + 1] = wheel_pts[j].py;
				{
					int ax = polygon_points_xy[j * 2] < 0 ? -polygon_points_xy[j * 2] : polygon_points_xy[j * 2];
					int ay = polygon_points_xy[j * 2 + 1] < 0 ? -polygon_points_xy[j * 2 + 1] : polygon_points_xy[j * 2 + 1];
                    if (ax > WHEEL_VERTEX_CONTROL_ABS_MAX || ay > WHEEL_VERTEX_CONTROL_ABS_MAX) {
						drop_wheel = 1;
					}
				}
			}
            if (drop_wheel) {
                continue;
            }
			if (draw_health_enabled) {
				draw_health_note_poly(&draw_health, polygon_points_xy, 4, *(unsigned short*)polyinfoptr);
				draw_health.drawn_total++;
			}
            /* Original ASM: mov ax, (offset trkObjectList.ss_ssOvelay+460h)
               This is a compile-time constant = dseg offset 8344 + 8 + 1120 = 9472.
               Used as Q1.14 fixed-point interpolation factor (9472/16384 ≈ 0.578)
               controlling the inner wheel ring radius (tire width ratio). */
            wheel_interp = WHEEL_INTERP_Q14;
            preRender_wheel(polygon_points_xy,
				wheel_interp,
				matcolor,
                clrlist[((unsigned)mattype + 1u) * 2u],
                clrlist[((unsigned)mattype + 2u) * 2u]);
			break;
		}
		case 5: /* pixel */
			putpixel_single_clipped(
                *(unsigned *)(polyinfoptr + POLYINFO_LINE_X0_OFFSET),
                *(unsigned *)(polyinfoptr + POLYINFO_LINE_Y0_OFFSET),
				matcolor);
			break;
		default:
			break;
		}
	}
	polyinfo_reset();
    (void)draw_health_every;
    (void)draw_strict_every;
    (void)draw_strict_max_abs_coord;
}

/**
 * @brief Render a filled primitive from a flat integer vertex array.
 * @param fill_color Fill color index.
 * @param vertex_line_count Number of vertices.
 * @param vertex_lines Interleaved x/y coordinates.
 */
void preRender_default_alt(unsigned fill_color, unsigned vertex_line_count, unsigned* vertex_lines) {
	//return preRender_default_alt_(fill_color, vertex_line_count, vertex_lines);

    spritefunc = &draw_filled_lines;
    imagefunc = &preRender_line;
    preRender_default_impl(fill_color, vertex_line_count, (int*)vertex_lines, 0);
}

/**
 * @brief Render a filled polygon using the default rasteriser path.
 *
 * @param fill_color         Fill colour index.
 * @param vertex_line_count  Number of vertex pairs.
 * @param vertex_lines       Array of vertex coordinate pairs.
 */
void preRender_default(unsigned fill_color, unsigned vertex_line_count, unsigned* vertex_lines) {
	//return preRender_default_(fill_color, vertex_line_count, vertex_lines);

    spritefunc = &draw_filled_lines;
    imagefunc = &preRender_line;
    preRender_default_impl(fill_color, vertex_line_count, (int*)vertex_lines, 1);
}

/** @brief Draw wheel quad.
 * @param fill_color Parameter value.
 * @param vertex_line_count Parameter value.
 * @param vertex_lines Parameter value.
 */
void draw_wheel_quad(unsigned fill_color, unsigned vertex_line_count, struct POINT2D vertex_lines[]) {

    //return draw_wheel_quad_(fill_color, vertex_line_count, &vertex_lines);

    spritefunc = &draw_filled_lines;
    imagefunc = &preRender_line;
    preRender_default_impl(fill_color, vertex_line_count, (int*)vertex_lines, 0);
}


/** @brief Draw two color patterned lines sprite.
 * @param color Parameter value.
 * @param numlines Parameter value.
 * @param y Parameter value.
 * @param x2arr Parameter value.
 * @param x1arr Parameter value.
 * @return Function return value.
 */
static void draw_two_color_patterned_lines_sprite(unsigned short color, unsigned short numlines, unsigned short y,
    unsigned short* x2arr, unsigned short* x1arr) {
    draw_two_color_patterned_lines((unsigned char)color, numlines, y, x2arr, x1arr);
}

/** @brief Draw patterned lines sprite.
 * @param color Parameter value.
 * @param numlines Parameter value.
 * @param y Parameter value.
 * @param x2arr Parameter value.
 * @param x1arr Parameter value.
 * @return Function return value.
 */
static void draw_patterned_lines_sprite(unsigned short color, unsigned short numlines, unsigned short y,
    unsigned short* x2arr, unsigned short* x1arr) {
    draw_patterned_lines((unsigned char)color, numlines, y, x2arr, x1arr);
}

/**
 * @brief Internal: render a polygon with a two-colour dither pattern.
 *
 * @param pattern            Pattern type selector.
 * @param alt_color          Alternate colour index.
 * @param fill_color         Primary fill colour index.
 * @param vertex_line_count  Number of vertex pairs.
 * @param vertex_lines       Vertex coordinate array.
 * @param variant_flag       Variant rendering flag.
 */
static void preRender_two_color_pattern_impl(unsigned pattern, unsigned alt_color, unsigned fill_color, unsigned vertex_line_count, struct POINT2D* vertex_lines, unsigned variant_flag) {
    spritefunc = draw_two_color_patterned_lines_sprite;
    imagefunc = &preRender_line;

    polygon_pattern_type = pattern;
    polygon_alternate_color = alt_color;
    preRender_default_impl(fill_color, vertex_line_count, (int*)vertex_lines, variant_flag);
}

/** @brief Prerender two color pattern flag1.
 * @param pattern Parameter value.
 * @param alt_color Parameter value.
 * @param fill_color Parameter value.
 * @param vertex_line_count Parameter value.
 * @param vertex_lines Parameter value.
 */
void preRender_two_color_pattern_flag1(unsigned pattern, unsigned alt_color, unsigned fill_color, unsigned vertex_line_count, struct POINT2D* vertex_lines) {

    preRender_two_color_pattern_impl(pattern, alt_color, fill_color, vertex_line_count, vertex_lines, 1);
}

/** @brief Prerender patterned.
 * @param pattern_type Parameter value.
 * @param fill_color Parameter value.
 * @param vertex_line_count Parameter value.
 * @param vertex_lines Parameter value.
 */
void preRender_patterned(unsigned pattern_type, unsigned fill_color, unsigned vertex_line_count, struct POINT2D* vertex_lines) {

	//return preRender_patterned_(pattern_type, fill_color, vertex_line_count, vertex_lines);

    spritefunc = draw_patterned_lines_sprite;
	imagefunc = &preRender_line;
	polygon_pattern_type = pattern_type;
	
    preRender_default_impl(fill_color, vertex_line_count, (int*)vertex_lines, 0);
}

/**
 * @brief Core polygon rasteriser: clip, scan-convert, and fill a polygon.
 *
 * @param fill_color         Fill colour index.
 * @param vertex_line_count  Number of vertex line entries.
/** @brief Array.
 * @param variant_flag Parameter value.
 * @return Function return value.
 */
void preRender_default_impl(unsigned fill_color, unsigned vertex_line_count, int* vertex_lines, unsigned variant_flag) {
    int16_t scanline_x_bounds[SCANLINE_HEIGHT + SCANLINE_HEIGHT];
    int16_t* scanline_bounds;
    int min_y, max_y;
    int right_clip_bound, left_clip_bound;

	int* vertex_line_ptr;
    int minx, maxx;
    unsigned i;
    int temp0x, temp0y, temp1y;

    int sprite1_sprite_left2 = sprite1.sprite_left2;
        (void)variant_flag;

    int sprite1_sprite_widthsum = sprite1.sprite_widthsum;
    int sprite1_sprite_top = sprite1.sprite_top;
    int sprite1_sprite_height = sprite1.sprite_height;

    /* Initialize scanline extent buffer: every scanline has empty span (right < left) */
    {
        int si;
        for (si = 0; si < SCANLINE_HEIGHT; si++) {
            scanline_x_bounds[si]       = (int16_t)sprite1_sprite_widthsum; /* x1 (left) = far right -> empty */
            scanline_x_bounds[SCANLINE_HEIGHT + si] = (int16_t)(sprite1_sprite_left2 - 1); /* x2 (right) = far left -> empty */
        }
    }
	
    vertex_line_ptr = vertex_lines;
    scanline_bounds = scanline_x_bounds;
    left_clip_bound = sprite1_sprite_left2;
    right_clip_bound = sprite1_sprite_widthsum - 1;
    if (vertex_line_count == 0) {
        return;
    }
    max_y = min_y = vertex_line_ptr[1];
	maxx = minx = vertex_line_ptr[0];
	if (vertex_line_count - 1 == 0) {
        imagefunc((unsigned short)vertex_line_ptr[0],
                  (unsigned short)vertex_line_ptr[1],
                  (unsigned short)vertex_line_ptr[0],
                  (unsigned short)vertex_line_ptr[1],
                  fill_color);
		return ;
	}

	for (i = 1; i < vertex_line_count; i++) {
        if (vertex_lines[i * 2 + 1] <= min_y) {
            min_y = vertex_lines[i*2 + 1];
		}
        if (vertex_lines[i * 2 + 1] > max_y) {
            max_y = vertex_lines[i * 2 + 1];
		}
		
		if (vertex_lines[i * 2 + 0] < minx) {
			minx = vertex_lines[i * 2 + 0];
		}
		if (vertex_lines[i * 2 + 0] > maxx) {
			maxx = vertex_lines[i * 2 + 0];
		}
		
	}

    if (maxx < left_clip_bound) {
        return;
    }
    if (minx >= right_clip_bound) {
        return ;
    }
    if (max_y < sprite1_sprite_top) {
        return ;
    }
    if (min_y >= sprite1_sprite_height) {
        return ;
    }
    if (max_y == min_y) {
        /* Single-scanline polygon: draw as a clipped horizontal fill.
/** @brief Imagefunc.
 * @param them Parameter value.
 * @param sprite1_sprite_height Parameter value.
 * @return Function return value.
 */
        if (min_y >= sprite1_sprite_top && min_y < sprite1_sprite_height) {
            int cx1 = minx, cx2 = maxx;
            if (cx1 < left_clip_bound) cx1 = left_clip_bound;
            if (cx2 > right_clip_bound) cx2 = right_clip_bound;
            if (cx1 <= cx2) {
                scanline_x_bounds[min_y] = (int16_t)cx1;
                scanline_x_bounds[SCANLINE_HEIGHT + min_y] = (int16_t)cx2;
                spritefunc((unsigned short)fill_color, 1, (unsigned short)min_y,
                           (unsigned short*)&scanline_x_bounds[SCANLINE_HEIGHT + min_y],
                           (unsigned short*)&scanline_x_bounds[min_y]);
            }
        }
        return;
    }
    if (maxx == minx) {
        /* Single-column polygon: draw a clipped vertical line */
        if (minx >= left_clip_bound && minx <= right_clip_bound) {
            int cy1 = min_y, cy2 = max_y;
            if (cy1 < sprite1_sprite_top) cy1 = sprite1_sprite_top;
            if (cy2 >= sprite1_sprite_height) cy2 = sprite1_sprite_height - 1;
            if (cy1 <= cy2) {
                imagefunc((unsigned short)minx, (unsigned short)cy1,
                          (unsigned short)minx, (unsigned short)cy2, fill_color);
            }
        }
        return;
    }

    for (i = 0; i < vertex_line_count; i++) {
        unsigned j = (i + 1u) % vertex_line_count;
        int x0 = vertex_lines[i * 2 + 0];
        int y0 = vertex_lines[i * 2 + 1];
        int x1 = vertex_lines[j * 2 + 0];
        int y1 = vertex_lines[j * 2 + 1];
        int y_start;
        int y_end;
        int dy;
        long long x_fp;
        long long x_step_fp;
        int y;

        if (y0 == y1) {
            continue;
        }
        if (y0 > y1) {
            int tx = x0; x0 = x1; x1 = tx;
            { int ty = y0; y0 = y1; y1 = ty; }
        }

        y_start = y0;
        y_end = y1;
        if (y_end < sprite1_sprite_top || y_start >= sprite1_sprite_height) {
            continue;
        }
        if (y_start < sprite1_sprite_top) {
            y_start = sprite1_sprite_top;
        }
        if (y_end >= sprite1_sprite_height) {
            y_end = sprite1_sprite_height - 1;
        }
        if (y_end < y_start) {
            continue;
        }

        dy = y1 - y0;
        x_step_fp = (((long long)(x1 - x0)) << FIXED_SHIFT_16) / (long long)dy;
        x_fp = ((long long)x0 << FIXED_SHIFT_16) + (long long)(y_start - y0) * x_step_fp;

        for (y = y_start; y <= y_end; y++) {
            int x_floor = (int)(x_fp >> FIXED_SHIFT_16);
            int x_ceil = x_floor + (((x_fp & FIXED_FRAC_MASK_16) != 0) ? 1 : 0);

            /* No X clamping here: out-of-bounds x values are handled by
               draw_filled_lines which clamps to sprite bounds. Clamping here
               would overwrite the x1=sprite_widthsum sentinel with
               right_clip_bound, creating spurious 1-pixel fills at x=319. */

            if (x_floor < (int)scanline_bounds[y]) {
                scanline_bounds[y] = (int16_t)x_floor;
            }
            if (x_ceil > (int)scanline_bounds[SCANLINE_HEIGHT + y]) {
                scanline_bounds[SCANLINE_HEIGHT + y] = (int16_t)x_ceil;
            }
            x_fp += x_step_fp;
        }
    }

    temp0y = max_y;

	if (temp0y >= sprite1_sprite_height) 
		temp0y = sprite1_sprite_height - 1;
    temp1y = min_y;
	if (temp1y < sprite1_sprite_top)
		temp1y = sprite1_sprite_top;
	
	temp0x = temp0y - temp1y;
	if (temp0x <= 0) {
        return ;
    }
	temp0x++;
	
    spritefunc((unsigned short)fill_color,
               (unsigned short)temp0x,
               (unsigned short)temp1y,
               (unsigned short*)&scanline_x_bounds[SCANLINE_HEIGHT + temp1y],
               (unsigned short*)&scanline_x_bounds[temp1y]);

}
// generate_poly_edges is called preRender_helper in the IDB.
/**
 * @brief Generate polygon edge data into the scanline bounds buffer.
 *
 * Also known as preRender_helper in the IDB.
 *
 * @param scanline_bounds  Scanline min/max X buffer.
 * @param regsi            Pointer to polygon edge data.
 * @param mode             0 = primary path, 1 = secondary path.
 */
void generate_poly_edges(int16_t* scanline_bounds, int* regsi, int mode) {

    int sprite1_sprite_left2 = sprite1.sprite_left2;
    int sprite1_sprite_widthsum = sprite1.sprite_widthsum;
    int i, count, ofs;
    unsigned long value;
    unsigned long temp;

    if (mode == 1) {
        goto preRender_helper2;
    }
	count = regsi[10];
	if (count > 0) {
        ofs = edge_state_start_row_from_count(regsi, count);
		for (i = 0; i < count; i++) {
            int idx = ofs + i;
            if (idx >= 0 && idx < SCANLINE_HEIGHT) {
                scanline_bounds[idx] = (int16_t)sprite1_sprite_left2;
                scanline_bounds[SCANLINE_HEIGHT + idx] = (int16_t)(sprite1_sprite_left2 - 1);
            }
		}
	}
	
	count = regsi[12];
	if (count > 0) {
        ofs = edge_state_start_row_from_count(regsi, count);
		for (i = 0; i < count; i++) {
            int idx = ofs + i;
            if (idx >= 0 && idx < SCANLINE_HEIGHT) {
                scanline_bounds[idx] = (int16_t)sprite1_sprite_widthsum;
                scanline_bounds[SCANLINE_HEIGHT + idx] = (int16_t)(sprite1_sprite_widthsum - 1);
            }
		}
	}
	
	count = regsi[11];
	if (count > 0) {
		ofs = regsi[5] + 1;
		for (i = 0; i < count; i++) {
            int idx = ofs + i;
            if (idx >= 0 && idx < SCANLINE_HEIGHT) {
                scanline_bounds[idx] = (int16_t)sprite1_sprite_left2;
                scanline_bounds[SCANLINE_HEIGHT + idx] = (int16_t)(sprite1_sprite_left2 - 1);
            }
		}
	}
	
	count = regsi[13];
	if (count > 0) {
		ofs = regsi[5] + 1;
		for (i = 0; i < count; i++) {
            int idx = ofs + i;
            if (idx >= 0 && idx < SCANLINE_HEIGHT) {
                scanline_bounds[idx] = (int16_t)sprite1_sprite_widthsum;
                scanline_bounds[SCANLINE_HEIGHT + idx] = (int16_t)(sprite1_sprite_widthsum - 1);
            }
		}
	}

preRender_helper2:

	count = regsi[7];
	if (count <= 0) return ;

	ofs = regsi[3];

	switch (regsi[9]) {
		case 0:
		case 1:
			return;
		case 2:
			for (i = 0; i < count; i++) {
                int idx = ofs + i;
                if (idx >= 0 && idx < SCANLINE_HEIGHT) {
                    scanline_bounds[idx] = (int16_t)regsi[1];
                    scanline_bounds[SCANLINE_HEIGHT + idx] = (int16_t)regsi[1];
                }
			}
			return ;
		case 3:
			for (i = 0; i < count; i++) {
                int idx = ofs + i;
                if (idx >= 0 && idx < SCANLINE_HEIGHT) {
                    scanline_bounds[idx] = (int16_t)(regsi[1] - i);
                    scanline_bounds[SCANLINE_HEIGHT + idx] = (int16_t)(regsi[1] - i);
                }
			}
			return ;
		case 4:
			for (i = 0; i < count; i++) {
                int idx = ofs + i;
                if (idx >= 0 && idx < SCANLINE_HEIGHT) {
                    scanline_bounds[idx] = (int16_t)(regsi[1] + i);
                    scanline_bounds[SCANLINE_HEIGHT + idx] = (int16_t)(regsi[1] + i);
                }
			}
			return ;
		case 5:
            value = (((unsigned long)(unsigned int)regsi[1]) << FIXED_SHIFT_16) | (unsigned int)regsi[2];
            value += FIXED_HALF_16;
			for (i = 0; i < count; i++) {
				int idx = ofs + i;
                if (idx >= 0 && idx < SCANLINE_HEIGHT) {
                    scanline_bounds[idx] = (int16_t)(value >> FIXED_SHIFT_16);
                    scanline_bounds[SCANLINE_HEIGHT + idx] = (int16_t)(value >> FIXED_SHIFT_16);
                }
				value -= (unsigned int)regsi[6];
			}
			return ;
		case 6:
            value = (((unsigned long)(unsigned int)regsi[1]) << FIXED_SHIFT_16) | (unsigned int)regsi[2];
            value += FIXED_HALF_16;
			for (i = 0; i < count; i++) {
				int idx = ofs + i;
                if (idx >= 0 && idx < SCANLINE_HEIGHT) {
                    scanline_bounds[idx] = (int16_t)(value >> FIXED_SHIFT_16);
                    scanline_bounds[SCANLINE_HEIGHT + idx] = (int16_t)(value >> FIXED_SHIFT_16);
                }
				
				value += (unsigned int)regsi[6];
			}
			return ;
		case 7:
			value = (unsigned int)regsi[1];
			temp = (unsigned int)regsi[2];
			if (temp + FIXED_HALF_16 > USHRT_MAX)
				ofs++;
			temp = (temp + FIXED_HALF_16) & FIXED_FRAC_MASK_16;
            if (ofs >= 0 && ofs < SCANLINE_HEIGHT) {
                scanline_bounds[SCANLINE_HEIGHT + ofs] = (int16_t)value;
            }
			for (i = 0; i < count; i++) {
				if (temp + (unsigned int)regsi[6] <= USHRT_MAX) {
					value--;
					if (i == count - 1) {
                        if (ofs >= 0 && ofs < SCANLINE_HEIGHT) {
                            scanline_bounds[ofs] = (int16_t)(value + 1);
                        }
					}
				} else {
                    if (ofs >= 0 && ofs < SCANLINE_HEIGHT) {
                        scanline_bounds[ofs] = (int16_t)value;
                    }
					value--;
					ofs++;
                    if (ofs >= 0 && ofs < SCANLINE_HEIGHT) {
                        scanline_bounds[SCANLINE_HEIGHT + ofs] = (int16_t)value;
                    }
				}
				temp = (temp + (unsigned int)regsi[6])  & FIXED_FRAC_MASK_16;
			}
			return ;

		case 8:
			value = (unsigned int)regsi[1];
			temp = (unsigned int)regsi[2];
			if (temp + FIXED_HALF_16 > USHRT_MAX)
				ofs++;
            temp = (temp + FIXED_HALF_16) & FIXED_FRAC_MASK_16;
            if (ofs >= 0 && ofs < SCANLINE_HEIGHT) {
                scanline_bounds[ofs] = (int16_t)value;
            }
			for (i = 0; i < count; i++) {
				if (temp + (unsigned int)regsi[6] <= USHRT_MAX) {
					value++;
					if (i == count - 1) {
                        if (ofs >= 0 && ofs < SCANLINE_HEIGHT) {
                            scanline_bounds[SCANLINE_HEIGHT + ofs] = (int16_t)(value - 1);
                        }
					}
				} else {
                    if (ofs >= 0 && ofs < SCANLINE_HEIGHT) {
                        scanline_bounds[SCANLINE_HEIGHT + ofs] = (int16_t)value;
                    }
					value++;
					ofs++;
                    if (ofs >= 0 && ofs < SCANLINE_HEIGHT) {
                        scanline_bounds[ofs] = (int16_t)value;
                    }
				}
				temp = (temp + (unsigned int)regsi[6])  & FIXED_FRAC_MASK_16;
			}
			return ;

			
		/*case 8:
			value = regsi[1];
			temp = regsi[2] + 32768;
			if (temp <= 0)
				ofs++;
            scanline_bounds[ofs] = value;
			for (i = 0; i < count; i++) {
				temp += (unsigned int)regsi[6];
				if (temp >= 0) {
					value++;
				} else {
                    scanline_bounds[480 + ofs + i] = value;
					value++;
                    scanline_bounds[ofs + i] = value;
				}
				
			}
			if (temp > 0) {
                scanline_bounds[480 + ofs + i] = value - 1;
			}
			return ;*/
		case 9:
		default:
			return ;
	}

    #undef CLAMP_X
}


// aka preRender_helper3 in the IDB
/**
 * @brief Accumulate edge contributions into scanline min/max bounds.
 *
 * Also known as preRender_helper3 in the IDB.
 *
 * @param regsi              Pointer to polygon edge data.
/** @brief Unused.
 * @param scanline_bounds Parameter value.
 * @return Function return value.
 */
void accumulate_scanline_bounds(int* regsi, unsigned unused_flag, unsigned apply_border_clip, int16_t* scanline_bounds) {
	int count;
	int ofs;
	int i;
	int x;
	int mode;
	unsigned long accum;

    (void)unused_flag;

	#define UPDATE_MIN_MAX(row, xv) do { \
        int _x = (xv); \
        if ((row) >= 0 && (row) < SCANLINE_HEIGHT) { \
            if (_x < (int)scanline_bounds[(row)]) scanline_bounds[(row)] = (int16_t)_x; \
            if (_x > (int)scanline_bounds[SCANLINE_HEIGHT + (row)]) scanline_bounds[SCANLINE_HEIGHT + (row)] = (int16_t)_x; \
		} \
	} while(0)

	count = regsi[7];
	if (count <= 0) {
		goto done_clip;
	}

	ofs = regsi[3];
	mode = regsi[9];

	if (mode == 7) {
        unsigned long temp = (unsigned int)regsi[2];
        unsigned long step = (unsigned int)regsi[6];
        unsigned long sum;
        int ax = regsi[1];
        int row = ofs;
        int rem = count;

        sum = temp + FIXED_HALF_16;
        if (sum > FIXED_FRAC_MASK_16) row++;
        temp = sum & FIXED_FRAC_MASK_16;

        while (rem > 0) {
            if (row >= 0 && row < SCANLINE_HEIGHT && scanline_bounds[SCANLINE_HEIGHT + row] < ax) {
                scanline_bounds[SCANLINE_HEIGHT + row] = (int16_t)ax;
            }

            sum = temp + step;
            temp = sum & FIXED_FRAC_MASK_16;
            if (sum > FIXED_FRAC_MASK_16) {
                if (row >= 0 && row < SCANLINE_HEIGHT && scanline_bounds[row] > ax) {
                    scanline_bounds[row] = (int16_t)ax;
                }
                row++;
                ax--;
                rem--;
                continue;
            }

            ax--;
            rem--;
            while (rem > 0) {
                sum = temp + step;
                temp = sum & FIXED_FRAC_MASK_16;
                if (sum > FIXED_FRAC_MASK_16) {
                    if (row >= 0 && row < SCANLINE_HEIGHT && scanline_bounds[row] > ax) {
                        scanline_bounds[row] = (int16_t)ax;
                    }
                    row++;
                    ax--;
                    rem--;
                    break;
                }
                ax--;
                rem--;
            }

            if (rem == 0) {
                ax++;
                if (row >= 0 && row < SCANLINE_HEIGHT && scanline_bounds[row] > ax) {
                    scanline_bounds[row] = (int16_t)ax;
                }
            }
        }
		goto done_clip;
	} else if (mode == 8) {
        unsigned long temp = (unsigned int)regsi[2];
        unsigned long step = (unsigned int)regsi[6];
        unsigned long sum;
        int ax = regsi[1];
        int row = ofs;
        int rem = count;

        sum = temp + FIXED_HALF_16;
        if (sum > FIXED_FRAC_MASK_16) row++;
        temp = sum & FIXED_FRAC_MASK_16;

        while (rem > 0) {
            if (row >= 0 && row < SCANLINE_HEIGHT && scanline_bounds[row] > ax) {
                scanline_bounds[row] = (int16_t)ax;
            }

            sum = temp + step;
            temp = sum & FIXED_FRAC_MASK_16;
            if (sum > FIXED_FRAC_MASK_16) {
                if (row >= 0 && row < SCANLINE_HEIGHT && scanline_bounds[SCANLINE_HEIGHT + row] < ax) {
                    scanline_bounds[SCANLINE_HEIGHT + row] = (int16_t)ax;
                }
                row++;
                ax++;
                rem--;
                continue;
            }

            ax++;
            rem--;
            while (rem > 0) {
                sum = temp + step;
                temp = sum & FIXED_FRAC_MASK_16;
                if (sum > FIXED_FRAC_MASK_16) {
                    if (row >= 0 && row < SCANLINE_HEIGHT && scanline_bounds[SCANLINE_HEIGHT + row] < ax) {
                        scanline_bounds[SCANLINE_HEIGHT + row] = (int16_t)ax;
                    }
                    row++;
                    ax++;
                    rem--;
                    break;
                }
                ax++;
                rem--;
            }

            if (rem == 0) {
                ax--;
                if (row >= 0 && row < SCANLINE_HEIGHT && scanline_bounds[SCANLINE_HEIGHT + row] < ax) {
                    scanline_bounds[SCANLINE_HEIGHT + row] = (int16_t)ax;
                }
            }
        }
		goto done_clip;
	}

    accum = (((unsigned long)(unsigned int)regsi[1]) << FIXED_SHIFT_16) | (unsigned int)regsi[2];

	for (i = 0; i < count; i++) {
		switch (mode) {
			case 3:
				x = regsi[1] - i;
				break;
			case 4:
				x = regsi[1] + i;
				break;
            case 5:
                x = (int)((accum + FIXED_HALF_16) >> FIXED_SHIFT_16);
				accum -= (unsigned int)regsi[6];
				break;
			case 6:
                x = (int)((accum + FIXED_HALF_16) >> FIXED_SHIFT_16);
				accum += (unsigned int)regsi[6];
				break;
			case 2:
			default:
				x = regsi[1];
				break;
		}

		if (ofs + i >= 0 && ofs + i < SCANLINE_HEIGHT) {
            if (x < (int)scanline_bounds[ofs + i]) {
                scanline_bounds[ofs + i] = (int16_t)x;
			}
            if (x > (int)scanline_bounds[SCANLINE_HEIGHT + ofs + i]) {
                scanline_bounds[SCANLINE_HEIGHT + ofs + i] = (int16_t)x;
			}
		}
	}

	#undef UPDATE_MIN_MAX

done_clip:
    if (apply_border_clip != 0) {
		count = regsi[10];
		if (count > 0) {
			ofs = regsi[3] - count;
			if (regsi[2] < 0) {
				ofs++;
			}
            for (i = 0; i < count; i++) {
                if (ofs + i >= 0 && ofs + i < SCANLINE_HEIGHT) {
                    scanline_bounds[ofs + i] = sprite1.sprite_left2;
				}
			}
		}

		count = regsi[12];
		if (count > 0) {
			ofs = regsi[3] - count;
			if (regsi[2] < 0) {
				ofs++;
			}
            for (i = 0; i < count; i++) {
                if (ofs + i >= 0 && ofs + i < SCANLINE_HEIGHT) {
                    scanline_bounds[SCANLINE_HEIGHT + ofs + i] = sprite1.sprite_widthsum - 1;
				}
			}
		}

		count = regsi[11];
		if (count > 0) {
			ofs = regsi[5] + 1;
            for (i = 0; i < count; i++) {
                if (ofs + i >= 0 && ofs + i < SCANLINE_HEIGHT) {
                    scanline_bounds[ofs + i] = sprite1.sprite_left2;
				}
			}
		}

		count = regsi[13];
		if (count > 0) {
			ofs = regsi[5] + 1;
            for (i = 0; i < count; i++) {
                if (ofs + i >= 0 && ofs + i < SCANLINE_HEIGHT) {
                    scanline_bounds[SCANLINE_HEIGHT + ofs + i] = sprite1.sprite_widthsum - 1;
				}
			}
		}
	}
}

unsigned  legacy_interp_default_scale = 50;

static unsigned  g_interp_blob[1177] = {
    2256u, 32768u, 21845u, 43690u, 16384u, 32768u, 49152u, 13107u, 26214u, 39321u,
    52428u, 10922u, 21845u, 32768u, 43690u, 54613u, 9362u, 18724u, 28086u, 37449u,
    46811u, 56173u, 8192u, 16384u, 24576u, 32768u, 40960u, 49152u, 57344u, 7281u,
    14563u, 21845u, 29127u, 36408u, 43690u, 50972u, 58254u, 6553u, 13107u, 19660u,
    26214u, 32768u, 39321u, 45875u, 52428u, 58982u, 5957u, 11915u, 17873u, 23831u,
    29789u, 35746u, 41704u, 47662u, 53620u, 59578u, 5461u, 10922u, 16384u, 21845u,
    27306u, 32768u, 38229u, 43690u, 49152u, 54613u, 60074u, 5041u, 10082u, 15123u,
    20164u, 25206u, 30247u, 35288u, 40329u, 45371u, 50412u, 55453u, 60494u, 4681u,
    9362u, 14043u, 18724u, 23405u, 28086u, 32768u, 37449u, 42130u, 46811u, 51492u,
    56173u, 60854u, 4369u, 8738u, 13107u, 17476u, 21845u, 26214u, 30583u, 34952u,
    39321u, 43690u, 48059u, 52428u, 56797u, 61166u, 4096u, 8192u, 12288u, 16384u,
    20480u, 24576u, 28672u, 32768u, 36864u, 40960u, 45056u, 49152u, 53248u, 57344u,
    61440u, 3855u, 7710u, 11565u, 15420u, 19275u, 23130u, 26985u, 30840u, 34695u,
    38550u, 42405u, 46260u, 50115u, 53970u, 57825u, 61680u, 3640u, 7281u, 10922u,
    14563u, 18204u, 21845u, 25486u, 29127u, 32768u, 36408u, 40049u, 43690u, 47331u,
    50972u, 54613u, 58254u, 61895u, 3449u, 6898u, 10347u, 13797u, 17246u, 20695u,
    24144u, 27594u, 31043u, 34492u, 37941u, 41391u, 44840u, 48289u, 51738u, 55188u,
    58637u, 62086u, 3276u, 6553u, 9830u, 13107u, 16384u, 19660u, 22937u, 26214u,
    29491u, 32768u, 36044u, 39321u, 42598u, 45875u, 49152u, 52428u, 55705u, 58982u,
    62259u, 3120u, 6241u, 9362u, 12483u, 15603u, 18724u, 21845u, 24966u, 28086u,
    31207u, 34328u, 37449u, 40569u, 43690u, 46811u, 49932u, 53052u, 56173u, 59294u,
    62415u, 2978u, 5957u, 8936u, 11915u, 14894u, 17873u, 20852u, 23831u, 26810u,
    29789u, 32768u, 35746u, 38725u, 41704u, 44683u, 47662u, 50641u, 53620u, 56599u,
    59578u, 62557u, 2849u, 5698u, 8548u, 11397u, 14246u, 17096u, 19945u, 22795u,
    25644u, 28493u, 31343u, 34192u, 37042u, 39891u, 42740u, 45590u, 48439u, 51289u,
    54138u, 56987u, 59837u, 62686u, 2730u, 5461u, 8192u, 10922u, 13653u, 16384u,
    19114u, 21845u, 24576u, 27306u, 30037u, 32768u, 35498u, 38229u, 40960u, 43690u,
    46421u, 49152u, 51882u, 54613u, 57344u, 60074u, 62805u, 2621u, 5242u, 7864u,
    10485u, 13107u, 15728u, 18350u, 20971u, 23592u, 26214u, 28835u, 31457u, 34078u,
    36700u, 39321u, 41943u, 44564u, 47185u, 49807u, 52428u, 55050u, 57671u, 60293u,
    62914u, 2520u, 5041u, 7561u, 10082u, 12603u, 15123u, 17644u, 20164u, 22685u,
    25206u, 27726u, 30247u, 32768u, 35288u, 37809u, 40329u, 42850u, 45371u, 47891u,
    50412u, 52932u, 55453u, 57974u, 60494u, 63015u, 2427u, 4854u, 7281u, 9709u,
    12136u, 14563u, 16990u, 19418u, 21845u, 24272u, 26699u, 29127u, 31554u, 33981u,
    36408u, 38836u, 41263u, 43690u, 46117u, 48545u, 50972u, 53399u, 55826u, 58254u,
    60681u, 63108u, 2340u, 4681u, 7021u, 9362u, 11702u, 14043u, 16384u, 18724u,
    21065u, 23405u, 25746u, 28086u, 30427u, 32768u, 35108u, 37449u, 39789u, 42130u,
    44470u, 46811u, 49152u, 51492u, 53833u, 56173u, 58514u, 60854u, 63195u, 2259u,
    4519u, 6779u, 9039u, 11299u, 13559u, 15819u, 18078u, 20338u, 22598u, 24858u,
    27118u, 29378u, 31638u, 33897u, 36157u, 38417u, 40677u, 42937u, 45197u, 47457u,
    49716u, 51976u, 54236u, 56496u, 58756u, 61016u, 63276u, 2184u, 4369u, 6553u,
    8738u, 10922u, 13107u, 15291u, 17476u, 19660u, 21845u, 24029u, 26214u, 28398u,
    30583u, 32768u, 34952u, 37137u, 39321u, 41506u, 43690u, 45875u, 48059u, 50244u,
    52428u, 54613u, 56797u, 58982u, 61166u, 63351u, 2114u, 4228u, 6342u, 8456u,
    10570u, 12684u, 14798u, 16912u, 19026u, 21140u, 23254u, 25368u, 27482u, 29596u,
    31710u, 33825u, 35939u, 38053u, 40167u, 42281u, 44395u, 46509u, 48623u, 50737u,
    52851u, 54965u, 57079u, 59193u, 61307u, 63421u, 2048u, 4096u, 6144u, 8192u,
    10240u, 12288u, 14336u, 16384u, 18432u, 20480u, 22528u, 24576u, 26624u, 28672u,
    30720u, 32768u, 34816u, 36864u, 38912u, 40960u, 43008u, 45056u, 47104u, 49152u,
    51200u, 53248u, 55296u, 57344u, 59392u, 61440u, 63488u, 1985u, 3971u, 5957u,
    7943u, 9929u, 11915u, 13901u, 15887u, 17873u, 19859u, 21845u, 23831u, 25817u,
    27803u, 29789u, 31775u, 33760u, 35746u, 37732u, 39718u, 41704u, 43690u, 45676u,
    47662u, 49648u, 51634u, 53620u, 55606u, 57592u, 59578u, 61564u, 63550u, 1927u,
    3855u, 5782u, 7710u, 9637u, 11565u, 13492u, 15420u, 17347u, 19275u, 21202u,
    23130u, 25057u, 26985u, 28912u, 30840u, 32768u, 34695u, 36623u, 38550u, 40478u,
    42405u, 44333u, 46260u, 48188u, 50115u, 52043u, 53970u, 55898u, 57825u, 59753u,
    61680u, 63608u, 1872u, 3744u, 5617u, 7489u, 9362u, 11234u, 13107u, 14979u,
    16852u, 18724u, 20597u, 22469u, 24341u, 26214u, 28086u, 29959u, 31831u, 33704u,
    35576u, 37449u, 39321u, 41194u, 43066u, 44938u, 46811u, 48683u, 50556u, 52428u,
    54301u, 56173u, 58046u, 59918u, 61791u, 63663u, 1820u, 3640u, 5461u, 7281u,
    9102u, 10922u, 12743u, 14563u, 16384u, 18204u, 20024u, 21845u, 23665u, 25486u,
    27306u, 29127u, 30947u, 32768u, 34588u, 36408u, 38229u, 40049u, 41870u, 43690u,
    45511u, 47331u, 49152u, 50972u, 52792u, 54613u, 56433u, 58254u, 60074u, 61895u,
    63715u, 1771u, 3542u, 5313u, 7084u, 8856u, 10627u, 12398u, 14169u, 15941u,
    17712u, 19483u, 21254u, 23026u, 24797u, 26568u, 28339u, 30111u, 31882u, 33653u,
    35424u, 37196u, 38967u, 40738u, 42509u, 44281u, 46052u, 47823u, 49594u, 51366u,
    53137u, 54908u, 56679u, 58451u, 60222u, 61993u, 63764u, 1724u, 3449u, 5173u,
    6898u, 8623u, 10347u, 12072u, 13797u, 15521u, 17246u, 18970u, 20695u, 22420u,
    24144u, 25869u, 27594u, 29318u, 31043u, 32768u, 34492u, 36217u, 37941u, 39666u,
    41391u, 43115u, 44840u, 46565u, 48289u, 50014u, 51738u, 53463u, 55188u, 56912u,
    58637u, 60362u, 62086u, 63811u, 1680u, 3360u, 5041u, 6721u, 8402u, 10082u,
    11762u, 13443u, 15123u, 16804u, 18484u, 20164u, 21845u, 23525u, 25206u, 26886u,
    28566u, 30247u, 31927u, 33608u, 35288u, 36969u, 38649u, 40329u, 42010u, 43690u,
    45371u, 47051u, 48731u, 50412u, 52092u, 53773u, 55453u, 57133u, 58814u, 60494u,
    62175u, 63855u, 1638u, 3276u, 4915u, 6553u, 8192u, 9830u, 11468u, 13107u,
    14745u, 16384u, 18022u, 19660u, 21299u, 22937u, 24576u, 26214u, 27852u, 29491u,
    31129u, 32768u, 34406u, 36044u, 37683u, 39321u, 40960u, 42598u, 44236u, 45875u,
    47513u, 49152u, 50790u, 52428u, 54067u, 55705u, 57344u, 58982u, 60620u, 62259u,
    63897u, 1598u, 3196u, 4795u, 6393u, 7992u, 9590u, 11189u, 12787u, 14385u,
    15984u, 17582u, 19181u, 20779u, 22378u, 23976u, 25575u, 27173u, 28771u, 30370u,
    31968u, 33567u, 35165u, 36764u, 38362u, 39960u, 41559u, 43157u, 44756u, 46354u,
    47953u, 49551u, 51150u, 52748u, 54346u, 55945u, 57543u, 59142u, 60740u, 62339u,
    63937u, 1560u, 3120u, 4681u, 6241u, 7801u, 9362u, 10922u, 12483u, 14043u,
    15603u, 17164u, 18724u, 20284u, 21845u, 23405u, 24966u, 26526u, 28086u, 29647u,
    31207u, 32768u, 34328u, 35888u, 37449u, 39009u, 40569u, 42130u, 43690u, 45251u,
    46811u, 48371u, 49932u, 51492u, 53052u, 54613u, 56173u, 57734u, 59294u, 60854u,
    62415u, 63975u, 1524u, 3048u, 4572u, 6096u, 7620u, 9144u, 10668u, 12192u,
    13716u, 15240u, 16765u, 18289u, 19813u, 21337u, 22861u, 24385u, 25909u, 27433u,
    28957u, 30481u, 32005u, 33530u, 35054u, 36578u, 38102u, 39626u, 41150u, 42674u,
    44198u, 45722u, 47246u, 48770u, 50295u, 51819u, 53343u, 54867u, 56391u, 57915u,
    59439u, 60963u, 62487u, 64011u, 1489u, 2978u, 4468u, 5957u, 7447u, 8936u,
    10426u, 11915u, 13405u, 14894u, 16384u, 17873u, 19362u, 20852u, 22341u, 23831u,
    25320u, 26810u, 28299u, 29789u, 31278u, 32768u, 34257u, 35746u, 37236u, 38725u,
    40215u, 41704u, 43194u, 44683u, 46173u, 47662u, 49152u, 50641u, 52130u, 53620u,
    55109u, 56599u, 58088u, 59578u, 61067u, 62557u, 64046u, 1456u, 2912u, 4369u,
    5825u, 7281u, 8738u, 10194u, 11650u, 13107u, 14563u, 16019u, 17476u, 18932u,
    20388u, 21845u, 23301u, 24758u, 26214u, 27670u, 29127u, 30583u, 32039u, 33496u,
    34952u, 36408u, 37865u, 39321u, 40777u, 42234u, 43690u, 45147u, 46603u, 48059u,
    49516u, 50972u, 52428u, 53885u, 55341u, 56797u, 58254u, 59710u, 61166u, 62623u,
    64079u, 1424u, 2849u, 4274u, 5698u, 7123u, 8548u, 9972u, 11397u, 12822u,
    14246u, 15671u, 17096u, 18521u, 19945u, 21370u, 22795u, 24219u, 25644u, 27069u,
    28493u, 29918u, 31343u, 32768u, 34192u, 35617u, 37042u, 38466u, 39891u, 41316u,
    42740u, 44165u, 45590u, 47014u, 48439u, 49864u, 51289u, 52713u, 54138u, 55563u,
    56987u, 58412u, 59837u, 61261u, 62686u, 64111u, 1394u, 2788u, 4183u, 5577u,
    6971u, 8366u, 9760u, 11155u, 12549u, 13943u, 15338u, 16732u, 18126u, 19521u,
    20915u, 22310u, 23704u, 25098u, 26493u, 27887u, 29282u, 30676u, 32070u, 33465u,
    34859u, 36253u, 37648u, 39042u, 40437u, 41831u, 43225u, 44620u, 46014u, 47409u,
    48803u, 50197u, 51592u, 52986u, 54380u, 55775u, 57169u, 58564u, 59958u, 61352u,
    62747u, 64141u, 1365u, 2730u, 4096u, 5461u, 6826u, 8192u, 9557u, 10922u,
    12288u, 13653u, 15018u, 16384u, 17749u, 19114u, 20480u, 21845u, 23210u, 24576u,
    25941u, 27306u, 28672u, 30037u, 31402u, 32768u, 34133u, 35498u, 36864u, 38229u,
    39594u, 40960u, 42325u, 43690u, 45056u, 46421u, 47786u, 49152u, 50517u, 51882u,
    53248u, 54613u, 55978u, 57344u, 58709u, 60074u, 61440u, 62805u, 64170u, 1337u,
    2674u, 4012u, 5349u, 6687u, 8024u, 9362u, 10699u, 12037u, 13374u, 14712u,
    16049u, 17387u, 18724u, 20062u, 21399u, 22736u, 24074u, 25411u, 26749u, 28086u,
    29424u, 30761u, 32099u, 33436u, 34774u, 36111u, 37449u, 38786u, 40124u, 41461u,
    42799u, 44136u, 45473u, 46811u, 48148u, 49486u, 50823u, 52161u, 53498u, 54836u,
    56173u, 57511u, 58848u, 60186u, 61523u, 62861u, 64198u,
};

static unsigned g_interp_offsets_rel[50] = {
    0u, 0u, 0u, 2u, 6u, 12u, 20u, 30u, 42u, 56u,
    72u, 90u, 110u, 132u, 156u, 182u, 210u, 240u, 272u, 306u,
    342u, 380u, 420u, 462u, 506u, 552u, 600u, 650u, 702u, 756u,
    812u, 870u, 930u, 992u, 1056u, 1122u, 1190u, 1260u, 1332u, 1406u,
    1482u, 1560u, 1640u, 1722u, 1806u, 1892u, 1980u, 2070u, 2162u, 2256u,
};

unsigned  off_2F44A[50] = { 0 };
static unsigned char g_interp_tables_ready = 0;

/**
 * @brief Initialise interpolation lookup tables for edge rendering.
 */
static void __attribute__((unused)) shape3d_init_interp_tables(void)
{
    unsigned i;
    uintptr_t base_off;

    if (g_interp_tables_ready != 0) {
        return;
    }

    base_off = (uintptr_t)g_interp_blob;
    for (i = 0; i < INTERP_TABLE_COUNT; i++) {
        off_2F44A[i] = (unsigned)(base_off + (uintptr_t)g_interp_offsets_rel[i]);
    }

    g_interp_tables_ready = 1;
}

unsigned draw_line_related_impl(unsigned start_x, unsigned start_y, unsigned end_x, unsigned end_y, int* edge_state, unsigned allow_steep_modes);

/**
/** @brief Segment.
 * @param edge_state Parameter value.
 * @return Function return value.
 */
unsigned draw_line_related(unsigned start_x, unsigned start_y, unsigned end_x, unsigned end_y, int* edge_state) {
    //return draw_line_related_(start_x, start_y, end_x, end_y, edge_state);
    return draw_line_related_impl(start_x, start_y, end_x, end_y, edge_state, 0);
}

/**
/** @brief Segment.
 * @param edge_state Parameter value.
 * @return Function return value.
 */
unsigned draw_line_related_alt(unsigned start_x, unsigned start_y, unsigned end_x, unsigned end_y, int* edge_state) {
    //return draw_line_related_alt_(start_x, start_y, end_x, end_y, edge_state);
    return draw_line_related_impl(start_x, start_y, end_x, end_y, edge_state, 1);
}


/**
 * @brief Core implementation: compute edge state for a line segment.
 *
 * @param start_x            Start X coordinate.
 * @param start_y            Start Y coordinate.
 * @param end_x              End X coordinate.
 * @param end_y              End Y coordinate.
 * @param edge_state         Output edge state array.
 * @param allow_steep_modes  Non-zero to enable steep-line processing modes.
 * @return Edge processing result code.
 */
unsigned draw_line_related_impl(unsigned start_x, unsigned start_y, unsigned end_x, unsigned end_y, int* edge_state, unsigned allow_steep_modes) {
    int x0;
    int y0;
    int x1;
    int y1;
    int left;
    int right;
    int top;
    int bottom;
    int dx;
    int dy;
    int yclip_top;
    int yclip_bottom;
    int xstep;
    int mode;
    long long accum;

    if (edge_state == 0) {
        return 0;
    }

    for (dx = 0; dx < EDGE_STATE_WORD_COUNT; dx++) {
        edge_state[dx] = 0;
    }

    x0 = (int)(signed short)start_x;
    y0 = (int)(signed short)start_y;
    x1 = (int)(signed short)end_x;
    y1 = (int)(signed short)end_y;

    if (y1 < y0) {
        int tx = x0;
        int ty = y0;
        x0 = x1;
        y0 = y1;
        x1 = tx;
        y1 = ty;
    }

    left = (int)sprite1.sprite_left2;
    right = (int)sprite1.sprite_widthsum - 1;
    top = (int)sprite1.sprite_top;
    bottom = (int)sprite1.sprite_height - 1;

    if (y1 < top || y0 > bottom) {
        return 1;
    }

    yclip_top = 0;
    yclip_bottom = 0;
    if (y0 < top) {
        yclip_top = top - y0;
        y0 = top;
    }
    if (y1 > bottom) {
        yclip_bottom = y1 - bottom;
        y1 = bottom;
    }
    if (y1 < y0) {
        return 1;
    }

    dx = x1 - x0;
    dy = y1 - y0;

    edge_state[3] = y0;
    edge_state[5] = y1;
    edge_state[7] = dy + 1;
    edge_state[10] = yclip_top;
    edge_state[12] = yclip_bottom;

    if (dy == 0) {
        int xh = x0;
        if (xh < left) {
            xh = left;
        }
        if (xh > right) {
            xh = right;
        }
        edge_state[1] = xh;
        edge_state[9] = 2;
        return 0;
    }

    accum = ((long long)x0) << FIXED_SHIFT_16;
    xstep = (int)(((long long)dx << FIXED_SHIFT_16) / dy);

    if (xstep == 0) {
        mode = 2;
        edge_state[1] = x0;
        edge_state[7] = dy + 1;
    } else if (xstep == FIXED_ONE_16) {
        mode = 4;
        edge_state[1] = x0;
        edge_state[7] = dy + 1;
    } else if (xstep == -FIXED_ONE_16) {
        mode = 3;
        edge_state[1] = x0;
        edge_state[7] = dy + 1;
    } else if (allow_steep_modes != 0 && xstep > FIXED_ONE_16) {
        /* Steep right: dx > dy → mode 8 (x-major DDA, no y-clip adjustment needed) */
        int ystep_steep = (int)(((long long)dy << FIXED_SHIFT_16) / (long long)dx);
        mode = 8;
        edge_state[1] = x0;       /* x = startX */
        edge_state[2] = 0;        /* yfrac = 0 */
        edge_state[6] = ystep_steep; /* y step per x advance */
        edge_state[7] = dx + 1;   /* count = x-pixel steps */
    } else if (allow_steep_modes != 0 && xstep < -FIXED_ONE_16) {
        /* Steep left: |dx| > dy → mode 7 (x-major DDA, no y-clip adjustment needed) */
        int absdx = -dx;
        int ystep_steep = (int)(((long long)dy << FIXED_SHIFT_16) / (long long)absdx);
        mode = 7;
        edge_state[1] = x0;       /* x = startX */
        edge_state[2] = 0;        /* yfrac = 0 */
        edge_state[6] = ystep_steep; /* y step per x advance */
        edge_state[7] = absdx + 1;  /* count = x-pixel steps */
    } else if (xstep > 0) {
        mode = 6;
        edge_state[1] = (int)(accum >> FIXED_SHIFT_16);
        edge_state[2] = (int)(accum & FIXED_FRAC_MASK_16);
        edge_state[6] = xstep;
        edge_state[7] = dy + 1;
    } else {
        mode = 5;
        edge_state[1] = (int)(accum >> FIXED_SHIFT_16);
        edge_state[2] = (int)(accum & FIXED_FRAC_MASK_16);
        edge_state[6] = -xstep;
        edge_state[7] = dy + 1;
    }

    edge_state[4] = edge_state[2];
    edge_state[8] = x1;
    edge_state[9] = mode;

    if (allow_steep_modes == 0) {
        /* X clipping when allow_steep_modes == 0 (draw_line_related, not _alt) */
        int x_start = x0;
        int x_end = x1;
        int count = edge_state[7];
        
        /* Early-out: line completely outside x bounds */
        if (x_start < left && x_end < left) {
            return 2;
        }
        if (x_start > right && x_end > right) {
            return 2;
        }
        
        /* Clip x coordinates based on mode */
        if (mode == 2) {
            /* Vertical line: clip x to bounds */
            if (x0 < left) {
                edge_state[1] = left;
            } else if (x0 > right) {
                edge_state[1] = right;
            }
        } else if (mode == 3 || mode == 5) {
/** @brief Left.
 * @param high Parameter value.
 * @param left Parameter value.
 * @param right Parameter value.
 * @param xstep Parameter value.
 * @return Function return value.
 */
            /* Line going left (x decreasing as y increases):
             * Start x = x0 (high), End x = x1 (low)
             * Clip: if x_end < left, reduce count from bottom
             *       if x_start > right, reduce count from top
             */
            if (x_end < left && count > 0 && xstep != 0) {
                /* End of line is left of viewport - clip bottom */
                int rows_outside = (left - x_end);
                if (mode == 3) {
                    /* dx/dy = -1, so 1 pixel per row */
                    edge_state[12] += rows_outside;  /* yclip_bottom */
                    edge_state[7] -= rows_outside;
                    edge_state[5] -= rows_outside;
                    if (edge_state[7] <= 0) return 2;
                }
            }
            if (x_start > right && count > 0 && xstep != 0) {
                /* Start of line is right of viewport - clip top */
                int rows_outside = (x_start - right);
                if (mode == 3) {
                    edge_state[10] += rows_outside;  /* yclip_top */
                    edge_state[7] -= rows_outside;
                    edge_state[3] += rows_outside;
                    edge_state[1] = right;
                    if (edge_state[7] <= 0) return 2;
                }
            }
        } else if (mode == 4 || mode == 6) {
/** @brief Right.
 * @param low Parameter value.
 * @param left Parameter value.
 * @param right Parameter value.
 * @param xstep Parameter value.
 * @return Function return value.
 */
            /* Line going right (x increasing as y increases):
             * Start x = x0 (low), End x = x1 (high)
             * Clip: if x_start < left, reduce count from top
             *       if x_end > right, reduce count from bottom
             */
            if (x_start < left && count > 0 && xstep != 0) {
                /* Start of line is left of viewport - clip top */
                int rows_outside = (left - x_start);
                if (mode == 4) {
                    edge_state[10] += rows_outside;  /* yclip_top */
                    edge_state[7] -= rows_outside;
                    edge_state[3] += rows_outside;
                    edge_state[1] = left;
                    if (edge_state[7] <= 0) return 2;
                }
            }
            if (x_end > right && count > 0 && xstep != 0) {
                /* End of line is right of viewport - clip bottom */
                int rows_outside = (x_end - right);
                if (mode == 4) {
                    edge_state[12] += rows_outside;  /* yclip_bottom */
                    edge_state[7] -= rows_outside;
                    edge_state[5] -= rows_outside;
                    if (edge_state[7] <= 0) return 2;
                }
            }
        }
    }

    return 0;
}

/**
 * @brief Try to locate and initialise a named shape from a resource.
 *
 * @param resptr       Loaded resource pointer.
 * @param name         Shape name to locate.
 * @param shape_index  Index into game3dshapes[].
 * @return 1 on success, 0 if shape not found.
 */
static int shape3d_try_init_shape(char* resptr, const char* name, int shape_index) {
    char* shape_ptr;

    if (resptr == 0 || shape_index < 0 || shape_index >= SHAPE3D_TOTAL_SHAPES) {
        return 0;
    }

    shape_ptr = locate_shape_nofatal(resptr, (char*)name);
    if (shape_ptr == 0) {
        memset(&game3dshapes[shape_index], 0, sizeof(game3dshapes[shape_index]));
        return 0;
    }

    shape3d_init_shape(shape_ptr, &game3dshapes[shape_index]);
    return 1;
}

/**
 * @brief Load player and opponent car 3-D shapes and wheel vertex data.
 *
 * @param player_car_id    4-character player car identifier.
 * @param opponent_car_id  4-character opponent car identifier.
 */
void shape3d_load_car_shapes(char player_car_id[], char opponent_car_id[]) {
	int i;
	struct VECTOR * wheel_vertices_base_ptr;
    unsigned long car_resource_size_bytes;
	aStxxx[2] = player_car_id[0];
	aStxxx[3] = player_car_id[1];
	aStxxx[4] = player_car_id[2];
	aStxxx[5] = player_car_id[3];
    aStxxx[6] = 0;
	carresptr = file_load_3dres(aStxxx);

    if (carresptr == 0) {
        memset(&game3dshapes[124], 0, sizeof(game3dshapes[124]));
        memset(&game3dshapes[126], 0, sizeof(game3dshapes[126]));
        memset(&game3dshapes[128], 0, sizeof(game3dshapes[128]));
        return;
    }

    shape3d_try_init_shape((char*)carresptr, "car0", 124);
    shape3d_try_init_shape((char*)carresptr, "car1", 126);

    if (game3dshapes[126].shape3d_verts != 0 && game3dshapes[126].shape3d_numverts >= CAR_SHAPE_MIN_VERTS) {
        wheel_vertices_base_ptr = &(game3dshapes[126].shape3d_verts[8]);
        carshapevec.z = wheel_vertices_base_ptr[0].z;
        carshapevec.x = (wheel_vertices_base_ptr[3].x + wheel_vertices_base_ptr[0].x)/2;
        carshapevec2.z = wheel_vertices_base_ptr[6].z;
        carshapevec2.x = (wheel_vertices_base_ptr[6].x + wheel_vertices_base_ptr[9].x) /2;
		
        for (i = 0; i < 6; i++) {
            carshapevecs[i].x = carshapevec.x - wheel_vertices_base_ptr[i + 0].x;
            carshapevecs[i].z = carshapevec.z - wheel_vertices_base_ptr[i + 0].z;
            carshapevecs[i].y = wheel_vertices_base_ptr[i + 0].y;
            carshapevecs2[i].x = carshapevec2.x - wheel_vertices_base_ptr[i + 6].x;
            carshapevecs2[i].z = carshapevec2.z - wheel_vertices_base_ptr[i + 6].z;
            carshapevecs2[i].y = wheel_vertices_base_ptr[i + 6].y;
            carshapevecs3[i] = wheel_vertices_base_ptr[i + 12];
            carshapevecs4[i] = wheel_vertices_base_ptr[i + 18];
        }
    }

	for (i = 0; i < 5; i++) {
		viewport_clipping_bounds[i] = 0;
	}

    shape3d_try_init_shape((char*)carresptr, "car2", 128);
    shape3d_try_init_shape((char*)carresptr, "exp0", 116);
    shape3d_try_init_shape((char*)carresptr, "exp1", 117);
    shape3d_try_init_shape((char*)carresptr, "exp2", 118);
    shape3d_try_init_shape((char*)carresptr, "exp3", 119);

	if (opponent_car_id[0] != -1) {
		if (player_car_id[0] == opponent_car_id[0] && player_car_id[1] == opponent_car_id[1] &&
			player_car_id[2] == opponent_car_id[2] && player_car_id[3] == opponent_car_id[3])
		{
            car_resource_size_bytes = mmgr_get_chunk_size_bytes((char*)carresptr);
			car2resptr = mmgr_alloc_resbytes("car2", car_resource_size_bytes);
            memcpy((char*)car2resptr, (const char*)carresptr, (size_t)car_resource_size_bytes);
		} else {
			aStxxx[2] = opponent_car_id[0];
			aStxxx[3] = opponent_car_id[1];
			aStxxx[4] = opponent_car_id[2];
			aStxxx[5] = opponent_car_id[3];
            aStxxx[6] = 0;
			car2resptr = file_load_3dres(aStxxx);
		}

        shape3d_try_init_shape((char*)car2resptr, "car0", 125);
        shape3d_try_init_shape((char*)car2resptr, "car1", 127);

        /* Reference uses game3dshapes[126] (player's car1) for opponent wheel setup */
        if (game3dshapes[126].shape3d_verts != 0 && game3dshapes[126].shape3d_numverts >= CAR_SHAPE_MIN_VERTS) {
            wheel_vertices_base_ptr = &(game3dshapes[126].shape3d_verts[8]);
            oppcarshapevec.z = wheel_vertices_base_ptr[0].z;
            oppcarshapevec.x = (wheel_vertices_base_ptr[3].x + wheel_vertices_base_ptr[0].x)/2;
            oppcarshapevec2.z = wheel_vertices_base_ptr[6].z;
            oppcarshapevec2.x = (wheel_vertices_base_ptr[6].x + wheel_vertices_base_ptr[9].x) /2;

            for (i = 0; i < 6; i++) {
                oppcarshapevecs[i].x = oppcarshapevec.x - wheel_vertices_base_ptr[i + 0].x;
                oppcarshapevecs[i].z = oppcarshapevec.z - wheel_vertices_base_ptr[i + 0].z;
                oppcarshapevecs[i].y = wheel_vertices_base_ptr[i + 0].y;
                oppcarshapevecs2[i].x = oppcarshapevec2.x - wheel_vertices_base_ptr[i + 6].x;
                oppcarshapevecs2[i].z = oppcarshapevec2.z - wheel_vertices_base_ptr[i + 6].z;
                oppcarshapevecs2[i].y = wheel_vertices_base_ptr[i + 6].y;
                oppcarshapevecs3[i] = wheel_vertices_base_ptr[i + 12];
                oppcarshapevecs4[i] = wheel_vertices_base_ptr[i + 18];
            }
        }
		for (i = 0; i < 5; i++) {
			game_frame_pointer[i] = 0;
		}
        shape3d_init_shape(locate_shape_fatal((char*)car2resptr, "car2"), &game3dshapes[129]);
        shape3d_init_shape(locate_shape_fatal((char*)car2resptr, "exp0"), &game3dshapes[120]);
        shape3d_init_shape(locate_shape_fatal((char*)car2resptr, "exp1"), &game3dshapes[121]);
        shape3d_init_shape(locate_shape_fatal((char*)car2resptr, "exp2"), &game3dshapes[122]);
        shape3d_init_shape(locate_shape_fatal((char*)car2resptr, "exp3"), &game3dshapes[123]);
	} else {
		car2resptr = 0;
	}
}

/**
 * @brief Update car wheel vertex positions from rotation angle and suspension.
 *
 * @param wheel_vertices         Output vertex array for wheel mesh.
 * @param wheel_rotation_angle   Current wheel rotation angle.
 * @param wheel_compression_src  Suspension compression values per wheel.
 * @param wheel_state_cache      Cached wheel state for delta detection.
 * @param wheel_vertex_offsets   Base vertex offsets for wheel rings.
 * @param wheel_center_points    Centre points for inner/outer wheel rings.
 */
void shape3d_update_car_wheel_vertices(struct VECTOR * wheel_vertices, int wheel_rotation_angle, short* wheel_compression_src, short* wheel_state_cache, struct VECTOR* wheel_vertex_offsets, struct VECTOR* wheel_center_points) {
	int i, j;
    int sin_half_angle;
    int cos_half_angle;
    int rotated_component;
    int wheel_drop;
    int wheel_vertex_end;
    //return shape3d_update_car_wheel_vertices_legacy(wheel_vertices, wheel_rotation_angle, wheel_compression_src, wheel_state_cache, wheel_vertex_offsets, wheel_center_points);
    if (wheel_state_cache[4] != 0) {
        sin_half_angle = sin_fast(wheel_rotation_angle / 2);
        cos_half_angle = cos_fast(wheel_rotation_angle / 2);

        for (i = 0; i < WHEEL_POINTS_PER_RING; i++) {
            rotated_component = multiply_and_scale(wheel_vertex_offsets[i].x, cos_half_angle);
            wheel_vertices[i].x = multiply_and_scale(wheel_vertex_offsets[i].z, sin_half_angle) + wheel_center_points[0].x + rotated_component;
            rotated_component = multiply_and_scale(wheel_vertex_offsets[i].z, cos_half_angle);
            wheel_vertices[i].z = multiply_and_scale(wheel_vertex_offsets[i].x, sin_half_angle) + wheel_center_points[0].z + rotated_component;
		}
        for (i = WHEEL_POINTS_PER_RING; i < WHEEL_POINTS_BOTH_RINGS; i++) {
            rotated_component = multiply_and_scale(wheel_vertex_offsets[i].x, cos_half_angle);
            wheel_vertices[i].x = multiply_and_scale(wheel_vertex_offsets[i].z, sin_half_angle) + wheel_center_points[1].x + rotated_component;
            rotated_component = multiply_and_scale(wheel_vertex_offsets[i].z, cos_half_angle);
            wheel_vertices[i].z = multiply_and_scale(wheel_vertex_offsets[i].x, sin_half_angle) + wheel_center_points[1].z + rotated_component;
		}
        wheel_state_cache[4] = wheel_rotation_angle;
	}

	for (j = 0; j < WHEEL_COUNT; j++) {
		
    wheel_drop = labs(labs(wheel_compression_src[j]) >> WHEEL_COMPRESSION_SHIFT);
        //wheel_drop = (wheel_compression_src[j]) >> 6;
        if (wheel_state_cache[j] == wheel_drop)
			continue;
		i = j * WHEEL_POINTS_PER_RING;
        wheel_vertex_end = (j * WHEEL_POINTS_PER_RING) + WHEEL_POINTS_PER_RING;

        for (; i < wheel_vertex_end; i++) {
            wheel_vertices[i].y = (wheel_vertex_offsets[i].y) - wheel_drop;
		}
        wheel_state_cache[j] = wheel_drop;
	}

	return ;
}

/**
 * @brief Reset wheel vertices to neutral position and free car shape resources.
 */
void shape3d_free_car_shapes() {
	if (car2resptr != 0) {
		shape3d_update_car_wheel_vertices(&(game3dshapes[127].shape3d_verts[8]), 0, (short*)car_wheel_vertex_data, game_frame_pointer, oppcarshapevecs, &oppcarshapevec);
        mmgr_release((char*)car2resptr);
	}
        shape3d_update_car_wheel_vertices(&(game3dshapes[126].shape3d_verts[8]), 0, (short*)car_wheel_vertex_data, viewport_clipping_bounds, carshapevecs, &carshapevec);
    mmgr_free((char*)carresptr);
}

// Draw a line from (startX, startY) to (endX, endY) with specified color
/** @brief Prerender line.
 * @param startX Parameter value.
 * @param startY Parameter value.
 * @param endX Parameter value.
 * @param endY Parameter value.
 * @param color Parameter value.
 */
void preRender_line(unsigned short startX, unsigned short startY,
                    unsigned short endX, unsigned short endY, unsigned short color) {
    int x0 = (int)startX;
    int y0 = (int)startY;
    int x1 = (int)endX;
    int y1 = (int)endY;
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    for (;;) {
        putpixel_single_clipped((int)color, (unsigned short)y0, (unsigned short)x0);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        {
            int e2 = err << 1;
            if (e2 > -dy) {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y0 += sy;
            }
        }
    }
}

