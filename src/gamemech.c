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

/**
 * @file gamemech.c
 * @brief Game mechanics — seg005.asm functions ported to C.
 *
 * Original 16-bit Borland C++ 5.2 medium model (int=short=16-bit).
 *
 * Contains: handle_ingame_kb_shortcuts, set_frame_callback, remove_frame_callback,
 *           frame_callback, replay_capture_frame_input, update_follow_camera_vectors,
 *           setup_car_shapes, mouse_minmax_position, replay_apply_steering_correction,
 *           loop_game
 */

#include <stdlib.h>
#include <stdio.h>
#define GAME_IMPL
#include "stunts.h"
#include "shape2d.h"
#include "memmgr.h"
#include "ressources.h"
#include "keyboard.h"
#include "math.h"
#include "font.h"
#include "ui_widgets.h"

/* Variables moved from data_game.c */
static void * digshapes[10] = { 0 };
static void * gnobshapes[6] = { 0 };
static void * rplyshapes[23] = { 0 };
static void * stdaresptr = 0;
static void * stdbresptr = 0;
static void * whlshapes[9] = { 0 };

/* Variables moved from data_game.c (private to this translation unit) */
static unsigned char car_camera_mode_array[2] = { 0, 0 };
static unsigned char car_collision_flags[2] = { 0, 0 };
static unsigned short car_damage_data_table[2] = { 0, 0 };
static unsigned short car_damage_state_array[2] = { 0, 0 };
static unsigned short car_orientation_data[2] = { 0, 0 };
static unsigned short car_position_x_array[2] = { 0, 0 };
static unsigned short car_position_xyz_array[2] = { 0, 0 };
static unsigned short car_position_y_array[2] = { 0, 0 };
static unsigned short car_position_z_array[2] = { 0, 0 };
static unsigned char car_track_interaction_state = 0;
static unsigned short car_velocity_vector_x[2] = { 0, 0 };
static unsigned char palette_fade_state_buffer = 0;
static unsigned char race_driver_status_array[2] = { 0, 0 };
static unsigned short race_timer_display_values[2] = { 0, 0 };
static unsigned char sprite_offset_table[64] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char sprite_palette_indices[64] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char suspension_compression_state[18] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static char tire_grip_state = 0;
static unsigned char wheel_ground_contact[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static char wheel_rotation_speed = 0;
static unsigned char wheel_slip_direction[2] = { 0, 0 };
static unsigned short wheel_velocity_state[2] = { 0, 0 };
static struct SPRITE * whlsprite1 = 0;
static struct SPRITE * whlsprite2 = 0;
static struct SPRITE * whlsprite3 = 0;


/* file-local data (moved from data_global.c) */
static unsigned char collision_surface_type[34] = { 0, 0, 0, 0, 0, 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 84, 90, 98, 106, 114, 121, 127, 127, 127 };
static char aStdaxxxx[] = "stdaxxxx";
static char aStdbxxxx[] = "stdbxxxx";
static unsigned char minimum_bump_counter = 6;
static unsigned char sprite_animation_frame[10] = { 1, 7, 3, 4, 5, 6, 7, 8, 8, 0 };


/* shape/sprite pointers (arrays of  ptrs in dseg) */


/* Pointer to 'dast' shape (replaces split dastbmp_y2/dastseg storage) */
void * g_dast_shape_ptr = NULL;


/* display/gui arrays */

/* replay bar button arrays */

/* menu navigation tables (moved from data_global.c) */
static unsigned char particle_effect_state[10] = { 0, 0, 2, 2, 3, 4, 5, 1, 7, 0 };
static unsigned char light_level_table[10] = { 2, 6, 2, 3, 4, 5, 6, 7, 8, 0 };
static unsigned char shadow_render_flags[10] = { 0, 1, 0, 0, 1, 1, 1, 7, 8, 0 };

/* game camera button tables (moved from data_global.c) */
static unsigned char game_camera_buttons_count[4] = { 6, 6, 8, 7 };
static unsigned short game_camera_buttons_x1[9] = { 272, 109, 274, 232, 190, 151, 108, 66, 10 };
static unsigned short game_camera_buttons_x2[9] = { 314, 151, 314, 274, 232, 190, 151, 91, 47 };
static unsigned short game_camera_buttons_y1[9] = { 176, 176, 156, 156, 156, 156, 156, 156, 156 };
static unsigned short game_camera_buttons_y2[9] = { 193, 193, 173, 173, 173, 173, 173, 193, 193 };
static unsigned short gameunk_button_x1 = 0;
static unsigned short gameunk_button_x2 = 104;
static unsigned short gameunk_button_y1 = 151;
static unsigned short gameunk_button_y2 = 200;

/* misc button/display coords (moved from data_global.c) */
static unsigned short lighting_intensity_lookup = 156;
static unsigned short lap_time_accumulator = 193;
static unsigned short dither_pattern_config = 156;
static unsigned short race_position_table = 193;
static unsigned short projection_scale_factor = 10;
static unsigned short shadow_depth_table = 47;
static unsigned short terrain_height_table = 1;
static unsigned short track_segment_data_array = 4;


/* ---- forward declarations ---- */
int  handle_ingame_kb_shortcuts(int command);
void set_frame_callback(void);
void remove_frame_callback(void);
void frame_callback(void);
void replay_capture_frame_input(int command);
void update_follow_camera_vectors(void);
void setup_car_shapes(int command);
void mouse_minmax_position(int command);
void replay_apply_steering_correction(void);
void loop_game(int command, int context_value, int frame_value);

/**
 * @brief Process keyboard shortcuts during game.
 *
/** @brief Keys.
 * @param F4 Parameter `F4`.
 * @param handled Parameter `handled`.
 * @param command Parameter `command`.
 * @return Function result.
 */
int  handle_ingame_kb_shortcuts(int command)
{
	int ax = command;

	if (ax == 283 || ax == 27) { /* ESC */
		if (game_replay_mode == 0) {
			update_crash_state(0, 4);
		}
		game_finish_state = 1;
		return 1;
	}

	if (ax == 67 || ax == 99) { /* 'C' or 'c' */
		if (game_replay_mode == 1) {
			/* exit replay, go live */
			game_replay_mode = 0;
			game_pause_counter = 0;
			framespersec = framespersec2;
			*(unsigned char*)&gameconfig.game_framespersec = (unsigned char)framespersec2;
			init_game_state(-1);
		}
		return 1;
	}

	if (ax == 68 || ax == 100) { /* 'D' or 'd' */
		dashb_toggle ^= 1;
		return 1;
	}

	if (ax == 72 || ax == 104) { /* 'H' or 'h' */
		HKeyFlag[0] ^= 1;
		return 1;
	}

	if (ax == 77 || ax == 109) { /* 'M' or 'm' */
		do_mou_restext();
		mouse_minmax_position((char)mouse_motion_state_flag);
		return 1;
	}

	if (ax == 82 || ax == 114) { /* 'R' or 'r' */
		replaybar_toggle ^= 1;
		return 1;
	}

	if (ax == 116) { /* 't' */
		if (gameconfig.game_opponenttype != 0) {
			followOpponentFlag ^= 1;
		}
		return 1;
	}

	if (ax == 15104) { /* F1 */
		cameramode = 0;
		return 1;
	}
	if (ax == 15360) { /* F2 */
		cameramode = 1;
		return 1;
	}
	if (ax == 15616) { /* F3 */
		cameramode = 2;
		return 1;
	}
	if (ax == 15872) { /* F4 */
		cameramode = 3;
		return 1;
	}

	/* default: unrecognized key while in replay mode */
	if (game_replay_mode == 1) {
		game_replay_mode = 0;
		game_pause_counter = 0;
		framespersec = framespersec2;
		*(unsigned char*)&gameconfig.game_framespersec = (unsigned char)framespersec2;
		init_game_state(-1);
	}
	return 0;
}


/**
 * @brief Register the game frame timer callback.
 *
/** @brief Frame callback.
 * @param set_frame_callback Parameter `set_frame_callback`.
 * @return Function result.
 */
void set_frame_callback(void)
{
	game_mode_state_register = 0;
	timer_reg_callback((void *)frame_callback);
	palette_fade_state_buffer = 0;
}

/**
 * @brief Remove the game frame timer callback.
 *
 * Waits for pending timer ticks to drain, then unregisters
/** @brief Frame callback.
 * @param remove_frame_callback Parameter `remove_frame_callback`.
 * @return Function result.
 */
void remove_frame_callback(void)
{
	timer_wait_ticks_and_get_counter(10L);
	timer_remove_callback((void *)frame_callback);
}


/**
 * @brief Timer ISR callback for game frame processing.
 *
 * Called from the timer interrupt. Advances audio frame playback,
 * checks game-finish / checkpoint conditions, and manages replay
 * frame capture at the configured FPS rate.
 */
void frame_callback(void)
{
	if (!compare_ds_ss())
		return;

	if (palette_fade_state_buffer != 0)
		return;

	palette_fade_state_buffer++;
	if (palette_fade_state_buffer != 1)
		goto done;

	sprite_row_offset_table++;
	if (sprite_row_offset_table >= inverse_fps_hundredths) {
		if (audio_frame_index != menu_selection_buffer) {
			audio_replay_update_engine_sounds((unsigned short*)(replay_audio_frame_buffer + 34 * audio_frame_index), 34 * audio_frame_index + 38876);
			sprite_row_offset_table = 0;
			audio_frame_index++;
			if (audio_frame_index == 40)
				audio_frame_index = 0;
		}
	}

	if (game_finish_state != 0) goto done;
	if (checkpoint_lap_trigger != 0) goto done;

	if (is_in_replay != 0 && game_replay_mode == 2) {
		if (replay_autoplay_active == 0)
			goto done;
		/* auto-play: fall through to frame advancement */
	}

	if (game_replay_mode == 0) {
		if (state.game_frame_in_sec >= state.game_frames_per_sec) {
			is_in_replay = 1;
			audio_sync_car_audio();
			goto done;
		}
	}

	replay_fps_inverse_scale--;
	if (replay_fps_inverse_scale != 0) goto done;

	replay_fps_inverse_scale = (unsigned char)inverse_fps_hundredths;
	game_mode_state_register++;

	if (game_replay_mode == 2) {
		if (screen_shake_intensity == 2) {
			replay_capture_frame_skip_counter--;
			if (replay_capture_frame_skip_counter != 0) goto done;
			replay_capture_frame_input(0);
			replay_capture_frame_skip_counter = 2;
			goto done;
		} else if (screen_shake_intensity == 3) {
			replay_capture_frame_input(0);
		}
	}

	replay_capture_frame_input(0);

done:
	palette_fade_state_buffer--;
}


/**
 * @brief Frame recording and input sampling.
 *
 * Samples keyboard, mouse, or joystick input each game frame and
 * stores it into the replay buffer. Handles the replay time limit,
 * the half-lap checkpoint shift, and auto-play termination in
 * replay-capture mode.
 *
 * @param command  When non-zero, stores a zero-input frame immediately
/** @brief Replay capture frame input.
 * @param command Parameter `command`.
 */
void replay_capture_frame_input(int command)
{
	int si, di;
	int replayTimeLimit;

	if (command != 0) {
		si = 0;
		goto store_input;
	}

	if (game_replay_mode == 2) {
		if (gameconfig.game_recordedframes > replay_frame_counter) {
			replay_frame_counter++;
			return;
		}
		replay_autoplay_active = 0;
		if (game_finish_state != 0)
			goto set_end;
		is_in_replay = 1;
		audio_sync_car_audio();
		game_finish_state = 1;
		return;
	}

	if (game_finish_state != 0) { si = 0; goto store_input; }
	if (state.game_3F6autoLoadEvalFlag != 0) { si = 0; goto store_input; }
	if (game_replay_mode == 1) { si = 0; goto store_input; }

	/* anti-piracy check */
	if (passed_security == 0 && game_pause_counter == 0) {
		if ((unsigned short)(framespersec << 2) < (unsigned short)state.game_frame) {
			update_crash_state(0, 1);
		}
	}

	/* read mouse or joystick input */
	if (mouse_motion_state_flag != 0 || joystick_assigned_flags != 0) {
		if (mouse_motion_state_flag != 0) {
			mouse_get_state((unsigned short*)&mouse_butstate,
							(unsigned short*)&mouse_xpos,
							(unsigned short*)&mouse_ypos);

			di = mouse_xpos - 160;
			if (abs(di) < 18) {
				di = 0;
			} else if (di > 0) {
				di -= 18;
			} else {
				di += 18;
			}
			car_track_interaction_state = (unsigned char)di;

			if (mouse_butstate & 1) {
				si = 2;
			} else if (mouse_butstate & 2) {
				si = 1;
			} else {
				si = 0;
			}
		} else {
			/* joystick path */
			{
				char al;
				joystick_get_scaled_x();
				al = (char)car_track_interaction_state;
				if (al > 0) {
					car_track_interaction_state = collision_surface_type[(int)(unsigned char)al];
				} else if ((char)car_track_interaction_state < 0) {
					char neg_al = -((char)car_track_interaction_state);
					car_track_interaction_state = -(char)collision_surface_type[(int)(unsigned char)neg_al];
				}
			}
			si = get_kb_or_joy_flags() & 51;
		}

		/* store steering data into circular buffer */
		di = replay_frame_counter & 63;
		*((char*)&sprite_offset_table + di) = (char)car_track_interaction_state;
		*((char*)&sprite_palette_indices + di) = 1;
	} else {
		si = get_kb_or_joy_flags();
	}

	/* check 'A' key (scan 30) = flag 16 */
	if (kb_get_key_state(30))
		si |= 16;
	/* check 'Z' key (scan 44) = flag 32 */
	if (kb_get_key_state(44))
		si |= 32;

store_input:
	/* time limit check: 1500 * framespersec ticks */
	replayTimeLimit = 1500 * framespersec;
	if ((unsigned short)replayTimeLimit <= (unsigned short)(replay_frame_counter + elapsed_time1)) {
		update_crash_state(0, 4);
		game_finish_state = 1;
		return;
	}

	/* at the 12000-tick mark, check for half-lap */
	if (replay_frame_counter == 12000) {
		if (elapsed_time1 == 0 && *(unsigned char*)&lap_completion_trigger_flag == 0) {
			*(unsigned char*)&lap_completion_trigger_flag = 1;
			checkpoint_lap_trigger = 1;
			return;
		}

		/* shift cvx buffer and replay data down */
		{
			int replayShiftFrames;
			int segCount, i;

			replayShiftFrames = 30 * framespersec;
			segCount = 12000 / replayShiftFrames - 1;

			for (i = 0; i < segCount; i++) {
				unsigned char * base = (unsigned char *)cvxptr;
				unsigned char * dst;

				/* shift cvxptr block: copy block i+1 -> block i */
				/* each block = 1120 bytes, offset starts at 1440 */
				/* Subtract replayShiftFrames from dst block at computed offset */
				{
					dst = base + (unsigned short)((long)1120 * i + 1440);
					*(short *)dst -= (short)replayShiftFrames;
				}

				/* Copy 1120 bytes from srcOfs to dstOfs within cvxptr  */
				/* The ASM uses  memory copy (rep movsw of 560 words) */
				{
					unsigned char * srcBlock;
					unsigned char * dstBlock;
					int j;
					srcBlock = base + (unsigned short)((long)1120 * (i + 1));
					dstBlock = base + (unsigned short)((long)1120 * i);
					for (j = 0; j < 1120; j++)
						dstBlock[j] = srcBlock[j];
				}
			}

			/* shift replay buffer down */
			for (i = 0; (unsigned short)i < (unsigned short)(12000 - replayShiftFrames); i++) {
				unsigned char * rpl = (unsigned char *)replay_buffer;
				rpl[i] = rpl[i + replayShiftFrames];
			}

			replay_frame_counter -= replayShiftFrames;
			gameconfig.game_recordedframes -= replayShiftFrames;
			elapsed_time1 += replayShiftFrames;
			state.game_frame -= replayShiftFrames;
		}
	}

	/* store input byte */
	{
		unsigned char * rpl = (unsigned char *)replay_buffer;
		rpl[replay_frame_counter] = (unsigned char)si;
		replay_frame_counter++;
		gameconfig.game_recordedframes++;
	}
	return;

set_end:
	is_in_replay = 1;
	audio_sync_car_audio();
	game_finish_state = 1;
}


/**
 * @brief Camera position update and opponent tracking.
 *
/** @brief Car.
 * @param player Parameter `player`.
 * @param present Parameter `present`.
 * @param height Parameter `height`.
 * @param target Parameter `target`.
 * @param threshold Parameter `threshold`.
 * @param update_follow_camera_vectors Parameter `update_follow_camera_vectors`.
 * @return Function result.
 */
void update_follow_camera_vectors(void)
{
	int si, di;
	int trackedCarCount;
	struct CARSTATE* pState;
	int carPosX, carPosY, carPosZ;
	int cameraTarget[3]; /* copy of car_waypoint_target or pos */
	int carHeadingAngle;
	int targetAngle;
	int cameraYOffset;
	int checkpointDistance;
	unsigned char trackIndex;
	int nearestTrackDistance;

	trackedCarCount = 1;
	if (gameconfig.game_opponenttype != 0)
		trackedCarCount = 2;

	for (si = 0; si < trackedCarCount; si++) {
		/* copy game_vec1 -> game_vec3 for this player */
		{
			struct VECTOR* src = &state.game_vec1[si];
			struct VECTOR* dst = (&state.game_vec3) + si;
			dst->x = src->x;
			dst->y = src->y;
			dst->z = src->z;
		}

		if (si == 0)
			pState = &state.playerstate;
		else
			pState = &state.opponentstate;

		/* extract world position >> 6 */
		carPosY = (int)(pState->car_posWorld1.ly >> 6);
		carPosX = (int)(pState->car_posWorld1.lx >> 6);
		carPosZ = (int)(pState->car_posWorld1.lz >> 6);

		/* copy car_waypoint_target */
		cameraTarget[0] = pState->car_waypoint_target.x;
		cameraTarget[1] = pState->car_waypoint_target.y;
		cameraTarget[2] = pState->car_waypoint_target.z;
		carHeadingAngle = pState->car_heading_angle;

			/* check if we should use actual position vs temporary projected vector */
		if (si == 0 && state.game_flyover_state == 0 && state.game_flyover_counter == 0) {
			/* player and no special flags: check car state */
		} else {
			goto use_pos;
		}

		if (pState->car_collision_contact_flag != 0) goto use_pos;
		if (pState->car_crashBmpFlag != 0) goto use_pos;
		if (pState->car_waypoint_seq_index == -1) goto use_pos;
		if (carHeadingAngle > 128 && carHeadingAngle < 896) {
					/* use temporary projected vector as-is */
			goto after_pos;
		}

use_pos:
		cameraTarget[0] = carPosX;
		cameraTarget[1] = carPosY;
		cameraTarget[2] = carPosZ;
after_pos:

		/* height adjustment */
		{
			int target_y = carPosY + 270;
			struct VECTOR* vec1 = &state.game_vec1[si];

			cameraYOffset = vec1->y - target_y;
			if (cameraYOffset != 0) {
				di = cameraYOffset;
				if (di > 30) di = 30;
				else if (di < -30) di = -30;
				vec1->y -= di;
			}
		}

		/* compute angle to target */
		{
			struct VECTOR* vec1 = &state.game_vec1[si];
			targetAngle = polarAngle(cameraTarget[2] - vec1->z, cameraTarget[0] - vec1->x);

			di = polarRadius2D(carPosZ - vec1->z, carPosX - vec1->x);
		}

		/* move camera toward target if distance > 450 */
		if (di > 450) {
			int move = di - 450;
			struct VECTOR* vec1 = &state.game_vec1[si];
			if (framespersec == 20) {
				if (move > 120) move = 120;
			} else if (framespersec >= 30) {
				if (move > 80) move = 80;
			} else {
				if (move > 240) move = 240;
			}
			vec1->x += multiply_and_scale(move, sin_fast(targetAngle));
			vec1->z += multiply_and_scale(move, cos_fast(targetAngle));
		}

		/* checkpoint tracking (every half-fps frames) */
		{
			int checkpoint_period = framespersec / 2;
			if (checkpoint_period <= 0) {
				checkpoint_period = 1;
			}
			if (state.game_frame % checkpoint_period != 0)
				continue;
		}

		nearestTrackDistance = 10000;
		trackIndex = 0;

		while ((char)trackIndex < (char)game_exit_request_flag) {
			long ax_l, bx_l;
			int deltaX, deltaZ;
			short * trk = waypoint_world_pos + (int)trackIndex * 3;

			/* dx = trk[0] - carPosX (sign-extended to 32-bit) */
			ax_l = (long)trk[0] - (long)carPosX;
			deltaX = (int)ax_l;

			/* dz = trk[2] - carPosZ */
			bx_l = (long)trk[2] - (long)carPosZ;
			deltaZ = (int)bx_l;

			/* abs check on dx */
			{
				long abs_dx = ax_l < 0 ? -ax_l : ax_l;
				long abs_dz = bx_l < 0 ? -bx_l : bx_l;

				if (abs_dx < (long)nearestTrackDistance && abs_dz < (long)nearestTrackDistance) {
					checkpointDistance = polarRadius2D(deltaZ, deltaX);
					if (checkpointDistance < nearestTrackDistance) {
						state.game_track_indices[si] = trackIndex;
						nearestTrackDistance = checkpointDistance;
					}
				}
			}
			trackIndex++;
		}
	}
}


/**
 * @brief Load and manage dashboard elements.
 *
 * Multi-mode function that handles all dashboard sprite lifecycle:
/** @brief Shapes.
 * @param dashboard Parameter `dashboard`.
 * @param wheels Parameter `wheels`.
 * @param gauges Parameter `gauges`.
 * @param knob Parameter `knob`.
 * @param dot Parameter `dot`.
 * @param position Parameter `position`.
 * @param knob Parameter `knob`.
 * @param needles Parameter `needles`.
 * @param command Parameter `command`.
 * @return Function result.
 */
void setup_car_shapes(int command)
{
	void * shapePtr;
	int arrayIndex, screenSlot;
	int steeringBucket;
	int si, di;
	char steeringState;
	char gaugeMode;
	int hundredsDigit;
	char steeringDotX, steeringDotY;
	char steeringWheelChanged;
	char restoredDotThisFrame;
	char speedHasLeadingDigit;

	if (command == 0) {
		/* --- case 0: load car shapes --- */
		aStdaxxxx[4] = gameconfig.game_playercarid[0];
		aStdaxxxx[5] = gameconfig.game_playercarid[1];
		aStdaxxxx[6] = gameconfig.game_playercarid[2];
		aStdaxxxx[7] = gameconfig.game_playercarid[3];
		aStdbxxxx[4] = gameconfig.game_playercarid[0];
		aStdbxxxx[5] = gameconfig.game_playercarid[1];
		aStdbxxxx[6] = gameconfig.game_playercarid[2];
		aStdbxxxx[7] = gameconfig.game_playercarid[3];

		stdaresptr = file_load_resource(3, aStdaxxxx);
		stdbresptr = file_load_resource(2, aStdbxxxx);

		locate_many_resources(stdaresptr, "whl1whl2whl3ins2gboxins1ins3inm1inm3", (void *)whlshapes);
		locate_many_resources(stdbresptr, "gnobgnabdot dotadot1dot2", (void *)gnobshapes);

		if (simd_player.spdcenter.py == 0) {
			locate_many_resources(stdbresptr, "dig0dig1dig2dig3dig4dig5dig6dig7dig8dig9", (void *)digshapes);
		}

		/* create wheel sprites */
		{
			struct SHAPE2D * shp;

			shp = (struct SHAPE2D *)whlshapes[3]; /* ins2 */
			whlsprite1 = sprite_make_wnd(shp->s2d_width * video_flag1_is1, shp->s2d_height, 15);

			shp = (struct SHAPE2D *)whlshapes[4]; /* gbox */
			whlsprite2 = sprite_make_wnd(shp->s2d_width * video_flag1_is1, shp->s2d_height, 15);

			shp = (struct SHAPE2D *)whlshapes[4]; /* gbox */
			whlsprite3 = sprite_make_wnd(shp->s2d_width * video_flag1_is1, shp->s2d_height, 15);
		}
		/* "dash" */
		shapePtr = locate_shape_fatal(stdaresptr, "dash");

		/* Init whlsprite3: draw the dashboard ("dash") into it at gbox-local coords.
		 * whlsprite3 is the restore buffer — blitting it back to the screen at
		 * (gbox.pos_x, gbox.pos_y) restores the correct dashboard background
		 * after a gear shift ends. */
		sprite_set_1_from_argptr(whlsprite3);
		{
			struct SHAPE2D * dashShape = (struct SHAPE2D *)shapePtr;
			struct SHAPE2D * gboxShape = (struct SHAPE2D *)whlshapes[4];
			shape2d_draw_rle_copy_clipped_at(shapePtr,
				dashShape->s2d_pos_x - gboxShape->s2d_pos_x,
				dashShape->s2d_pos_y - gboxShape->s2d_pos_y);
		}
		sprite_copy_2_to_1();
		dashbmp_y = ((struct SHAPE2D *)shapePtr)->s2d_pos_y;

		/* "roof" */
		if (locate_shape_nofatal(stdaresptr, "roof") != 0) {
			shapePtr = locate_shape_fatal(stdaresptr, "roof");
			roofbmpheight = ((struct SHAPE2D *)shapePtr)->s2d_height;
		} else {
			roofbmpheight = 0;
		}

		/* "dast" */
		shapePtr = locate_shape_nofatal(stdaresptr, "dast");
		if (shapePtr != 0) {
			dastbmp_y = ((struct SHAPE2D *)shapePtr)->s2d_pos_y;
			g_dast_shape_ptr = (struct SHAPE2D *)shapePtr;
			dasmshapeptr = locate_shape_fatal(stdaresptr, "dasm");
		} else {
			dastbmp_y = 0;
			g_dast_shape_ptr = NULL;
		}
		return;
	}

	if (command == 1) {
		/* --- case 1: setup display for driving mode --- */
		si = 0;
		mouse_draw_opaque_check();

		if (locate_shape_nofatal(stdaresptr, "roof") != 0) {
			shapePtr = locate_shape_fatal(stdaresptr, "roof");
			shape2d_draw_rle_copy(shapePtr);
		}

		shapePtr = locate_shape_fatal(stdaresptr, "dash");
		shape2d_draw_rle_copy_clipped(shapePtr);
		shape2d_draw_rle_copy_clipped(whlshapes[1]); /* whl2 */

		mouse_draw_transparent_check();

		screenSlot = (int)(char)screen_display_toggle_flag;
		mouse_button_press_state[screenSlot] = 0;
		wheel_slip_direction[screenSlot] = 0;
		car_damage_data_table[screenSlot] = si;
		race_driver_status_array[screenSlot] = 0;
		si = -1;
		car_damage_state_array[screenSlot] = si;
		car_orientation_data[screenSlot] = si;
		car_position_xyz_array[screenSlot] = si;
		return;
	}

	if (command == 2) {
		/* --- case 2: update steering wheel and gauges --- */
		restoredDotThisFrame = 0;

		/* Steering wheel position: angle / 8 */
		{
			int sa = state.playerstate.car_steeringAngle;
			if (sa < 0) sa = -sa;
			steeringBucket = sa >> 3;
			if (state.playerstate.car_steeringAngle < 0)
				steeringBucket = -steeringBucket;
		}

		steeringState = 1; /* center */
		if (steeringBucket < -10) {
			steeringState = 0;   /* left */
		} else if (steeringBucket > 10) {
			steeringState = 2;   /* right */
		}

		/* Gear shift knob */
		if (state.playerstate.car_fpsmul2 == 0 && state.playerstate.car_changing_gear == 0) {
			arrayIndex = (int)(char)screen_display_toggle_flag;
			if (wheel_slip_direction[arrayIndex] != 0) {
				if (video_flag5_is0 == 0)
					mouse_draw_opaque_check();

				sprite_set_1_size(0, 320, 0, height_above_replaybar);
				{
					struct SHAPE2D * shp = (struct SHAPE2D *)whlshapes[4];
					sprite_putimage_and_alt(whlsprite3->sprite_bitmapptr,
						shp->s2d_pos_x,
						shp->s2d_pos_y);
				}
				wheel_slip_direction[arrayIndex] = 0;
				goto do_steeringwheel;
			}
		}

		/* check if gearshift actually changed */
		arrayIndex = (int)(char)screen_display_toggle_flag;
		if (wheel_slip_direction[arrayIndex] == state.playerstate.car_changing_gear) {
			screenSlot = arrayIndex * 2;
			if (car_position_z_array[screenSlot / 2] == state.playerstate.car_knob_x &&
				car_position_y_array[screenSlot / 2] == state.playerstate.car_knob_y) {
				if (state.playerstate.car_fpsmul2 == 0)
					goto do_steeringwheel;
				if (wheel_slip_direction[arrayIndex] != 0)
					goto do_steeringwheel;  /* normal: already drawn at this position */
			}
		}

		/* redraw gear knob */
		sprite_set_1_from_argptr(whlsprite2);
		wheel_slip_direction[arrayIndex] = 1;
		shape2d_draw_rle_copy_clipped_at(whlshapes[4], 0, 0); /* gbox */

		{
			int kx = state.playerstate.car_knob_x;
			int ky = state.playerstate.car_knob_y;
			arrayIndex = (int)(char)screen_display_toggle_flag * 2;
			car_position_z_array[arrayIndex / 2] = kx;
			car_position_y_array[arrayIndex / 2] = ky;

			sprite_putimage_and_at_shape_origin(gnobshapes[1], kx, ky); /* gnab */
			sprite_putimage_or_at_shape_origin(gnobshapes[0], kx, ky);   /* gnob */
		}

		if (video_flag5_is0 != 0) {
			setup_mcgawnd2();
		} else {
			sprite_copy_2_to_1();
			mouse_draw_opaque_check();
		}

		sprite_set_1_size(0, 320, 0, height_above_replaybar);
		{
			struct SHAPE2D * shp = (struct SHAPE2D *)whlshapes[4];
			sprite_putimage_and_alt(whlsprite2->sprite_bitmapptr,
				shp->s2d_pos_x,
				shp->s2d_pos_y);
		}

do_steeringwheel:
		/* Check if steering wheel position changed */
		arrayIndex = (int)(char)screen_display_toggle_flag;
		if (race_driver_status_array[arrayIndex] == steeringState && race_condition_state_flag == 0)
			goto do_gauges;

		if (video_flag5_is0 == 0)
			mouse_draw_opaque_check();

		/* restore previous dot position if needed */
		arrayIndex = (int)(char)screen_display_toggle_flag * 2;
		if (car_damage_data_table[arrayIndex / 2] != 0) {
			sprite_putimage_and_alt(
				gnobshapes[(int)(char)current_screen_buffer_selector + 4],
				car_velocity_vector_x[arrayIndex / 2],
				car_damage_data_table[arrayIndex / 2]);
			car_damage_data_table[(int)(char)screen_display_toggle_flag] = 0;
			restoredDotThisFrame = 1;
		}

		/* draw new whl bitmap */
		if (steeringState == 0) {
			shape2d_draw_rle_copy_clipped(whlshapes[0]); /* whl1 - left */
		} else if (steeringState == 1) {
			shape2d_draw_rle_copy_clipped(whlshapes[1]); /* whl2 - center */
		} else { /* steeringState == 2 */
			shape2d_draw_rle_copy_clipped(whlshapes[2]); /* whl3 - right */
		}
		race_driver_status_array[(int)(char)screen_display_toggle_flag] = steeringState;
		steeringWheelChanged = 1;
		goto do_gauges_entry;

do_gauges:
		steeringWheelChanged = 0;

do_gauges_entry:
		/* Speedometer / rev-counter */
		if ((short)simd_player.spdcenter.py == -1) {
			si = 0;
			gaugeMode = 2;
		} else if (simd_player.spdcenter.py == 0) {
			/* digital speedometer */
			gaugeMode = 1;
			si = state.playerstate.car_speed >> 8;
			goto do_revcalc;
		} else {
			gaugeMode = 0;
			si = state.playerstate.car_speed / 640;
			if (si >= simd_player.spdnumpoints)
				si = simd_player.spdnumpoints - 1;
			goto do_revcalc;
		}

do_revcalc:
		di = state.playerstate.car_currpm >> 7;
		if (di >= simd_player.revnumpoints)
			di = simd_player.revnumpoints - 1;

		/* check if gauges need redraw */
		if (steeringWheelChanged == 0 && race_condition_state_flag == 0) {
			arrayIndex = (int)(char)screen_display_toggle_flag * 2;
			if (car_orientation_data[arrayIndex / 2] == si && car_position_xyz_array[arrayIndex / 2] == di)
				goto do_steeringdot;
		}

		if (video_flag5_is0 == 0)
			mouse_draw_opaque_check();

		/* restore previous dot */
		arrayIndex = (int)(char)screen_display_toggle_flag * 2;
		if (car_damage_data_table[arrayIndex / 2] != 0) {
			sprite_putimage_and_alt(
				gnobshapes[(int)(char)current_screen_buffer_selector + 4],
				car_velocity_vector_x[arrayIndex / 2],
				car_damage_data_table[arrayIndex / 2]);
			car_damage_data_table[(int)(char)screen_display_toggle_flag] = 0;
			restoredDotThisFrame = 1;
		}

		/* draw instrument sprite */
		sprite_set_1_from_argptr(whlsprite1);
		shape2d_draw_rle_copy_at(whlshapes[3], 0, 0); /* ins2 */

		arrayIndex = (int)(char)screen_display_toggle_flag * 2;
		car_orientation_data[arrayIndex / 2] = si;
		car_position_xyz_array[arrayIndex / 2] = di;

		if (gaugeMode == 1) {
			/* digital speed display */
			int speed_val = si;
			hundredsDigit = 0;
			speedHasLeadingDigit = 0;

			if (speed_val >= 200) {
				hundredsDigit = 2;
				speed_val -= 200;
			} else if (speed_val >= 100) {
				hundredsDigit = 1;
				speed_val -= 100;
			}

			if (hundredsDigit != 0) {
				sprite_putimage_or(digshapes[hundredsDigit],
					(unsigned char)simd_player.spdpoints[0],
					(unsigned char)simd_player.spdpoints[1]);
				speedHasLeadingDigit = 1;
			}

			{
				int tens = speed_val / 10;
				if (tens != 0 || speedHasLeadingDigit) {
					sprite_putimage_or(digshapes[tens],
						(unsigned char)simd_player.spdpoints[2],
						(unsigned char)simd_player.spdpoints[3]);
					speed_val -= tens * 10;
					speedHasLeadingDigit = 1;
				}
			}

			sprite_putimage_or(digshapes[speed_val],
				(unsigned char)simd_player.spdpoints[4],
				(unsigned char)simd_player.spdpoints[5]);
		} else if (gaugeMode == 0) {
			/* analog speedometer needle */
			arrayIndex = si * 2;
			preRender_line(
				simd_player.spdcenter.px,
				simd_player.spdcenter.py,
				(unsigned char)simd_player.spdpoints[arrayIndex],
				(unsigned char)simd_player.spdpoints[arrayIndex + 1],
				meter_needle_color);
		}

		/* rev counter needle */
		arrayIndex = di * 2;
		preRender_line(
			simd_player.revcenter.px,
			simd_player.revcenter.py,
			(unsigned char)simd_player.revpoints[arrayIndex],
			(unsigned char)simd_player.revpoints[arrayIndex + 1],
			meter_needle_color);

		/* draw steering wheel overlay */
		if (steeringState == 0) {
			shape2d_render_bmp_as_mask(whlshapes[7]); /* inm1 */
			shape2d_draw_rle_or(whlshapes[5]); /* ins1 */
		} else if (steeringState == 2) {
			shape2d_render_bmp_as_mask(whlshapes[8]); /* inm3 */
			shape2d_draw_rle_or(whlshapes[6]); /* ins3 */
		}

		if (video_flag5_is0 != 0) {
			setup_mcgawnd2();
		} else {
			sprite_copy_2_to_1();
		}

		sprite_set_1_size(0, 320, 0, height_above_replaybar);
		{
			struct SHAPE2D * shp = (struct SHAPE2D *)whlshapes[3]; /* ins2 */
			sprite_putimage_and_alt(whlsprite1->sprite_bitmapptr,
				shp->s2d_pos_x,
				shp->s2d_pos_y);
		}

do_steeringdot:
		/* Steering dot overlay */
		arrayIndex = (int)(char)screen_display_toggle_flag;
		if (car_damage_state_array[arrayIndex] == steeringBucket && race_condition_state_flag == 0 && restoredDotThisFrame == 0)
			goto steer_done;

		if (video_flag5_is0 == 0)
			mouse_draw_opaque_check();

		sprite_set_1_size(0, 320, 0, height_above_replaybar);

		/* restore background at old dot position */
		arrayIndex = (int)(char)screen_display_toggle_flag * 2;
		if (car_damage_data_table[arrayIndex / 2] != 0) {
			sprite_putimage_and_alt(
				gnobshapes[(int)(char)current_screen_buffer_selector + 4],
				car_velocity_vector_x[arrayIndex / 2],
				car_damage_data_table[arrayIndex / 2]);
			car_damage_data_table[(int)(char)screen_display_toggle_flag] = 0;
		}

		/* compute dot position from steering table */
		{
			int absSteer = steeringBucket < 0 ? -steeringBucket : steeringBucket;
			unsigned char* dotEntry = (unsigned char*)&simd_player.steeringdots + absSteer * 2;
			unsigned char centerDotX = (unsigned char)simd_player.steeringdots[0];
			steeringDotY = dotEntry[1];
			steeringDotX = dotEntry[0];
			if (steeringBucket < 0) {
				steeringDotX -= ((dotEntry[0] - centerDotX) << 1);
			}
		}

		/* store and draw new dot */
		arrayIndex = (int)(char)screen_display_toggle_flag * 2;
		{
			struct SHAPE2D * dotShape = (struct SHAPE2D *)gnobshapes[2]; /* dot */
			car_velocity_vector_x[arrayIndex / 2] = ((unsigned char)steeringDotX - dotShape->s2d_hotspot_x) & video_flag3_isFFFF;
			car_damage_data_table[arrayIndex / 2] = (unsigned char)steeringDotY - dotShape->s2d_hotspot_y;
		}

		sprite_clear_shape_alt(
			gnobshapes[(int)(char)current_screen_buffer_selector + 4],
			car_velocity_vector_x[arrayIndex / 2],
			car_damage_data_table[arrayIndex / 2]);

		sprite_putimage_and_at_shape_origin(gnobshapes[3], /* dota */
			(unsigned char)steeringDotX,
			(unsigned char)steeringDotY);
		sprite_putimage_or_at_shape_origin(gnobshapes[2], /* dot  */
			(unsigned char)steeringDotX,
			(unsigned char)steeringDotY);

		car_damage_state_array[(int)(char)screen_display_toggle_flag] = steeringBucket;

steer_done:
		mouse_draw_transparent_check();
		return;
	}

	if (command == 3) {
		/* --- case 3: free car shapes --- */
		if (whlsprite3 != 0) {
			sprite_free_wnd(whlsprite3);
			whlsprite3 = 0;
		}
		if (whlsprite2 != 0) {
			sprite_free_wnd(whlsprite2);
			whlsprite2 = 0;
		}
		if (whlsprite1 != 0) {
			sprite_free_wnd(whlsprite1);
			whlsprite1 = 0;
		}
		if (stdbresptr != 0) {
			mmgr_free((char *)stdbresptr);
			stdbresptr = 0;
		}
		if (stdaresptr != 0) {
			mmgr_free((char *)stdaresptr);
			stdaresptr = 0;
		}
		return;
	}
}


/**
 * @brief Set mouse bounding box.
 *
 * Configures the mouse cursor movement area. When @p command is non-zero
 * the cursor is constrained to the gameplay region and centred;
 * otherwise it is allowed full-screen movement.
 *
 * @param command  Non-zero for gameplay-constrained bounds, 0 for full screen.
 */
void mouse_minmax_position(int command)
{
	if (command != 0) {
		mouse_set_minmax(15, 0, 305, 200);
		mouse_set_position(160, 100);
	} else {
		mouse_set_minmax(0, 0, 320, 200);
	}
}


/**
 * @brief Apply steering corrections to the replay buffer.
 *
 * Reads the pending mouse/joystick steering target from the circular
 * input buffer, computes a speed-dependent response step, and writes
 * corrective left/right flags into the current replay frame so that
 * the car converges toward the desired steering angle.
 */
void replay_apply_steering_correction(void)
{
	int si, di;
	char responseStep, correctionMask;

	si = state.game_frame & 63;

	if (*((unsigned char*)&sprite_palette_indices + si) == 0)
		return;

	di = (int)(char)*((char*)&sprite_offset_table + si);

	/* compute response rate from speed */
	responseStep = *((unsigned char*)steerWhlRespTable_ptr +
		((state.playerstate.car_speed2 >> 10) & 252) + 1);

	/* double response if steering same direction */
	if (state.playerstate.car_steeringAngle < di) {
		if (state.playerstate.car_steeringAngle < -1)
			responseStep <<= 2;
	} else if (state.playerstate.car_steeringAngle > di) {
		if (state.playerstate.car_steeringAngle > 1)
			responseStep <<= 2;
	}

	/* determine corrective action */
	if (state.playerstate.car_steeringAngle > di) {
		if (state.playerstate.car_steeringAngle - (int)(char)responseStep >= di) {
			correctionMask = 8; /* steer left */
		} else {
			correctionMask = 0;
		}
	} else if (state.playerstate.car_steeringAngle < di) {
		if (state.playerstate.car_steeringAngle + (int)(char)responseStep <= di) {
			correctionMask = 4; /* steer right */
		} else {
			correctionMask = 0;
		}
	} else {
		correctionMask = 0;
	}

	if (correctionMask != 0) {
		unsigned char * rpl = (unsigned char *)replay_buffer;
		rpl[state.game_frame] |= (unsigned char)correctionMask;
	}

	*((unsigned char*)&sprite_palette_indices + si) = 0;
}


/**
 * @brief Main game / replay UI loop.
 *
 * Multiplex function that drives the in-game and replay-bar interface:
 *   - @p command == 0 — Locate replay-bar shapes and initialise buttons.
/** @brief Display.
 * @param time Parameter `time`.
 * @param bar Parameter `bar`.
 * @param icon Parameter `icon`.
 * @param input Parameter `input`.
 * @param controls Parameter `controls`.
 * @param pan Parameter `pan`.
 * @param navigation Parameter `navigation`.
 * @param ESC Parameter `ESC`.
 * @param replay Parameter `replay`.
 * @param restart Parameter `restart`.
 * @param continue Parameter `continue`.
 * @param mode Parameter `mode`.
 * @param mode Parameter `mode`.
 * @param mode Parameter `mode`.
 * @param frame_value Parameter `frame_value`.
 * @return Function result.
 */
void loop_game(int command, int context_value, int frame_value)
{
	int si, di;
	int screenIndex, screenValue;
	unsigned char ctrlModifierActive;
	char savedGameconfigSnapshot[26]; /* saved gameconfig for replay load compare */
	int unused24, unused22;
	unsigned char buttonIndex;
	char saveDialogState;
	int unused18;
	int inputCode;
	unsigned short dialogFlags[16]; /* show_dialog indexes as flags[button*2], so allocate pairs */
	int savedSkyId;
	char dialogChoice;
	int unused12, unused10, unusedE, unusedC, unusedA;

	(void)unused24;
	(void)unused22;
	(void)unused18;
	(void)unused12;
	(void)unused10;
	(void)unusedE;
	(void)unusedC;
	(void)unusedA;

	if (command == 0) {
		/* locate replay bar shapes */
		locate_many_resources(sdgameresptr, "rplyrpicrpacrpmcrptcbof6bof5bof4bof3bof2bof1bof0zoompannbon6bon5bon4bon3bon2bon1", (void *)rplyshapes);
		context_value = 4;
		/* fall through to case 2 */
	}

	if (command == 0 || command == 2) {
		/* init button state */
		for (si = 0; si < 9; si++)
			wheel_ground_contact[si] = 0;
		wheel_ground_contact[context_value] = 1;
		return;
	}

	if (command == 1) {
		/* --- replay bar UI update --- */
		screenValue = (int)(char)screen_display_toggle_flag;
		if (mouse_button_press_state[screenValue] == 0) {
			/* first init */
			mouse_button_press_state[screenValue] = 1;
			car_camera_mode_array[screenValue] = 255;
			car_collision_flags[screenValue] = 255;

			for (si = 0; si < 9; si++) {
				suspension_compression_state[si * 2 + screenValue] = 0;
			}

			mouse_draw_opaque_check();
			shape2d_draw_rle_copy(rplyshapes[0]); /* rply */

			screenValue = (int)(char)screen_display_toggle_flag * 2;
			race_timer_display_values[screenValue / 2] = 65535;
			wheel_velocity_state[screenValue / 2] = 65535;

			/* display time */
			format_frame_as_string(resID_byte1, gameconfig.game_recordedframes + elapsed_time1, 1);
			font_set_colors(dialog_fnt_colour, 0);
			font_set_fontdef2(fontledresptr);
			sprite_draw_text_opaque(resID_byte1, 216, 187);
			font_set_fontdef();
		}

		/* update time display if changed */
		screenValue = frame_value + elapsed_time1;
		{
			unsigned short *time_display_ptr = &race_timer_display_values[(int)(char)screen_display_toggle_flag];
			if (*time_display_ptr != (unsigned short)screenValue) {
				*time_display_ptr = (unsigned short)screenValue;
			format_frame_as_string(resID_byte1, screenValue, 1);
			font_set_colors(dialog_fnt_colour, 0);
			mouse_draw_opaque_check();
			font_set_fontdef2(fontledresptr);
			sprite_draw_text_opaque(resID_byte1, 152, 187);
			font_set_fontdef();
			}
		}

		/* update camera mode icon */
		screenIndex = (int)(char)screen_display_toggle_flag;
		if (car_camera_mode_array[screenIndex] != cameramode) {
			car_camera_mode_array[screenIndex] = cameramode;
			wheel_velocity_state[(int)(char)screen_display_toggle_flag] = 65535;
			mouse_draw_opaque_check();
			shape2d_draw_rle_copy(rplyshapes[1 + (int)(unsigned char)cameramode]); /* rpic + cameramode */

			{
				unsigned char bcount = game_camera_buttons_count[(int)(unsigned char)cameramode];
				if (bcount < minimum_bump_counter)
					minimum_bump_counter = bcount;
			}

			if (car_collision_flags[(int)(char)screen_display_toggle_flag] > 6)
				car_collision_flags[(int)(char)screen_display_toggle_flag] = 255;
		}

		/* compute progress bar positions */
		if (gameconfig.game_recordedframes == 0) {
			si = 0;
			di = 0;
		} else {
			si = (int)((long)110 * context_value / gameconfig.game_recordedframes);
			di = (int)((long)110 * frame_value / gameconfig.game_recordedframes);
		}

		/* update progress bar if changed */
		screenIndex = (int)(char)screen_display_toggle_flag * 2;
		if (wheel_velocity_state[screenIndex / 2] != si || car_position_x_array[screenIndex / 2] != di) {
			mouse_draw_opaque_check();
			wheel_velocity_state[screenIndex / 2] = si;
			car_position_x_array[screenIndex / 2] = di;

			sprite_fill_rect(154, 177, 116, 6, terrain_height_table);
			sprite_fill_rect(si + 154, 177, 6, 6, dialog_fnt_colour);
			sprite_draw_rect_outline(di + 154, 177, di + 159, 182, track_segment_data_array);
		}

		/* update button highlight */
		if (car_collision_flags[(int)(char)screen_display_toggle_flag] != minimum_bump_counter) {
			mouse_draw_opaque_check();
			screenIndex = (int)(char)screen_display_toggle_flag;

			if (car_collision_flags[screenIndex] == 255)
				goto skip_unhighlight;

			screenValue = car_collision_flags[screenIndex];
			if (suspension_compression_state[screenValue * 2 + screenIndex] != 0) {
				shape2d_draw_rle_copy(rplyshapes[14 + screenValue]); /* highlighted */
			} else {
				shape2d_draw_rle_copy(rplyshapes[5 + screenValue]); /* normal */
			}
			car_collision_flags[(int)(char)screen_display_toggle_flag] = 255;

skip_unhighlight:
			/* unhighlight inactive buttons */
			for (buttonIndex = 0; buttonIndex < 7; buttonIndex++) {
				if (wheel_ground_contact[buttonIndex] == 0) {
					if (suspension_compression_state[buttonIndex * 2 + (int)(char)screen_display_toggle_flag] != 0) {
						shape2d_draw_rle_copy(rplyshapes[5 + buttonIndex]); /* normal */
						suspension_compression_state[buttonIndex * 2 + (int)(char)screen_display_toggle_flag] = 0;
					}
				}
			}

			/* highlight active buttons */
			for (buttonIndex = 0; buttonIndex < 7; buttonIndex++) {
				if (wheel_ground_contact[buttonIndex] != 0) {
					suspension_compression_state[buttonIndex * 2 + (int)(char)screen_display_toggle_flag] = 1;
					shape2d_draw_rle_copy(rplyshapes[14 + buttonIndex]); /* highlighted */
					suspension_compression_state[buttonIndex * 2 + (int)(char)screen_display_toggle_flag] = 1;
				}
			}

			car_collision_flags[(int)(char)screen_display_toggle_flag] = minimum_bump_counter;

			if (minimum_bump_counter != 255) {
				screenIndex = minimum_bump_counter * 2;
				sprite_draw_rect_outline(
					game_camera_buttons_x1[screenIndex / 2],
					game_camera_buttons_y1[screenIndex / 2],
					game_camera_buttons_x2[screenIndex / 2],
					game_camera_buttons_y2[screenIndex / 2],
					track_segment_data_array);
			}
		}

		mouse_draw_transparent_check();
		return;
	}

	if (command == 3) {
		/* --- replay bar input handling --- */
		{
			unsigned char bcount;
			bcount = game_camera_buttons_count[(int)(unsigned char)cameramode];
			if (bcount < minimum_bump_counter && cameramode != 2)
				minimum_bump_counter = bcount;
		}

		sprite_copy_2_to_1();

		if (video_flag5_is0 != 0) {
			screen_display_toggle_flag = current_screen_buffer_selector ^ 1;
		}

input_loop:
		inputCode = input_checking(timer_get_delta_alt());

		/* hit-test replay bar buttons */
		{
			unsigned char bcount_check = game_camera_buttons_count[(int)(unsigned char)cameramode];
			(void)bcount_check;
		}
		buttonIndex = mouse_multi_hittest(
			game_camera_buttons_count[(int)(unsigned char)cameramode] + 1,
			game_camera_buttons_x1,
			game_camera_buttons_x2,
			game_camera_buttons_y1,
			game_camera_buttons_y2);

		if (buttonIndex != 255) {
			if (buttonIndex != minimum_bump_counter) {
				if (inputCode == 0)
					inputCode = 1;
			}
			minimum_bump_counter = buttonIndex;

			if (inputCode == 32 || inputCode == 13) {
			/* Only convert to directional for buttons >= 7 (special controls) */
			if (minimum_bump_counter >= 7) {
			if (minimum_bump_counter == 7) {
			/* button 7: up/down slider based on mouse position */
			int mid = (lighting_intensity_lookup + lap_time_accumulator) / 2;
			if (mouse_ypos >= mid)
			inputCode = 20480;
			else
			inputCode = 18432;
			} else {
			/* button 8+: dial control based on angle */
			int dx = mouse_xpos - (projection_scale_factor + shadow_depth_table) / 2;
			int dy = (dither_pattern_config + race_position_table) / 2 - mouse_ypos;
			int angle = polarAngle(dx, dy);
			angle = (angle + 128) & 1023;
			angle >>= 8;
			if (angle == 0) inputCode = 18432;
			else if (angle == 1) inputCode = 19712;
			else if (angle == 2) inputCode = 20480;
			else if (angle == 3) inputCode = 19200;
			}
			}
			/* For buttons 0-6, inputCode stays as 32/13 for action handling */
			}

		} else {
			/* hit-test secondary button */
			buttonIndex = mouse_multi_hittest(1,
				&gameunk_button_x1,
				&gameunk_button_x2,
				&gameunk_button_y1,
				&gameunk_button_y2);
			if (buttonIndex == 0 && (inputCode == 32 || inputCode == 13))
				inputCode = 99; /* 'c' */
		}

		/* pass to keyboard shortcut handler */
		if (inputCode != 0 && inputCode != 283 && inputCode != 27) {
			if (handle_ingame_kb_shortcuts(inputCode))
				return;
		}

		/* Check for ESC key before early returns */
		if (inputCode == 283 || inputCode == 27) {
			/* ESC pressed - always enter replay/pause mode and init UI */
			replay_autoplay_active = 0;
			is_in_replay = 1;
			audio_sync_car_audio();
			loop_game(2, 4, 0);
			loop_game(1, state.game_frame, state.game_frame);
			/* Flush only the keyboard queue so the menu dialog does not
			 * see a queued ESC on its first poll. Do NOT call check_input()
			 * here - that blocks until mouse buttons are released too. */
			kb_poll_sdl_input();
			while (kb_get_char() != 0) { /* drain */ }
			goto handle_esc_dialog;
		}

		/* if not in replay and no input, update frame time display */
		if (is_in_replay == 0) {
			if (inputCode == 0 && replaybar_enabled != 0) {
				loop_game(1, state.game_frame, state.game_frame);
				return;
			}
			return;
		}

		if (replaybar_enabled == 0) {
			is_in_replay_copy = 255;
			timer_tick_counter = 65535;
		}

		/* check for fast-forward/rewind buttons */
		if (is_in_replay != 0 && (wheel_rotation_speed != 0 || tire_grip_state != 0)) {
			loop_game(2, 4, 0);
		}

		loop_game(1, state.game_frame, state.game_frame);

		/* Ctrl key check */
		ctrlModifierActive = 0;
		if (kb_get_key_state(29) || (minimum_bump_counter == 8 && (kbjoyflags & 48)))
			ctrlModifierActive = 1;

		if (ctrlModifierActive) {
			/* Ctrl-modified keys */
			if (inputCode == '+') goto do_zoom_in;
			if (inputCode == '-') goto do_zoom_out;
			if (inputCode == 18432) { /* Up */
				if (rotation_y_angle + (short)16 < (short)256) {
					rotation_y_angle += 16;
					return;
				}
				inputCode = 0;
			}
			if (inputCode == 19200) { /* Left */
				rotation_z_angle -= 16;
				return;
			}
			if (inputCode == 19712) { /* Right */
				rotation_z_angle += 16;
				return;
			}
			if (inputCode == 20480) { /* Down */
				if ((short)rotation_y_angle - (short)16 >= (short)-256) {
					rotation_y_angle -= 16;
					return;
				}
				inputCode = 0;
			}
		}

		/* handle directional keys */
		if (inputCode == '+') {
do_zoom_in:
			if (cameramode == 3) {
				if (camera_y_offset >= (short)900) goto input_loop;
				camera_y_offset += 30;
			} else {
				if (rotation_x_angle <= (short)120) goto input_loop;
				rotation_x_angle -= 30;
			}
			return;
		}

		if (inputCode == '-') {
do_zoom_out:
			if (cameramode == 3) {
				if (camera_y_offset <= 0) goto input_loop;
				camera_y_offset -= 30;
			} else {
				if (rotation_x_angle >= (short)1500) goto input_loop;
				rotation_x_angle += 30;
			}
			return;
		}

		/* navigation */
		if (inputCode == 19200) { /* Left */
			minimum_bump_counter = sprite_animation_frame[(int)(unsigned char)minimum_bump_counter];
			goto do_update;
		}
		if (inputCode == 19712) { /* Right */
			minimum_bump_counter = particle_effect_state[(int)(unsigned char)minimum_bump_counter];
			goto do_update;
		}
		if (inputCode == 18432) { /* Up */
			if (minimum_bump_counter == 7) goto do_zoom_in;
			minimum_bump_counter = light_level_table[(int)(unsigned char)minimum_bump_counter];
			goto do_update;
		}
		if (inputCode == 20480) { /* Down */
			if (minimum_bump_counter == 7) goto do_zoom_out;
			minimum_bump_counter = shadow_render_flags[(int)(unsigned char)minimum_bump_counter];
			goto do_update;
		}

		/* Enter/Space = action on selected button */
		if (inputCode == 13 || inputCode == 32 || inputCode == 7181 || inputCode == 14624) {
			if ((unsigned char)minimum_bump_counter <= 6) {
				switch (minimum_bump_counter) {
				case 0: /* Retire */
					update_crash_state(0, 4);
					game_finish_state = 2;
					return;
				case 1: /* Restart */
					check_input();
					framespersec = framespersec2;
					*(unsigned char*)&gameconfig.game_framespersec = (unsigned char)framespersec2;
					init_game_state(-1);
					replay_frame_counter = 0;
					gameconfig.game_recordedframes = 0;
					*(unsigned char*)&lap_completion_trigger_flag = 0;
					replay_mode_state_flag = 1;
					goto setup_drive;
				case 2: /* Continue */
					if (replay_mode_state_flag & 2) {
						replay_mode_state_flag = 3;
					} else {
						if (replay_frame_counter != gameconfig.game_recordedframes) {
							unsigned short dlg_result;
							dlg_result = ui_dialog_show_restext(UI_DIALOG_CONFIRM, 0,
								locate_text_res(gameresptr, "con"),
								UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, performGraphColor, 0, 0);
							if ((int)dlg_result < 1)
								goto after_dialog;
						}
						replay_mode_state_flag = 1;
					}
					replay_frame_counter = state.game_frame;
					gameconfig.game_recordedframes = state.game_frame;

setup_drive:
					dashb_toggle = 1;
					show_penalty_counter = 0;
					followOpponentFlag = 0;
					game_replay_mode = 0;
					cameramode = 0;
					state.game_3F6autoLoadEvalFlag = 0;
					state.game_frame_in_sec = 0;
					screen_shake_intensity = 0;
					loop_game(2, 3, 0);
					is_in_replay = 0;
					mouse_minmax_position((int)(char)mouse_motion_state_flag);
					check_input();
					kbormouse = 0;
					goto after_dialog;

				case 3: /* Load Replay */
					replay_mode_state_flag = 0;
					audio_sync_car_audio();
					{
						void * textres = locate_text_res(mainresptr, "rep");
						si = do_fileselect_dialog(replay_file_path_buffer, aDefault_1, ".rpl", textres);
					}
					if (!si) goto after_dialog;

					waitflag = 150;
					show_waiting();
					{
						int j;
						/* Actually: save 26 bytes from gameconfig beginning */
						for (j = 0; j < 26; j++)
							savedGameconfigSnapshot[j] = ((char*)&gameconfig)[j];
					}

					/* save sky ID */
					{
						unsigned char * elemMap = (unsigned char *)track_elem_map;
						savedSkyId = elemMap[900];
					}

					file_load_replay(replay_file_path_buffer, aDefault_1);
					if (gameconfig.game_recordedframes == 0) {
						/* that didn't work, I guess */
					}

					dashb_toggle = 0;
					track_setup();
					{
						unsigned char * elemMap = (unsigned char *)track_elem_map;
						si = 0;
						if (elemMap[900] != (unsigned char)savedSkyId)
							si = 1;
					}

					/* check if car changed */
					if (gameconfig.game_playercarid[0] != savedGameconfigSnapshot[0] ||
						gameconfig.game_playercarid[1] != savedGameconfigSnapshot[1] ||
						gameconfig.game_playercarid[2] != savedGameconfigSnapshot[2] ||
						gameconfig.game_playercarid[3] != savedGameconfigSnapshot[3]) {
						si = 1;
					} else {
						if (gameconfig.game_opponenttype != savedGameconfigSnapshot[8]) {
							si = 1;
						} else if (gameconfig.game_opponenttype != 0) {
							if (gameconfig.game_opponentcarid[0] != savedGameconfigSnapshot[9] ||
								gameconfig.game_opponentcarid[1] != savedGameconfigSnapshot[10] ||
								gameconfig.game_opponentcarid[2] != savedGameconfigSnapshot[11] ||
								gameconfig.game_opponentcarid[3] != savedGameconfigSnapshot[12]) {
								si = 1;
							} else {
								ensure_file_exists(2);
								load_opponent_data();
							}
						}
					}

					if (si) {
						free_player_cars();
						setup_player_cars();
					}

					*(unsigned char*)&gameconfig.game_framespersec = (unsigned char)gameconfig.game_framespersec;
					framespersec = (unsigned char)gameconfig.game_framespersec;
					init_game_state(-1);
					goto after_dialog;

				case 4: /* Save Replay */
					audio_sync_car_audio();
					saveDialogState = 0;
save_retry:
					if (saveDialogState != 0) goto after_dialog;
					{
						void * textres = locate_text_res(mainresptr, "rep");
						si = do_savefile_dialog(replay_file_path_buffer, aDefault_1, textres);
					}
					if (!si) {
						saveDialogState = 255;
						goto save_check;
					}

					file_build_path(replay_file_path_buffer, aDefault_1, ".rpl", g_path_buf);
					saveDialogState = 1;
					g_is_busy = 1;

					if (file_find(g_path_buf)) {
						si = ui_dialog_show_restext(UI_DIALOG_CONFIRM, 0,
							locate_text_res(mainresptr, "fex"),
							UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, performGraphColor, 0, 0);
						if (si == -1) {
							saveDialogState = 255;
						} else if (si == 0) {
							saveDialogState = 0;
						}
					}
					g_is_busy = 0;
save_check:
					if (saveDialogState != 1) goto save_retry;

					file_write_replay(g_path_buf);
					/* buttonIndex = result, but ignored */

					ui_dialog_show_restext(UI_DIALOG_INFO, 0,
						locate_text_res(mainresptr, "ser"),
						UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, performGraphColor, 0, 0);
					goto save_retry;

				case 5: /* Options */
					{
						int k;
						for (k = 0; k < 16; k++)
							dialogFlags[k] = 0;
						for (k = 0; k < 5; k++)
							dialogFlags[k * 2] = 1;
						if (gameconfig.game_opponenttype == 0)
							dialogFlags[4 * 2] = 0; /* disable follow-opponent if no opponent */
					}
					{
						void * textres = locate_text_res(gameresptr, "mdo");
						dialogChoice = (char)ui_dialog_show_restext(UI_DIALOG_CONFIRM, 0, textres, UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, dialogarg2, (unsigned short *)dialogFlags, 0);
					}
					switch (dialogChoice) {
					case 0: dashb_toggle ^= 1; break;
					case 1: replaybar_toggle ^= 1; break;
					case 2:
						cameramode++;
						if (cameramode == 4) cameramode = 0;
						break;
					case 3: do_mrl_textres(); break;
					case 4: followOpponentFlag ^= 1; break;
					}
					goto after_dialog;

				case 6: /* Quit race */
					update_crash_state(0, 4);
					replay_mode_state_flag = 0;
					game_finish_state = 2;
					return;
				}
			}
		}

handle_esc_dialog:
		/* ESC = show pause/menu dialog */
		if (inputCode == 283 || inputCode == 27) {
			/* show pause dialog
			 * Button layout (0 = dismiss/return, 1..7 = actions):
			 *   1: Follow opponent (only in replay w/ opponent — always off in modern engine)
			 *   2: Retire          — off if security not passed
			 *   3: Continue Driving — off if crashed or security not passed
			 *   4: Restart         — always on
			 *   5: Save Replay     — off if nothing recorded or time-shifted
			 *   6: Load Replay     — always on
			 *   7: Quit Race       — always on
			 */
			{
				int k;
				for (k = 0; k < 16; k++)
					dialogFlags[k] = 0;
				for (k = 0; k < 8; k++)
					dialogFlags[k * 2] = 1;
			}
			/* button 1 — follow-opponent replay mode (byte_43966 bit 2 never set in modern) */
			if (!(replay_mode_state_flag & 4))
				dialogFlags[1 * 2] = 0;
			/* button 2 — retire: disabled if copy-protection not passed */
			if (passed_security == 0)
				dialogFlags[2 * 2] = 0;
			/* button 3 — continue driving: disabled if crashed or security not passed */
			if (state.playerstate.car_crashBmpFlag != 0 || passed_security == 0)
				dialogFlags[3 * 2] = 0;
			/* button 5 — save replay: disabled if nothing recorded or time-shifted */
			if (gameconfig.game_recordedframes == 0 || elapsed_time1 != 0)
				dialogFlags[5 * 2] = 0;

			race_condition_state_flag = video_flag6_is1;
			{
				void * textres = locate_text_res(gameresptr, "men");
				dialogChoice = (char)ui_dialog_show_restext(UI_DIALOG_CONFIRM, 0, textres, UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, dialogarg2, (unsigned short *)dialogFlags, 0);
			}
			dialogChoice -= 1; /* ASM: sub ax, 1 — button 0 is "Return to Replay" (dismiss) */

			switch (dialogChoice) {
			case 0: goto case_retire;
			case 1: goto case_restart;
			case 2: goto case_continue;
			case 3: goto case_loadreplay;
			case 4: goto case_savereplay;
			case 5: goto case_options;
			case 6: goto case_quitrace;
			default: goto after_dialog;
			}

case_retire:
			update_crash_state(0, 4);
			game_finish_state = 2;
			goto after_dialog;
case_restart:
			check_input();
			framespersec = framespersec2;
			*(unsigned char*)&gameconfig.game_framespersec = (unsigned char)framespersec2;
			init_game_state(-1);
			replay_frame_counter = 0;
			gameconfig.game_recordedframes = 0;
			*(unsigned char*)&lap_completion_trigger_flag = 0;
			replay_mode_state_flag = 1;
			goto setup_drive;
case_continue:
			if (replay_mode_state_flag & 2) {
				replay_mode_state_flag = 3;
			} else {
				if (replay_frame_counter != gameconfig.game_recordedframes) {
					short dlg_result;
					dlg_result = ui_dialog_show_restext(UI_DIALOG_CONFIRM, 0,
						locate_text_res(gameresptr, "con"),
						UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, performGraphColor, 0, 0);
					if (dlg_result < 0)
						goto after_dialog;
				}
				replay_mode_state_flag = 1;
			}
			replay_frame_counter = state.game_frame;
			gameconfig.game_recordedframes = state.game_frame;
			goto setup_drive;
case_loadreplay:
			replay_mode_state_flag = 0;
			audio_sync_car_audio();
			{
				void * textres = locate_text_res(mainresptr, "rep");
				si = do_fileselect_dialog(replay_file_path_buffer, aDefault_1, ".rpl", textres);
			}
			if (!si) goto after_dialog;
			waitflag = 150;
			show_waiting();
			{
				int j;
				for (j = 0; j < 26; j++)
					savedGameconfigSnapshot[j] = ((char*)&gameconfig)[j];
			}
			{
				unsigned char * elemMap = (unsigned char *)track_elem_map;
				savedSkyId = elemMap[900];
			}
			file_load_replay(replay_file_path_buffer, aDefault_1);
			if (gameconfig.game_recordedframes == 0) { /* nothing */ }
			dashb_toggle = 0;
			track_setup();
			{
				unsigned char * elemMap = (unsigned char *)track_elem_map;
				si = 0;
				if (elemMap[900] != (unsigned char)savedSkyId)
					si = 1;
			}
			if (gameconfig.game_playercarid[0] != savedGameconfigSnapshot[0] ||
				gameconfig.game_playercarid[1] != savedGameconfigSnapshot[1] ||
				gameconfig.game_playercarid[2] != savedGameconfigSnapshot[2] ||
				gameconfig.game_playercarid[3] != savedGameconfigSnapshot[3]) {
				si = 1;
			} else {
				if (gameconfig.game_opponenttype != savedGameconfigSnapshot[8]) {
					si = 1;
				} else if (gameconfig.game_opponenttype != 0) {
					if (gameconfig.game_opponentcarid[0] != savedGameconfigSnapshot[9] ||
						gameconfig.game_opponentcarid[1] != savedGameconfigSnapshot[10] ||
						gameconfig.game_opponentcarid[2] != savedGameconfigSnapshot[11] ||
						gameconfig.game_opponentcarid[3] != savedGameconfigSnapshot[12]) {
						si = 1;
					} else {
						ensure_file_exists(2);
						load_opponent_data();
					}
				}
			}
			if (si) {
				free_player_cars();
				setup_player_cars();
			}
			framespersec = (unsigned char)gameconfig.game_framespersec;
			init_game_state(-1);
			goto after_dialog;
case_savereplay:
			audio_sync_car_audio();
			saveDialogState = 0;
save_retry2:
			if (saveDialogState != 0) goto after_dialog;
			{
				void * textres = locate_text_res(mainresptr, "rep");
				si = do_savefile_dialog(replay_file_path_buffer, aDefault_1, textres);
			}
			if (!si) {
				saveDialogState = 255;
				goto save_check2;
			}
			file_build_path(replay_file_path_buffer, aDefault_1, ".rpl", g_path_buf);
			saveDialogState = 1;
			g_is_busy = 1;
			if (file_find(g_path_buf)) {
				si = ui_dialog_show_restext(UI_DIALOG_CONFIRM, 0,
					locate_text_res(mainresptr, "fex"),
					UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, performGraphColor, 0, 0);
				if (si == -1) saveDialogState = 255;
				else if (si == 0) saveDialogState = 0;
			}
			g_is_busy = 0;
save_check2:
			if (saveDialogState != 1) goto save_retry2;
			file_write_replay(g_path_buf);
			ui_dialog_show_restext(UI_DIALOG_INFO, 0,
				locate_text_res(mainresptr, "ser"),
				UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, performGraphColor, 0, 0);
			goto save_retry2;
case_options:
			{
				int k;
				for (k = 0; k < 16; k++)
					dialogFlags[k] = 0;
				for (k = 0; k < 5; k++)
					dialogFlags[k * 2] = 1;
				if (gameconfig.game_opponenttype == 0)
					dialogFlags[4 * 2] = 0;
			}
			{
				void * textres = locate_text_res(gameresptr, "mdo");
				dialogChoice = (char)ui_dialog_show_restext(UI_DIALOG_CONFIRM, 0, textres, UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, dialogarg2, (unsigned short *)dialogFlags, 0);
			}
			switch (dialogChoice) {
			case 0: dashb_toggle ^= 1; break;
			case 1: replaybar_toggle ^= 1; break;
			case 2:
				cameramode++;
				if (cameramode == 4) cameramode = 0;
				break;
			case 3: do_mrl_textres(); break;
			case 4: followOpponentFlag ^= 1; break;
			}
			goto after_dialog;
case_quitrace:
			update_crash_state(0, 4);
			replay_mode_state_flag = 0;
			game_finish_state = 2;
			goto after_dialog;
		}

after_dialog:
		check_input();
		return;

do_update:
		loop_game(1, state.game_frame, state.game_frame);
		goto input_loop;
	}

	/* command >= 4: should not happen in practice */
}
