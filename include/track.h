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

#ifndef TRACK_H
#define TRACK_H

#include "math.h"
#include "game_types.h"

/* Track setup and management */
int   track_setup(void);
char  setup_track(void);
void  init_rect_arrays(void);
void  load_tracks_menu_shapes(void);
void  draw_track_preview(void);
char  subst_hillroad_track(int terr, int elem);

/* Waypoint / navigation */
short track_waypoint_lookup(short car_waypoint_seq_index, struct VECTOR* waypoint_out,
                             short waypoint_index, short* lookup_aux);

/* Replay */
void replay_apply_steering_correction(void);
void replay_capture_frame_input(int mode);

/* Car physics / state update */
void  update_player_car_state(char);
void  update_opponent_car_state(void);
int   car_car_speed_adjust_collision(struct CARSTATE*, struct CARSTATE*);
short carState_update_wheel_suspension(struct CARSTATE*, short, short);
short detect_penalty(short* extVar2ptr, short* extVar1Eptr);
short car_car_detect_collision(short* pCollPoints, short* pWorldCrds,
                                short* oCollPoints, short* oWorldCrds);
void  upd_statef20_from_steer_input(unsigned char steeringInput);
void  state_spawn_debris_particles(int arg0, int arg2, int arg4);
void  update_world_debris_particles(void);
void  update_grip(struct CARSTATE* carstate, struct SIMD* simd, int isPlayerFlag);
void  update_follow_camera_vectors(void);
void  update_car_speed(char carInputFlags, int isOpponentCar,
                        struct CARSTATE* carState, struct SIMD* simdData);
void  update_crash_state(int a1, int a2);

/* Aerodynamics */
void setup_aero_trackdata(void * carresptr, int is_opponent);

/* BTO track helpers */
int bto_auxiliary1(int tile_col, int tile_row, struct VECTOR* out_points);

#endif /* TRACK_H */
