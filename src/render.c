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

/* render.c - Ported from seg003.asm
 * Contains: render_present_ingame_view, init_rect_arrays, draw_skybox_rect_slice, skybox_op,
 *           transformed_shape_add_for_sort, draw_track_preview,
 *           draw_ingame_text, do_sinking, init_crak, load_skybox,
 *           unload_skybox, load_sdgame2_shapes, free_sdgame2,
 *           setup_intro, intro_op
 */

/* Render distance multiplier for modern hardware */
#ifndef REND_DIST_MULT
#define REND_DIST_MULT 8
#endif

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "stunts.h"
#include "math.h"
#include "shape3d.h"
#include "shape2d.h"
#include "memmgr.h"
#include "ressources.h"
#include "keyboard.h"
#include "font.h"

/* Variables moved from data_game.c */
static char dirty_rect_count = 0;
static void * sdgame2ptr = 0;
static short skybox_wat_color = 0;
static void * skyboxes[] = { 0, 0, 0, 0 };
static char texture_page_index = 0;

/* Variables moved from data_game.c (private to this translation unit) */
static short angular_velocity_state = 0;
static short collision_detection_state = 0;
static struct RECTANGLE dirty_rect_array[45] = { { 0 } };
static short dirty_rect_indices[45] = { 0 };
static struct RECTANGLE intro_dirty_clip_rect = { 0, 0, 0, 0 };
static short memory_pointer_boundary_max = 0;
static short physics_constants_table = 16;
static struct RECTANGLE rect_skybox = { 0, 0, 0, 0 };
static char rect_sort_indices[15] = { 0 };
static short skybox_current = 0;
static short skybox_ptr1 = 0;
static short skybox_ptr2 = 0;
static short skybox_ptr3 = 0;
static short skybox_ptr4 = 0;
static short skybox_sky_color = 0;


/* file-local data (moved from data_global.c) */
static short angle_sine_table_start = (short)15360;
static short angle_sine_table_offset = (short)20200;
static short angle_sine_table_stride = (short)62736;
static short angle_correction_offset_x = (short)15360;
static short angle_correction_offset_y = (short)2800;
static short angle_correction_offset_z = (short)10960;
static unsigned char aDm1[] = "dm1";
static unsigned char aDm2[] = "dm2";
static unsigned char aPre[] = "pre";
static unsigned char aSe1[] = "se1";
static unsigned char aSe2[] = "se2";
static unsigned char aWww[] = "www";
static unsigned char aOpp[] = "opp";
static unsigned char aOpp_0[] = "opp";
static unsigned char aPen[] = "pen";
static unsigned char aRpl_0[] = "rpl";
static unsigned char aCrak[] = "crak";
static unsigned char aCinf[] = "cinf";
static unsigned char aScensce2sce3sce4[] = "scensce2sce3sce4";
static unsigned char aSdgame2[] = "sdgame2";
static unsigned char aEx01ex02ex03leftrigh[] = "ex01ex02ex03leftrigh";
static unsigned char aTitle[] = "title";
static unsigned char aLogolog2brav[] = "logolog2brav";
static unsigned char aCarcoun_0[] = "carcoun";
static unsigned char preview_camera_forward_vector[6] = { 0, 0, 140, 216, 120, 65 };
static short intro_colorvalue = 1;

/* External variables from dseg */
static struct RECTANGLE fullscreen_rect = { 0, 320, 0, 200 };


static struct RECTANGLE rect_ingame_text = { 0, 0, 0, 0 };
static struct RECTANGLE rect_ingame_text2 = { 148, 172, 93, 108 };
static struct RECTANGLE rect_ingame_text3 = { 68, 92, 113, 128 };
static struct RECTANGLE rect_ingame_text4 = { 228, 252, 113, 128 };

/* skybox_res pointer - replaces seg/ofs pair */
static char * g_skybox_res_ptr = NULL;
static char mouse_button_state_vector = 0;


static struct SHAPE3D intro_logoshape;
static struct SHAPE3D intro_logo2shape;
static struct SHAPE3D intro_bravshape;


static struct RECTANGLE trackpreview_cliprect = { 0, 320, 0, 200 };

enum { TRACKOBJECT_RAW_SIZE = 14 };

/**
 * @brief Return a pointer to a track-object table entry by index.
 *
 * @param table  Base of the track-object table.
 * @param index  Entry index.
 * @return Pointer to the 14-byte raw entry.
 */
static inline const unsigned char* trkobj_entry(const unsigned char* table, unsigned index)
{
    return table + index * TRACKOBJECT_RAW_SIZE;
}

/**
 * @brief Return the raw byte pointer of a track-object entry.
 *
 * @param obj  Track-object entry pointer.
 * @return Same pointer cast to unsigned char.
 */
static inline const unsigned char* trkobj_raw(const unsigned char* obj)
{
    return (const unsigned char*)obj;
}

/**
 * @brief Extract the primary shape offset from a track-object entry.
 *
 * @param obj  Track-object entry pointer.
 * @return 16-bit DOS dseg offset of the shape.
 */
static inline unsigned short trkobj_ofs_shape(const unsigned char* obj)
{
    const unsigned char* raw = trkobj_raw(obj);
    return (unsigned short)raw[4] | ((unsigned short)raw[5] << 8);
}

/**
 * @brief Extract the low-detail shape offset from a track-object entry.
 *
 * @param obj  Track-object entry pointer.
 * @return 16-bit DOS dseg offset of the low-detail shape.
 */
static inline unsigned short trkobj_ofs_loshape(const unsigned char* obj)
{
    const unsigned char* raw = trkobj_raw(obj);
    return (unsigned short)raw[6] | ((unsigned short)raw[7] << 8);
}

/*
 * DOS dseg layout: game3dshapes[0] resides at offset 30284 within dseg,
 * each SHAPE3D is 22 bytes (22) in the original 16-bit binary.
 * Convert a stored dseg offset to a game3dshapes[] pointer.
 */
#define GAME3DSHAPES_DOS_BASE   30284u
#define GAME3DSHAPES_DOS_STRIDE 22u
#define GAME3DSHAPES_MAX        130

enum {
    RENDER_SCREEN_LEFT = 0,
    RENDER_SCREEN_TOP = 0,
    RENDER_SCREEN_WIDTH = 320,
    RENDER_SCREEN_HEIGHT = 200,
    RENDER_SCREEN_RIGHT_EDGE = 319,
    RENDER_ANGLE_MASK = 1023,
    RENDER_ANGLE_HALF_TURN = 512,
    RENDER_ANGLE_FULL_TURN = 1024,
    RENDER_HORIZON_VECTOR_X = 18000,
    RENDER_HORIZON_VECTOR_NEG_X = 47536,
    RENDER_HORIZON_VECTOR_Z = 15000,
    RENDER_MAX_STRIP_COUNT = 32,
    RENDER_WRAP_DELTA_MAX = 96,
    RENDER_INVALID_POINT_COORD = 32768,
    RENDER_RECT_ARRAY_COUNT = 15,
    RENDER_SKYBOX_COUNT = 4,
    RENDER_TRACK_GRID_SIZE = 30,
    RENDER_STAR_COUNT = 100,
    RENDER_COLOR_INDEX_SKY = 17,
    RENDER_COLOR_INDEX_GROUND = 16,
    RENDER_COLOR_INDEX_WATER = 100,
    RENDER_MATERIAL_ENTRY_COUNT = 129,
    RENDER_MATERIAL_ENTRY_STRIDE = 2,
    RENDER_MATERIAL_PTR_VALID_BASE = 1048576,
    RENDER_SKYBOX_ALT_FLAG = 8,
    RENDER_SKYBOX_INDEX_MASK = 7,
    RENDER_TRACK_MAP_DELTA = 901,
    RENDER_TERRAIN_HILL = 6,
    RENDER_TERRAIN_HILLROAD_MIN = 7,
    RENDER_TERRAIN_HILLROAD_MAX_EXCL = 11,
    RENDER_TRACK_BRIDGE_MIN = 105,
    RENDER_TRACK_BRIDGE_MAX = 108,
    RENDER_TRACK_MARKER_MIN = 253,
    RENDER_VISIBILITY_BASE = 1024,
    RENDER_STAR_MIN_Z = 200,
    RENDER_INTRO_WND_DEPTH = 15,
    RENDER_INTRO_CAMERA_CENTER = 1024,
    RENDER_INTRO_CAMERA_Y_START = 300,
    RENDER_INTRO_CAMERA_Y_CRUISE = 90,
    RENDER_INTRO_CAMERA_Y_STEP = 20,
    RENDER_INTRO_CAMERA_ADJUST_STEP = 10,
    RENDER_INTRO_LOGO_FLAGS_TRACKED = 12
};

#define DO_SINKING_RECT_PTR ((struct RECTANGLE*)43534)

/**
 * @brief Convert a DOS dseg offset to a game3dshapes[] pointer.
 *
 * @param ofs  16-bit DOS segment offset.
 * @return Pointer to the SHAPE3D, or NULL if invalid.
 */
static inline struct SHAPE3D* shape3d_from_dos_dseg_offset(unsigned short ofs)
{
    int idx;
    if (ofs == 0) return (struct SHAPE3D*)0;
    if (ofs < GAME3DSHAPES_DOS_BASE) return (struct SHAPE3D*)0;
    idx = (int)((unsigned)(ofs - GAME3DSHAPES_DOS_BASE) / GAME3DSHAPES_DOS_STRIDE);
    if (idx < 0 || idx >= GAME3DSHAPES_MAX) return (struct SHAPE3D*)0;
    if (game3dshapes[idx].shape3d_verts == 0) return (struct SHAPE3D*)0;
    return &game3dshapes[idx];
}

/**
 * @brief Get the primary SHAPE3D pointer for a track-object entry.
 *
 * @param obj  Track-object entry pointer.
 * @return Pointer to the SHAPE3D, or NULL.
 */
static inline struct SHAPE3D* trkobj_shape(const unsigned char* obj)
{
    return shape3d_from_dos_dseg_offset(trkobj_ofs_shape(obj));
}

/**
 * @brief Get the low-detail SHAPE3D pointer for a track-object entry.
 *
 * @param obj  Track-object entry pointer.
 * @return Pointer to the SHAPE3D, or NULL.
 */
static inline struct SHAPE3D* trkobj_loshape(const unsigned char* obj)
{
    return shape3d_from_dos_dseg_offset(trkobj_ofs_loshape(obj));
}

/**
 * @brief Extract the Y-rotation angle from a track-object entry.
 *
 * @param obj  Track-object entry pointer.
 * @return Rotation angle as a signed short.
 */
static inline short trkobj_roty(const unsigned char* obj)
{
    const unsigned char* raw = trkobj_raw(obj);
    return (short)((unsigned short)raw[2] | ((unsigned short)raw[3] << 8));
}

/**
 * @brief Extract the overlay flag byte from a track-object entry.
 *
 * @param obj  Track-object entry pointer.
 * @return Overlay byte value.
 */
static inline unsigned char trkobj_overlay(const unsigned char* obj)
{
    return trkobj_raw(obj)[8];
}

/**
 * @brief Extract the surface type from a track-object entry.
 *
 * @param obj  Track-object entry pointer.
 * @return Surface type as a signed char.
 */
static inline signed char trkobj_surface(const unsigned char* obj)
{
    return (signed char)trkobj_raw(obj)[9];
}

/**
 * @brief Extract the z-bias ignore flag from a track-object entry.
 *
 * @param obj  Track-object entry pointer.
 * @return Non-zero if z-bias should be ignored.
 */
static inline unsigned char trkobj_ignore_zbias(const unsigned char* obj)
{
    return trkobj_raw(obj)[10];
}

/**
 * @brief Extract the multi-tile flag from a track-object entry.
 *
 * @param obj  Track-object entry pointer.
 * @return Multi-tile flag byte.
 */
static inline unsigned char trkobj_multi(const unsigned char* obj)
{
    return trkobj_raw(obj)[11];
}


static struct RECTANGLE intro_cliprect = { 0, 320, 0, 200 };


/**
 * @brief Look up a 16-bit material colour by index from the colour list.
 *
/** @brief Index.
 * @param index Parameter `index`.
 * @return Function result.
 */
static unsigned short render_get_material_color(unsigned short index)
{
    const unsigned char * color_bytes;
    uintptr_t src_addr;
    const unsigned int material_entries = (unsigned int)RENDER_MATERIAL_ENTRY_COUNT;

    if ((unsigned int)index >= material_entries) {
        index = 0;
    }

    src_addr = (uintptr_t)material_clrlist_ptr_cpy;
    if (material_clrlist_ptr_cpy != 0 && src_addr >= (uintptr_t)RENDER_MATERIAL_PTR_VALID_BASE) {
        color_bytes = ((const unsigned char *)material_clrlist_ptr_cpy) + ((unsigned int)index * RENDER_MATERIAL_ENTRY_STRIDE);
    } else {
        color_bytes = ((const unsigned char *)material_color_list) + ((unsigned int)index * RENDER_MATERIAL_ENTRY_STRIDE);
    }

    return (unsigned short)(
        (unsigned short)color_bytes[0] |
        ((unsigned short)color_bytes[1] << 8)
    );
}


/* Forward declarations */
/** @brief Draw one rectangular slice of the active skybox. */
void draw_skybox_rect_slice(struct RECTANGLE* rectptr, int angY, int skyheight);
void intro_op(int camera_x, int camera_y, int camera_z, int camera_pitch, int camera_yaw,
              int draw_opponent, int use_primary_logo, short* star_positions, struct POINT2D* star_screen_points,
              short* star_count, struct RECTANGLE* current_rect,
              struct RECTANGLE* clip_rect, struct RECTANGLE* previous_rect_ptr);

/**--------------------------------------------------------------
 * @brief Free the SDGAME2 resource block.
 /*--------------------------------------------------------------*/
void free_sdgame2(void) {
    mmgr_free(sdgame2ptr);
}

/**--------------------------------------------------------------
 * @brief Unload the skybox resource and reset its state.
 /*--------------------------------------------------------------*/
void unload_skybox(void) {
    if (mouse_button_state_vector != 0) {
        mmgr_free(g_skybox_res_ptr);
    }
    mouse_button_state_vector = 0;
    g_skybox_res_ptr = NULL;
}

/**--------------------------------------------------------------
 * @brief Load the SDGAME2 shape resources and cache their widths.
 /*--------------------------------------------------------------*/
void load_sdgame2_shapes(void) {
    int i;
    sdgame2ptr = (char *)file_load_resource(8, (char *)aSdgame2);
    locate_many_resources(sdgame2ptr, (char *)aEx01ex02ex03leftrigh, (char **)sdgame2shapes);
    for (i = 0; i < 3; i++) {
        sdgame2_widths[i] = ((struct SHAPE2D *)sdgame2shapes[i])->s2d_width;
    }
}

/**--------------------------------------------------------------
 * @brief Transform a 3D shape position and add it to the depth-sort list.
 *
 * @param z_adjust  Z-depth adjustment for sorting.
 * @param sort_key        Secondary sort parameter stored per shape.
 /*--------------------------------------------------------------*/
void transformed_shape_add_for_sort(int z_adjust, int sort_key) {
    struct VECTOR shapepos;
    struct VECTOR transformedpos;
    int si, di;

    /* Copy pos from curtransshape_ptr */
    shapepos.x = curtransshape_ptr->pos.x;
    shapepos.y = curtransshape_ptr->pos.y;
    shapepos.z = curtransshape_ptr->pos.z;

    mat_mul_vector(&shapepos, &mat_temp, &transformedpos);

    si = (int)(unsigned char)transformedshape_counter;
    di = si;
    transformedshape_zarray[di] = transformedpos.z + z_adjust;
    transformedshape_arg2array[si] = (char)sort_key;
    transformedshape_indices[di] = si;
    transformedshape_counter++;

    curtransshape_ptr++;
}

/**--------------------------------------------------------------
 * @brief Initialise the rectangle dirty-tracking arrays.
 /*--------------------------------------------------------------*/
void init_rect_arrays(void) {
    int i;
    if (timertestflag_copy != 0) {
        /* Copy fullscreen_rect to first entry of both arrays */
        rect_buffer_front[0] = fullscreen_rect;
        rect_buffer_back[0] = fullscreen_rect;
        /* Fill rest with rect_invalid */
        for (i = 1; i < RENDER_RECT_ARRAY_COUNT; i++) {
            rect_buffer_front[i] = rect_invalid;
            rect_buffer_back[i] = rect_invalid;
        }
    }
}

/**--------------------------------------------------------------
 * @brief Render a sinking water-surface effect and return its bounding rect.
 *
/**--------------------------------------------------------------
 * @brief Compute and draw the animated sinking rectangle effect.
 * @param frame_count  Current frame counter for the effect.
 * @param base_y  Vertical base position.
 * @param sink_height  Total height of the sinking region.
 * @return Pointer to the computed bounding rectangle.
 /*--------------------------------------------------------------*/
struct RECTANGLE*  do_sinking(int frame_count, int base_y, int sink_height) {
    int di, si;

    di = framespersec << 2;
    if (frame_count > di) frame_count = di;

    si = (int)((long)sink_height * (long)frame_count / ((long)framespersec * 4L));

    rect_ingame_text.left = RENDER_SCREEN_LEFT;
    rect_ingame_text.right = RENDER_SCREEN_WIDTH;
    di = base_y + sink_height;
    rect_ingame_text.top = di - si;
    rect_ingame_text.bottom = di;
    sprite_set_1_size(RENDER_SCREEN_LEFT, RENDER_SCREEN_WIDTH, di - si, di);
    sprite_clear_sprite1_color(skybox_wat_color);

    return DO_SINKING_RECT_PTR;
}

/**--------------------------------------------------------------
 * @brief Load a skybox scenery set and set sky, ground and water colours.
 * @param skybox_index Skybox selector value from track metadata.
 /*--------------------------------------------------------------*/
void load_skybox(unsigned char skybox_index) {
    short si;
    static const char* const skybox_names[] = {
        "desert",
        "tropical",
        "alpine",
        "city",
        "country"
    };
    unsigned char skybox_idx;

    if (skybox_index & RENDER_SKYBOX_ALT_FLAG) {
        skybox_index &= RENDER_SKYBOX_INDEX_MASK;
    } else {
        if (mouse_button_state_vector != 0 && texture_page_index == skybox_index) {
            goto set_colors_only;
        }
        unload_skybox();
        skybox_idx = skybox_index;
        if (skybox_idx >= (unsigned char)(sizeof(skybox_names) / sizeof(skybox_names[0]))) {
            skybox_idx = 0;
        }

        texture_page_index = skybox_idx;
        mouse_button_state_vector = 1;

        {
            g_skybox_res_ptr = (char *)file_load_shape2d_fatal_thunk((char*)skybox_names[skybox_idx]);
        }

        locate_many_resources(g_skybox_res_ptr,
                      (char *)aScensce2sce3sce4, (char **)skyboxes);

        skybox_ptr1 = ((struct SHAPE2D *)skyboxes[0])->s2d_height;
        skybox_ptr2 = ((struct SHAPE2D *)skyboxes[1])->s2d_height;
        skybox_ptr3 = ((struct SHAPE2D *)skyboxes[2])->s2d_height;
        skybox_ptr4 = ((struct SHAPE2D *)skyboxes[3])->s2d_height;

        /* skybox_current = min of all ptrs */
        si = skybox_ptr1;
        if (si > skybox_ptr2) si = skybox_ptr2;
        if (si > skybox_ptr3) si = skybox_ptr3;
        if (si > skybox_ptr4) si = skybox_ptr4;
        skybox_current = si;

        /* memory_pointer_boundary_max = max of all ptrs */
        si = skybox_ptr1;
        if (si < skybox_ptr2) si = skybox_ptr2;
        if (si < skybox_ptr3) si = skybox_ptr3;
        if (si < skybox_ptr4) si = skybox_ptr4;
        memory_pointer_boundary_max = si;
    }

    /* Set colors from safe material color storage */
    skybox_sky_color = (short)render_get_material_color(RENDER_COLOR_INDEX_SKY);
    skybox_grd_color = render_get_material_color(RENDER_COLOR_INDEX_GROUND);
    skybox_wat_color = (short)render_get_material_color(RENDER_COLOR_INDEX_WATER);
    meter_needle_color = dialog_fnt_colour;

set_colors_only:
    return;
}

/**--------------------------------------------------------------
 * @brief Draw one vertical slice of the skybox within a clipping rectangle.
 *
 * @param rectptr    Clipping rectangle for the slice.
 * @param angY       Current camera yaw angle.
 * @param skyheight  Pixel row of the horizon.
 /*--------------------------------------------------------------*/
void draw_skybox_rect_slice(struct RECTANGLE* rectptr, int angY, int skyheight) {
    int si, di;
    int blit_top, blit_bottom;

    if (timertestflag2 == 4) {
        si = skyheight - rectptr->top;
    } else {
        si = skyheight - rectptr->top - skybox_current;
    }

    /* Clamp to rect height */
    if (rectptr->bottom - rectptr->top < si) {
        si = rectptr->bottom - rectptr->top;
    }

    /* Draw sky portion */
    if (si > 0) {
        sprite_set_1_size(rectptr->left, rectptr->right,
                          rectptr->top, rectptr->top + si);
        sprite_clear_sprite1_color(skybox_sky_color);
    }

    if (timertestflag2 == 4) {
        goto draw_ground;
    }

    /* Draw skybox images */
    si = angY + RENDER_ANGLE_HALF_TURN;
    si &= RENDER_ANGLE_MASK;
    si -= RENDER_ANGLE_FULL_TURN;

    if (rectptr->top >= skyheight) {
        goto draw_ground;
    }
    if (skyheight - memory_pointer_boundary_max > rectptr->bottom) {
        goto draw_ground;
    }

    blit_top = rectptr->top;
    if (blit_top < skyheight - memory_pointer_boundary_max) {
        blit_top = skyheight - memory_pointer_boundary_max;
    }
    blit_bottom = rectptr->bottom;
    if (blit_bottom > skyheight) {
        blit_bottom = skyheight;
    }

    if (blit_top >= blit_bottom) {
        goto draw_ground;
    }

    sprite_set_1_size(rectptr->left, rectptr->right,
                      blit_top, blit_bottom);
    sprite_clear_sprite1_color(skybox_sky_color);

    sprite_putimage_copy_at(skyboxes[0], si, skyheight - skybox_ptr1);
    sprite_putimage_copy_at(skyboxes[1], si + RENDER_SCREEN_WIDTH, skyheight - skybox_ptr2);
    sprite_putimage_copy_at(skyboxes[2], si + RENDER_ANGLE_HALF_TURN, skyheight - skybox_ptr3);
    sprite_putimage_copy_at(skyboxes[3], si + (RENDER_ANGLE_HALF_TURN + RENDER_SCREEN_WIDTH), skyheight - skybox_ptr4);
    sprite_putimage_copy_at(skyboxes[0], si + RENDER_ANGLE_FULL_TURN, skyheight - skybox_ptr1);

draw_ground:
    /* Draw ground portion below skybox */
    if (rectptr->top >= skyheight) {
        di = rectptr->top;
    } else {
        di = skyheight;
    }
    si = rectptr->bottom - di;
    if (si > 0) {
        sprite_set_1_size(rectptr->left, rectptr->right,
                          di, di + si);
        sprite_clear_sprite1_color(skybox_grd_color);
    }
}

/*--------------------------------------------------------------
 * skybox_op
 /*--------------------------------------------------------------*/
/** @brief Render skybox layer.
 * @param view_index Parameter `view_index`.
 * @param clip_rect Parameter `clip_rect`.
 * @param sky_dir_sign Parameter `sky_dir_sign`.
 * @param camera_matrix Parameter `camera_matrix`.
 * @param view_roll Parameter `view_roll`.
 * @param view_yaw Parameter `view_yaw`.
 * @param camera_y Parameter `camera_y`.
 * @return Function result.
 */
int  render_skybox_layer(int view_index, struct RECTANGLE* clip_rect, int sky_dir_sign,
                  struct MATRIX* camera_matrix, int view_roll, int view_yaw, int camera_y) {
    int line_intersections[60];  /* buffer for draw_line_related */
    int skybox_drawn;
    int has_wraparound;
    struct VECTOR horizon_vec;
    struct VECTOR horizon_vec2;
    int sky_height;
    struct POINT2D horizon_point_a, horizon_point_b;
    /* intermediate extra horizon points removed — no longer needed after strip-based tilted skybox */
    int strip_left;
    int horizon_y_left;
    struct RECTANGLE slice_rect;
    int horizon_y_delta;
    struct VECTOR horizon_vec_a_cam, horizon_vec_b_cam;
    int di, si;


    dirty_rect_count = 0;
    skybox_drawn = 0;

    sprite_set_1_size(RENDER_SCREEN_LEFT, RENDER_SCREEN_WIDTH, clip_rect->top, clip_rect->bottom);

    if (view_roll == 0) goto no_horizon;

    /* Compute upper vector */
    horizon_vec.x = (short)(RENDER_HORIZON_VECTOR_X * sky_dir_sign);
    horizon_vec.y = (short)(-camera_y);
    horizon_vec.z = (short)(RENDER_HORIZON_VECTOR_Z * sky_dir_sign);
    mat_mul_vector(&horizon_vec, camera_matrix, &horizon_vec_a_cam);

    /* Compute lower vector */
    horizon_vec2.x = (short)(RENDER_HORIZON_VECTOR_NEG_X * sky_dir_sign);
    horizon_vec2.y = (short)(-camera_y);
    horizon_vec2.z = (short)(RENDER_HORIZON_VECTOR_Z * sky_dir_sign);
    mat_mul_vector(&horizon_vec2, camera_matrix, &horizon_vec_b_cam);

    if (horizon_vec_a_cam.z < 0 || horizon_vec_b_cam.z < 0) {
        di = skybox_sky_color;
fill_and_return:
        sprite_set_1_size(RENDER_SCREEN_LEFT, RENDER_SCREEN_WIDTH, clip_rect->top, clip_rect->bottom);
        sprite_clear_sprite1_color(di);
        goto done;
    }

    vector_to_point(&horizon_vec_a_cam, &horizon_point_a);
    vector_to_point(&horizon_vec_b_cam, &horizon_point_b);

    if (horizon_point_a.px == (int16_t)RENDER_INVALID_POINT_COORD && horizon_point_a.py == (int16_t)RENDER_INVALID_POINT_COORD &&
        horizon_point_b.px == (int16_t)RENDER_INVALID_POINT_COORD && horizon_point_b.py == (int16_t)RENDER_INVALID_POINT_COORD) {
        goto no_horizon;
    }

    /* Check if both points are off-screen right */
    if (horizon_point_a.px > RENDER_SCREEN_WIDTH && horizon_point_b.px > RENDER_SCREEN_WIDTH) {
        if (horizon_point_a.py < horizon_point_b.py) goto fill_sky;
        goto fill_grd;
    }
    /* Check if both points are off-screen left */
    if (horizon_point_a.px < 0 && horizon_point_b.px < 0) {
        if (horizon_point_a.py <= horizon_point_b.py) goto fill_grd;
        goto fill_sky;
    }

    /* Check if both below bottom */
    if (clip_rect->bottom < horizon_point_a.py &&
        clip_rect->bottom < horizon_point_b.py) {
        if (horizon_point_a.px <= horizon_point_b.px) goto fill_grd;
        goto fill_sky;
    }

    /* Check if both above top */
    if (clip_rect->top > horizon_point_a.py &&
        clip_rect->top > horizon_point_b.py) {
        if (horizon_point_a.px >= horizon_point_b.px) goto fill_grd;
        goto fill_sky;
    }

    has_wraparound = 0;

    /* Wrap-around case detection */
    if (timertestflag2 != 4 &&
        ((horizon_point_b.px < 0 && horizon_point_a.px > RENDER_SCREEN_WIDTH) ||
         (horizon_point_a.px < 0 && horizon_point_b.px > RENDER_SCREEN_WIDTH))) {
        int wrap_x0;
        int wrap_y0;
        int wrap_x1;
        int wrap_y1;

        if (horizon_point_b.px < 0 && horizon_point_a.px > RENDER_SCREEN_WIDTH) {
            wrap_x0 = horizon_point_b.px;
            wrap_y0 = horizon_point_b.py;
            wrap_x1 = horizon_point_a.px;
            wrap_y1 = horizon_point_a.py;
        } else {
            wrap_x0 = horizon_point_a.px;
            wrap_y0 = horizon_point_a.py;
            wrap_x1 = horizon_point_b.px;
            wrap_y1 = horizon_point_b.py;
        }

        if (draw_line_related(wrap_x0, wrap_y0,
                              wrap_x1, wrap_y1, line_intersections) == 0) {
            /* edge x is line_intersections[1], y0 is [3], y1 is [5] */
            di = line_intersections[3] - line_intersections[5];
            if (di < 0) di = -di;
            if (di < RENDER_WRAP_DELTA_MAX) {
                if (line_intersections[1] == 0) {
                    horizon_y_left = line_intersections[3];
                    horizon_y_delta = line_intersections[5] - horizon_y_left;
                    has_wraparound = 1;
                } else if (line_intersections[1] == RENDER_SCREEN_RIGHT_EDGE) {
                    horizon_y_left = line_intersections[5];
                    horizon_y_delta = line_intersections[3] - horizon_y_left;
                    has_wraparound = 1;
                }
            }
        }
    }

    if (has_wraparound == 0) goto no_wraparound;


    /* Has wrap-around */
    if (timertestflag_copy == 0) goto simple_skybox;

    /* Full rect tracking path */
    slice_rect.left = 0;
    rect_skybox.left = 0;
    slice_rect.right = 320;
    rect_skybox.right = 320;

    if (race_condition_state_flag != 0) {
        rect_skybox.top = clip_rect->top;
        rect_skybox.bottom = clip_rect->bottom;
        goto setup_rect_done;
    }

    /* Compute sky top */
    {
        int tmp;
        tmp = horizon_y_left + horizon_y_delta;
        if (tmp > horizon_y_left) tmp = horizon_y_left;
        tmp -= memory_pointer_boundary_max;
        rect_skybox.top = tmp;
        if (clip_rect->top > tmp) {
            rect_skybox.top = clip_rect->top;
        }
    }

    /* Compute sky bottom */
    {
        int tmp;
        tmp = horizon_y_left + horizon_y_delta;
        if (tmp < horizon_y_left) tmp = horizon_y_left;
        rect_skybox.bottom = tmp;
    }

    /* Fill sky area above skybox */
    for (si = 0; si < RENDER_RECT_ARRAY_COUNT; si++) {
        rect_sort_indices[si] = 1;
    }
    rect_sort_indices[5] = 3;

    slice_rect.top = RENDER_SCREEN_TOP;
    slice_rect.bottom = rect_skybox.top;

    if (rect_intersect(&slice_rect, clip_rect) == 0) {
        dirty_rect_count = 0;
        rectlist_add_rects(RENDER_RECT_ARRAY_COUNT, rect_sort_indices, rect_buffer_primary,
                           frame_dirty_rects, &slice_rect,
                           &dirty_rect_count, dirty_rect_array);
        for (di = 0; di < (int)(signed char)dirty_rect_count; di++) {
            sprite_set_1_size(dirty_rect_array[di].left, dirty_rect_array[di].right,
                              dirty_rect_array[di].top, dirty_rect_array[di].bottom);
            sprite_clear_sprite1_color(skybox_sky_color);
        }
    }

    /* Fill ground area below skybox */
    slice_rect.top = rect_skybox.bottom;
    slice_rect.bottom = RENDER_SCREEN_HEIGHT;
    if (rect_intersect(&slice_rect, clip_rect) == 0) {
        dirty_rect_count = 0;
        rectlist_add_rects(RENDER_RECT_ARRAY_COUNT, rect_sort_indices, rect_buffer_primary,
                           frame_dirty_rects, &slice_rect,
                           &dirty_rect_count, dirty_rect_array);
        for (di = 0; di < (int)(signed char)dirty_rect_count; di++) {
            sprite_set_1_size(dirty_rect_array[di].left, dirty_rect_array[di].right,
                              dirty_rect_array[di].top, dirty_rect_array[di].bottom);
            sprite_clear_sprite1_color(skybox_grd_color);
        }
    }

setup_rect_done:
    slice_rect.top = rect_skybox.top;
    slice_rect.bottom = rect_skybox.bottom;
    goto setup_skybox_strip;

simple_skybox:
    slice_rect.top = clip_rect->top;
    slice_rect.bottom = clip_rect->bottom;

setup_skybox_strip:
    slice_rect.left = RENDER_SCREEN_LEFT;
    slice_rect.right = RENDER_SCREEN_WIDTH;
    if (rect_intersect(&slice_rect, clip_rect) != 0) goto done;

    /* Render skybox strips */
    strip_left = RENDER_SCREEN_LEFT;
    di = (horizon_y_delta < 0 ? -horizon_y_delta : horizon_y_delta) + 1;
    if (di > RENDER_MAX_STRIP_COUNT) di = RENDER_MAX_STRIP_COUNT;

    for (si = 0; si < di; si++) {
        slice_rect.left = strip_left;
        slice_rect.right = ((RENDER_SCREEN_WIDTH * si + RENDER_SCREEN_WIDTH) / di) & video_flag3_isFFFF;
        if (slice_rect.left != slice_rect.right) {
            sky_height = horizon_y_delta * si / di + horizon_y_left;
            draw_skybox_rect_slice(&slice_rect, view_yaw, sky_height);
            strip_left = slice_rect.right;
        }
    }
    goto done;

no_wraparound:


    /* Render tilted skybox using vertical strips with interpolated horizon height.
     * horizon_point_a and horizon_point_b define the horizon line endpoints.
     * Interpolate the horizon Y across the screen width and render the skybox
    * image in each strip via draw_skybox_rect_slice. */
    {
        int strip_left, strip_right, strip_count, strip_idx;
        int left_y, right_y, py_delta;

        /* Determine horizon height at the left and right screen edges.
/** @brief Points.
 * @param px Parameter `px`.
 * @return Function result.
 */
        if (horizon_point_a.px != horizon_point_b.px) {
            left_y  = horizon_point_a.py + (int)(((long)(horizon_point_b.py - horizon_point_a.py)) * (0 - horizon_point_a.px)) / (horizon_point_b.px - horizon_point_a.px);
            right_y = horizon_point_a.py + (int)(((long)(horizon_point_b.py - horizon_point_a.py)) * (RENDER_SCREEN_WIDTH - horizon_point_a.px)) / (horizon_point_b.px - horizon_point_a.px);
        } else {
            left_y  = (horizon_point_a.py + horizon_point_b.py) / 2;
            right_y = left_y;
        }

        horizon_y_left = left_y;
        horizon_y_delta = right_y - left_y;

        py_delta = (horizon_y_delta < 0) ? -horizon_y_delta : horizon_y_delta;
        strip_count = py_delta + 1;
        if (strip_count > RENDER_MAX_STRIP_COUNT) strip_count = RENDER_MAX_STRIP_COUNT;

        strip_left = RENDER_SCREEN_LEFT;
        slice_rect.top = clip_rect->top;
        slice_rect.bottom = clip_rect->bottom;
        for (strip_idx = 0; strip_idx < strip_count; strip_idx++) {
            strip_right = ((RENDER_SCREEN_WIDTH * strip_idx + RENDER_SCREEN_WIDTH) / strip_count) & video_flag3_isFFFF;
            if (strip_left != strip_right) {
                slice_rect.left = strip_left;
                slice_rect.right = strip_right;
                sky_height = horizon_y_delta * strip_idx / strip_count + horizon_y_left;
                draw_skybox_rect_slice(&slice_rect, view_yaw, sky_height);
                strip_left = strip_right;
            }
        }
    }
    goto done2;

fill_sky:
    di = skybox_sky_color;
    goto fill_and_return;

fill_grd:
    di = skybox_grd_color;
    goto fill_and_return;

no_horizon:

    /* No horizon visible - simple sky/ground split */
    horizon_vec.x = 0;
    horizon_vec.y = (short)(-camera_y);
    horizon_vec.z = (short)(RENDER_HORIZON_VECTOR_Z * sky_dir_sign);
    mat_mul_vector(&horizon_vec, camera_matrix, &horizon_vec_a_cam);

    if (horizon_vec_a_cam.z < 0) {
        /* Looking down = all sky */
        sprite_clear_sprite1_color(skybox_sky_color);
        if (timertestflag_copy == 0) goto done;
        skybox_drawn = 1;
        rect_skybox.left = RENDER_SCREEN_LEFT;
        rect_skybox.right = RENDER_SCREEN_WIDTH;
        rect_skybox.top = clip_rect->top;
        rect_skybox.bottom = clip_rect->bottom;
        goto done;
    }

    /* Have a horizon point */
    vector_to_point(&horizon_vec_a_cam, &horizon_point_a);
    sky_height = horizon_point_a.py;
    if (clip_rect->top > sky_height) {
        sky_height = clip_rect->top;
    }

    if (sky_dir_sign == 1) {

        /* Full skybox rendering */
        if (timertestflag_copy == 0) {
            /* Simple mode */
            slice_rect.left = RENDER_SCREEN_LEFT;
            slice_rect.right = RENDER_SCREEN_WIDTH;
            slice_rect.top = clip_rect->top;
            slice_rect.bottom = clip_rect->bottom;
            draw_skybox_rect_slice(&slice_rect, view_yaw, sky_height);
            goto done;
        }

        if (timertestflag2 == 4) {
            rect_skybox.top = sky_height - 1;
        } else {
            rect_skybox.top = sky_height - memory_pointer_boundary_max;
        }
        rect_skybox.left = RENDER_SCREEN_LEFT;
        rect_skybox.right = RENDER_SCREEN_WIDTH;
        rect_skybox.bottom = sky_height;

        if (race_condition_state_flag != 0) {
            /* Simple mode B */
            slice_rect.left = RENDER_SCREEN_LEFT;
            slice_rect.right = RENDER_SCREEN_WIDTH;
            slice_rect.top = clip_rect->top;
            slice_rect.bottom = clip_rect->bottom;
            draw_skybox_rect_slice(&slice_rect, view_yaw, sky_height);
            goto done;
        }

        /* Multi-rect tracking path */
        for (si = 0; si < RENDER_RECT_ARRAY_COUNT; si++) {
            rect_sort_indices[si] = 1;
        }

        if (timertestflag2 == 4) {
            angle_rotation_state[view_index] = sprite_transformation_angle;
        }

        if (angle_rotation_state[view_index] == view_yaw) {
            if (((struct RECTANGLE*)rect_buffer_primary)[5].left == rect_skybox.left &&
                ((struct RECTANGLE*)rect_buffer_primary)[5].right == rect_skybox.right &&
                ((struct RECTANGLE*)rect_buffer_primary)[5].top == rect_skybox.top &&
                ((struct RECTANGLE*)rect_buffer_primary)[5].bottom == rect_skybox.bottom) {
                rect_sort_indices[5] = 0;
            } else {
                rect_sort_indices[5] = 3;
            }
        } else {
            rect_sort_indices[5] = 3;
        }

        dirty_rect_count = 0;
        rectlist_add_rects(15, rect_sort_indices, rect_buffer_primary,
                           frame_dirty_rects, clip_rect,
                           &dirty_rect_count, dirty_rect_array);

        for (di = 0; di < (int)(signed char)dirty_rect_count; di++) {
            draw_skybox_rect_slice(&dirty_rect_array[di], view_yaw, sky_height);
        }
        goto done;
    }

    /* sky_dir_sign != 1: Simple ground/sky fill */

    si = sky_height - clip_rect->top;
    if (clip_rect->bottom - clip_rect->top < si) {
        si = clip_rect->bottom - clip_rect->top;
    }
    if (si > 0) {
        sprite_set_1_size(RENDER_SCREEN_LEFT, RENDER_SCREEN_WIDTH, clip_rect->top, clip_rect->top + si);
        sprite_clear_sprite1_color(skybox_grd_color);
    }
    si = clip_rect->bottom - sky_height;
    if (si > 0) {
        sprite_set_1_size(RENDER_SCREEN_LEFT, RENDER_SCREEN_WIDTH, sky_height, sky_height + si);
        sprite_clear_sprite1_color(skybox_sky_color);
    }

done2:
    skybox_drawn = 1;
done:
    return skybox_drawn;
}

/**--------------------------------------------------------------
 * @brief Render the 3D track preview shown before a race.
 /*--------------------------------------------------------------*/
void draw_track_preview(void) {
    int cell_index, unused_row_temp;
    int object_index;
    unsigned char unused_marker, terrain_id;
    unsigned char row, unused_col_temp;
    int pitch_angle, horizon_y;
    unsigned char col, track_elem;
    int projected_horizon_y = 0, corner_idx;
    struct MATRIX* camera_mat;
    struct TRANSFORMEDSHAPE3D transformed_shape;
    int element_world_z;
    struct POINT2D projected_point;
    struct VECTOR object_pos;
    int radius_xz, unused_tmp;
    struct MATRIX* matptr;
    unsigned char* elem_map_ptr;
    unsigned char* terr_map_ptr;
    int i, si;

    (void)unused_col_temp;
    (void)unused_marker;
    (void)unused_tmp;
    (void)i;

    if (track_terrain_map == 0) {
        return;
    }
    {
        const uintptr_t terr_ptr = (uintptr_t)track_terrain_map;
        const uintptr_t expected_elem_ptr = terr_ptr - RENDER_TRACK_MAP_DELTA;
        if (track_elem_map == 0 ||
            (uintptr_t)track_elem_map >= terr_ptr ||
            (terr_ptr - (uintptr_t)track_elem_map) != RENDER_TRACK_MAP_DELTA) {
            track_elem_map = (unsigned char*)expected_elem_ptr;
        }
    }

    radius_xz = polarRadius2D(angle_correction_offset_x - angle_sine_table_start, angle_correction_offset_z - angle_sine_table_stride);
    pitch_angle = polarAngle(angle_correction_offset_y - angle_sine_table_offset, radius_xz);
    matptr = mat_rot_zxy(0, pitch_angle, 0, 1);
    camera_mat = matptr;

    mat_mul_vector((struct VECTOR*)preview_camera_forward_vector, camera_mat, &object_pos);
    vector_to_point(&object_pos, &projected_point);
    projected_horizon_y = projected_point.py;

    horizon_y = projected_horizon_y;
    if (horizon_y < 0) horizon_y = 0;

    /* Draw sky */
    {
        int sky_bottom = horizon_y - skybox_current;
        if (sky_bottom < 0) sky_bottom = 0;
        if (sky_bottom > RENDER_SCREEN_HEIGHT) sky_bottom = RENDER_SCREEN_HEIGHT;
        sprite_set_1_size(RENDER_SCREEN_LEFT, RENDER_SCREEN_WIDTH, RENDER_SCREEN_TOP, (unsigned short)sky_bottom);
        if (sky_bottom > 0) {
            sprite_clear_sprite1_color(skybox_sky_color);
        }
    }

    /* Draw skybox images */
    sprite_set_1_size(RENDER_SCREEN_LEFT, RENDER_SCREEN_WIDTH, RENDER_SCREEN_TOP, RENDER_COLOR_INDEX_WATER);
    sprite_putimage_copy_at(skyboxes[2], RENDER_SCREEN_LEFT, horizon_y - skybox_ptr3);
    sprite_putimage_copy_at(skyboxes[3], RENDER_SCREEN_WIDTH, horizon_y - skybox_ptr4);

    /* Draw ground */
    {
        int ground_top = horizon_y;
        if (ground_top < 0) ground_top = 0;
        if (ground_top > RENDER_SCREEN_HEIGHT) ground_top = RENDER_SCREEN_HEIGHT;
        sprite_set_1_size(RENDER_SCREEN_LEFT, RENDER_SCREEN_WIDTH, (unsigned short)ground_top, RENDER_SCREEN_HEIGHT);
        if (ground_top < RENDER_SCREEN_HEIGHT) {
            sprite_clear_sprite1_color(skybox_grd_color);
        }
    }

    /* Full-screen clear for 3D */
    sprite_set_1_size(RENDER_SCREEN_LEFT, RENDER_SCREEN_WIDTH, RENDER_SCREEN_TOP, RENDER_SCREEN_HEIGHT);
    select_cliprect_rotate(0, pitch_angle, 0, &trackpreview_cliprect, 1);

    terr_map_ptr = track_terrain_map;
    elem_map_ptr = track_elem_map;
    if (terr_map_ptr == 0) {
        return;
    }
    if (elem_map_ptr == 0 ||
        (uintptr_t)elem_map_ptr >= (uintptr_t)terr_map_ptr ||
        ((uintptr_t)terr_map_ptr - (uintptr_t)elem_map_ptr) != RENDER_TRACK_MAP_DELTA) {
        elem_map_ptr = terr_map_ptr - RENDER_TRACK_MAP_DELTA;
    }

    transformed_shape.rotvec.x = 0;
    transformed_shape.rotvec.y = 0;
    transformed_shape.shape_visibility_threshold = RENDER_VISIBILITY_BASE * REND_DIST_MULT;

    /* Iterate track grid (matches original draw_track_preview ASM in seg003.asm) */
    (void)unused_row_temp; /* unused_row_temp not used in loop; row index via row directly */
    for (row = 0; row < RENDER_TRACK_GRID_SIZE; row++) {
        for (col = 0; col < RENDER_TRACK_GRID_SIZE; ) {
            /*
             * di: Y-height offset in track-preview camera units.
             * Use hillHeightConsts[1] for hill elevation.
             * For non-hill terrain and track elements, di = 0.
             */
            int di = 0;

            cell_index = (int)col;
            track_elem = elem_map_ptr[trackrows[row] + cell_index];
            terrain_id = terr_map_ptr[terrainrows[row] + cell_index];

/** @brief Markers.
 * @param element Parameter `element`.
 * @param RENDER_TRACK_MARKER_MIN Parameter `RENDER_TRACK_MARKER_MIN`.
 * @return Function result.
 */
            /* Bridge/fence markers (253-255): suppress element, nothing renders */
            if (track_elem >= RENDER_TRACK_MARKER_MIN) {
                terrain_id = 0;
                track_elem = 0;
                /* Hill-road substitution: replace element id when on hill-type terrain */
            } else if (track_elem != 0) {
                if (terrain_id >= RENDER_TERRAIN_HILLROAD_MIN && terrain_id < RENDER_TERRAIN_HILLROAD_MAX_EXCL) {
                    track_elem = subst_hillroad_track(terrain_id, track_elem);
                    terrain_id = 0;
                }
            }

            /*
/** @brief Path.
 * @param RENDER_TERRAIN_HILL Parameter `RENDER_TERRAIN_HILL`.
 * @return Function result.
 */
            if (terrain_id == RENDER_TERRAIN_HILL) {
                di = hillHeightConsts[1];
                if (track_elem != 0) {
                    terrain_id = 0;
                }
                if (terrain_id != 0) {
                    /* Render hill terrain (loshape) at elevated Y */
                    object_index = (int)terrain_id;
                    transformed_shape.shapeptr = trkobj_loshape(trkobj_entry(sceneshapes2, (unsigned)object_index));
                    transformed_shape.pos.x = (trackcenterpos2[(int)col] - angle_sine_table_start) >> 1;
                    transformed_shape.pos.y = (di - angle_sine_table_offset) >> 1;
                    /* DOS: sub ax, word_3C10C (62736) / sar ax,1 — must wrap at 16 bits */
                    transformed_shape.pos.z = (short)((uint16_t)trackcenterpos[row] - (uint16_t)(unsigned short)angle_sine_table_stride) >> 1;
                    transformed_shape.rotvec.x = 0;
                    transformed_shape.rotvec.y = 0;
                    transformed_shape.rotvec.z = trkobj_roty(trkobj_entry(sceneshapes2, (unsigned)object_index));
                    transformed_shape.ts_flags = 5 | TRANSFORM_FLAG_TERRAIN_DOUBLE_SIDED;
                    transformed_shape.shape_visibility_threshold = RENDER_VISIBILITY_BASE * REND_DIST_MULT;
                    transformed_shape.material = 0;
                    shape3d_render_transformed(&transformed_shape);
                }
            } else {
/** @brief Path.
 * @param RENDER_TRACK_BRIDGE_MAX Parameter `RENDER_TRACK_BRIDGE_MAX`.
 * @return Function result.
 */
                /* Non-hill terrain path (loc_1CE04 in original ASM) */
                if (track_elem >= RENDER_TRACK_BRIDGE_MIN && track_elem <= RENDER_TRACK_BRIDGE_MAX) {
                    /* Bridge/overpass element: render terrain at all 4 surrounding corners.
                     * Uses hishape per original ASM [bx+4]. */
                    for (corner_idx = 0; corner_idx < 4; corner_idx++) {
                        unsigned char r, c;
                        switch (corner_idx) {
                            case 0: r = col;     c = row;     break;
                            case 1: r = col + 1; c = row;     break;
                            case 2: r = col;     c = row + 1; break;
                            default: r = col + 1; c = row + 1; break;
                        }
                        {
                            unsigned char tv = terr_map_ptr[terrainrows[(int)c] + (int)r];
                            if (tv != 0) {
                                object_index = (int)tv;
                                transformed_shape.shapeptr = trkobj_shape(trkobj_entry(sceneshapes2, (unsigned)object_index));
                                transformed_shape.pos.x = (trackcenterpos2[(int)r] - angle_sine_table_start) >> 1;
                                transformed_shape.pos.y = (-angle_sine_table_offset) >> 1;
                                transformed_shape.pos.z = (short)((uint16_t)trackcenterpos[(int)c] - (uint16_t)(unsigned short)angle_sine_table_stride) >> 1;
                                transformed_shape.rotvec.x = 0;
                                transformed_shape.rotvec.y = 0;
                                transformed_shape.rotvec.z = trkobj_roty(trkobj_entry(sceneshapes2, (unsigned)object_index));
                                transformed_shape.ts_flags = 5 | TRANSFORM_FLAG_TERRAIN_DOUBLE_SIDED;
                                transformed_shape.shape_visibility_threshold = RENDER_VISIBILITY_BASE * REND_DIST_MULT;
                                transformed_shape.material = 0;
                                shape3d_render_transformed(&transformed_shape);
                            }
                        }
                    }
                    terrain_id = 0;
                } else if (terrain_id != 0) {
                    /* Regular flat terrain (loshape) */
                    object_index = (int)terrain_id;
                    transformed_shape.shapeptr = trkobj_loshape(trkobj_entry(sceneshapes2, (unsigned)object_index));
                    transformed_shape.pos.x = (trackcenterpos2[(int)col] - angle_sine_table_start) >> 1;
                    transformed_shape.pos.y = (-angle_sine_table_offset) >> 1;
                    transformed_shape.pos.z = (short)((uint16_t)trackcenterpos[row] - (uint16_t)(unsigned short)angle_sine_table_stride) >> 1;
                    transformed_shape.rotvec.x = 0;
                    transformed_shape.rotvec.y = 0;
                    transformed_shape.rotvec.z = trkobj_roty(trkobj_entry(sceneshapes2, (unsigned)object_index));
                    transformed_shape.ts_flags = 5 | TRANSFORM_FLAG_TERRAIN_DOUBLE_SIDED;
                    transformed_shape.shape_visibility_threshold = RENDER_VISIBILITY_BASE * REND_DIST_MULT;
                    transformed_shape.material = 0;
                    shape3d_render_transformed(&transformed_shape);
                }
            }

            /* Skip to next cell if no track element */
            if (track_elem == 0) {
                get_a_poly_info();
                col++;
                continue;
            }

            if (track_elem < RENDER_TRACK_MARKER_MIN) {
                /*
                 * Render track element.
                 * Original ASM (loc_1D042) uses loshape [bx+6] for both the
                 * overlay and the main shape — low-res shapes for the preview view.
                 */
                object_index = (int)track_elem;
                {
                    const unsigned char* tobj = trkobj_entry(trkObjectList, (unsigned)object_index);

/** @brief Position.
 * @param tobj Parameter `tobj`.
 * @return Function result.
 */
                    /* Z position (row axis) */
                    if (trkobj_multi(tobj) & 1) {
                        element_world_z = trackpos[row];
                    } else {
                        element_world_z = trackcenterpos[row];
                    }
/** @brief Position.
 * @param tobj Parameter `tobj`.
 * @return Function result.
 */
                    /* X position (column axis) */
                    if (trkobj_multi(tobj) & 2) {
                        si = trackpos2[col + 1];
                    } else {
                        si = trackcenterpos2[(int)col];
                    }

                    object_pos.x = (si - angle_sine_table_start) >> 1;
                    object_pos.y = (di - angle_sine_table_offset) >> 1;  /* elevated on hills */
                    object_pos.z = (short)((uint16_t)element_world_z - (uint16_t)(unsigned short)angle_sine_table_stride) >> 1;

                    /* Overlay: loshape of overlay entry, roty from main trkobj */
                    if (trkobj_overlay(tobj) != 0) {
                        const unsigned char* overlay = trkobj_entry(trkObjectList, (unsigned)trkobj_overlay(tobj));
                        if (trkobj_loshape(overlay) != 0) {
                            transformed_shape.shapeptr = trkobj_loshape(overlay);
                            transformed_shape.pos = object_pos;
                            transformed_shape.rotvec.z = trkobj_roty(tobj);
                            transformed_shape.ts_flags = 5;
                            if (trkobj_surface(overlay) >= 0) {
                                transformed_shape.material = trkobj_surface(overlay);
                            } else {
                                transformed_shape.material = 0;
                            }
                            shape3d_render_transformed(&transformed_shape);
                        }
                    }

                    /* Main shape: loshape (lo-res for preview distance) */
                    transformed_shape.shapeptr = trkobj_loshape(tobj);
                    transformed_shape.pos = object_pos;
                    transformed_shape.rotvec.z = trkobj_roty(tobj);
                    transformed_shape.ts_flags = trkobj_ignore_zbias(tobj) | 4;
                    if (trkobj_surface(tobj) >= 0) {
                        transformed_shape.material = trkobj_surface(tobj);
                    } else {
                        transformed_shape.material = 0;
                    }
                    if (transformed_shape.shapeptr) {
                        shape3d_render_transformed(&transformed_shape);
                    }
                }
            }
            /* track_elem >= 253 already cleared to 0 above; never reached here */

            get_a_poly_info();
            col++;
        }
    }
}

/**--------------------------------------------------------------
 * @brief Draw race HUD overlays (messages, arrows and status text).
 /*--------------------------------------------------------------*/
struct RECTANGLE*  draw_ingame_text(void) {
    int si;

    rect_ingame_text = rect_invalid;

    if (idle_expired != 0) {
        /* "Professional Driver" + "on Closed Circuit" */
        copy_string(resID_byte1, locate_text_res(gameresptr, (char *)aDm1));
        rect_union(intro_draw_text(resID_byte1,
               font_get_centered_x(resID_byte1), 170, dialog_fnt_colour, 0),
                   &rect_ingame_text, &rect_ingame_text);

        copy_string(resID_byte1, locate_text_res(gameresptr, (char *)aDm2));
        goto draw_text_at_B6;
    }

    if (game_replay_mode != 0) goto replay_check;

    if (state.game_inputmode == 0) {
        /* "Fasten Your Seatbelt" */
        copy_string(resID_byte1, locate_text_res(gameresptr, (char *)aPre));
draw_text_at_B6:
        /* Common path: draw text at computed X, specified Y */
        {
            int x = font_get_centered_x(resID_byte1);
            struct RECTANGLE* r = intro_draw_text(resID_byte1, x, 182,
                                                  dialog_fnt_colour, 0);
            rect_union(r, &rect_ingame_text, &rect_ingame_text);
        }
        goto done_text;
    }

    if (passed_security == 0) {
        /* Security system warning */
        copy_string(resID_byte1, locate_text_res(gameresptr, (char *)aSe1));
        rect_union(intro_draw_text(resID_byte1,
               font_get_centered_x(resID_byte1), 93, dialog_fnt_colour, 0),
                   &rect_ingame_text, &rect_ingame_text);

        copy_string(resID_byte1, locate_text_res(gameresptr, (char *)aSe2));
        goto draw_text_at_B6;
    }

    if (followOpponentFlag != 0) goto done_text;
    if (cameramode != 0) goto done_text;
    if (state.playerstate.car_crashBmpFlag != 0) goto done_text;

    /* Direction arrows / messages */
    switch ((signed char)state.game_collision_type) {
        case 1:
            /* Left arrow */
            sprite_putimage_transparent(sdgame2shapes[3], 148, 93);
            rect_union(&rect_ingame_text2, &rect_ingame_text, &rect_ingame_text);
            break;
        case 2:
            /* Right arrow */
            sprite_putimage_transparent(sdgame2shapes[4], 148, 93);
            rect_union(&rect_ingame_text2, &rect_ingame_text, &rect_ingame_text);
            break;
        case 3:
            /* "Wrong Way" */
            copy_string(resID_byte1, locate_text_res(gameresptr, (char *)aWww));
            rect_union(intro_draw_text(resID_byte1,
                       font_get_centered_x(resID_byte1), 93, dialog_fnt_colour, 0),
                       &rect_ingame_text, &rect_ingame_text);
            break;
    }

    /* Bottom text based on field_45E */
    resID_byte1[0] = 0;
    switch ((signed char)state.game_flyover_check) {
        case 1:
            /* Opponent near + left arrow */
            sprite_putimage_transparent(sdgame2shapes[3], 68, 113);
            rect_union(&rect_ingame_text3, &rect_ingame_text, &rect_ingame_text);
            copy_string(resID_byte1, locate_text_res(gameresptr, (char *)aOpp));
            break;
        case 2:
            /* Opponent near + right arrow */
            sprite_putimage_transparent(sdgame2shapes[4], 228, 113);
            rect_union(&rect_ingame_text4, &rect_ingame_text, &rect_ingame_text);
            copy_string(resID_byte1, locate_text_res(gameresptr, (char *)aOpp_0));
            break;
    }

    if (resID_byte1[0] != 0) {
        rect_union(intro_draw_text(resID_byte1,
                   font_get_centered_x(resID_byte1), 116, dialog_fnt_colour, 0),
                   &rect_ingame_text, &rect_ingame_text);
    }

    /* Penalty display */
    if (show_penalty_counter == 0) goto done_text;
    copy_string(resID_byte1, locate_text_res(gameresptr, (char *)aPen));
    format_frame_as_string(resID_byte1 + strlen(resID_byte1), penalty_time, 0);
    {
        int x = font_get_centered_x(resID_byte1);
        struct RECTANGLE* r = intro_draw_text(resID_byte1, x, 102,
                                              dialog_fnt_colour, 0);
        rect_union(r, &rect_ingame_text, &rect_ingame_text);
    }
    goto done_text;

replay_check:
    if (game_replay_mode != 2) goto done_text;
    {
        unsigned int frame_mod;
        si = state.game_frame;
        frame_mod = (unsigned)si % framespersec;
        if ((int)(framespersec >> 1) <= (int)frame_mod) goto done_text;

        copy_string(resID_byte1, locate_text_res(gameresptr, (char *)aRpl_0));
        {
            int x = 312 - (int)strlen(resID_byte1) * 8;
            struct RECTANGLE* r = intro_draw_text(resID_byte1, x, 15,
                                                  dialog_fnt_colour, 0);
            rect_union(r, &rect_ingame_text, &rect_ingame_text);
        }
    }

done_text:
    return &rect_ingame_text;
}

/**--------------------------------------------------------------
 * @brief Render the windshield crack animation overlay.
 *
 * @param frame_count  Current animation frame.
 * @param crack_y_offset  Vertical offset for the crack.
 * @param crack_y_scale   Vertical scale for the crack.
 * @return Pointer to the bounding rectangle of the crack overlay.
 /*--------------------------------------------------------------*/
struct RECTANGLE*  init_crak(int frame_count, int crack_y_offset, int crack_y_scale) {
    int frame_index, segment_count;
    int x2, y2, x1, y1;
    struct POINT2D point;
    char * cracshape;
    char * cinfshape;
    int si, count;
    int cx;

    cracshape = locate_shape_alt(gameresptr, (char *)aCrak);
    cinfshape = locate_shape_alt(gameresptr, (char *)aCinf);

    cx = framespersec / 7;
    frame_index = frame_count / cx;

    count = *(short *)cinfshape;
    if (frame_index >= count) {
        frame_index = count - 1;
    }

    segment_count = ((short *)cinfshape)[frame_index + 1];

    rect_ingame_text = rect_invalid;

    for (si = 0; si < segment_count; si++) {
        short * crk = (short *)cracshape;
        x1 = crk[si * 4 + 0];
        y1 = crk[si * 4 + 1];
        x2 = crk[si * 4 + 2];
        y2 = crk[si * 4 + 3];

        /* Scale to screen size */
        y1 = (int)((long)y1 * (long)crack_y_scale / 200L);
        y2 = (int)((long)y2 * (long)crack_y_scale / 200L);

        /* Draw 3 lines: black outline + colored center */
        preRender_line(x1, y1 + crack_y_offset - 1, x2, y2 + crack_y_offset - 1, 0);
        preRender_line(x1, y1 + crack_y_offset + 1, x2, y2 + crack_y_offset + 1, 0);
        preRender_line(x1, y1 + crack_y_offset, x2, y2 + crack_y_offset, dialog_fnt_colour);

        if (timertestflag_copy != 0) {
            /* Update bounding rect */
            point.px = x1;
            point.py = y1 + crack_y_offset - 1;
            rect_adjust_from_point(&point, &rect_ingame_text);

            point.px = x2;
            point.py = y2 + crack_y_offset + 1;
            rect_adjust_from_point(&point, &rect_ingame_text);

            point.px = x1;
            point.py = y1 + crack_y_offset + 1;
            rect_adjust_from_point(&point, &rect_ingame_text);

            point.px = x2;
            point.py = y2 + crack_y_offset - 1;
            rect_adjust_from_point(&point, &rect_ingame_text);
        }
    }

    return &rect_ingame_text;
}

/**--------------------------------------------------------------
 * @brief Run the title intro animation sequence setup and frame loop.
 /*--------------------------------------------------------------*/
unsigned short  setup_intro(void) {
    short intro_frame_counter;
    short* star_point_count_ptr = 0;
    struct RECTANGLE star_points_buffer_a[50];
    short star_point_count_a;
    struct RECTANGLE star_points_buffer_b[50];
    short star_point_count_b;
    struct POINT2D* star_points_ptr = 0;
    short draw_car_flag;
    short camera_pitch;
    short opponent_x, opponent_y, opponent_z;
    struct { short x, y, z; } stars[100];
    short camera_delta;
    short frame_delta;
    short camera_x, camera_y, camera_z;
    char intro_interrupted;
    short rect_buffer_index;
    struct RECTANGLE dirty_rect;
    short camera_yaw;
    struct RECTANGLE combined_dirty_rect;
    char * logo_shapes[3];
    struct RECTANGLE rendered_rect;
    short end_phase;
    short camera_distance;
    short target_x = RENDER_INTRO_CAMERA_CENTER, target_y = RENDER_INTRO_CAMERA_Y_CRUISE, target_z = RENDER_INTRO_CAMERA_CENTER;
    char * titleres;
    int i;

    (void)i;

    intro_interrupted = 0;

    /* Load title 3D resource */
    titleres = (char *)file_load_3dres((char *)aTitle);

    /* Locate logo shapes */
    locate_many_resources(titleres, (char *)aLogolog2brav, logo_shapes);
    shape3d_init_shape(logo_shapes[0], &intro_logoshape);
    shape3d_init_shape(logo_shapes[1], &intro_logo2shape);
    shape3d_init_shape(logo_shapes[2], &intro_bravshape);

    /* Create sprite window if needed */
    if (video_flag5_is0 == 0) {
        wndsprite = sprite_make_wnd(RENDER_SCREEN_WIDTH, RENDER_SCREEN_HEIGHT, RENDER_INTRO_WND_DEPTH);
    }

    /* Initialize star positions */
    for (camera_delta = 0; camera_delta < RENDER_STAR_COUNT; camera_delta++) {
        stars[camera_delta].x = (get_kevinrandom() << 7) - 16384;
        stars[camera_delta].y = 5000 - (get_kevinrandom() << 7);
        stars[camera_delta].z = (get_kevinrandom() << 7) - 16384;
    }

    set_projection(40, 40, RENDER_SCREEN_WIDTH, RENDER_SCREEN_HEIGHT);

    camera_x = RENDER_INTRO_CAMERA_CENTER;
    camera_z = RENDER_INTRO_CAMERA_CENTER;
    camera_y = RENDER_INTRO_CAMERA_Y_START;
    end_phase = 0;
    intro_frame_counter = 0;

    /* Load car model for intro */
    {
        char * carres;
        carres = (char *)file_load_resfile((char *)aCarcoun_0);
        setup_aero_trackdata(carres, 1);
        unload_resource(carres);
    }

    init_plantrak_();
    timer_get_delta();

/** @brief Fps.
 * @param gameplay Parameter `gameplay`.
 * @param framespersec Parameter `framespersec`.
 * @return Function result.
 */
    /* The intro always runs at 20 fps (matching DOS hardware detection default).
     * framespersec is 0 at first launch (only set during gameplay), so initialise
     * it here when needed — same value the DOS speed-detection would have chosen. */
    if (framespersec == 0) {
        framespersec = 30;
    }
    inverse_fps_hundredths = 100 / framespersec;
    angular_velocity_state = 0;

    star_point_count_b = 0;
    star_point_count_a = 0;
    timertestflag_copy = timertestflag;

    frame_dirty_rects[0].left = RENDER_SCREEN_LEFT;
    frame_dirty_rects[0].right = RENDER_SCREEN_WIDTH;
    frame_dirty_rects[0].top = RENDER_SCREEN_TOP;
    frame_dirty_rects[0].bottom = RENDER_SCREEN_HEIGHT;
    terrain_rect = frame_dirty_rects[0];
    intro_dirty_clip_rect = frame_dirty_rects[0];

    rect_buffer_index = 0;

    /* Main intro loop */
    for (;;) {
        /* Poll SDL events first to prevent blocking on timer when events are pending */
        kb_poll_sdl_input();
        
        frame_delta = (short)timer_get_delta();
        if (inverse_fps_hundredths == 0) {
            inverse_fps_hundredths = 1;
        }
        angular_velocity_state += frame_delta;

        while (angular_velocity_state > inverse_fps_hundredths) {
            angular_velocity_state -= inverse_fps_hundredths;
            update_opponent_car_state();

            /* Check if intro should end */
            {
                int limit;
                limit = framespersec;
                limit = ((limit << 2) + limit) << 1;
                limit += framespersec;  /* limit = framespersec * 11 */
                intro_frame_counter++;
                if (intro_frame_counter > limit) {
                    end_phase = 1;
                    camera_y += RENDER_INTRO_CAMERA_Y_STEP;
                    camera_z -= 5;

                    /* Oscillate camera angle toward intro center */
                    camera_delta = camera_x - RENDER_INTRO_CAMERA_CENTER;
                    if (camera_delta < 0) {
                        if (camera_delta < -RENDER_INTRO_CAMERA_ADJUST_STEP) camera_x += RENDER_INTRO_CAMERA_ADJUST_STEP;
                    } else if (camera_delta > 0) {
                        if (camera_delta >= RENDER_INTRO_CAMERA_ADJUST_STEP) camera_x -= RENDER_INTRO_CAMERA_ADJUST_STEP;
                        else camera_x = RENDER_INTRO_CAMERA_CENTER;
                    }

                    if (target_x > RENDER_INTRO_CAMERA_CENTER) target_x--;
                    else if (target_x < RENDER_INTRO_CAMERA_CENTER) target_x++;

                    if (target_z > RENDER_INTRO_CAMERA_CENTER) target_z--;
                    else if (target_z < RENDER_INTRO_CAMERA_CENTER) target_z++;
                }
            }
        }

        /* Always render, even if physics didn't tick (fixes intro refresh bug) */
        /* if (no_tick_flag == 0) goto check_input; */

        if (video_flag5_is0 != 0) {
            setup_mcgawnd2();
        } else {
            sprite_select_wnd_as_sprite1();
        }

        /* Set camera position from opponent car */
        camera_yaw = -1;
        draw_car_flag = 1;
        opponent_x = (short)((long)state.opponentstate.car_posWorld1.lx >> 6);
        opponent_y = (short)((long)state.opponentstate.car_posWorld1.ly >> 6);
        opponent_z = (short)((long)state.opponentstate.car_posWorld1.lz >> 6);

        {
            int early_limit;
            early_limit = framespersec;
            early_limit = (early_limit << 1) + early_limit;
            early_limit <<= 1; /* framespersec * 6 */
            if (intro_frame_counter < early_limit) {
                draw_car_flag = 0;
                camera_yaw = state.opponentstate.car_rotate.x & 1023;
                camera_pitch = 0;
                camera_x = opponent_x;
                camera_y = opponent_y + 20;
                camera_z = opponent_z;
            } else {
                int mid_limit;
                mid_limit = framespersec;
                mid_limit = ((mid_limit << 2) + mid_limit) << 1;
                mid_limit += framespersec; /* framespersec * 11 */
                if (intro_frame_counter < mid_limit) {
                    camera_x = RENDER_INTRO_CAMERA_CENTER;
                    camera_z = RENDER_INTRO_CAMERA_CENTER;
                    camera_y = RENDER_INTRO_CAMERA_Y_CRUISE;
                    target_x = opponent_x;
                    target_y = opponent_y;
                    target_z = opponent_z;
                }
            }
        }

        if (camera_yaw == -1) {
            /* Compute camera angle from position delta */
            camera_yaw = -polarAngle(target_x - camera_x, target_z - camera_z) & RENDER_ANGLE_MASK;
            camera_distance = polarRadius2D(target_x - camera_x, target_z - camera_z);
            camera_pitch = polarAngle(target_y - camera_y, camera_distance) & RENDER_ANGLE_MASK;
        }

        /* Set up rect tracking buffers */
        if (timertestflag_copy != 0) {
            if (rect_buffer_index == 0) {
                star_points_ptr = (struct POINT2D*)star_points_buffer_b;
                star_point_count_ptr = (short*)&star_point_count_b;
            } else {
                star_points_ptr = (struct POINT2D*)star_points_buffer_a;
                star_point_count_ptr = (short*)&star_point_count_a;
            }
        }

        /* Call intro_op */
        {
            struct RECTANGLE callrect;
            int bx = rect_buffer_index;
            callrect = frame_dirty_rects[bx];

            intro_op(camera_x, camera_y, camera_z, camera_yaw, camera_pitch,
                     draw_car_flag, end_phase, (short*)stars, star_points_ptr,
                     star_point_count_ptr, &callrect, &rendered_rect, &dirty_rect);
        }

        if (video_flag5_is0 != 0) {
            mouse_draw_opaque_check();
            setup_mcgawnd1();
            mouse_draw_transparent_check();
            if (timertestflag_copy != 0) {
                frame_dirty_rects[rect_buffer_index] = rendered_rect;
            }
            rect_buffer_index ^= 1;
        } else {
            sprite_copy_2_to_1();
            if (timertestflag_copy != 0) {
                rect_union(&dirty_rect, &world_object_rect, &combined_dirty_rect);
                if (rect_intersect(&combined_dirty_rect, &intro_dirty_clip_rect) == 0) {
                    sprite_set_1_size(combined_dirty_rect.left, combined_dirty_rect.right,
                                      combined_dirty_rect.top, combined_dirty_rect.bottom);
                    mouse_draw_opaque_check();
                    sprite_putimage(wndsprite->sprite_bitmapptr);
                    mouse_draw_transparent_check();
                    frame_dirty_rects[0] = rendered_rect;
                    world_object_rect = dirty_rect;
                }
            } else {
                mouse_draw_opaque_check();
                sprite_putimage(wndsprite->sprite_bitmapptr);
                mouse_draw_transparent_check();
            }
        }

        video_refresh();

        if (input_do_checking(frame_delta) != 0) {
            intro_interrupted = 1;
            break;
        }
        if (23 * framespersec <= intro_frame_counter) {
            break;
        }
    }

    /* Cleanup */
    if (video_flag5_is0 == 0) {
        sprite_free_wnd(wndsprite);
    }

    mmgr_free(titleres);

    return (unsigned short)(signed char)intro_interrupted;
}

/*--------------------------------------------------------------
 * intro_op
 /*--------------------------------------------------------------*/
/** @brief Intro op.
 * @param camera_x Parameter `camera_x`.
 * @param camera_y Parameter `camera_y`.
 * @param camera_z Parameter `camera_z`.
 * @param camera_pitch Parameter `camera_pitch`.
 * @param camera_yaw Parameter `camera_yaw`.
 * @param draw_opponent Parameter `draw_opponent`.
 * @param use_primary_logo Parameter `use_primary_logo`.
 * @param star_positions Parameter `star_positions`.
 * @param star_screen_points Parameter `star_screen_points`.
 * @param star_count Parameter `star_count`.
 * @param current_rect Parameter `current_rect`.
 * @param clip_rect Parameter `clip_rect`.
 * @param previous_rect_ptr Parameter `previous_rect_ptr`.
 */
void intro_op(int camera_x, int camera_y, int camera_z, int camera_pitch, int camera_yaw,
              int draw_opponent, int use_primary_logo, short* star_positions, struct POINT2D* star_screen_points,
              short* star_count, struct RECTANGLE* current_rect,
              struct RECTANGLE* clip_rect, struct RECTANGLE* previous_rect_ptr) {
    struct VECTOR star_camera_vec;
    struct POINT2D screen_point;
    struct VECTOR star_offset_vec;
    struct RECTANGLE union_rect;
    struct RECTANGLE dirty_rect;
    struct TRANSFORMEDSHAPE3D intro_shape;
    struct RECTANGLE rendered_rect;
    int si, di;

    rendered_rect = rect_invalid;

    select_cliprect_rotate(0, camera_yaw, camera_pitch, &intro_cliprect, 0);

    /* Set up logo shape */
    if (use_primary_logo != 0) {
        intro_shape.shapeptr = &intro_logoshape;
    } else {
        intro_shape.shapeptr = &intro_logo2shape;
    }

    intro_shape.pos.x = RENDER_INTRO_CAMERA_CENTER - camera_x;
    intro_shape.pos.y = -camera_y;
    intro_shape.pos.z = RENDER_INTRO_CAMERA_CENTER - camera_z;

    if (timertestflag_copy != 0) {
        intro_shape.rectptr = &rendered_rect;
        intro_shape.ts_flags = RENDER_INTRO_LOGO_FLAGS_TRACKED;
    } else {
        intro_shape.ts_flags = 4;
    }

    intro_shape.rotvec.x = 0;
    intro_shape.rotvec.y = 0;
    intro_shape.rotvec.z = 0;
    intro_shape.shape_visibility_threshold = RENDER_VISIBILITY_BASE * REND_DIST_MULT;
    intro_shape.material = 0;
    shape3d_render_transformed(&intro_shape);

    /* Draw opponent car if visible */
    if (draw_opponent != 0) {
        intro_shape.pos.x = (short)((long)state.opponentstate.car_posWorld1.lx >> 6) - camera_x;
        intro_shape.pos.y = (short)((long)state.opponentstate.car_posWorld1.ly >> 6) - camera_y;
        intro_shape.pos.z = (short)((long)state.opponentstate.car_posWorld1.lz >> 6) - camera_z;

        intro_shape.shapeptr = &intro_bravshape;
        if (timertestflag_copy != 0) {
            intro_shape.rectptr = &rendered_rect;
            intro_shape.ts_flags = RENDER_INTRO_LOGO_FLAGS_TRACKED;
        } else {
            intro_shape.ts_flags = 4;
        }

        intro_shape.rotvec.x = 0;
        intro_shape.rotvec.y = 0;
        intro_shape.rotvec.z = -state.opponentstate.car_rotate.x;
        intro_shape.shape_visibility_threshold = RENDER_VISIBILITY_BASE * REND_DIST_MULT;
        intro_shape.material = 0;
        shape3d_render_transformed(&intro_shape);
    }

    /* Rect tracking: clear old points */
    if (timertestflag_copy != 0) {
        if (*star_count != 0) {
            for (si = 0; si < *star_count; si++) {
                screen_point = star_screen_points[si];
                putpixel_single_clipped(0, screen_point.py, screen_point.px);
            }
        }

        /* Compute combined dirty rect */
        rect_union(current_rect, clip_rect, &union_rect);
        if (rect_intersect(&union_rect, &intro_dirty_clip_rect) == 0) {
            sprite_set_1_size(union_rect.left, union_rect.right,
                              union_rect.top, union_rect.bottom);
            sprite_clear_sprite1_color(0);
        }
        dirty_rect = rendered_rect;
    } else {
        /* Simple mode: clear entire intro clip rect */
        sprite_set_1_size(intro_cliprect.left, intro_cliprect.right,
                          intro_cliprect.top, intro_cliprect.bottom);
        sprite_clear_sprite1_color(0);
    }

    /* Draw intro clip rect background */
    sprite_set_1_size(intro_cliprect.left, intro_cliprect.right,
                      intro_cliprect.top, intro_cliprect.bottom);

    /* Render stars */
    di = 0;
    for (si = 0; si < RENDER_STAR_COUNT; si++) {
        int starofs = si * 3;
        star_offset_vec.x = star_positions[starofs + 0] - camera_x;
        star_offset_vec.y = star_positions[starofs + 1] - camera_y;
        star_offset_vec.z = star_positions[starofs + 2] - camera_z;

        mat_mul_vector(&star_offset_vec, &mat_temp, &star_camera_vec);

        if (star_camera_vec.z > RENDER_STAR_MIN_Z) {
            vector_to_point(&star_camera_vec, &screen_point);
            putpixel_single_clipped(intro_colorvalue, screen_point.py, screen_point.px);

            if (timertestflag_copy != 0) {
                star_screen_points[di] = screen_point;
                di++;
                rect_adjust_from_point(&screen_point, &dirty_rect);
            }

            intro_colorvalue++;
            if (intro_colorvalue == physics_constants_table) {
                intro_colorvalue = 1;
            }
        }
    }

    if (timertestflag_copy != 0) {
        *star_count = di;
    }

    get_a_poly_info();

    if (timertestflag_copy != 0) {
        *clip_rect = rendered_rect;
        *previous_rect_ptr = dirty_rect;
    }
}

/**--------------------------------------------------------------
 * @brief Present the rendered in-game view to screen, using dirty-rect optimisation.
 *
 * @param frame_rect  Bounding rectangle of the current frame.
 /*--------------------------------------------------------------*/
void render_present_ingame_view(struct RECTANGLE* frame_rect) {
    int si, di;
    struct RECTANGLE* rp;

    (void)di;

    if (video_flag5_is0 != 0) return;

    sprite_copy_2_to_1();

    if (race_condition_state_flag != 0) {
        /* Page flip mode */
        mouse_draw_opaque_check();
        sprite_putimage(wndsprite->sprite_bitmapptr);
        goto finish;
    }

    if (timertestflag_copy == 0) {
        /* No rect tracking: blit full provided rect */
        sprite_set_1_size(frame_rect->left, frame_rect->right,
                  frame_rect->top, frame_rect->bottom);
        goto blit_full;
    }

    /* Full rect-tracking path */
    for (si = 0; si < RENDER_RECT_ARRAY_COUNT; si++) {
        rect_sort_indices[si] = 3;
    }

    if (timertestflag2 == 4) {
        collision_detection_state = sprite_transformation_angle;
    }

    /* Check if rect at index 5 changed */
    if (collision_detection_state == sprite_transformation_angle &&
        rect_buffer_front[5].left == rect_buffer_back[5].left &&
        rect_buffer_front[5].right == rect_buffer_back[5].right &&
        rect_buffer_front[5].top == rect_buffer_back[5].top &&
        rect_buffer_front[5].bottom == rect_buffer_back[5].bottom) {
        rect_sort_indices[5] = 0;
    }

    dirty_rect_count = 0;
    rectlist_add_rects(RENDER_RECT_ARRAY_COUNT, rect_sort_indices, rect_buffer_front,
                       rect_buffer_back, frame_rect,
                       &dirty_rect_count, dirty_rect_array);

    if (dirty_rect_count == 0) {
        /* No dirty rects: use full screen */
        sprite_set_1_size(RENDER_SCREEN_LEFT, RENDER_SCREEN_WIDTH, frame_rect->top, frame_rect->bottom);
        goto blit_full;
    }

    /* Sort and blit dirty rects */
    rect_array_sort_by_top((int)(signed char)dirty_rect_count,
                           dirty_rect_array, dirty_rect_indices);
    mouse_draw_opaque_check();

    for (si = 0; si < (int)(signed char)dirty_rect_count; si++) {
        int idx = dirty_rect_indices[si];
        rp = &dirty_rect_array[idx];
        sprite_set_1_size(rp->left, rp->right, rp->top, rp->bottom);
        sprite_putimage(wndsprite->sprite_bitmapptr);
    }
    goto done;

blit_full:
    mouse_draw_opaque_check();
    sprite_putimage(wndsprite->sprite_bitmapptr);

done:
    /* Cleanup */
finish:
    mouse_draw_transparent_check();
    video_refresh();

    if (timertestflag_copy != 0) {
        collision_detection_state = sprite_transformation_angle;

        /* Copy rect_buffer_front to rect_buffer_back */
        for (si = 0; si < RENDER_RECT_ARRAY_COUNT; si++) {
            rect_buffer_back[si] = rect_buffer_front[si];
        }
    }
}

/**
 * @brief Calculate sphere/ellipse rendering vertices using fixed-point scaling.
 *
 * Ported from seg015.asm.
 *
 * @param src_ptr  Pointer to 6 control-point coordinates (x1,y1,x2,y2,x3,y3).
 * @param dst_ptr  Output vertex buffer written as 16-bit coordinate pairs.
 */
void build_sphere_vertex_buffer(const unsigned short* src_ptr, unsigned* dst_ptr)
{
    const short* src = (const short*)src_ptr;
    short* d = (short*)dst_ptr;
    short half_width, half_height, width_three_quarters, height_three_quarters;
    short quarter_width, quarter_height, vertex_index;
    short half_dx, half_dy, dx_three_quarters, dy_three_quarters, quarter_dx, quarter_dy;
    short si_val, di_val;

    /* src offsets: 0=x1, 1=y1, 2=x2, 3=y2, 4=x3, 5=y3 */
    si_val = src[0];

    /* d[0] = x2 - x1 = width */
    d[0] = src[2] - si_val;
    /* d[1] = y2 - y1 = height */
    d[1] = src[3] - src[1];
    /* d[16] = x3 - x1 = dx (offset 32) */
    d[16] = src[4] - si_val;
    /* d[17] = y3 - y1 = dy (offset 34) */
    d[17] = src[5] - src[1];

    /* Calculate intermediate values */
    half_width = d[0] >> 1;
    quarter_width = half_width >> 1;
    width_three_quarters = half_width + quarter_width;

    half_height = d[1] >> 1;
    quarter_height = half_height >> 1;
    height_three_quarters = half_height + quarter_height;

    half_dx = d[16] >> 1;
    quarter_dx = half_dx >> 1;
    dx_three_quarters = half_dx + quarter_dx;

    half_dy = d[17] >> 1;
    quarter_dy = half_dy >> 1;
    dy_three_quarters = half_dy + quarter_dy;

    /* Scale calculations with 11585 (approx 0.707 = sin/cos 45 degrees) */
    d[8] = multiply_and_scale(d[16] + d[0], 11585);
    d[9] = multiply_and_scale(d[1] + d[17], 11585);

    /* Scale calculations with 14654 (approx 0.900) */
    d[4] = multiply_and_scale(d[0] + half_dx, 14654);
    d[5] = multiply_and_scale(d[1] + half_dy, 14654);
    d[12] = multiply_and_scale(d[16] + half_width, 14654);
    d[13] = multiply_and_scale(d[17] + half_height, 14654);

    /* Scale calculations with 15895 (approx 0.970) */
    d[2] = multiply_and_scale(d[0] + quarter_dx, 15895);
    d[3] = multiply_and_scale(d[1] + quarter_dy, 15895);
    d[14] = multiply_and_scale(d[16] + quarter_width, 15895);
    d[15] = multiply_and_scale(d[17] + quarter_height, 15895);

    /* Scale calculations with 13107 (approx 0.800) */
    d[6] = multiply_and_scale(d[0] + dx_three_quarters, 13107);
    d[7] = multiply_and_scale(d[1] + dy_three_quarters, 13107);
    d[10] = multiply_and_scale(d[16] + width_three_quarters, 13107);
    d[11] = multiply_and_scale(d[17] + height_three_quarters, 13107);

    /* Second half: negative direction calculations */
    si_val = -d[0];

    d[24] = multiply_and_scale(d[16] + si_val, 11585);
    di_val = -d[1];
    d[25] = multiply_and_scale(d[17] + di_val, 11585);

    d[28] = multiply_and_scale(half_dx + si_val, 14654);
    d[29] = multiply_and_scale(half_dy + di_val, 14654);
    d[20] = multiply_and_scale(d[16] - half_width, 14654);
    d[21] = multiply_and_scale(d[17] - half_height, 14654);

    si_val = -d[0];
    d[30] = multiply_and_scale(quarter_dx + si_val, 15895);
    di_val = -d[1];
    d[31] = multiply_and_scale(quarter_dy + di_val, 15895);
    d[18] = multiply_and_scale(d[16] - quarter_width, 15895);
    d[19] = multiply_and_scale(d[17] - quarter_height, 15895);

    d[26] = multiply_and_scale(dx_three_quarters + si_val, 13107);
    d[27] = multiply_and_scale(dy_three_quarters + di_val, 13107);
    d[22] = multiply_and_scale(d[16] - width_three_quarters, 13107);
    d[23] = multiply_and_scale(d[17] - height_three_quarters, 13107);

    /* Final loop: create mirrored vertices */
    for (vertex_index = 0; vertex_index < 16; vertex_index++) {
        short* si_ptr = d + (vertex_index * 2);
        di_val = src[0];
        si_ptr[32] = di_val - si_ptr[0];
        si_ptr[33] = src[1] - si_ptr[1];
        si_ptr[0] += di_val;
        si_ptr[1] += src[1];
    }
}
