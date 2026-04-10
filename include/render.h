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

#ifndef RENDER_H
#define RENDER_H

#include "math.h"
#include "game_types.h"
#include "shape2d.h"

struct TRANSFORMEDSHAPE3D;

extern short angle_rotation_state[];
extern unsigned short button_highlight_color;
extern unsigned short button_shadow_color;
extern unsigned short button_text_color;
extern unsigned short camera_view_matrix;
extern short camera_y_offset;
extern struct PLANE *current_planptr;
extern short current_rotation_angle_value;
extern char current_screen_buffer_selector;
extern void *dasmshapeptr;
extern int dashbmp_y;
extern int dastbmp_y;
extern int dialog_fnt_colour;
extern unsigned short dialogarg2;
extern struct RECTANGLE frame_dirty_rects[];
extern unsigned short framespersec2;
extern int height_above_replaybar;
extern char idle_expired;
extern struct MATRIX mat_car_orientation;
extern short meter_needle_color;
extern struct SPRITE *mmouspriteptr;
extern struct SPRITE *mouseunkspriteptr;
extern unsigned short object_visibility_state;
extern void *opp_res;
extern short pState_f36Mminf40sar2;
extern short pState_minusRotate_x_2;
extern short pState_minusRotate_y_2;
extern short pState_minusRotate_z_2;
extern int performGraphColor;
extern struct RECTANGLE rect_buffer_back[];
extern struct RECTANGLE rect_buffer_front[];
extern struct RECTANGLE rect_invalid;
extern unsigned char replay_fps_inverse_scale;
extern char resID_byte1[2048];
extern int roofbmpheight;
extern int screen_display_toggle_flag;
extern char screen_shake_intensity;
extern unsigned short sdgame2_widths[];
extern void *sdgame2shapes[];
extern unsigned short skybox_grd_color;
extern struct SPRITE *smouspriteptr;
extern short sprite_row_offset_table;
extern short sprite_transformation_angle;
extern struct RECTANGLE terrain_rect;
extern struct PLANE *planptr;
extern struct VECTOR vec_movement_local;
extern struct VECTOR vec_planerotopresult;
extern short video_flag1_is1;
extern short video_flag2_is1;
extern short video_flag3_isFFFF;
extern short video_flag4_is1;
extern bool video_flag5_is0;
extern char video_flag6_is1;
extern int waitflag;
extern struct SPRITE *wndsprite;
extern struct RECTANGLE world_object_rect;
extern char cameramode;
extern unsigned char timertestflag2;
extern unsigned short rotation_x_angle;
extern unsigned short rotation_z_angle;
extern unsigned short rotation_y_angle;

/* Video mode control */
void video_set_mode4(void);
void video_set_mode7(void);
void video_sdl_init(void);
void video_set_palette(unsigned short start, unsigned short count, unsigned char *colors);
unsigned char *video_get_framebuffer(void);
void video_present_frame(void);
void video_refresh(void);
void video_wait_menu_refresh(void);
int video_is_sdl_active(void);
int video_get_status(void);

/* Frame / render cycle */
void setup_car_shapes(int);
void update_frame(int, struct RECTANGLE *rc);
void set_frame_callback(void);
void remove_frame_callback(void);
void render_present_ingame_view(struct RECTANGLE *r);
void setup_mcgawnd2(void);
void preRender_line(unsigned short startX, unsigned short startY, unsigned short endX,
                    unsigned short endY, unsigned short color);
void sprite_fill_rect(unsigned short x, unsigned short y, unsigned short width,
                      unsigned short height, unsigned char color);
extern struct TRANSFORMEDSHAPE3D *curtransshape_ptr;
extern unsigned char transformedshape_arg2array[];
extern char transformedshape_counter;
extern short transformedshape_indices[];
extern short transformedshape_zarray[];
extern struct RECTANGLE *rect_buffer_primary;
extern struct RECTANGLE *rect_buffer_secondary;

/* Sprite / window management */
void sprite_set_1_size(unsigned short left, unsigned short right, unsigned short top,
                       unsigned short height);
void sprite_clear_sprite1_color(unsigned char);
void sprite_select_wnd_as_sprite1(void);
void sprite_select_wnd_as_sprite1_and_clear(void);
unsigned short sprite_blit_to_video(struct SPRITE *sprite, unsigned short mode);
void sprite_copy_2_to_1(void);
void sprite_shape_to_1(struct SHAPE2D *shape, unsigned short x, unsigned short y);
void sprite_draw_rect_outline(unsigned short x1, unsigned short y1, unsigned short x2,
                              unsigned short y2, unsigned short color);
void sprite_draw_dithered_pass(int idx, struct SPRITE *sprite);
void sprite_copy_rect_2_to_1(int left, int top, int right, int bottom, int color);
void sprite_draw_text_opaque(char *text, int a2, int a3);
void sprite_draw_shape_with_palmap(void *shape);
void sprite_clear_shape_alt(void *shape, unsigned short x, unsigned short y);
void sprite_blit_shape_to_sprite1(void *shape);

/* Sprite blitting helpers */
void sprite_putimage_transparent(void *shape, unsigned short x, unsigned short y);
void sprite_putimage(struct SHAPE2D *shape);
void sprite_putimage_and_alt(void *shapeptr, unsigned short x, unsigned short y);
void sprite_putimage_and_at_shape_origin(void *shapeptr, unsigned short x, unsigned short y);
void sprite_putimage_or_at_shape_origin(void *shapeptr, unsigned short x, unsigned short y);
void sprite_putimage_and_alt2(void *shapeptr, unsigned short x, unsigned short y);
void sprite_putimage_or_alt(void *shapeptr, unsigned short x, unsigned short y);

/* Icon pixel helpers */
void putpixel_iconMask(void *shape, unsigned short x, unsigned short y);
void putpixel_iconFillings(void *shape, unsigned short x, unsigned short y);

/* 3D / car shape loading */
void shape3d_load_car_shapes(char *carid, char *oppcarid);
void shape3d_free_car_shapes(void);
void load_opponent_data(void);
void load_sdgame2_shapes(void);
void free_sdgame2(void);
void load_skybox(unsigned char res_id);
void unload_skybox(void);

/* NOP stubs */
void nullsub_1(void);
void nullsub_2(void *resptr, unsigned short idx);

/* Low-level line/fill primitives */
void draw_filled_lines(unsigned short color, unsigned short numlines, unsigned short y,
                       unsigned short *x2arr, unsigned short *x1arr);
unsigned draw_line_related(unsigned col, unsigned numlines, unsigned y, unsigned x, int *arr);

/* Sphere geometry */
void build_sphere_vertex_buffer(const unsigned short *src_ptr, unsigned *dst_ptr);

#endif /* RENDER_H */
