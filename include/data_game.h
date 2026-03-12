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

#ifdef DATA_GAME_IMPL
#  define _DG_
#else
#  define _DG_ extern
#endif

_DG_ short         viewport_clipping_bounds[];
_DG_ unsigned short polygon_alternate_color;
_DG_ unsigned short polygon_pattern_type;
_DG_ char          backlights_paint_override;

/* 3-D math scratch-pads */
_DG_ struct MATRIX  mat_temp;
_DG_ unsigned char  cos80[];
_DG_ unsigned char  sin80[];
_DG_ unsigned char  projectiondata5[2];
_DG_ unsigned char  projectiondata8[2];
_DG_ unsigned char  projectiondata9[2];
_DG_ unsigned char  projectiondata10[2];
_DG_ struct RECTANGLE select_rect_rc;

/* Car shape resource pointers and geometry */
_DG_ short *        carresptr;
_DG_ struct VECTOR  carshapevec;
_DG_ struct VECTOR  carshapevecs[];
_DG_ struct VECTOR  oppcarshapevec;
_DG_ struct VECTOR  oppcarshapevecs[];

/* 3-D shape tables */
_DG_ struct SHAPE3D game3dshapes[];
_DG_ short          game_frame_pointer[];
_DG_ unsigned char  trkObjectList[];

/* Material / texture lists */
_DG_ unsigned char* material_clrlist2_ptr_cpy;
_DG_ unsigned char* material_clrlist_ptr_cpy;
_DG_ unsigned char  material_color_list[];
_DG_ unsigned char* material_patlist2_ptr_cpy;
_DG_ unsigned char* material_patlist_ptr_cpy;
_DG_ unsigned char  material_pattern2_list[];
_DG_ unsigned char  material_pattern_list[];

/* Game state */
_DG_ struct GAMESTATE state;


/* Key callback function pointer table (defined in data_game.c) */


/* --- data_game.c globals (additional) --- */
_DG_ short            angle_rotation_state[];
_DG_ int              audio_engine_sound_handle;
_DG_ short            audio_frame_index;
_DG_ char             audio_replay_apply_state;
_DG_ char             audiodriverstring[];
_DG_ unsigned short   button_highlight_color;
_DG_ unsigned short   button_shadow_color;
_DG_ unsigned short   button_text_color;
_DG_ unsigned short   camera_view_matrix;
_DG_ short            camera_y_offset;
_DG_ char             checkpoint_lap_trigger;
_DG_ int              crash_sound_handle;
_DG_ struct PLANE    *current_planptr;
_DG_ short            current_rotation_angle_value;
_DG_ char             current_screen_buffer_selector;
_DG_ char             current_surf_type;
_DG_ struct TRANSFORMEDSHAPE3D *curtransshape_ptr;
_DG_ struct GAMESTATE *cvxptr;
_DG_ char             dashb_toggle;
_DG_ int              dashbmp_y;
_DG_ void            *dasmshapeptr;
_DG_ int              dastbmp_y;
_DG_ int              dialog_fnt_colour;
_DG_ unsigned short   dialogarg2;
_DG_ unsigned short   distance_accumulator_counter;
_DG_ int              elRdWallRelated;
_DG_ unsigned short   elapsed_time1;
_DG_ int              elem_xCenter;
_DG_ int              elem_zCenter;
_DG_ char             followOpponentFlag;
_DG_ struct RECTANGLE frame_dirty_rects[];
_DG_ unsigned short   framespersec;
_DG_ unsigned short   framespersec2;
_DG_ short            gState_144;
_DG_ unsigned short   gState_frame;
_DG_ short            gState_impactSpeed;
_DG_ short            gState_jumpCount;
_DG_ short            gState_pEndFrame;
_DG_ short            gState_penalty;
_DG_ short            gState_topSpeed;
_DG_ short            gState_total_finish_time;
_DG_ long             gState_travDist;
_DG_ unsigned const char g_ascii_props[];
_DG_ char             g_path_buf[];
_DG_ unsigned char    game_exit_request_flag;
_DG_ unsigned char    game_finish_state;
_DG_ unsigned char    game_mode_state_register;
_DG_ unsigned char    game_pause_counter;
_DG_ int              game_startup_flag;
_DG_ unsigned short   game_timer_milliseconds;
_DG_ struct GAMEINFO  gameconfig;
_DG_ void            *gameresptr;
_DG_ char             gnam_string[];
_DG_ char             gsna_string[];
_DG_ int              height_above_replaybar;
_DG_ unsigned short   highscore_primary_index[];
_DG_ char             hillFlag;
_DG_ unsigned int     idle_counter;
_DG_ char             idle_expired;
_DG_ char             is_in_replay;
_DG_ char             is_in_replay_copy;
_DG_ unsigned short   joyinputcode;
_DG_ unsigned char    kbinput[];
_DG_ unsigned short   kbjoyflags;
_DG_ short            lap_completion_trigger_flag;
_DG_ void            *mainresptr;
_DG_ struct MATRIX    mat_car_orientation;
_DG_ unsigned char   *material_clrlist2_ptr;
_DG_ unsigned char   *material_clrlist_ptr;
_DG_ unsigned char   *material_patlist2_ptr;
_DG_ unsigned char   *material_patlist_ptr;
_DG_ int              menu_selection_buffer;
_DG_ short            meter_needle_color;
_DG_ struct SPRITE   *mmouspriteptr;
_DG_ char             mouse_button_press_state[];
_DG_ unsigned short   mouse_oldbut;
_DG_ unsigned short   mousebutinputcode;
_DG_ struct SPRITE   *mouseunkspriteptr;
_DG_ unsigned short   newjoyflags;
_DG_ unsigned short   object_visibility_state;
_DG_ void            *opp_res;
_DG_ char             opponent_audio_state;
_DG_ unsigned char    opponent_speed_table[];
_DG_ short            pState_f36Mminf40sar2;
_DG_ short            pState_minusRotate_x_2;
_DG_ short            pState_minusRotate_y_2;
_DG_ short            pState_minusRotate_z_2;
_DG_ char             passed_security;
_DG_ char            *path_col;
_DG_ char            *path_conn_flags;
_DG_ char            *path_row;
_DG_ short            penalty_time;
_DG_ int              performGraphColor;
_DG_ short            planindex;
_DG_ short            planindex_copy;
_DG_ struct PLANE    *planptr;
_DG_ char             player_audio_state;
_DG_ char             player_name_buffer[];
_DG_ char             race_condition_state_flag;
_DG_ struct RECTANGLE rect_buffer_back[];
_DG_ struct RECTANGLE rect_buffer_front[];
_DG_ struct RECTANGLE rect_invalid;
_DG_ char             replay_audio_frame_buffer[];
_DG_ unsigned char    replay_autoplay_active;
_DG_ char            *replay_buffer;
_DG_ unsigned char    replay_capture_frame_skip_counter;
_DG_ unsigned char    replay_fps_inverse_scale;
_DG_ unsigned short   replay_frame_counter;
_DG_ char            *replay_header;
_DG_ char             replay_mode_state_flag;
_DG_ char             replaybar_enabled;
_DG_ char             replaybar_toggle;
_DG_ char             resID_byte1[2048];
_DG_ int              roofbmpheight;
_DG_ int              run_game_random;
_DG_ unsigned char    sceneshapes2[];
_DG_ unsigned char    sceneshapes3[];
_DG_ char             screen_display_toggle_flag;
_DG_ char             screen_shake_intensity;
_DG_ unsigned short   sdgame2_widths[];
_DG_ void            *sdgame2shapes[];
_DG_ void            *sdgameresptr;
_DG_ unsigned char    shapeinfos[];
_DG_ char             show_penalty_counter;
_DG_ struct SIMD      simd_player;
_DG_ unsigned short   skybox_grd_color;
_DG_ struct SPRITE   *smouspriteptr;
_DG_ void            *songfileptr;
_DG_ short            sprite_row_offset_table;
_DG_ short            sprite_transformation_angle;
_DG_ char             startcol2;
_DG_ char             startrow2;
_DG_ void            *steerWhlRespTable_ptr;
_DG_ int              terrainHeight;
_DG_ struct RECTANGLE terrain_rect;
_DG_ int              terraincenterpos[];
_DG_ int              terrainpos[];
_DG_ int              terrainrows[];
_DG_ unsigned char   *tile_obstacle_map;
_DG_ short            timer_tick_counter;
_DG_ unsigned short   timertestflag;
_DG_ unsigned short   timertestflag_copy;
_DG_ short            track_angle;
_DG_ char            *track_cam_height_base;
_DG_ unsigned char    track_camera_coords[];
_DG_ unsigned char   *track_elem_map;
_DG_ char            *track_elem_ordered;
_DG_ char            *track_element_height_ofs;
_DG_ char             track_object_render_enabled;
_DG_ short            track_pieces_counter;
_DG_ unsigned char   *track_terrain_map;
_DG_ char            *track_waypoint_order;
_DG_ int              trackcenterpos[];
_DG_ int              trackcenterpos2[];
_DG_ int              trackpos[];
_DG_ int              trackpos2[];
_DG_ int              trackrows[];
_DG_ char             transformedshape_arg2array[];
_DG_ char             transformedshape_counter;
_DG_ short            transformedshape_indices[];
_DG_ short            transformedshape_zarray[];
_DG_ struct VECTOR    vec_movement_local;
_DG_ struct VECTOR    vec_planerotopresult;
_DG_ short            video_flag1_is1;
_DG_ short            video_flag2_is1;
_DG_ short            video_flag3_isFFFF;
_DG_ short            video_flag4_is1;
_DG_ short            video_flag5_is0;
_DG_ char             video_flag6_is1;
_DG_ void            *voicefileptr;
_DG_ int              waitflag;
_DG_ int              wallHeight;
_DG_ short            wallOrientation;
_DG_ int              wallStartX;
_DG_ int              wallStartZ;
_DG_ int              wallindex;
_DG_ short           *wallptr;
_DG_ short           *waypoint_world_pos;
_DG_ char            *window_save_buf;
_DG_ struct SPRITE   *wndsprite;
_DG_ struct RECTANGLE world_object_rect;

/* Additional data_game.c globals */
_DG_ short           *aero_table_opponent;
_DG_ short           *aero_table_player;
_DG_ unsigned char    game_replay_mode;
_DG_ char            *highscore_data;
_DG_ short            inverse_fps_hundredths;
_DG_ short           *obstacle_rot_z;
_DG_ unsigned char   *obstacle_scene_index;
_DG_ short           *obstacle_world_pos;
_DG_ char             textresprefix;
_DG_ short           *track_waypoint_alt;
_DG_ short           *track_waypoint_next;
_DG_ unsigned char   *transshapepolyinfo;
_DG_ unsigned char   *transshapeprimindexptr;
_DG_ unsigned char   *transshapeprimitives;
_DG_ unsigned char   *transshapeprimptr;
_DG_ struct RECTANGLE *transshaperectptr;
_DG_ struct VECTOR   *transshapeverts;

#undef _DG_

#endif /* DATA_GAME_H */
