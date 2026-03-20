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

#ifndef DATA_GAME_H
#define DATA_GAME_H

#include <stdint.h>

#include "math.h"
#include "shape2d.h"
#include "shape3d.h"
#include "game_types.h"
#include "memmgr.h"

/* Global variable declarations - definitions in src/data_game.c and related data files */


/* Input status stack arrays (defined in stunts.c) */

/* ============================================================
 * Global variables defined in src/data_game.c
 * ============================================================ */

/* Function pointers for sprite / image blitter (data_game.c) */

/* Polygon / shape-2D state */

/* Palette / display */

/* Projection data arrays */

/* Trigonometry tables */

/* Matrix / vector scratch-pads */

/* Selection / preview rectangles */

/* Polygon z-order / info pool */

/* Track object / shape tables */

/* SDGAME2 / resource pointers */

/* Car shape resource pointers */

/* Skybox state */

/* Viewport / render geometry */

/* Transformed shape buffers */

/* Angle / rotation state */

/* Physics constants */

/* Car state arrays (two-player capable) */

/* Circular steering buffer (64 bytes each, typed as char scalar for base-address usage) */

/* Track / road interaction shared state */

/* Car physics / BTO shared state */

/* Game control */

/* Exit handler table (defined in data_game.c, managed by stunts.c) */

/* Additional car geometry (second car / opponent variant) */

/* Audio configuration tables */

/* Video / input state (defined in data_game.c) */

/* Track segment properties */

/* Miscellaneous game flags */

/* Keyboard / input arrays (defined in data_game.c) */

/* Render / viewport settings */


extern short         viewport_clipping_bounds[];
extern unsigned short polygon_alternate_color;
extern unsigned short polygon_pattern_type;
extern char          backlights_paint_override;

/* 3-D math scratch-pads */
extern struct MATRIX  mat_temp;
extern unsigned char  cos80[];
extern unsigned char  sin80[];
extern unsigned char  projectiondata5[2];
extern unsigned char  projectiondata8[2];
extern unsigned char  projectiondata9[2];
extern unsigned char  projectiondata10[2];
extern struct RECTANGLE select_rect_rc;

/* Car shape resource pointers and geometry */
extern short *        carresptr;
extern struct VECTOR  carshapevec;
extern struct VECTOR  carshapevecs[];
extern struct VECTOR  oppcarshapevec;
extern struct VECTOR  oppcarshapevecs[];

/* 3-D shape tables */
extern struct SHAPE3D game3dshapes[];
extern short          game_frame_pointer[];
extern unsigned char  trkObjectList[];

/* Material / texture lists */
extern unsigned char* material_clrlist2_ptr_cpy;
extern unsigned char* material_clrlist_ptr_cpy;
extern unsigned char  material_color_list[];
extern unsigned char* material_patlist2_ptr_cpy;
extern unsigned char* material_patlist_ptr_cpy;
extern unsigned char  material_pattern2_list[];
extern unsigned char  material_pattern_list[];

/* Game state */
extern struct GAMESTATE state;


/* Key callback function pointer table (defined in data_game.c) */


/* --- data_game.c globals (additional) --- */
extern short            angle_rotation_state[];
extern int              audio_engine_sound_handle;
extern short            audio_frame_index;
extern char             audio_replay_apply_state;
extern char             audiodriverstring[];
extern unsigned short   button_highlight_color;
extern unsigned short   button_shadow_color;
extern unsigned short   button_text_color;
extern unsigned short   camera_view_matrix;
extern short            camera_y_offset;
extern char             checkpoint_lap_trigger;
extern int              crash_sound_handle;
extern struct PLANE    *current_planptr;
extern short            current_rotation_angle_value;
extern char             current_screen_buffer_selector;
extern char             current_surf_type;
extern struct TRANSFORMEDSHAPE3D *curtransshape_ptr;
extern struct GAMESTATE *cvxptr;
extern char             dashb_toggle;
extern int              dashbmp_y;
extern void            *dasmshapeptr;
extern int              dastbmp_y;
extern int              dialog_fnt_colour;
extern unsigned short   dialogarg2;
extern unsigned short   distance_accumulator_counter;
extern int              elRdWallRelated;
extern unsigned short   elapsed_time1;
extern int              elem_xCenter;
extern int              elem_zCenter;
extern char             followOpponentFlag;
extern struct RECTANGLE frame_dirty_rects[];
extern unsigned short   framespersec;
extern unsigned short   framespersec2;
extern short            gState_144;
extern unsigned short   gState_frame;
extern short            gState_impactSpeed;
extern short            gState_jumpCount;
extern short            gState_pEndFrame;
extern short            gState_penalty;
extern short            gState_topSpeed;
extern short            gState_total_finish_time;
extern long             gState_travDist;
extern unsigned const char g_ascii_props[];
extern char             g_path_buf[GAME_PATH_BUF_SIZE];
extern unsigned char    game_exit_request_flag;
extern unsigned char    game_finish_state;
extern unsigned char    game_mode_state_register;
extern unsigned char    game_pause_counter;
extern int              game_startup_flag;
extern unsigned short   game_timer_milliseconds;
extern struct GAMEINFO  gameconfig;
extern void            *gameresptr;
extern char             gnam_string[];
extern char             gsna_string[];
extern int              height_above_replaybar;
extern unsigned short   highscore_primary_index[];
extern int              hillFlag;
extern unsigned int     idle_counter;
extern char             idle_expired;
extern char             is_in_replay;
extern char             is_in_replay_copy;
extern unsigned short   joyinputcode;
extern unsigned char    kbinput[];
extern unsigned short   kbjoyflags;
extern short            lap_completion_trigger_flag;
extern void            *mainresptr;
extern struct MATRIX    mat_car_orientation;
extern unsigned char   *material_clrlist2_ptr;
extern unsigned char   *material_clrlist_ptr;
extern unsigned char   *material_patlist2_ptr;
extern unsigned char   *material_patlist_ptr;
extern int              menu_selection_buffer;
extern short            meter_needle_color;
extern struct SPRITE   *mmouspriteptr;
extern char             mouse_button_press_state[];
extern unsigned short   mouse_oldbut;
extern unsigned short   mousebutinputcode;
extern struct SPRITE   *mouseunkspriteptr;
extern unsigned short   newjoyflags;
extern unsigned short   object_visibility_state;
extern void            *opp_res;
extern char             opponent_audio_state;
extern unsigned char    opponent_speed_table[];
extern short            pState_f36Mminf40sar2;
extern short            pState_minusRotate_x_2;
extern short            pState_minusRotate_y_2;
extern short            pState_minusRotate_z_2;
extern char             passed_security;
extern char            *path_col;
extern char            *path_conn_flags;
extern char            *path_row;
extern short            penalty_time;
extern int              performGraphColor;
extern short            planindex;
extern short            planindex_copy;
extern struct PLANE    *planptr;
extern char             player_audio_state;
extern char             player_name_buffer[];
extern char             race_condition_state_flag;
extern struct RECTANGLE rect_buffer_back[];
extern struct RECTANGLE rect_buffer_front[];
extern struct RECTANGLE rect_invalid;
extern char             replay_audio_frame_buffer[];
extern unsigned char    replay_autoplay_active;
extern char            *replay_buffer;
extern unsigned char    replay_capture_frame_skip_counter;
extern unsigned char    replay_fps_inverse_scale;
extern unsigned short   replay_frame_counter;
extern char            *replay_header;
extern char             replay_mode_state_flag;
extern char             replaybar_enabled;
extern char             replaybar_toggle;
extern char             resID_byte1[2048];
extern int              roofbmpheight;
extern int              run_game_random;
extern unsigned char    sceneshapes2[];
extern unsigned char    sceneshapes3[];
extern int              screen_display_toggle_flag;
extern char             screen_shake_intensity;
extern unsigned short   sdgame2_widths[];
extern void            *sdgame2shapes[];
extern void            *sdgameresptr;
extern unsigned char    shapeinfos[];
extern char             show_penalty_counter;
extern struct SIMD      simd_player;
extern unsigned short   skybox_grd_color;
extern struct SPRITE   *smouspriteptr;
extern void            *songfileptr;
extern short            sprite_row_offset_table;
extern short            sprite_transformation_angle;
extern int              startcol2;
extern int              startrow2;
extern void            *steerWhlRespTable_ptr;
extern int              terrainHeight;
extern struct RECTANGLE terrain_rect;
extern int              terraincenterpos[];
extern int              terrainpos[];
extern int              terrainrows[];
extern unsigned char   *tile_obstacle_map;
extern short            timer_tick_counter;
extern unsigned short   timertestflag;
extern unsigned short   timertestflag_copy;
extern short            track_angle;
extern char            *track_cam_height_base;
extern unsigned char    track_camera_coords[];
extern unsigned char   *track_elem_map;
extern char            *track_elem_ordered;
extern char            *track_element_height_ofs;
extern char             track_object_render_enabled;
extern short            track_pieces_counter;
extern unsigned char   *track_terrain_map;
extern char            *track_waypoint_order;
extern int              trackcenterpos[];
extern int              trackcenterpos2[];
extern int              trackpos[];
extern int              trackpos2[];
extern int              trackrows[];
extern char             transformedshape_arg2array[];
extern char             transformedshape_counter;
extern short            transformedshape_indices[];
extern short            transformedshape_zarray[];
extern struct VECTOR    vec_movement_local;
extern struct VECTOR    vec_planerotopresult;
extern short            video_flag1_is1;
extern short            video_flag2_is1;
extern short            video_flag3_isFFFF;
extern short            video_flag4_is1;
extern short            video_flag5_is0;
extern char             video_flag6_is1;
extern void            *voicefileptr;
extern int              waitflag;
extern int              wallHeight;
extern short            wallOrientation;
extern int              wallStartX;
extern int              wallStartZ;
extern int              wallindex;
extern short           *wallptr;
extern short           *waypoint_world_pos;
extern char            *window_save_buf;
extern struct SPRITE   *wndsprite;
extern struct RECTANGLE world_object_rect;

/* Additional data_game.c globals */
extern short           *aero_table_opponent;
extern short           *aero_table_player;
extern unsigned char    game_replay_mode;
extern char            *highscore_data;
extern short            inverse_fps_hundredths;
extern short           *obstacle_rot_z;
extern unsigned char   *obstacle_scene_index;
extern short           *obstacle_world_pos;
extern char             textresprefix;
extern short           *track_waypoint_alt;
extern short           *track_waypoint_next;
extern unsigned char   *transshapepolyinfo;
extern unsigned char   *transshapeprimindexptr;
extern unsigned char   *transshapeprimitives;
extern unsigned char   *transshapeprimptr;
extern struct RECTANGLE *transshaperectptr;
extern struct VECTOR   *transshapeverts;

/* Terrain / physics constants (moved from data_core.c) */
extern short            hillHeightConsts[];
extern char             aMain[];
extern unsigned char    aDesert[];
extern unsigned char    aAlpine[];
extern unsigned char    aCity[];
extern unsigned char    aCountry[];

/* Camera / detail-level control state (moved from data_core.c) */
extern char             cameramode;
extern unsigned char    timertestflag2;
extern unsigned short   rotation_x_angle;
extern unsigned short   rotation_z_angle;
extern unsigned short   rotation_y_angle;

/* Frame buffer pointers — defined in frame.c */
extern struct RECTANGLE *rect_buffer_primary;
extern struct RECTANGLE *rect_buffer_secondary;


#endif /* DATA_GAME_H */
