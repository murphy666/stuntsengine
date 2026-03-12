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

#include "stunts.h"
#include "math.h"
#include "shape3d.h"
#include "font.h"
#include <stdlib.h>
#include <string.h>

/* Variables moved from data_game.c (private to this translation unit) */
static struct TRANSFORMEDSHAPE3D currenttransshape[29] = { 0 };
static struct RECTANGLE hud_timer_rect = { 0, 0, 0, 0 };
static struct RECTANGLE opponent_car_rect = { 0, 0, 0, 0 };
static struct RECTANGLE player_car_rect = { 0, 0, 0, 0 };
static struct RECTANGLE sky_starfield_rect = { 0, 0, 0, 0 }; /* was 64 bytes padding - only 1 RECTANGLE needed */


/* Render distance multiplier.
 * Must be 1 to match the original DOS behaviour.
 * shape_visibility_threshold doubles as the paint-cull-mask
 * threshold in shape3d.c: inflating it by >1 keeps paint_cull_mask=0
 * for nearby shapes, disabling the direction-based bypass of the
 * winding check and causing edge-on hill terrain to be rejected. */
#define REND_DIST_MULT 1

/* file-local data (moved from data_global.c) */
static unsigned short off_3BE44[8] = { 31340, 31318, 31296, 31340, 31318, 31296, 31340, 31318 };
static char tile_scan_pattern_octant_7[135] = {
	255,248,2, 0,248,2, 1,248,2, /* dist=8 (3 tiles) */
	254,249,2, 255,249,2, 0,249,2, 1,249,2, 2,249,2, /* dist=7 (5 tiles) */
	254,250,2, 255,250,2, 0,250,2, 1,250,2, 2,250,2, /* dist=6 (5 tiles) */
	253,251,2, 254,251,2, 255,251,2, 0,251,2, 1,251,2, 2,251,2, 3,251,2, /* dist=5 (7 tiles) */
	254,252,2, 255,252,2, 0,252,2, 1,252,2, 2,252,2, /* dist=4 (5 tiles) [original] */
	254,253,1, 255,253,1, 0,253,1, 1,253,1, 2,253,1, /* dist=3 (5 tiles) [original] */
	254,254,1, 255,254,0, 0,254,0, 1,254,0, 2,254,1, /* dist=2 (5 tiles) [original] */
	254,255,0, 255,255,0, 0,255,0, 1,255,0, 2,255,0, /* dist=1 (5 tiles) [original] */
	255,0,0, 1,0,0, 0,0,0, /* dist=0 (3 tiles) [original] */
	255,1,2, 1,1,2 /* dist=-1 behind (2 tiles) */
};
static char tile_scan_pattern_octant_0[135] = {
	1,248,2, 0,248,2, 255,248,2, /* dist=8 (3 tiles) */
	2,249,2, 1,249,2, 0,249,2, 255,249,2, 254,249,2, /* dist=7 (5 tiles) */
	2,250,2, 1,250,2, 0,250,2, 255,250,2, 254,250,2, /* dist=6 (5 tiles) */
	3,251,2, 2,251,2, 1,251,2, 0,251,2, 255,251,2, 254,251,2, 253,251,2, /* dist=5 (7 tiles) */
	2,252,2, 1,252,2, 0,252,2, 255,252,2, 254,252,2, /* dist=4 (5 tiles) [original] */
	2,253,1, 1,253,1, 0,253,1, 255,253,1, 254,253,1, /* dist=3 (5 tiles) [original] */
	2,254,1, 1,254,0, 0,254,0, 255,254,0, 254,254,1, /* dist=2 (5 tiles) [original] */
	2,255,0, 1,255,0, 0,255,0, 255,255,0, 254,255,0, /* dist=1 (5 tiles) [original] */
	1,0,0, 255,0,0, 0,0,0, /* dist=0 (3 tiles) [original] */
	1,1,2, 255,1,2 /* dist=-1 behind (2 tiles) */
};
static char tile_scan_pattern_octant_1[135] = {
	8,255,2, 8,0,2, 8,1,2, /* dist=8 (3 tiles) */
	7,254,2, 7,255,2, 7,0,2, 7,1,2, 7,2,2, /* dist=7 (5 tiles) */
	6,254,2, 6,255,2, 6,0,2, 6,1,2, 6,2,2, /* dist=6 (5 tiles) */
	5,253,2, 5,254,2, 5,255,2, 5,0,2, 5,1,2, 5,2,2, 5,3,2, /* dist=5 (7 tiles) */
	4,254,2, 4,255,2, 4,0,2, 4,1,2, 4,2,2, /* dist=4 (5 tiles) [original] */
	3,254,1, 3,255,1, 3,0,1, 3,1,1, 3,2,1, /* dist=3 (5 tiles) [original] */
	2,254,1, 2,255,0, 2,0,0, 2,1,0, 2,2,1, /* dist=2 (5 tiles) [original] */
	1,254,0, 1,255,0, 1,0,0, 1,1,0, 2,2,0, /* dist=1 (5 tiles) [original] */
	0,255,0, 0,1,0, 0,0,0, /* dist=0 (3 tiles) [original] */
	255,255,2, 255,1,2 /* dist=-1 behind (2 tiles) */
};
static char tile_scan_pattern_octant_2[135] = {
	8,1,2, 8,0,2, 8,255,2, /* dist=8 (3 tiles) */
	7,2,2, 7,1,2, 7,0,2, 7,255,2, 7,254,2, /* dist=7 (5 tiles) */
	6,2,2, 6,1,2, 6,0,2, 6,255,2, 6,254,2, /* dist=6 (5 tiles) */
	5,3,2, 5,2,2, 5,1,2, 5,0,2, 5,255,2, 5,254,2, 5,253,2, /* dist=5 (7 tiles) */
	4,2,2, 4,1,2, 4,0,2, 4,255,2, 4,254,2, /* dist=4 (5 tiles) [original] */
	3,2,1, 3,1,1, 3,0,1, 3,255,1, 3,254,1, /* dist=3 (5 tiles) [original] */
	2,2,1, 2,1,0, 2,0,0, 2,255,0, 2,254,1, /* dist=2 (5 tiles) [original] */
	1,2,0, 1,1,0, 1,0,0, 1,255,0, 1,254,0, /* dist=1 (5 tiles) [original] */
	0,1,0, 0,255,0, 0,0,0, /* dist=0 (3 tiles) [original] */
	255,1,2, 255,255,2 /* dist=-1 behind (2 tiles) */
};
static char tile_scan_pattern_octant_4[135] = {
	255,8,2, 0,8,2, 1,8,2, /* dist=8 (3 tiles) */
	254,7,2, 255,7,2, 0,7,2, 1,7,2, 2,7,2, /* dist=7 (5 tiles) */
	254,6,2, 255,6,2, 0,6,2, 1,6,2, 2,6,2, /* dist=6 (5 tiles) */
	253,5,2, 254,5,2, 255,5,2, 0,5,2, 1,5,2, 2,5,2, 3,5,2, /* dist=5 (7 tiles) */
	254,4,2, 255,4,2, 0,4,2, 1,4,2, 2,4,2, /* dist=4 (5 tiles) [original] */
	254,3,1, 255,3,1, 0,3,1, 1,3,1, 2,3,1, /* dist=3 (5 tiles) [original] */
	254,2,1, 255,2,0, 0,2,0, 1,2,0, 2,2,1, /* dist=2 (5 tiles) [original] */
	254,1,0, 255,1,0, 0,1,0, 1,1,0, 2,1,0, /* dist=1 (5 tiles) [original] */
	255,0,0, 1,0,0, 0,0,0, /* dist=0 (3 tiles) [original] */
	255,255,2, 1,255,2 /* dist=-1 behind (2 tiles) */
};
static char tile_scan_pattern_octant_5[135] = {
	248,1,2, 248,0,2, 248,255,2, /* dist=8 (3 tiles) */
	249,2,2, 249,1,2, 249,0,2, 249,255,2, 249,254,2, /* dist=7 (5 tiles) */
	250,2,2, 250,1,2, 250,0,2, 250,255,2, 250,254,2, /* dist=6 (5 tiles) */
	251,3,2, 251,2,2, 251,1,2, 251,0,2, 251,255,2, 251,254,2, 251,253,2, /* dist=5 (7 tiles) */
	252,2,2, 252,1,2, 252,0,2, 252,255,2, 252,254,2, /* dist=4 (5 tiles) [original] */
	253,2,1, 253,1,1, 253,0,1, 253,255,1, 253,254,1, /* dist=3 (5 tiles) [original] */
	254,2,1, 254,1,0, 254,0,0, 254,255,0, 254,254,1, /* dist=2 (5 tiles) [original] */
	255,2,0, 255,1,0, 255,0,0, 255,255,0, 255,254,0, /* dist=1 (5 tiles) [original] */
	0,1,0, 0,255,0, 0,0,0, /* dist=0 (3 tiles) [original] */
	1,1,2, 1,255,2 /* dist=-1 behind (2 tiles) */
};
static char tile_scan_pattern_octant_6[135] = {
	248,255,2, 248,0,2, 248,1,2, /* dist=8 (3 tiles) */
	249,254,2, 249,255,2, 249,0,2, 249,1,2, 249,2,2, /* dist=7 (5 tiles) */
	250,254,2, 250,255,2, 250,0,2, 250,1,2, 250,2,2, /* dist=6 (5 tiles) */
	251,253,2, 251,254,2, 251,255,2, 251,0,2, 251,1,2, 251,2,2, 251,3,2, /* dist=5 (7 tiles) */
	252,254,2, 252,255,2, 252,0,2, 252,1,2, 252,2,2, /* dist=4 (5 tiles) */
	253,254,1, 253,255,1, 253,0,1, 253,1,1, 253,2,1, /* dist=3 (5 tiles) */
	254,254,1, 254,255,0, 254,0,0, 254,1,0, 254,2,1, /* dist=2 (5 tiles) */
	255,254,0, 255,255,0, 255,0,0, 255,1,0, 255,2,0, /* dist=1 (5 tiles) */
	0,255,0, 0,1,0, 0,0,0, /* dist=0 (3 tiles) */
	1,255,2, 1,1,2 /* dist=-1 behind (2 tiles) */
};
static unsigned char sprite_clip_region_selector[6] = { 2, 2, 1, 0, 0, 0 };
static short terrain_multitile_type1[2] = { 0, 0 };
static short terrain_multitile_type2[4] = { 0, 512, 0, (short)65024 };
static short terrain_multitile_type3[4] = { 512, 0, (short)65024, 0 };
static short terrain_multitile_type4[8] = { (short)65024, 512, (short)65024, (short)65024, 512, 512, 512, (short)65024 };
static unsigned char polygon_edge_direction_table[16] = { 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3 };
static unsigned short animation_frame_lookup[8] = { 0, 0, 256, 256, 512, 512, 768, 768 };
static unsigned char fence_TrkObjCodes[8] = { 214, 215, 214, 215, 214, 215, 214, 215 };
static signed char terrain_offset_type0[2] = { 0, 0 };
static signed char terrain_offset_type1[4] = { 0, 0, 0, 1 };
static signed char terrain_offset_type2[6] = { 0, 0, 1, 0, 0, 0 };
static signed char terrain_offset_type3[16] = { 0, 0, 1, 0, 0, 1, 1, 1, (signed char)128, 0, (signed char)128, 1, (signed char)128, 2, (signed char)128, 3 };


static char tile_scan_pattern_octant_3[135] = {
	1,8,2, 0,8,2, 255,8,2, /* dist=8 (3 tiles) */
	2,7,2, 1,7,2, 0,7,2, 255,7,2, 254,7,2, /* dist=7 (5 tiles) */
	2,6,2, 1,6,2, 0,6,2, 255,6,2, 254,6,2, /* dist=6 (5 tiles) */
	3,5,2, 2,5,2, 1,5,2, 0,5,2, 255,5,2, 254,5,2, 253,5,2, /* dist=5 (7 tiles) */
	2,4,2, 1,4,2, 0,4,2, 255,4,2, 254,4,2, /* dist=4 (5 tiles) */
	2,3,0, 1,3,1, 0,3,1, 255,3,1, 254,3,1, /* dist=3 (5 tiles) */
	2,2,1, 1,2,0, 0,2,0, 255,2,0, 254,2,1, /* dist=2 (5 tiles) */
	2,1,0, 1,1,0, 0,1,0, 255,1,0, 254,1,0, /* dist=1 (5 tiles) */
	1,0,0, 255,0,0, 0,0,0, /* dist=0 (3 tiles) */
	1,255,2, 255,255,2 /* dist=-1 behind (2 tiles) */
};
//extern struct TRANSFORMEDSHAPE3D transshapeunk;


void build_track_object(struct VECTOR* a, struct VECTOR* b);
void transformed_shape_add_for_sort(int a, int b);
char  subst_hillroad_track(int a, int b);
int render_skybox_layer(int a, struct RECTANGLE* rectptr, int c, struct MATRIX* matptr, int e, int f, int g);
struct RECTANGLE* draw_ingame_text(void);
struct RECTANGLE* init_crak(int frame, int top, int height);
struct RECTANGLE* do_sinking(int frame, int top, int height);
struct RECTANGLE* intro_draw_text(char* str, int a, int b, int c, int d);
void format_frame_as_string(char* dest, unsigned short frames, unsigned short showFractions);
void shape_op_explosion(int a, void * shp, int x, int y);
void heapsort_by_order(int n, int* heap, int* data);

/** @brief Transformedshape sort desc.
 * @param zvals Parameter `zvals`.
 * @param indices Parameter `indices`.
 * @param count Parameter `count`.
 * @return Function result.
 */
static void transformedshape_sort_desc(short* zvals, short* indices, int count)
{
	int i;
	int j;
	for (i = 0; i < count - 1; i++) {
		for (j = 0; j < count - 1 - i; j++) {
			if (zvals[j] < zvals[j + 1]) {
				short tz = zvals[j];
				short ti = indices[j];
				zvals[j] = zvals[j + 1];
				indices[j] = indices[j + 1];
				zvals[j + 1] = tz;
				indices[j + 1] = ti;
			}
		}
	}
}

enum {
	TRACKOBJECT_RAW_SIZE = 14,
	TRACKOBJECT_ROT_Y_OFFSET = 2,
	TRACKOBJECT_SHAPE_OFS_OFFSET = 4,
	TRACKOBJECT_LOSHAPE_OFS_OFFSET = 6,
	TRACKOBJECT_OVERLAY_OFFSET = 8,
	TRACKOBJECT_SURFACE_OFFSET = 9,
	TRACKOBJECT_IGNORE_ZBIAS_OFFSET = 10,
	TRACKOBJECT_MULTI_OFFSET = 11,
	TRACKOBJECT_PHYS_OFFSET = 12
};
enum { TRACKOBJECT_LIST_COUNT = 215, SCENESHAPES2_COUNT = 19, SCENESHAPES3_COUNT = 13 };

/** @brief Trkobj entry.
 * @param table Parameter `table`.
 * @param index Parameter `index`.
 * @return Function result.
 */
static inline const unsigned char* trkobj_entry(const unsigned char* table, unsigned index)
{
	return table + index * TRACKOBJECT_RAW_SIZE;
}

/** @brief Trkobj entry legacy scene index.
 * @param index Parameter `index`.
 * @return Function result.
 */
static inline const unsigned char* trkobj_entry_legacy_scene_index(unsigned index)
{
	if (index < TRACKOBJECT_LIST_COUNT) {
		return trkobj_entry(trkObjectList, index);
	}
	index -= TRACKOBJECT_LIST_COUNT;
	if (index < SCENESHAPES2_COUNT) {
		return trkobj_entry(sceneshapes2, index);
	}
	index -= SCENESHAPES2_COUNT;
	if (index < SCENESHAPES3_COUNT) {
		return trkobj_entry(sceneshapes3, index);
	}
	return 0;
}

/** @brief Trkobj u16 field.
 * @param obj Parameter `obj`.
 * @param offset Parameter `offset`.
 * @return Function result.
 */
static inline unsigned short trkobj_u16_field(const unsigned char* obj, unsigned offset)
{
	return (unsigned short)obj[offset] | ((unsigned short)obj[offset + 1] << 8);
}

/** @brief Trkobj u8 field.
 * @param obj Parameter `obj`.
 * @param offset Parameter `offset`.
 * @return Function result.
 */
static inline unsigned char trkobj_u8_field(const unsigned char* obj, unsigned offset)
{
	return obj[offset];
}

/** @brief Trkobj ofs shape.
 * @param obj Parameter `obj`.
 * @return Function result.
 */
static inline unsigned short trkobj_ofs_shape(const unsigned char* obj)
{
	return trkobj_u16_field(obj, TRACKOBJECT_SHAPE_OFS_OFFSET);
}

/** @brief Trkobj ofs loshape.
 * @param obj Parameter `obj`.
 * @return Function result.
 */
static inline unsigned short trkobj_ofs_loshape(const unsigned char* obj)
{
	return trkobj_u16_field(obj, TRACKOBJECT_LOSHAPE_OFS_OFFSET);
}

#define GAME3DSHAPES_DOS_BASE   30284u
#define GAME3DSHAPES_DOS_STRIDE 22u
#define GAME3DSHAPES_MAX        130

enum {
	FRAME_RECT_ARRAY_COUNT = 15,
	FRAME_TILE_SCAN_COUNT = 45,
	FRAME_TILE_SCAN_LAST = 44,
	FRAME_TILE_MAX_INDEX = 29,
	FRAME_TRACK_BORDER_LEVEL = 12,
	FRAME_SCREEN_WIDTH = 320,
	FRAME_SCREEN_HEIGHT = 200,
	FRAME_ANGLE_MASK = 1023,
	FRAME_VISIBILITY_NEAR = 100,
	FRAME_VISIBILITY_MEDIUM = 300,
	FRAME_VISIBILITY_FAR = 1024,
	FRAME_VISIBILITY_HILL = 2048,
	FRAME_CAMERA_HEIGHT_OFFSET = 90,
	FRAME_VIEW_PITCH_OFFSET = 50,
	FRAME_STARFIELD_Z = 15000,
	FRAME_SKY_PROBE_Z = 1000,
	FRAME_CAMERA_LONG_Z = 16384,
	FRAME_START_ARROW_RADIUS = 36,
	FRAME_START_ARROW_FORWARD = 438,
	FRAME_START_ARROW_Z_BIAS = 56,
	FRAME_SORT_BIAS = 2048,
	FRAME_SORT_BIAS_OVERLAY = 1024,
	FRAME_FLAG_TRACKED_SHAPE = 12
};

/** @brief Shape3d from dos offset.
 * @param ofs Parameter `ofs`.
 * @return Function result.
 */
static inline struct SHAPE3D* shape3d_from_dos_dseg_offset(unsigned short ofs)
{
	int idx;
	if (ofs == 0) {
		return (struct SHAPE3D*)0;
	}
	if (ofs < GAME3DSHAPES_DOS_BASE) {
		return (struct SHAPE3D*)0;
	}
	idx = (int)((unsigned)(ofs - GAME3DSHAPES_DOS_BASE) / GAME3DSHAPES_DOS_STRIDE);
	if (idx < 0 || idx >= GAME3DSHAPES_MAX) {
		return (struct SHAPE3D*)0;
	}
	if (game3dshapes[idx].shape3d_verts == 0) {
		return (struct SHAPE3D*)0;
	}
	return &game3dshapes[idx];
}

/** @brief Trkobj shape.
 * @param obj Parameter `obj`.
 * @return Function result.
 */
static inline struct SHAPE3D* trkobj_shape(const unsigned char* obj)
{
	return shape3d_from_dos_dseg_offset(trkobj_ofs_shape(obj));
}

/** @brief Trkobj loshape.
 * @param obj Parameter `obj`.
 * @return Function result.
 */
static inline struct SHAPE3D* trkobj_loshape(const unsigned char* obj)
{
	return shape3d_from_dos_dseg_offset(trkobj_ofs_loshape(obj));
}

/** @brief Trkobj roty.
 * @param obj Parameter `obj`.
 * @return Function result.
 */
static inline short trkobj_roty(const unsigned char* obj)
{
	return (short)trkobj_u16_field(obj, TRACKOBJECT_ROT_Y_OFFSET);
}

/** @brief Trkobj overlay.
 * @param obj Parameter `obj`.
 * @return Function result.
 */
static inline unsigned char trkobj_overlay(const unsigned char* obj)
{
	return trkobj_u8_field(obj, TRACKOBJECT_OVERLAY_OFFSET);
}

/** @brief Trkobj surface.
 * @param obj Parameter `obj`.
 * @return Function result.
 */
static inline signed char trkobj_surface(const unsigned char* obj)
{
	return (signed char)trkobj_u8_field(obj, TRACKOBJECT_SURFACE_OFFSET);
}

/** @brief Trkobj ignore zbias.
 * @param obj Parameter `obj`.
 * @return Function result.
 */
static inline unsigned char trkobj_ignore_zbias(const unsigned char* obj)
{
	return trkobj_u8_field(obj, TRACKOBJECT_IGNORE_ZBIAS_OFFSET);
}

/** @brief Trkobj multi.
 * @param obj Parameter `obj`.
 * @return Function result.
 */
static inline unsigned char trkobj_multi(const unsigned char* obj)
{
	return trkobj_u8_field(obj, TRACKOBJECT_MULTI_OFFSET);
}

/** @brief Trkobj physical.
 * @param obj Parameter `obj`.
 * @return Function result.
 */
static inline unsigned char trkobj_physical(const unsigned char* obj)
{
	return trkobj_u8_field(obj, TRACKOBJECT_PHYS_OFFSET);
}

/** @brief Update frame.
 * @param view_index Parameter `view_index`.
 * @param clip_rect Parameter `clip_rect`.
 */
void update_frame(int view_index, struct RECTANGLE* clip_rect) {
	int si;
	char base_ts_flags;
	char default_material;
	char crash_overlay_drawn[2];
	struct RECTANGLE* active_rect_ptr;
	struct MATRIX view_rot_mat, y_rot_mat;
	struct MATRIX* rot_mat_ptr;
	struct VECTOR camera_pos, car_pos, temp_vec_a, temp_vec_b, temp_vec_c;
	int car_pitch_raw, car_yaw_raw, car_roll_raw;
	int view_pitch, view_yaw, view_roll_raw;
	int camera_distance_xz, view_roll;
	int render_result;
	int clip_heading;
	char* lookahead_tiles;
	int sky_dir_sign;
	int loop_count, wheel_index;
	int tile_row_bias, tile_col_bias;
	int tile_row, tile_col;
	int tile_row_adj, tile_col_adj;
	int player_tile_col = 0, player_tile_row = 0;
	unsigned char tile_terrain_ids[48];
	char tile_scan_state[48];
	char tile_detail_levels[48];
	char tile_row_table[48];
	char tile_col_table[48];
	unsigned char tile_element_ids[48];
	char clip_region_limit;
	char player_contact_col;
	char player_contact_row = 0;
	char opponent_contact_row = 0;
	char opponent_contact_col;
	char pending_overlay_shape;
	int player_sort_bias;
	int opponent_sort_bias;
	int tile_hill_height;
	int multi_tile_value;
	const unsigned char* trk_object_ptr;
	const unsigned char* trk_object_entry;
	char tile_detail_level;
	signed char* terrain_offset_table = terrain_offset_type2;
	int di = 0;
	int skybox_result;
	int object_center_z;
	int object_center_x;
	short* hill_multitile_offsets = terrain_multitile_type1;
	int start_tile_sort_mask;
	unsigned char obstacle_slot;
	struct RECTANGLE player_crash_rect, opponent_crash_rect;
	struct VECTOR * arrow_shape_verts;
	struct CARSTATE* follow_state_ptr;
	unsigned char elem_map_value;
	unsigned char terr_map_value;

	crash_overlay_drawn[0] = 0;
	crash_overlay_drawn[1] = 0;
	if (video_flag5_is0 == 0 || view_index == 0) {
		rect_buffer_primary = rect_buffer_front;
		rect_buffer_secondary = rect_buffer_back;
	} else {
		rect_buffer_secondary = rect_buffer_front;
		rect_buffer_primary = rect_buffer_back;
	}

	if (timertestflag_copy != 0) {
		base_ts_flags = 8;
		active_rect_ptr = frame_dirty_rects;
		for (si = 0; si < FRAME_RECT_ARRAY_COUNT; si++) {
			*active_rect_ptr = rect_invalid;
			active_rect_ptr++;
		}
	} else {
		base_ts_flags = 0;
	}

	if (followOpponentFlag == 0) {		
		car_pos.x = state.playerstate.car_posWorld1.lx >> 6;
		car_pos.y = state.playerstate.car_posWorld1.ly >> 6;
		car_pos.z = state.playerstate.car_posWorld1.lz >> 6;
		car_pitch_raw = state.playerstate.car_rotate.y;
		car_roll_raw = state.playerstate.car_rotate.z;
		car_yaw_raw = state.playerstate.car_rotate.x;
	} else {
		car_pos.x = state.opponentstate.car_posWorld1.lx >> 6;
		car_pos.y = state.opponentstate.car_posWorld1.ly >> 6;
		car_pos.z = state.opponentstate.car_posWorld1.lz >> 6;
		car_pitch_raw = state.opponentstate.car_rotate.y;
		car_roll_raw = state.opponentstate.car_rotate.z;
		car_yaw_raw = state.opponentstate.car_rotate.x;
	}

	view_yaw = -1;
	view_roll_raw = 0;
	
	if (cameramode == 0) {
		view_yaw = car_yaw_raw & FRAME_ANGLE_MASK;
		view_pitch = car_pitch_raw & FRAME_ANGLE_MASK;
		view_roll_raw   = car_roll_raw & FRAME_ANGLE_MASK;
		rot_mat_ptr = mat_rot_zxy(-car_roll_raw, -car_pitch_raw, -car_yaw_raw, 0);
		temp_vec_a.x = 0;
		temp_vec_a.z = 0;
		temp_vec_a.y = simd_player.car_height - 6;

		mat_mul_vector(&temp_vec_a, rot_mat_ptr, &temp_vec_b);
		camera_pos.x = car_pos.x + temp_vec_b.x;
		camera_pos.y = car_pos.y + temp_vec_b.y;
		camera_pos.z = car_pos.z + temp_vec_b.z;
	} else if (cameramode == 1) {
		camera_pos.x = state.game_vec1[(unsigned char)followOpponentFlag].x;
		camera_pos.z = state.game_vec1[(unsigned char)followOpponentFlag].z;
		camera_pos.y = state.game_vec1[(unsigned char)followOpponentFlag].y;
	} else if (cameramode == 2) {
		temp_vec_a.x = 0;
		temp_vec_a.y = 0;
		temp_vec_a.z = FRAME_CAMERA_LONG_Z;
		rot_mat_ptr = mat_rot_zxy(-car_roll_raw, -car_pitch_raw, -car_yaw_raw, 0);
		mat_mul_vector(&temp_vec_a, rot_mat_ptr, &temp_vec_b);

		temp_vec_a.x = 0;
		temp_vec_a.y = 0;
		temp_vec_a.z = rotation_x_angle;
		rot_mat_ptr = mat_rot_zxy(0, -rotation_y_angle, polarAngle(temp_vec_b.x, temp_vec_b.z) - rotation_z_angle, 0);

		mat_mul_vector(&temp_vec_a, rot_mat_ptr, &temp_vec_b);
		camera_pos.x = car_pos.x + temp_vec_b.x;
		camera_pos.y = car_pos.y + temp_vec_b.y;
		camera_pos.z = car_pos.z + temp_vec_b.z;
	} else if (cameramode == 3) {
		camera_pos.x = waypoint_world_pos[state.game_track_indices[(unsigned char)followOpponentFlag] * 3 + 0];
		camera_pos.y = waypoint_world_pos[state.game_track_indices[(unsigned char)followOpponentFlag] * 3 + 1] + camera_y_offset + FRAME_CAMERA_HEIGHT_OFFSET;
		camera_pos.z = waypoint_world_pos[state.game_track_indices[(unsigned char)followOpponentFlag] * 3 + 2];
	}

	if (view_yaw == -1) {
		build_track_object(&camera_pos, &camera_pos);
		if (camera_pos.y < terrainHeight) {
			camera_pos.y = terrainHeight;
		}

		if (track_object_render_enabled != 0) {		
			si = plane_get_collision_point(planindex, camera_pos.x, camera_pos.y, camera_pos.z);
			if (si < FRAME_TRACK_BORDER_LEVEL) {			
				vec_movement_local.x = 0;
				vec_movement_local.y = FRAME_TRACK_BORDER_LEVEL - si;
				vec_movement_local.z = 0;
				planindex_copy = planindex;
				pState_f36Mminf40sar2 = 0;
				pState_minusRotate_x_2 = 0;
				pState_minusRotate_z_2 = 0;
				pState_minusRotate_y_2 = 0;
				plane_apply_rotation_matrix();
				camera_pos.x += vec_planerotopresult.x;
				camera_pos.y += vec_planerotopresult.y;
				camera_pos.z += vec_planerotopresult.z;
			}
		}

		view_yaw = (-polarAngle(car_pos.x - camera_pos.x, car_pos.z - camera_pos.z)) & FRAME_ANGLE_MASK;
		camera_distance_xz = polarRadius2D(car_pos.x - camera_pos.x, car_pos.z - camera_pos.z);
		view_pitch = polarAngle(car_pos.y - camera_pos.y + FRAME_VIEW_PITCH_OFFSET, camera_distance_xz) & FRAME_ANGLE_MASK;
	}

	if (view_roll_raw > 1 && view_roll_raw < FRAME_ANGLE_MASK) {
		view_roll = view_roll_raw;
	} else {
		view_roll = 0;
	}

	if (state.game_frame == 0) {
		default_material = polygon_edge_direction_table[game_mode_state_register&15];
	} else {
		default_material = polygon_edge_direction_table[state.game_frame&15];
	}

	clip_heading = select_cliprect_rotate(view_roll, view_pitch, view_yaw, clip_rect, 0);
	{
		static char* lookahead_tiles_tables[8] = {
			tile_scan_pattern_octant_0,
			tile_scan_pattern_octant_1,
			tile_scan_pattern_octant_2,
			tile_scan_pattern_octant_3,
			tile_scan_pattern_octant_4,
			tile_scan_pattern_octant_5,
			tile_scan_pattern_octant_6,
			tile_scan_pattern_octant_7
		};
		lookahead_tiles = lookahead_tiles_tables[(clip_heading & FRAME_ANGLE_MASK) >> 7];
	}

	view_rot_mat = *mat_rot_zxy(view_roll, view_pitch, 0, 1);
	temp_vec_a.x = 0;
	temp_vec_a.y = 0;
	temp_vec_a.z = FRAME_SKY_PROBE_Z;
	mat_mul_vector(&temp_vec_a, &view_rot_mat, &temp_vec_c);
	if (temp_vec_c.z > 0) {
		sky_dir_sign = 1;
	} else {
		sky_dir_sign = -1;
	}

	if (timertestflag2 == 0) {
		currenttransshape[0].rectptr = &sky_starfield_rect;
		currenttransshape[0].ts_flags = base_ts_flags | 7;
		currenttransshape[0].rotvec.x = 0;
		currenttransshape[0].rotvec.y = 0;
		currenttransshape[0].shape_visibility_threshold = FRAME_VISIBILITY_FAR * REND_DIST_MULT;
		currenttransshape[0].material = 0;

		for (loop_count = 0; loop_count < 8; loop_count++) {
			si = (animation_duration_table[loop_count] + view_yaw + run_game_random) & FRAME_ANGLE_MASK;
			if (si < 135 || si > 889) {
				mat_rot_y(&y_rot_mat, si);
				temp_vec_a.x = 0;
				temp_vec_a.y = 2790 - camera_pos.y;
				temp_vec_a.z = FRAME_STARFIELD_Z; //15000
				mat_mul_vector(&temp_vec_a, &y_rot_mat, &temp_vec_b);
				temp_vec_b.z = FRAME_STARFIELD_Z; //15000
				mat_mul_vector(&temp_vec_b, &view_rot_mat, &currenttransshape[0].pos);
				if (currenttransshape[0].pos.z > FRAME_SCREEN_HEIGHT) {
					currenttransshape[0].shapeptr = shape3d_from_dos_dseg_offset(off_3BE44[loop_count]);
					currenttransshape[0].rotvec.z = -view_yaw;
					render_result = shape3d_render_transformed(&currenttransshape[0]);
				}
			}
		}
	}
	tile_col_bias = camera_pos.x >> 10;
	tile_row_bias = -((camera_pos.z >> 10) - FRAME_TILE_MAX_INDEX);
	player_tile_col = state.playerstate.car_posWorld1.lx >> 16;
	player_tile_row = FRAME_TILE_MAX_INDEX - (state.playerstate.car_posWorld1.lz >> 16);

	for (si = 0; si < FRAME_TILE_SCAN_COUNT; si++) {
		tile_scan_state[si] = 0;
	}

	clip_region_limit = sprite_clip_region_selector[timertestflag2];
	
	for (si = FRAME_TILE_SCAN_LAST; si >= 0; si--) {
		if (tile_scan_state[si] != 0)
			continue;

		if (lookahead_tiles[si * 3 + 2] <= clip_region_limit) {
			tile_col = lookahead_tiles[si * 3] + tile_col_bias;
			tile_row = lookahead_tiles[si * 3 + 1] + tile_row_bias;
			if (tile_col >= 0 && tile_col <= FRAME_TILE_MAX_INDEX && tile_row >= 0 && tile_row <= FRAME_TILE_MAX_INDEX) {
				elem_map_value = track_elem_map[tile_col + trackrows[tile_row]];
				terr_map_value = track_terrain_map[tile_col + terrainrows[tile_row]];
				
				if (elem_map_value != 0) {

					if (terr_map_value >= 7 && terr_map_value < 11) {
						elem_map_value = subst_hillroad_track(terr_map_value, elem_map_value);
						terr_map_value = 0;
					}
					
					if (elem_map_value == 253) {
						// the item on the top left needs this space
						tile_col--;
						tile_row--;
						elem_map_value = track_elem_map[tile_col + trackrows[tile_row]];
						terr_map_value = track_terrain_map[tile_col + terrainrows[tile_row]];
					} else if (elem_map_value == 254) {
						// the item on the top needs this space
						tile_row--;
						elem_map_value = track_elem_map[tile_col + trackrows[tile_row]];
						terr_map_value = track_terrain_map[tile_col + terrainrows[tile_row]];
					} else if (elem_map_value == 255) {
						// the item on the left needs this space
						tile_col--;
						elem_map_value = track_elem_map[tile_col + trackrows[tile_row]];
						terr_map_value = track_terrain_map[tile_col + terrainrows[tile_row]];
					}
				}

				tile_terrain_ids[si] = terr_map_value;
				tile_detail_levels[si] = lookahead_tiles[si * 3 + 2];

				if (elem_map_value != 0 && timertestflag2 != 0) {
					const unsigned char* elem_obj = trkobj_entry_legacy_scene_index(elem_map_value);
					if (elem_obj != 0 && trkobj_physical(elem_obj) >= 64 &&
						(tile_col != player_tile_col || tile_row != player_tile_row)) {
						elem_map_value = 0;
					}
				}

				tile_col_table[si] = tile_col;
				tile_row_table[si] = tile_row;
				tile_element_ids[si] = elem_map_value;
				if (elem_map_value != 0) {
					const unsigned char* tile_obj = trkobj_entry_legacy_scene_index(elem_map_value);
					if (tile_obj != 0) {
						multi_tile_value = trkobj_multi(tile_obj);
					} else {
						multi_tile_value = 0;
					}
					if (multi_tile_value != 0) {

						tile_col_adj = tile_col - tile_col_bias;
						tile_row_adj = tile_row - tile_row_bias;
						if (multi_tile_value == 1) {
							for (di = 0; di < si; di++) {
								if (lookahead_tiles[di * 3] == tile_col_adj && (lookahead_tiles[di * 3 + 1] == tile_row_adj || lookahead_tiles[di * 3 + 1] == tile_row_adj + 1)) {
									tile_scan_state[di] = 1;
								}
							}
						} else if (multi_tile_value == 2) {
							for (di = 0; di < si; di++) {
								if (lookahead_tiles[di * 3 + 1] == tile_row_adj && (lookahead_tiles[di * 3] == tile_col_adj || lookahead_tiles[di * 3] == tile_col_adj + 1)) {
									tile_scan_state[di] = 1;
								}
							}
						} else if (multi_tile_value == 3) {
							for (di = 0; di < si; di++) {
								if ((lookahead_tiles[di * 3] == tile_col_adj || lookahead_tiles[di * 3] == tile_col_adj + 1) && 
									(lookahead_tiles[di * 3 + 1] == tile_row_adj || lookahead_tiles[di * 3 + 1] == tile_row_adj + 1))
								{
									tile_scan_state[di] = 1;
								}
							}
						}
					}
				}
				
			} else {
				tile_scan_state[si] = 2;
			}
		} else {
			tile_scan_state[si] = 2;
		}
	}
	
//; -----------------------------------------------------------------------------
	
	player_contact_col = -1;
	player_sort_bias = 0;
	if (cameramode != 0 || followOpponentFlag != 0) {

		if (state.playerstate.car_crashBmpFlag != 2) {

			rot_mat_ptr = mat_rot_zxy(-state.playerstate.car_rotate.z, -state.playerstate.car_rotate.y, -state.playerstate.car_rotate.x, 0);
			multi_tile_value = -1;
			di = -1;
			for (wheel_index = 0; wheel_index < 4; wheel_index++) {
				temp_vec_a = simd_player.wheel_coords[wheel_index];
				mat_mul_vector(&temp_vec_a, rot_mat_ptr, &temp_vec_c); //; rotating car wheels, maybe?
				tile_col = (temp_vec_c.x + state.playerstate.car_posWorld1.lx) >> 16; // bits 16-24
				tile_row = -(((temp_vec_c.z + state.playerstate.car_posWorld1.lz) >> 16) - 29);

				for (si = 44; si > multi_tile_value; si--) {
					if (tile_scan_state[si] != 2 && lookahead_tiles[si * 3] + tile_col_bias == tile_col && lookahead_tiles[si * 3 + 1] + tile_row_bias == tile_row) {
						player_contact_col = tile_col;
						player_contact_row = tile_row;
						multi_tile_value = si;
						di = wheel_index;
					}
				}
			}

			if (di != -1) {
				if (state.playerstate.car_surfaceWhl[0] != 4 || state.playerstate.car_surfaceWhl[1] != 4 || state.playerstate.car_surfaceWhl[2] != 4 || state.playerstate.car_surfaceWhl[3] != 4) {
					temp_vec_a.x = 0;
					temp_vec_a.z = 0;
					temp_vec_a.y = 30000;
					mat_mul_vector(&temp_vec_a, rot_mat_ptr, &temp_vec_c);
					mat_mul_vector(&temp_vec_c, &mat_temp, &temp_vec_a);
					if (temp_vec_a.z <= 0) {
						player_sort_bias = -2048 ;
					} else {
						player_sort_bias = 2048;
					}
				}
			}
		}
	}

	opponent_contact_col = -1;
	opponent_sort_bias = 0;
	if (gameconfig.game_opponenttype != 0) {

		if (cameramode != 0 || followOpponentFlag == 0) {
			if (state.opponentstate.car_crashBmpFlag != 2) {
				rot_mat_ptr = mat_rot_zxy(-state.opponentstate.car_rotate.z, -state.opponentstate.car_rotate.y, -state.opponentstate.car_rotate.x, 0);
				multi_tile_value = -1;
				di = -1;

				for (wheel_index = 0; wheel_index < 4; wheel_index++) {
					temp_vec_a = simd_opponent_rt.wheel_coords[wheel_index];
					mat_mul_vector(&temp_vec_a, rot_mat_ptr, &temp_vec_c); //; rotating car wheels, maybe?
					tile_col = (temp_vec_c.x + state.opponentstate.car_posWorld1.lx) >> 16; // bits 16-24
					tile_row = -(((temp_vec_c.z + state.opponentstate.car_posWorld1.lz) >> 16) - 29);

					for (si = 44; si > multi_tile_value; si--) {
						if (tile_scan_state[si] != 2 && lookahead_tiles[si * 3] + tile_col_bias == tile_col && lookahead_tiles[si * 3 + 1] + tile_row_bias == tile_row) {
							opponent_contact_col = tile_col;
							opponent_contact_row = tile_row;
							multi_tile_value = si;
							di = wheel_index;
						}
					}
				}

				if (di != -1) {
						
					if (state.opponentstate.car_surfaceWhl[0] != 4 || state.opponentstate.car_surfaceWhl[1] != 4 || state.opponentstate.car_surfaceWhl[2] != 4 || state.opponentstate.car_surfaceWhl[3] != 4) {
						temp_vec_a.x = 0;
						temp_vec_a.y = 0;
						temp_vec_a.z = 30000;
						mat_mul_vector(&temp_vec_a, rot_mat_ptr, &temp_vec_c);
						mat_mul_vector(&temp_vec_c, &mat_temp, &temp_vec_a);
						if (temp_vec_a.z <= 0) {
							opponent_sort_bias = -2048; //63488; // signed number!
						} else {
							opponent_sort_bias = 2048;
						}
					}
				}
			}
		}
	}
//; -----------------------------------------------------------------------------


	pending_overlay_shape = 0;
	si = 0;
	
	for (si = 0; si < FRAME_TILE_SCAN_COUNT; si++) {
		if (tile_scan_state[si] != 0) {
			continue;
		}
		tile_col = tile_col_table[si];
		tile_row = tile_row_table[si];
		elem_map_value = tile_element_ids[si];
		terr_map_value = tile_terrain_ids[si];
		tile_detail_level = tile_detail_levels[si];
		start_tile_sort_mask = 0;
		loop_count = 1;
		terrain_offset_table = terrain_offset_type2;
		if (elem_map_value == 0) {
			loop_count = 1;
			terrain_offset_table = terrain_offset_type2;
		} else {
			trk_object_entry = trkobj_entry_legacy_scene_index(elem_map_value);
			if (trk_object_entry == 0) {
				elem_map_value = 0;
				loop_count = 1;
				terrain_offset_table = terrain_offset_type2;
			} else if (trkobj_multi(trk_object_entry) == 0) {
				loop_count = 1;
				terrain_offset_table = terrain_offset_type0;
			} else if (trkobj_multi(trk_object_entry) == 1) {
				loop_count = 2;
				terrain_offset_table = terrain_offset_type1;
			} else if (trkobj_multi(trk_object_entry) == 2) {
				loop_count = 3;
				terrain_offset_table = terrain_offset_type2;
			} else if (trkobj_multi(trk_object_entry) == 3) {
				loop_count = 4;
				terrain_offset_table = terrain_offset_type3;
			}
		}

		for (multi_tile_value = 0; multi_tile_value < loop_count; multi_tile_value++) {
			tile_col_adj = terrain_offset_table[multi_tile_value * 2] + tile_col;
			tile_row_adj = terrain_offset_table[multi_tile_value * 2 + 1] + tile_row;
			
			if (timertestflag2 < 3 || (tile_col_adj == player_tile_col && tile_row_adj == player_tile_row)) {
				if (tile_col_adj == 0) {
					if (tile_row_adj == 0) {
						di = 7;
					} else if (tile_row_adj == 29) {
						di = 5;
					} else {
						di = 6;
					}
				} else if (tile_col_adj == 29) {
					if (tile_row_adj == 0) {
						di = 1;
					} else
					if (tile_row_adj == 29) {
						di = 3;
					} else {
						di = 2;
					}
				} else {
					if (tile_row_adj == 0) {
						di = 0;
					} else if (tile_row_adj == 29) {
						di = 4;
					} else {
						di = -1;
					}
				}

				if (di != -1) {
					trk_object_ptr = trkobj_entry_legacy_scene_index(fence_TrkObjCodes[di]);
					if (trk_object_ptr != 0) {
						if (tile_detail_level == 0) {
							currenttransshape[0].shapeptr = trkobj_shape(trk_object_ptr);
						} else {
							currenttransshape[0].shapeptr = trkobj_loshape(trk_object_ptr);
						}

						currenttransshape[0].pos.x = trackcenterpos2[tile_col_adj] - camera_pos.x;
						currenttransshape[0].pos.y = -camera_pos.y;
						currenttransshape[0].pos.z = trackcenterpos[tile_row_adj] - camera_pos.z;
						currenttransshape[0].rectptr = &terrain_rect;
						currenttransshape[0].ts_flags = base_ts_flags | 5 | TRANSFORM_FLAG_TERRAIN_DOUBLE_SIDED;
						currenttransshape[0].rotvec.x = 0;
						currenttransshape[0].rotvec.y = 0;
						currenttransshape[0].rotvec.z = animation_frame_lookup[di];
						currenttransshape[0].shape_visibility_threshold = FRAME_VISIBILITY_FAR * REND_DIST_MULT;
						currenttransshape[0].material = 0;
						render_result = shape3d_render_transformed(&currenttransshape[0]);
						if (render_result > 0) {
							// exit entire tile loop (ASM: jmp loc_1B03C)
							goto exit_tile_loop;
						}
					}
				}
			}
		}

		// terrain type 6: a flat piece of land at an elevated level  
		if (terr_map_value != 6) {
			tile_hill_height = 0;
			if (elem_map_value >= 105 && elem_map_value <= 108) {
				for (multi_tile_value = 0; multi_tile_value < 4; multi_tile_value++) {
					if (multi_tile_value == 0) {
						tile_col_adj = tile_col;
						tile_row_adj = tile_row;
					} else if (multi_tile_value == 1) {
						tile_col_adj = tile_col + 1;
						tile_row_adj = tile_row;
					} else if (multi_tile_value == 2) {
						tile_col_adj = tile_col;
						tile_row_adj = tile_row + 1;
					} else if (multi_tile_value == 3) {
						tile_col_adj = tile_col + 1;
						tile_row_adj = tile_row + 1;
					}
					terr_map_value = track_terrain_map[tile_col_adj + terrainrows[tile_row_adj]];
					if (terr_map_value != 0) {
						trk_object_entry = trkobj_entry(sceneshapes2, terr_map_value);
						currenttransshape[0].shapeptr = trkobj_shape(trk_object_entry);
						currenttransshape[0].pos.x = trackcenterpos2[tile_col_adj] - camera_pos.x;
						currenttransshape[0].pos.y = -camera_pos.y;
						currenttransshape[0].pos.z = trackcenterpos[tile_row_adj] - camera_pos.z;
						currenttransshape[0].rectptr = &terrain_rect;
						currenttransshape[0].ts_flags = base_ts_flags | 5 | TRANSFORM_FLAG_TERRAIN_DOUBLE_SIDED;
						currenttransshape[0].rotvec.x = 0;
						currenttransshape[0].rotvec.y = 0;
						currenttransshape[0].rotvec.z = trkobj_roty(trk_object_entry);
						currenttransshape[0].shape_visibility_threshold = FRAME_VISIBILITY_FAR * REND_DIST_MULT;
						currenttransshape[0].material = 0;
						render_result = shape3d_render_transformed(&currenttransshape[0]);
						if (render_result > 0) {
							// exit entire tile loop (ASM: jmp loc_1B03C)
							goto exit_tile_loop;
						}
					}
				}
				
				terr_map_value = 0;
			}
		} else {
			tile_hill_height = hillHeightConsts[1];
			if (elem_map_value != 0) {
				terr_map_value = 0;
			}
		}

		if (terr_map_value != 0) {
			trk_object_entry = trkobj_entry(sceneshapes2, terr_map_value);
			currenttransshape[0].shapeptr = trkobj_shape(trk_object_entry);
			currenttransshape[0].pos.x = trackcenterpos2[tile_col] - camera_pos.x;
			currenttransshape[0].pos.y = tile_hill_height - camera_pos.y;
			currenttransshape[0].pos.z = trackcenterpos[tile_row] - camera_pos.z;
			if (tile_hill_height == 0) {
				currenttransshape[0].rectptr = &terrain_rect;
			} else {
				currenttransshape[0].rectptr = &world_object_rect;
			}

			currenttransshape[0].ts_flags = base_ts_flags | 5 | TRANSFORM_FLAG_TERRAIN_DOUBLE_SIDED;
			currenttransshape[0].rotvec.x = 0;
			currenttransshape[0].rotvec.y = 0;
			currenttransshape[0].rotvec.z = trkobj_roty(trk_object_entry);
			currenttransshape[0].shape_visibility_threshold = FRAME_VISIBILITY_FAR * REND_DIST_MULT;
			currenttransshape[0].material = 0;
			render_result = shape3d_render_transformed(&currenttransshape[0]);
			if (render_result > 0) {
				break;
			}
		}

		transformedshape_counter = 0;
		curtransshape_ptr = currenttransshape;
		if (elem_map_value == 0) {
			tile_col_adj = tile_col;
			tile_row_adj = tile_row;
		} else {
			trk_object_entry = trkobj_entry_legacy_scene_index(elem_map_value);
			if (trk_object_entry == 0) {
				continue;
			}
			if ((trkobj_multi(trk_object_entry) & 1) != 0) {
				object_center_z = trackpos[tile_row];
				tile_row_adj = tile_row + 1;
			} else {
				object_center_z = trackcenterpos[tile_row];
				tile_row_adj = tile_row;
			}

			if ((trkobj_multi(trk_object_entry) & 2) != 0) {
				object_center_x = trackpos2[1 + tile_col];
				tile_col_adj = tile_col + 1;
			} else {
				object_center_x = trackcenterpos2[tile_col];
				tile_col_adj = tile_col;
			}

			temp_vec_c.x = object_center_x - camera_pos.x;
			temp_vec_c.y = tile_hill_height - camera_pos.y;
			temp_vec_c.z = object_center_z - camera_pos.z;
			if (tile_hill_height != 0) {
				di = 1;
				hill_multitile_offsets = terrain_multitile_type1;
				if (trkobj_multi(trk_object_entry) == 0) {
					di = 1;
					hill_multitile_offsets = terrain_multitile_type1;
				} else if (trkobj_multi(trk_object_entry) == 1) {
					di = 2;
					hill_multitile_offsets = terrain_multitile_type2;
				} else if (trkobj_multi(trk_object_entry) == 2) {
					di = 2;
					hill_multitile_offsets = terrain_multitile_type3;
				} else if (trkobj_multi(trk_object_entry) == 3) {
					di = 4;
					hill_multitile_offsets = terrain_multitile_type4;
				}

				for (multi_tile_value = 0; multi_tile_value < di; multi_tile_value++) {
					currenttransshape[0].pos.x = *hill_multitile_offsets + temp_vec_c.x;
					hill_multitile_offsets++;
					currenttransshape[0].pos.y = temp_vec_c.y;
					currenttransshape[0].pos.z = *hill_multitile_offsets + temp_vec_c.z;
					hill_multitile_offsets++;
					currenttransshape[0].shapeptr = &game3dshapes[946 / GAME3DSHAPES_DOS_STRIDE];
					currenttransshape[0].rectptr = &world_object_rect;
					currenttransshape[0].ts_flags = base_ts_flags | 5;
					currenttransshape[0].rotvec.x = 0;
					currenttransshape[0].rotvec.y = 0;
					currenttransshape[0].rotvec.z = 0;
					currenttransshape[0].shape_visibility_threshold = FRAME_VISIBILITY_HILL * REND_DIST_MULT;
					currenttransshape[0].material = 0;
					render_result = shape3d_render_transformed(&currenttransshape[0]);
					if (render_result > 0)
						goto exit_tile_loop; // exit entire tile loop (ASM: jmp loc_1B03C)
				}
			}

			if (trkobj_overlay(trk_object_entry) != 0) {
				trk_object_ptr = trkobj_entry_legacy_scene_index(trkobj_overlay(trk_object_entry));
				if (trk_object_ptr != 0) {
					if (tile_detail_level != 0) {
						currenttransshape[1].shapeptr = trkobj_loshape(trk_object_ptr);
					} else {
						currenttransshape[1].shapeptr = trkobj_shape(trk_object_ptr);
					}

					if (currenttransshape[1].shapeptr != 0) {
						currenttransshape[1].pos = temp_vec_c;
						currenttransshape[1].rotvec.x = 0;
						currenttransshape[1].rotvec.y = 0;
						currenttransshape[1].rotvec.z = trkobj_roty(trk_object_ptr);
						if (trkobj_multi(trk_object_ptr) != 0) {
							currenttransshape[1].shape_visibility_threshold = FRAME_VISIBILITY_FAR * REND_DIST_MULT;
						} else {
							currenttransshape[1].shape_visibility_threshold = FRAME_VISIBILITY_HILL * REND_DIST_MULT;
						}

						if (trkobj_surface(trk_object_ptr) >= 0) {
							currenttransshape[1].material = trkobj_surface(trk_object_ptr);
						} else {
							currenttransshape[1].material = default_material;
						}

						currenttransshape[1].ts_flags = trkobj_ignore_zbias(trk_object_ptr) | base_ts_flags | 4;
						if ((currenttransshape[1].ts_flags & 1) != 0) {
							currenttransshape[1].rectptr = &terrain_rect;
							render_result = shape3d_render_transformed(&currenttransshape[1]);
							if (render_result > 0)
								break;
						} else {
							currenttransshape[1].rectptr = &world_object_rect;
							pending_overlay_shape = 1;
						}
					}
				}
			}

			if (tile_detail_level != 0) {
				currenttransshape[0].shapeptr = trkobj_loshape(trk_object_entry);
			} else {
				currenttransshape[0].shapeptr = trkobj_shape(trk_object_entry);
			}

			currenttransshape[0].pos = temp_vec_c; // whatever
			currenttransshape[0].rotvec.x = 0;
			currenttransshape[0].rotvec.y = 0;
			currenttransshape[0].rotvec.z = trkobj_roty(trk_object_entry);
			if (trkobj_multi(trk_object_entry) != 0) {
				currenttransshape[0].shape_visibility_threshold = FRAME_VISIBILITY_FAR * REND_DIST_MULT;
			} else {
				currenttransshape[0].shape_visibility_threshold = FRAME_VISIBILITY_HILL * REND_DIST_MULT;
			}

			currenttransshape[0].ts_flags = trkobj_ignore_zbias(trk_object_entry) | base_ts_flags | 4;
			if (trkobj_surface(trk_object_entry) >= 0) {
				currenttransshape[0].material = trkobj_surface(trk_object_entry);
			} else {
				currenttransshape[0].material = default_material;
			}

			if ((trkobj_ignore_zbias(trk_object_entry) & 1) != 0) {
				currenttransshape[0].rectptr = &terrain_rect;
				render_result = shape3d_render_transformed(&currenttransshape[0]);
				if (render_result > 0)
					break;

				/* If overlay was queued as pending, flush it here too.
				   Otherwise this tile loses its markings when main shape
				   uses the immediate terrain-render path. */
				if (pending_overlay_shape != 0) {
					pending_overlay_shape = 0;
					render_result = shape3d_render_transformed(&currenttransshape[1]);
					if (render_result > 0)
						break;
				}
			} else {
				currenttransshape[0].rectptr = &world_object_rect;
				transformed_shape_add_for_sort(0, 0);
				if (pending_overlay_shape != 0) {
					pending_overlay_shape = 0;
					transformed_shape_add_for_sort(-FRAME_SORT_BIAS /*63488*/, 0);
					if (player_sort_bias != 0) {
						player_sort_bias = -FRAME_SORT_BIAS_OVERLAY;//64512;
					}

					if (opponent_sort_bias != 0) {
						opponent_sort_bias -= FRAME_SORT_BIAS_OVERLAY;
					}
				}

				if (tile_col == startcol2 && tile_row == startrow2) {
					start_tile_sort_mask = 0;
				} else {
					start_tile_sort_mask = -1;
				}
			}

			obstacle_slot = tile_obstacle_map[tile_col + trackrows[tile_row]];
			if (obstacle_slot != 255) {
				if (state.game_obstacle_flags[obstacle_slot] == 0) {
					trk_object_entry = trkobj_entry_legacy_scene_index(212 + (unsigned)obstacle_scene_index[obstacle_slot]);
					if (trk_object_entry == 0) {
						continue;
					}
					curtransshape_ptr->pos.x = obstacle_world_pos[obstacle_slot * 3 + 0] - camera_pos.x;
					curtransshape_ptr->pos.y = obstacle_world_pos[obstacle_slot * 3 + 1] - camera_pos.y;
					curtransshape_ptr->pos.z = obstacle_world_pos[obstacle_slot * 3 + 2] - camera_pos.z;
					curtransshape_ptr->shapeptr = trkobj_shape(trk_object_entry);
					curtransshape_ptr->rectptr = &world_object_rect;
					curtransshape_ptr->ts_flags = base_ts_flags | 4;
					curtransshape_ptr->rotvec.x = 0;
					curtransshape_ptr->rotvec.y = 0;
					curtransshape_ptr->rotvec.z = obstacle_rot_z[obstacle_slot];
					curtransshape_ptr->shape_visibility_threshold = FRAME_VISIBILITY_NEAR * REND_DIST_MULT;
					curtransshape_ptr->material = 0;
					transformed_shape_add_for_sort(0, 0);
				} else if (state.game_obstacle_count != 0) {
					for (di = 0; di < 24; di++) {
						if (state.game_obstacle_active[di] != 0 && obstacle_slot + 2 == state.game_obstacle_status[di]) {
							trk_object_entry = trkobj_entry(sceneshapes3, state.game_obstacle_shape[di]);
							curtransshape_ptr->pos.x = (state.game_longs1[di] >> 6) + obstacle_world_pos[obstacle_slot * 3 + 0] - camera_pos.x;
							curtransshape_ptr->pos.y = (state.game_longs2[di] >> 6) + obstacle_world_pos[obstacle_slot * 3 + 1] - camera_pos.y;
							curtransshape_ptr->pos.z = (state.game_longs3[di] >> 6) + obstacle_world_pos[obstacle_slot * 3 + 2] - camera_pos.z;
							curtransshape_ptr->shapeptr = trkobj_shape(trk_object_entry);
							curtransshape_ptr->rectptr = &world_object_rect;
							curtransshape_ptr->ts_flags = base_ts_flags | 5;
							curtransshape_ptr->rotvec.x = -state.game_obstacle_rotx[di];
							curtransshape_ptr->rotvec.y = -state.game_obstacle_roty[di];
							curtransshape_ptr->rotvec.z = -state.game_obstacle_rotz[di];
							curtransshape_ptr->shape_visibility_threshold = FRAME_VISIBILITY_FAR * REND_DIST_MULT;
							curtransshape_ptr->material = 0;
							transformed_shape_add_for_sort(0, 0);
						}
					}
				}
			}
		}

		if ((player_contact_col == tile_col || player_contact_col == tile_col_adj) && (player_contact_row == tile_row || player_contact_row == tile_row_adj)) {
			if (state.game_obstacle_count != 0) {
				for (di = 0; di < 24; di++) {
					if (state.game_obstacle_active[di] != 0 && state.game_obstacle_status[di] == 0) {
						trk_object_entry = trkobj_entry(sceneshapes3, state.game_obstacle_shape[di]);
						curtransshape_ptr->pos.x = ((state.game_longs1[di] + state.playerstate.car_posWorld1.lx) >> 6) - camera_pos.x;
						curtransshape_ptr->pos.y = ((state.game_longs2[di] + state.playerstate.car_posWorld1.ly) >> 6) - camera_pos.y;
						curtransshape_ptr->pos.z = ((state.game_longs3[di] + state.playerstate.car_posWorld1.lz) >> 6) - camera_pos.z;
						curtransshape_ptr->shapeptr = trkobj_shape(trk_object_entry);
						curtransshape_ptr->rectptr = &world_object_rect;
						curtransshape_ptr->ts_flags = base_ts_flags | 5;
						curtransshape_ptr->rotvec.x = -state.game_obstacle_rotx[di];
						curtransshape_ptr->rotvec.y = -state.game_obstacle_roty[di];
						curtransshape_ptr->rotvec.z = -state.game_obstacle_rotz[di];
						curtransshape_ptr->shape_visibility_threshold = FRAME_VISIBILITY_FAR * REND_DIST_MULT;
						curtransshape_ptr->material = gameconfig.game_playermaterial;
						transformed_shape_add_for_sort(player_sort_bias & start_tile_sort_mask, 0);
					}
				}
			}

			trk_object_entry = trkobj_entry(trkObjectList, 2);
			curtransshape_ptr->pos.x = (state.playerstate.car_posWorld1.lx >> 6) - camera_pos.x;
			curtransshape_ptr->pos.y = (state.playerstate.car_posWorld1.ly >> 6) - camera_pos.y;
			curtransshape_ptr->pos.z = (state.playerstate.car_posWorld1.lz >> 6) - camera_pos.z;
			
			if (tile_detail_level != 0 || timertestflag2 > 2) {
				curtransshape_ptr->shapeptr = trkobj_loshape(trk_object_entry);
			} else {
				curtransshape_ptr->shapeptr = trkobj_shape(trk_object_entry);
				shape3d_update_car_wheel_vertices(&game3dshapes[2772 / GAME3DSHAPES_DOS_STRIDE].shape3d_verts[8], state.playerstate.car_steeringAngle, state.playerstate.car_rc2, viewport_clipping_bounds, carshapevecs, &carshapevec);
			}

			if (timertestflag_copy != 0) {
				curtransshape_ptr->rectptr = &player_car_rect;
				curtransshape_ptr->ts_flags = FRAME_FLAG_TRACKED_SHAPE;
			} else if (state.playerstate.car_crashBmpFlag != 1) {
				curtransshape_ptr->ts_flags = 4;
			} else {
				player_crash_rect = rect_invalid;
				curtransshape_ptr->rectptr = &player_crash_rect;
				curtransshape_ptr->ts_flags = FRAME_FLAG_TRACKED_SHAPE;
			}

			curtransshape_ptr->rotvec.x = -state.playerstate.car_rotate.z;
			curtransshape_ptr->rotvec.y = -state.playerstate.car_rotate.y;
			curtransshape_ptr->rotvec.z = -state.playerstate.car_rotate.x;
			curtransshape_ptr->shape_visibility_threshold = FRAME_VISIBILITY_MEDIUM * REND_DIST_MULT;
			curtransshape_ptr->material = gameconfig.game_playermaterial;
			transformed_shape_add_for_sort(player_sort_bias & start_tile_sort_mask, 2);
		}
		
		if ((opponent_contact_col == tile_col) || (opponent_contact_col == tile_col_adj)) {
			if ((opponent_contact_row == tile_row) || (opponent_contact_row == tile_row_adj)) {
				if (state.game_obstacle_count != 0) {
					for (di = 0; di < 24; di++) {
						if (state.game_obstacle_active[di] != 0) {
							if (state.game_obstacle_status[di] == 1) {
								trk_object_entry = trkobj_entry(sceneshapes3, state.game_obstacle_shape[di]);
								curtransshape_ptr->pos.x = ((state.game_longs1[di] + state.opponentstate.car_posWorld1.lx) >> 6) - camera_pos.x;
								curtransshape_ptr->pos.y = ((state.game_longs2[di] + state.opponentstate.car_posWorld1.ly) >> 6) - camera_pos.y;
								curtransshape_ptr->pos.z = ((state.game_longs3[di] + state.opponentstate.car_posWorld1.lz) >> 6) - camera_pos.z;
								curtransshape_ptr->shapeptr = trkobj_shape(trk_object_entry);
								curtransshape_ptr->rectptr = &world_object_rect;
								curtransshape_ptr->ts_flags = base_ts_flags | 5;
								curtransshape_ptr->rotvec.x = -state.game_obstacle_rotx[di];
								curtransshape_ptr->rotvec.y = -state.game_obstacle_roty[di];
								curtransshape_ptr->rotvec.z = -state.game_obstacle_rotz[di];
								curtransshape_ptr->shape_visibility_threshold = FRAME_VISIBILITY_FAR * REND_DIST_MULT;
								curtransshape_ptr->material = gameconfig.game_opponentmaterial;
								transformed_shape_add_for_sort(opponent_sort_bias & start_tile_sort_mask, 0);
							}
						}
					}
				}
				trk_object_entry = trkobj_entry(trkObjectList, 3);
				curtransshape_ptr->pos.x = (state.opponentstate.car_posWorld1.lx >> 6) - camera_pos.x;
				curtransshape_ptr->pos.y = (state.opponentstate.car_posWorld1.ly >> 6) - camera_pos.y;
				curtransshape_ptr->pos.z = (state.opponentstate.car_posWorld1.lz >> 6) - camera_pos.z;

				if (tile_detail_level != 0 || timertestflag2 > 2) {
					curtransshape_ptr->shapeptr = trkobj_loshape(trk_object_entry);
				} else {
					curtransshape_ptr->shapeptr = trkobj_shape(trk_object_entry);
					shape3d_update_car_wheel_vertices(&game3dshapes[2794 / GAME3DSHAPES_DOS_STRIDE].shape3d_verts[8], state.opponentstate.car_steeringAngle, state.opponentstate.car_rc2, game_frame_pointer, oppcarshapevecs, &oppcarshapevec);
				}

				if (timertestflag_copy != 0) {
					curtransshape_ptr->rectptr = &opponent_car_rect;
					curtransshape_ptr->ts_flags = FRAME_FLAG_TRACKED_SHAPE;
				} else {
					if (state.opponentstate.car_crashBmpFlag != 1) {
						curtransshape_ptr->ts_flags = 4;
					} else {
						opponent_crash_rect = rect_invalid;
						curtransshape_ptr->rectptr = &opponent_crash_rect;
						curtransshape_ptr->ts_flags = FRAME_FLAG_TRACKED_SHAPE;
					}
				}

				curtransshape_ptr->rotvec.x = -state.opponentstate.car_rotate.z;
				curtransshape_ptr->rotvec.y = -state.opponentstate.car_rotate.y;
				curtransshape_ptr->rotvec.z = -state.opponentstate.car_rotate.x;
				curtransshape_ptr->shape_visibility_threshold = FRAME_VISIBILITY_MEDIUM * REND_DIST_MULT;
				curtransshape_ptr->material = gameconfig.game_opponentmaterial;
				transformed_shape_add_for_sort(opponent_sort_bias & start_tile_sort_mask, 3);
			}
		}

		if (state.game_inputmode == 0) {
			if ((tile_col == startcol2 || tile_col_adj == startcol2) && (tile_row == startrow2 || tile_row_adj == startrow2)) {

				multi_tile_value = multiply_and_scale(cos_fast(current_rotation_angle_value), FRAME_START_ARROW_RADIUS);
				loop_count = multiply_and_scale(sin_fast(current_rotation_angle_value), FRAME_START_ARROW_RADIUS) + FRAME_START_ARROW_Z_BIAS;

				arrow_shape_verts = &game3dshapes[2442 / GAME3DSHAPES_DOS_STRIDE].shape3d_verts[8];
				arrow_shape_verts[0].x = multi_tile_value - FRAME_START_ARROW_RADIUS;
				arrow_shape_verts[1].x = multi_tile_value - FRAME_START_ARROW_RADIUS;
				arrow_shape_verts[2].x = FRAME_START_ARROW_RADIUS - multi_tile_value;
				arrow_shape_verts[3].x = FRAME_START_ARROW_RADIUS - multi_tile_value;

				arrow_shape_verts[0].z = loop_count;
				arrow_shape_verts[1].z = loop_count;
				arrow_shape_verts[2].z = loop_count;
				arrow_shape_verts[3].z = loop_count;
				 
				curtransshape_ptr->pos.x =
					multiply_and_scale(sin_fast(track_angle + 256), FRAME_START_ARROW_RADIUS) +
					multiply_and_scale(sin_fast(track_angle + 512), FRAME_START_ARROW_FORWARD) + 
					trackcenterpos2[(unsigned char)startcol2] - camera_pos.x;
				curtransshape_ptr->pos.y = hillHeightConsts[(unsigned char)hillFlag] - camera_pos.y;
				curtransshape_ptr->pos.z =
					multiply_and_scale(cos_fast(track_angle + 256), FRAME_START_ARROW_RADIUS) +
					multiply_and_scale(cos_fast(track_angle + 512), FRAME_START_ARROW_FORWARD) + 
					trackcenterpos[(unsigned char)startrow2] - camera_pos.z;

				curtransshape_ptr->shapeptr = &game3dshapes[2442 / GAME3DSHAPES_DOS_STRIDE];
				curtransshape_ptr->rectptr = &world_object_rect;
				curtransshape_ptr->ts_flags = base_ts_flags | 4;
				curtransshape_ptr->rotvec.x = 0;
				curtransshape_ptr->rotvec.y = 0;
				curtransshape_ptr->rotvec.z = track_angle;
				curtransshape_ptr->shape_visibility_threshold = FRAME_VISIBILITY_FAR * REND_DIST_MULT;
				multi_tile_value = current_rotation_angle_value >> 6;
				if (multi_tile_value > 3) {
					multi_tile_value = 3;
				}

				curtransshape_ptr->material = multi_tile_value;	
				transformed_shape_add_for_sort(start_tile_sort_mask & -FRAME_SORT_BIAS /*63488*/, 0);
			}
		}

		if (transformedshape_counter != 0) {
			if (transformedshape_counter > 1) {
				transformedshape_sort_desc(transformedshape_zarray, transformedshape_indices, (unsigned char)transformedshape_counter);
			}
			for (multi_tile_value = 0; multi_tile_value < transformedshape_counter; multi_tile_value++) {
				// di is used for index into currenttransshape elsewhere
				di = transformedshape_indices[multi_tile_value];
				if (transformedshape_arg2array[di] == 2) {
					if (state.playerstate.car_is_braking != 0) {
						backlights_paint_override = 47;
					} else {
						backlights_paint_override = 46;
					}
				} else if (transformedshape_arg2array[di] == 3) {
					if (state.opponentstate.car_is_braking == 0) {
						backlights_paint_override = 46;
					} else {
						backlights_paint_override = 47;
					}
				}

				render_result = shape3d_render_transformed(&currenttransshape[di]); // DI??
				if (render_result > 0) {
					// exit entire tile loop (ASM: jmp loc_1B03C)
					goto exit_tile_loop;
				}

				if (render_result == 0) {
					if (transformedshape_arg2array[di] == 2) {
						if (state.playerstate.car_crashBmpFlag == 1) {
							crash_overlay_drawn[0] = 1;
						}
					} else if (transformedshape_arg2array[di] == 3) {
						if (state.opponentstate.car_crashBmpFlag == 1) {
							crash_overlay_drawn[1] = 1;
						}
					}
				}
			}
		}
	}
exit_tile_loop:

	skybox_result = render_skybox_layer(view_index, clip_rect, sky_dir_sign, &view_rot_mat, view_roll, view_yaw, camera_pos.y);

	sprite_set_1_size(0, FRAME_SCREEN_WIDTH, clip_rect->top, clip_rect->bottom);

	get_a_poly_info();
	for (si = 0; si < 2; si++) {
		if (crash_overlay_drawn[si] == 0) {
			continue;
		}
		if (timertestflag_copy == 0) {
			if (si == 0) {
				active_rect_ptr = &player_crash_rect;
			} else {
				active_rect_ptr = &opponent_crash_rect;
			}
		} else {
			if (si == 0) {
				active_rect_ptr = &player_car_rect;
			} else {
				active_rect_ptr = &opponent_car_rect;
			}
		}

		if (rect_intersect(active_rect_ptr, clip_rect) == 0) {
			sprite_set_1_size(active_rect_ptr->left, active_rect_ptr->right, active_rect_ptr->top, active_rect_ptr->bottom);
			temp_vec_a.x = (active_rect_ptr->right + active_rect_ptr->left) >> 1;
			temp_vec_a.y = (active_rect_ptr->top + active_rect_ptr->bottom) >> 1;
			multi_tile_value = active_rect_ptr->right - active_rect_ptr->left;
			loop_count = active_rect_ptr->bottom - active_rect_ptr->top;
			if (loop_count > multi_tile_value) {
				multi_tile_value = loop_count;
			}

			di = (state.game_frame >> 2) % 3 ;
			loop_count = ((long)multi_tile_value << 8) / (long)sdgame2_widths[di];
			shape_op_explosion(loop_count, sdgame2shapes[di], temp_vec_a.x, temp_vec_a.y);
		}
	}

/*
; --------------------------------------------------------
*/

	sprite_set_1_size(0, FRAME_SCREEN_WIDTH, clip_rect->top, clip_rect->bottom);
	if (cameramode == 0) {

		if (followOpponentFlag != 0) {
			follow_state_ptr = &state.opponentstate;
			si = state.game_oEndFrame;
		} else {
			follow_state_ptr = &state.playerstate;
			si = state.game_pEndFrame;
		}

		if (follow_state_ptr->car_crashBmpFlag == 1) {
			if (timertestflag_copy != 0) {
				rect_union(init_crak(state.game_frame - si, clip_rect->top, clip_rect->bottom - clip_rect->top), frame_dirty_rects, frame_dirty_rects);
			} else {
				init_crak(state.game_frame - si, clip_rect->top, clip_rect->bottom - clip_rect->top);
			}
		} else if (follow_state_ptr->car_crashBmpFlag == 2) {
			if (timertestflag_copy != 0) {
				rect_union(do_sinking(state.game_frame - si, clip_rect->top, clip_rect->bottom - clip_rect->top), frame_dirty_rects, frame_dirty_rects);
			} else {
				do_sinking(state.game_frame - si, clip_rect->top, clip_rect->bottom - clip_rect->top);
			}
		}
	}

	if (game_replay_mode == 0) {
		if (state.game_inputmode != 0) {
			format_frame_as_string(resID_byte1, elapsed_time1 + replay_frame_counter, 0);
			font_set_fontdef2(fontledresptr);
			if (timertestflag_copy != 0) {
				rect_union(intro_draw_text(resID_byte1, 140, roofbmpheight + 2, 15, 0), &hud_timer_rect, &hud_timer_rect);
			} else {
				intro_draw_text(resID_byte1, 140, roofbmpheight + 2, 15, 0);
			}

			font_set_fontdef();
		}
	}

	if (timertestflag_copy != 0) {
		rect_union(draw_ingame_text(), frame_dirty_rects, frame_dirty_rects);
		if (skybox_result != 0) {
			frame_dirty_rects[0] = *clip_rect;
			for (si = 1; si < FRAME_RECT_ARRAY_COUNT; si++) {
				frame_dirty_rects[si] = rect_invalid;
			}
		}

		for (si = 0; si < FRAME_RECT_ARRAY_COUNT; si++) {
			rect_buffer_primary[si] = frame_dirty_rects[si];
		}
		angle_rotation_state[view_index] = view_yaw;
		sprite_transformation_angle = view_yaw;

	} else {
		draw_ingame_text();
	}

}
