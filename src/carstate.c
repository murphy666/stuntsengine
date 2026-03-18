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
#include "carstate.h"
#include "math.h"
#include <stdlib.h>
#include <stdio.h>

/* file-local data (moved from data_global.c) */
static unsigned char breakable_obstacle_collision_box[8] = { 5, 0, 40, 0, 5, 0, 10, 0 };
static unsigned char finish_pole_collision_box[8] = { 6, 0, 121, 0, 6, 0, 9, 0 };
static unsigned char roadside_pole_collision_box[8] = { 1, 0, 10, 0, 1, 0, 10, 0 };
static char sprite_boundary_marker = 255;


enum {
	STATECAR_FPS_10 = 10,
	STATECAR_FPS_20 = 20,
	STATECAR_FPS_30 = 30,
	STATECAR_GEARSHIFT_STEP_30FPS = 4,
	STATECAR_GEARSHIFT_STEP_20FPS = 6,
	STATECAR_GEARSHIFT_STEP_DEFAULT = 12,
	STATECAR_INPUT_SHIFT_UP_MASK = 16,
	STATECAR_INPUT_SHIFT_DOWN_MASK = 32,
	STATECAR_AERO_INDEX_SHIFT = 10,
	STATECAR_AERO_INDEX_MAX = 63,
	STATECAR_RPM_DROP_SHIFT_10FPS = 80,
	STATECAR_RPM_DROP_SHIFT_DEFAULT = 40,
	STATECAR_RPM_DROP_SHIFT_30FPS = 27,
	STATECAR_ACCEL_AIRBORNE_SPEED_MAX = 64000,
	STATECAR_ACCEL_AIRBORNE_BOOST = 768,
	STATECAR_RPM_IDLE_TORQUE_THRESHOLD = 2600,
	STATECAR_RPM_LIMITER_BLEND_MAX = 5000,
	STATECAR_SPEEDDELTA_MASS_SCALE = 25,
	STATECAR_OPPONENT_SPEED_BASE = 200,
	STATECAR_SPEEDDELTA_LIMIT = 296,
	STATECAR_ENGINE_LIMITER_SHORT = 5,
	STATECAR_SPEED_CLAMP_LO = 32768,
	STATECAR_SPEED_CLAMP_HI = 62720,
	STATECAR_SPEED_SYNC_THRESHOLD = 5120,
	STATECAR_RPM_SWING_THRESHOLD = 2000,
	STATECAR_IDLETORQUE_RATIO_THRESHOLD = 12000,
	STATECAR_ENGINE_LIMITER_LONG = 30,
	STATECAR_ENGINE_LIMITER_MED = 10,
	STATECAR_SPEED2_LIMITER_PENALTY = 1280,
	STATECAR_SHAPEINFOS_OFS_BASE = 6664,
	STATECAR_SHAPEINFOS_SIZE = 1680,
	STATECAR_STEER_INPUT_SPEED_MASK = 252,
	STATECAR_STEER_IDX_NEXT_OFFSET = 1,
	STATECAR_STEER_RECENTER_BOOST_SHIFT = 2,
	STATECAR_STEER_DAMP_SCALE_SHIFT = 1,
	STATECAR_STEER_DELTA_CLAMP_10FPS = 160,
	STATECAR_STEER_DELTA_CLAMP_DEFAULT = 80,
	STATECAR_STEER_DELTA_CLAMP_30FPS = 53,
	STATECAR_STEER_ANGLE_CLAMP = 240,
	STATECAR_STEER_DEADZONE = 8,
	STATECAR_ANGLE_MASK = 1023,
	STATECAR_ANGLE_QUARTER = 256,
	STATECAR_ANGLE_HALF = 512,
	STATECAR_ANGLE_THREE_QUARTER = 768,
	STATECAR_ANGLE_FULL = 1024,
	STATECAR_HEADING_SNAP_MARGIN = 128,
	STATECAR_HEADING_BIN_SHIFT = 8,
	STATECAR_HEADING_BIN_RIGHT = 1,
	STATECAR_HEADING_BIN_LEFT = 3,
	STATECAR_FLYOVER_EXIT_MAX = 896,
	STATECAR_DEBRIS_ANGLE_DIVISOR_WIDE = 1024,
	STATECAR_DEBRIS_ANGLE_DIVISOR_NARROW = 192,
	STATECAR_DEBRIS_MAX_WIDE = 18,
	STATECAR_AI_PLAYER_BEHIND_Y = 90,
	STATECAR_AI_LATERAL_MAX = 180,
	STATECAR_AI_DEPTH_MAX = 600,
	STATECAR_AI_DEPTH_MIN = (short)65356,
	STATECAR_AI_CHASE_DEPTH_TRIGGER = (short)65458,
	STATECAR_AI_WAYPOINT_ANGLE_MAX = 256,
	STATECAR_AI_STEER_TARGET_MAX = 65,
	STATECAR_AI_STEER_TARGET_MIN = (short)65471,
	STATECAR_AI_TARGET_SPEED_ARCADE = 16384,
	STATECAR_AI_SPEED_TARGET_LOWER_BAND = 256,
	STATECAR_AI_SPEED_TARGET_UPPER_BAND = 768,
	STATECAR_COLLISION_RELSPEED_MIN = 10,
	STATECAR_COLLISION_RELSPEED_MAX = 30,
	STATECAR_COLLISION_REDUCE_SCALE = 768,
	STATECAR_COLLISION_REDUCE_SHIFT = 2,
	STATECAR_ANGLE_SIGN_WRAP_NEG = (short)65024,

	/* update_player_state physics constants */
	STATECAR_SPEED_STEP_SCALE_NUM    = 1408,    /* speed-to-step scale numerator (1408) */
	STATECAR_SPEED_STEP_DENOM_10FPS  = 7680,   /* speed-to-step denominator at 10 fps (7680) */
	STATECAR_SPEED_STEP_DENOM_20FPS  = 15360,   /* speed-to-step denominator at 20 fps (15360) */
	STATECAR_SPEED_STEP_DENOM_30FPS  = 23040,   /* speed-to-step denominator at 30 fps (15360*3/2) */
	STATECAR_GRAVITY_PROBE_Z         = 130,     /* pseudo-gravity probe vector Z component (130) */
	STATECAR_TILT_PROBE_Y            = 30000,   /* tilt detection up-vector Y component (30000) */
	STATECAR_WHEEL_TILT_ADJUST       = 192,      /* wheel Y lift/drop for tilt correction */
	STATECAR_WHEEL_BASE_HEIGHT       = 384,    /* wheel model base height offset (384) */
	STATECAR_MAX_INTEGRATION_PASSES  = 5,        /* max wall-bounce integration passes before crash */
	STATECAR_SURFACE_AIRBORNE        = 5,        /* surface type indicating all wheels airborne */
	STATECAR_WALL_DEFLECT_X          = 768,      /* lateral deflection magnitude on wall bounce */
	STATECAR_WALL_IMPACT_COEFF       = 70,     /* wall impact threshold coefficient (70) */
	STATECAR_WALL_IMPACT_BASE        = 100,     /* wall impact threshold base (100) */
	STATECAR_TERRAIN_LIFT_THRESH     = 24,     /* terrain gap above which high-speed lift applies (24) */
	STATECAR_TERRAIN_SURFACE_THRESH  = 12,     /* terrain gap above which wheel leaves surface (12) */
	STATECAR_PLANE_SURFACE_STEP      = 64,     /* correction step when exactly on a plane surface (64) */
	STATECAR_PLANE_WARN_DEPTH        = -12,      /* plane penetration depth: warning level */
	STATECAR_PLANE_DEEP_DEPTH        = -24,      /* plane penetration depth: deep (reset to flat terrain) */
	STATECAR_PLANE_PUSH_OFFSET       = 6,        /* push-out offset after deep plane collision */
	STATECAR_SUSPENSION_EXT_WARN     = 250,     /* suspension extension warning limit (250) */
	STATECAR_SUSPENSION_EXT_CRASH    = 23275,   /* suspension extension crash limit (23275) */
	STATECAR_TRACK_BOUND_MAX         = 1962240, /* track world-space upper boundary */
	STATECAR_TRACK_BOUND_MAX_CLAMP   = 1962239, /* track world-space upper clamped value */
	STATECAR_TRACK_BOUND_MIN         = 3840,    /* track world-space lower boundary (3840) */
	STATECAR_TRACK_GRID_SIZE         = 30,     /* track grid dimension (30) */
	STATECAR_TRACK_GRID_OFFSET       = 29,     /* track grid row scaling offset (29) */
	STATECAR_POLE_OFFSET_DIST        = 126      /* start/finish pole lateral offset (126) */
};


/** @brief Update rpm from speed.
 * @param currpm Parameter value.
 * @param speed Parameter value.
 * @param gearratio Parameter value.
 * @param changing_gear Parameter value.
 * @param idle_rpm Parameter value.
 * @return Function return value.
 */
unsigned int update_rpm_from_speed(unsigned int currpm, unsigned int speed, unsigned int gearratio, int changing_gear, unsigned int idle_rpm) {
	if (changing_gear == 0) {
		currpm = ((unsigned long)speed * gearratio) >> 16;
	}

	if (currpm >= idle_rpm) {
		return currpm;
	}
	return idle_rpm;

}

/** @brief Update car speed.
 * @param carInputFlags Parameter value.
 * @param isOpponentCar Parameter value.
 * @param carState Parameter value.
 * @param simdData Parameter value.
 */
void update_car_speed(char carInputFlags, int isOpponentCar, struct CARSTATE *carState, struct SIMD *simdData) {


	int gearShiftStep;
	int deltaValue;
	unsigned int updatedSpeed;
	unsigned int aeroIndex;
	int speedDelta;
	unsigned char currentTorque;
	short *aeroTable;

	/* ---- frame-rate dependent gear-shift step ---- */
	gearShiftStep = (framespersec >= STATECAR_FPS_30)
	              ? STATECAR_GEARSHIFT_STEP_30FPS
	              : (framespersec == STATECAR_FPS_20)
	              ? STATECAR_GEARSHIFT_STEP_20FPS
	              : STATECAR_GEARSHIFT_STEP_DEFAULT;

	/* ---- engine limiter countdown ---- */
	if (carState->car_engineLimiterTimer != 0)
		carState->car_engineLimiterTimer--;

	/* ---- capture previous-frame values ---- */
	carState->car_speeddiff  = carState->car_speed2 - carState->car_lastspeed;
	carState->car_lastspeed  = carState->car_speed2;
	carState->car_lastrpm    = carState->car_currpm;

	/* ================================================================
	 * GEAR SELECTION
	 * Determine whether to shift up, shift down, or keep the current gear.
	 * ================================================================ */
	{
		int want_upshift   = 0;
		int want_downshift = 0;

		if (carState->car_transmission == 0 && carState->car_changing_gear == 0) {
			/* Manual / no-shift-in-progress: check driver input buttons. */
			if (carInputFlags & STATECAR_INPUT_SHIFT_UP_MASK)
				want_upshift = 1;
			else if (carInputFlags & STATECAR_INPUT_SHIFT_DOWN_MASK)
				want_downshift = 1;
		} else {
			/* Auto-transmission or a shift animation already playing:
			 * only auto-shift once the animation is idle, we are in gear,
			 * and the rear wheels are on the ground. */
			if (carState->car_current_gear != 0 &&
			    carState->car_changing_gear == 0 &&
			    carState->car_sumSurfRearWheels != 0) {
				if (carState->car_currpm > simdData->upshift_rpm)
					want_upshift = 1;
				else if (carState->car_currpm < simdData->downshift_rpm)
					want_downshift = 1;
			}
		}

		if (want_upshift && carState->car_current_gear < simdData->num_gears) {
			carState->car_current_gear++;
			carState->car_changing_gear  = 1;
			carState->car_fpsmul2        = (framespersec >> 1) + framespersec;
			carState->car_knob_x2        = simdData->knob_points[(int)carState->car_current_gear].px;
			carState->car_knob_y2        = simdData->knob_points[(int)carState->car_current_gear].py;
		} else if (want_downshift && carState->car_current_gear > 1) {
			carState->car_current_gear--;
			carState->car_changing_gear  = 1;
			carState->car_fpsmul2        = (framespersec >> 1) + framespersec;
			carState->car_knob_x2        = simdData->knob_points[(int)carState->car_current_gear].px;
			carState->car_knob_y2        = simdData->knob_points[(int)carState->car_current_gear].py;
		}
	}

	/* ================================================================
	 * GEAR KNOB ANIMATION
	 * Animate the gear-stick knob one step toward its target position.
	 * ================================================================ */
	if (carState->car_changing_gear != 0) {
		if (carState->car_knob_x != carState->car_knob_x2) {
			/* x not yet at target – first bring y to neutral, then slide x. */
			if (carState->car_knob_y != simdData->knob_points[0].py) {
				/* Move y toward neutral. */
				deltaValue = simdData->knob_points[0].py - carState->car_knob_y;
				if (abs(deltaValue) <= gearShiftStep) {
					carState->car_knob_y = simdData->knob_points[0].py;
				} else if (deltaValue > 0) {
					carState->car_knob_y += gearShiftStep;
				} else {
					carState->car_knob_y -= gearShiftStep;
				}
			} else {
				/* y is at neutral: slide x toward x2. */
				deltaValue = carState->car_knob_x2 - carState->car_knob_x;
				if (abs(deltaValue) <= gearShiftStep) {
					carState->car_knob_x = carState->car_knob_x2;
				} else if (deltaValue > 0) {
					carState->car_knob_x += gearShiftStep;
				} else {
					carState->car_knob_x -= gearShiftStep;
				}
			}
		} else {
			/* x is at target: move y toward y2. */
			deltaValue = carState->car_knob_y2 - carState->car_knob_y;
			if (deltaValue == 0) {
				/* Arrived: commit gear ratio. */
				carState->car_changing_gear  = 0;
				carState->car_gearratio      = simdData->gear_ratios[(int)carState->car_current_gear];
				carState->car_gearratioshr8  = carState->car_gearratio >> 8;
			} else if (abs(deltaValue) <= gearShiftStep) {
				carState->car_knob_y = carState->car_knob_y2;
			} else if (deltaValue > 0) {
				carState->car_knob_y += gearShiftStep;
			} else {
				carState->car_knob_y -= gearShiftStep;
			}
		}
	} else {
		/* Not shifting: count down the transmission engagement timer. */
		if (carState->car_fpsmul2 != 0)
			carState->car_fpsmul2--;
	}

	/* ================================================================
	 * SPEED DELTA BASE: gravity minus aerodynamic drag
	 * ================================================================ */
	updatedSpeed = carState->car_speed;
	aeroIndex    = updatedSpeed >> STATECAR_AERO_INDEX_SHIFT;
	if (aeroIndex > STATECAR_AERO_INDEX_MAX)
		aeroIndex = STATECAR_AERO_INDEX_MAX;

	aeroTable = (short *)(uintptr_t)simdData->aerorestable;
	if (aeroTable == NULL) {
		aeroTable = (simdData == &simd_opponent_rt) ? aero_table_opponent : aero_table_player;
	}
	speedDelta = (aeroTable != NULL)
	           ? carState->car_pseudoGravity - aeroTable[aeroIndex]
	           : carState->car_pseudoGravity;

	/* ================================================================
	 * THROTTLE / BRAKE INPUT → adjust speedDelta
	 * ================================================================ */
	if (carState->car_currpm > simdData->max_rpm) {
		/* Over rev limit: hard brake regardless of input. */
		carState->car_currpm = simdData->max_rpm - 1;
		speedDelta -= simdData->braking_eff;
	} else {
		int inputCmd = carInputFlags & 3;

		if (inputCmd == 1) {
			/* ---- Accelerating ---- */
			carState->car_is_braking      = 0;
			carState->car_is_accelerating = 1;

			if (carState->car_changing_gear != 0) {
				/* Rev-matching during shift: drop rpm, no torque contribution. */
				carState->car_engineLimiterTimer = 0;
				if (framespersec == STATECAR_FPS_10)
					carState->car_currpm -= STATECAR_RPM_DROP_SHIFT_10FPS;
				else if (framespersec >= STATECAR_FPS_30)
					carState->car_currpm -= STATECAR_RPM_DROP_SHIFT_30FPS;
				else
					carState->car_currpm -= STATECAR_RPM_DROP_SHIFT_DEFAULT;
			} else if (carState->car_sumSurfRearWheels == 0) {
				/* Airborne: tiny rpm boost but limited by speed cap. */
				if (carState->car_currpm < simdData->max_rpm &&
				    updatedSpeed < STATECAR_ACCEL_AIRBORNE_SPEED_MAX) {
					speedDelta += STATECAR_ACCEL_AIRBORNE_BOOST;
				}
			} else {
				/* On ground: compute torque and apply to speedDelta. */
				if (carState->car_current_gear <= 1 &&
				    carState->car_currpm < STATECAR_RPM_IDLE_TORQUE_THRESHOLD) {
					currentTorque = simdData->idle_torque;
				} else {
					currentTorque = simdData->torque_curve[carState->car_currpm >> 7];
				}

				/* Blend toward idle torque while limiter is active. */
				if (carState->car_engineLimiterTimer != 0 &&
				    carState->car_currpm < STATECAR_RPM_LIMITER_BLEND_MAX) {
					currentTorque = ((int)simdData->idle_torque + currentTorque) >> 1;
				}

				speedDelta += (carState->car_gearratioshr8 * currentTorque) >> 4;
				speedDelta = (((long)speedDelta * STATECAR_SPEEDDELTA_MASS_SCALE) / simdData->car_mass) >> 1;

				/* Opponent AI speed correction. */
				if (isOpponentCar != 0) {
					currentTorque = -((int)*opponent_speed_table - STATECAR_OPPONENT_SPEED_BASE) >> 1;
					if (currentTorque != 0)
						speedDelta -= ((long)currentTorque * speedDelta) / STATECAR_OPPONENT_SPEED_BASE;
				}

				if (speedDelta > STATECAR_SPEEDDELTA_LIMIT)
					carState->car_engineLimiterTimer = STATECAR_ENGINE_LIMITER_SHORT;
			}
		} else if (inputCmd == 2) {
			/* ---- Braking ---- */
			carState->car_is_accelerating  = 0;
			carState->car_engineLimiterTimer = 0;
			carState->car_is_braking        = 1;
			if (isOpponentCar == 0)
				speedDelta -= simdData->braking_eff;
			else
				speedDelta -= simdData->braking_eff << 1;
		} else {
			/* ---- Coasting ---- */
			carState->car_is_accelerating = 0;
			carState->car_is_braking      = 0;
		}
	}

	/* FPS scaling: adjust speedDelta to compensate for tick rate differences. */
	if (framespersec == STATECAR_FPS_10)
		speedDelta += speedDelta;
	else if (framespersec >= STATECAR_FPS_30)
		speedDelta = speedDelta * 2 / 3;

	/* ================================================================
	 * APPLY speedDelta TO updatedSpeed
	 * ================================================================ */
	if (speedDelta >= 0) {
		if (updatedSpeed < STATECAR_SPEED_CLAMP_LO) {
			updatedSpeed += speedDelta;
		} else {
			updatedSpeed += speedDelta;
			if (updatedSpeed < STATECAR_SPEED_CLAMP_LO || updatedSpeed > STATECAR_SPEED_CLAMP_HI)
				updatedSpeed = STATECAR_SPEED_CLAMP_HI;
		}
	} else {
		if ((unsigned int)(-speedDelta) > updatedSpeed)
			updatedSpeed = 0;
		else
			updatedSpeed += speedDelta;
	}

	/* ================================================================
	 * COMMIT SPEED
	 * ================================================================ */
	if (carState->car_sumSurfRearWheels == 0) {
		/* Airborne: only update car_speed (not car_speed2). */
		carState->car_speed = updatedSpeed;
	} else {
		deltaValue = (int)carState->car_speed2 - (int)updatedSpeed;
		if (deltaValue < 0)
			deltaValue = -deltaValue;

		if (deltaValue > STATECAR_SPEED_SYNC_THRESHOLD) {
			/* Large divergence: average the two speeds and flag limiter. */
			carState->car_speed  = ((long)carState->car_speed + carState->car_speed2) >> 1;
			carState->car_speed2 = carState->car_speed;
			carState->car_engineLimiterTimer = STATECAR_ENGINE_LIMITER_SHORT;
		} else {
			carState->car_speed  = updatedSpeed;
			carState->car_speed2 = updatedSpeed;
		}
	}

	/* ================================================================
	 * RPM UPDATE AND ENGINE OSCILLATION LIMITER
	 * ================================================================ */
	carState->car_currpm = update_rpm_from_speed(
	    carState->car_currpm, carState->car_speed,
	    carState->car_gearratio, carState->car_changing_gear,
	    simdData->idle_rpm);

	if (carState->car_sumSurfAllWheels != 0 &&
	    carState->car_lastrpm > carState->car_currpm) {
		unsigned int rpmDrop = carState->car_lastrpm - carState->car_currpm;
		if (rpmDrop > (unsigned int)STATECAR_RPM_SWING_THRESHOLD) {
			if (simdData->idle_torque * carState->car_gearratioshr8 > STATECAR_IDLETORQUE_RATIO_THRESHOLD)
				carState->car_engineLimiterTimer = STATECAR_ENGINE_LIMITER_LONG;
		} else {
			/* NOTE: signed comparison */
			if (carState->car_currpm - carState->car_lastrpm > STATECAR_RPM_SWING_THRESHOLD) {
				carState->car_engineLimiterTimer = STATECAR_ENGINE_LIMITER_MED;
				carState->car_speed2 -= STATECAR_SPEED2_LIMITER_PENALTY;
			}
		}
	}

	/* ---- track top speed ---- */
	if (carState->car_speed2 > state.game_topSpeed)
		state.game_topSpeed = carState->car_speed2;
}

#include "stunts.h"
#include "math.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* 
 * Map dseg 16-bit offset to actual pointer.
 * Handles shapeinfos range and camera coordinate data range.
 */
/** @brief State seg ptr16.
 * @param anchor Parameter value.
 * @param ofs Parameter value.
 * @return Function return value.
 */
static void* state_seg_ptr16(const void* anchor, unsigned short ofs)
{
	enum { CAMERA_DATA_OFS_BASE = 3220, CAMERA_DATA_SIZE = 3444 };
	const unsigned char* base = (const unsigned char*)anchor;
	if ((unsigned int)ofs >= STATECAR_SHAPEINFOS_OFS_BASE && (unsigned int)ofs < (STATECAR_SHAPEINFOS_OFS_BASE + STATECAR_SHAPEINFOS_SIZE)) {
		return (void*)(shapeinfos + ((unsigned int)ofs - STATECAR_SHAPEINFOS_OFS_BASE));
	}
	if ((unsigned int)ofs >= CAMERA_DATA_OFS_BASE && (unsigned int)ofs < (CAMERA_DATA_OFS_BASE + CAMERA_DATA_SIZE)) {
		return (void*)(track_camera_coords + ((unsigned int)ofs - CAMERA_DATA_OFS_BASE));
	}
	if (base != 0) {
		return (void*)(base + ofs);
	}
	return (void*)0;
}

#include "shape3d.h"


void update_car_speed(char, int, struct CARSTATE* carstate, struct SIMD* simd);
void update_grip(struct CARSTATE* carstate, struct SIMD* simd, int);
void update_player_state(struct CARSTATE* playerstate, struct SIMD* playersimd, struct CARSTATE* oppstate, struct SIMD* oppsimd, int);
int get_kevinrandom(void);
void state_spawn_debris_particles(int arg0, int arg2, int arg4);
void update_world_debris_particles(void);

/** @brief Upd statef20 from steer input.
 * @param steeringInput Parameter value.
 */
void upd_statef20_from_steer_input(unsigned char steeringInput) {

	int steeringAngle;
	int speed2shr10;
	unsigned char speedMask;
	int idx;
	int delta;
	int damp;
	int absAngle;

	steeringAngle = state.playerstate.car_steeringAngle;
	speed2shr10 = state.playerstate.car_speed2 >> 10;
	speedMask = (unsigned char)(speed2shr10 & STATECAR_STEER_INPUT_SPEED_MASK);

	idx = (signed char)speedMask + (signed char)steeringInput;
	delta = (signed char)((unsigned char*)steerWhlRespTable_ptr)[idx];

    if ((delta > 0 && steeringAngle < -1) || (delta < 0 && steeringAngle > 1)) {
		delta <<= STATECAR_STEER_RECENTER_BOOST_SHIFT;
	}

	if (delta == 0 && state.playerstate.car_speed2 != 0 && steeringAngle != 0) {
		damp = (signed char)((unsigned char*)steerWhlRespTable_ptr)[idx + STATECAR_STEER_IDX_NEXT_OFFSET];
		damp <<= STATECAR_STEER_DAMP_SCALE_SHIFT;

		absAngle = steeringAngle < 0 ? -steeringAngle : steeringAngle;
		if (absAngle > damp) {
			if (steeringAngle > 0) {
				delta = -damp;
            } else {
                delta = damp;
			}
		} else {
			delta = -steeringAngle;
		}
	}

	/* clamp delta based on FPS */
	if (framespersec == STATECAR_FPS_10) {
		if (delta > STATECAR_STEER_DELTA_CLAMP_10FPS) {
			delta = STATECAR_STEER_DELTA_CLAMP_10FPS;
		}
		if (delta < -STATECAR_STEER_DELTA_CLAMP_10FPS) {
			delta = -STATECAR_STEER_DELTA_CLAMP_10FPS;
		}
	} else if (framespersec >= STATECAR_FPS_30) {
		if (delta > STATECAR_STEER_DELTA_CLAMP_30FPS) {
			delta = STATECAR_STEER_DELTA_CLAMP_30FPS;
		}
		if (delta < -STATECAR_STEER_DELTA_CLAMP_30FPS) {
			delta = -STATECAR_STEER_DELTA_CLAMP_30FPS;
		}
	} else {
		if (delta > STATECAR_STEER_DELTA_CLAMP_DEFAULT) {
			delta = STATECAR_STEER_DELTA_CLAMP_DEFAULT;
		}
		if (delta < -STATECAR_STEER_DELTA_CLAMP_DEFAULT) {
			delta = -STATECAR_STEER_DELTA_CLAMP_DEFAULT;
		}
	}

	steeringAngle += delta;

	/* clamp steering angle */
	if (steeringAngle > STATECAR_STEER_ANGLE_CLAMP) {
		steeringAngle = STATECAR_STEER_ANGLE_CLAMP;
	}
	if (steeringAngle < -STATECAR_STEER_ANGLE_CLAMP) {
		steeringAngle = -STATECAR_STEER_ANGLE_CLAMP;
	}

	/* small-angle deadzone depending on table entry */
	if (((unsigned char*)steerWhlRespTable_ptr)[idx] == 0) {
		absAngle = steeringAngle < 0 ? -steeringAngle : steeringAngle;
		if (absAngle < STATECAR_STEER_DEADZONE) {
			steeringAngle = 0;
		}
	}

	state.playerstate.car_steeringAngle = steeringAngle;
}

/** @brief Update player car state.
 * @param carInputFlags Parameter value.
 */
void update_player_car_state(char carInputFlags) {

	struct VECTOR waypointLocalVec;
	struct VECTOR waypointDelta;
	struct VECTOR targetOffset;
	struct VECTOR pathPointsNext[4];
	struct VECTOR pathPointsPrev[4];
	struct MATRIX *carRotationMatrix;
	char bestWaypointSubIndex = 0;
	char previousFlyoverState;
	char lookupComplete;
	char waypointSubIndex;
	short waypointIndex = 0;
	short penaltyCounter = 0;
	int si;

	/* ---- countdown penalty display timer ---- */
	if (show_penalty_counter != 0)
		show_penalty_counter--;

	/* ---- crash-state throttle ---- */
	state.playerstate.car_position_initialized = 1;
	if (state.playerstate.car_crashBmpFlag != 0) {
		state.game_collision_type = 0;
		carInputFlags = 2;
		if (state.playerstate.car_speed2 == 0) {
			state.playerstate.car_position_initialized = 0;
			if (state.playerstate.car_speed == 0 &&
			    state.playerstate.car_rc1[0] == 0 && state.playerstate.car_rc1[1] == 0 &&
			    state.playerstate.car_rc1[2] == 0 && state.playerstate.car_rc1[3] == 0)
				return;
		}
	}

	/* ---- physics tick ---- */
	update_car_speed(carInputFlags, 0, &state.playerstate, &simd_player);
	upd_statef20_from_steer_input((carInputFlags >> 2) & 3);
	update_grip(&state.playerstate, &simd_player, 1);
	update_player_state(&state.playerstate, &simd_player, &state.opponentstate, &simd_opponent_rt, 0);
	state.game_travDist += state.playerstate.car_speed2;

	previousFlyoverState = state.game_flyover_state;
	waypointIndex        = state.game_current_waypoint_index;
	si = detect_penalty(&waypointIndex, &penaltyCounter);

	/* ---- penalty / waypoint-state update ---- */
	if (si != 0) {
		/* update flyover state from penalty type */
		if (penaltyCounter == -2) {
			state.game_flyover_state = 1;
		} else {
			if (state.game_flyover_state == 1)
				state.game_flyover_state = 0;
		}
		state.game_flyover_counter = 0;

		if (state.game_flyover_state == 0) {
			if (waypointIndex == 0 && state.game_track_segment_working_index == 0) {
				/* both zero: bump miss counter and commit index (no penalty time) */
				state.playerstate.car_impact_state_counter++;
				state.game_current_waypoint_index = waypointIndex;
				state.game_flyover_counter = 0;
			} else if (penaltyCounter >= 0 && penaltyCounter < 3) {
				/* light forward progress: accept waypoint, reset flyover */
				state.game_flyover_counter = 0;
				state.game_current_waypoint_index = waypointIndex;
			} else if (penaltyCounter != 3) {
				/* penaltyCounter == -1 or >= 4: check track adjacency */
				if (track_waypoint_next[state.game_track_segment_working_index] == waypointIndex ||
				    track_waypoint_alt[state.game_track_segment_working_index] == waypointIndex) {
					/* forward neighbour: increment flyover streak */
					state.game_flyover_counter++;
				} else {
					/* reverse adjacency check */
					if (track_waypoint_next[waypointIndex] == state.game_track_segment_working_index ||
					    track_waypoint_alt[waypointIndex] == state.game_track_segment_working_index)
						state.game_flyover_state = 2;
					state.game_flyover_counter = 1;
				}

				if (state.game_flyover_counter >= 3) {
					state.game_current_waypoint_index = waypointIndex;
					state.game_flyover_counter = 0;
					if (penaltyCounter > 0) {
						int penalty_time = penaltyCounter * framespersec * 3;
						show_penalty_counter = framespersec << 2;
						state.game_penalty += penalty_time;
					}
				}
			}
			/* penaltyCounter == 3: no extra action; fall through to segment update */
		}

		state.game_track_segment_working_index = waypointIndex;
	}

	/* ---- collision-type and heading computation ---- */
	state.game_collision_type = 0;

	/* flyover_state == 1: car is frozen above a bridge; nothing more to do */
	if (state.game_flyover_state == 1)
		return;

	carRotationMatrix = mat_rot_zxy(state.playerstate.car_rotate.z,
	                                state.playerstate.car_rotate.y,
	                                state.playerstate.car_rotate.x, 1);

	/* ---- determine path to next waypoint target ---- */
	{
		/* do_junction decides whether we scan all sub-paths at waypointIndex.
		 * do_advance skips the junction scan and goes straight to advancing the
		 * track index.  skip_heading means the waypoint is far enough ahead
		 * that we can skip both and jump to the impact check. */
		int do_junction = 0;
		int do_advance  = 0;
		int skip_heading = 0;

		if (state.game_flyover_state == 2) {
			/* crossing a flyover: tag collision type and scan the segment */
			if (state.playerstate.car_crashBmpFlag == 0)
				state.game_collision_type = 3;
			waypointIndex = state.game_track_segment_working_index;
			do_junction = 1;
		} else {
			/* flyover_state == 0: check saved track look-ahead index */
			si = 0;

			if (state.playerstate.car_waypoint_seq_index != -1) {
				/* Invalidate saved index when we just exited a flyover
				 * (previousFlyoverState != 0 && flyover_state == 0), or when
				 * it no longer neighbours the current waypoint. */
				int valid;
				if (previousFlyoverState != 0 && state.game_flyover_state == 0) {
					valid = 0;
				} else {
					valid = (state.playerstate.car_waypoint_seq_index == state.game_current_waypoint_index ||
					         track_waypoint_next[state.game_current_waypoint_index] == state.playerstate.car_waypoint_seq_index ||
					         track_waypoint_alt[state.game_current_waypoint_index] == state.playerstate.car_waypoint_seq_index);
				}

				if (!valid) {
					state.playerstate.car_waypoint_seq_index = -1;
				} else {
					/* measure lookahead distance in local space */
					struct VECTOR d;
					d.x = state.playerstate.car_waypoint_target.x - (state.playerstate.car_posWorld1.lx >> 6);
					d.y = (state.playerstate.car_waypoint_target.y != -1)
					    ? state.playerstate.car_waypoint_target.y - (state.playerstate.car_posWorld1.ly >> 6)
					    : 0;
					d.z = state.playerstate.car_waypoint_target.z - (state.playerstate.car_posWorld1.lz >> 6);
					mat_mul_vector(&d, carRotationMatrix, &waypointLocalVec);
					si = waypointLocalVec.z;
				}
			}

			if (si >= 275) {
				/* waypoint far enough ahead: skip straight to heading computation */
				skip_heading = 1;
			} else if (state.playerstate.car_waypoint_seq_index != -1) {
				/* close enough and valid: advance to next waypoint directly */
				do_advance = 1;
			} else {
				/* no valid track index: scan junction paths at current waypoint */
				waypointIndex = state.game_current_waypoint_index;
				do_junction = 1;
			}
		}

/** @brief Scan.
 * @param do_junction Parameter value.
 * @return Function return value.
 */
		/* ---- junction scan (find best sub-path) ---- */
		if (do_junction) {
			if (track_waypoint_alt[waypointIndex] != -1) {
				/* not a multi-path junction: skip to impact check */
				skip_heading = 1;
				do_advance   = 0;
			} else {
				/* scan all sub-paths; pick the one whose transformed z is
				 * smallest positive (closest ahead) */
				waypointDelta.z  = 0;
				bestWaypointSubIndex = 0;
				waypointSubIndex = 0;
				do {
					lookupComplete = track_waypoint_lookup(waypointIndex,
					                                       &state.playerstate.car_waypoint_target,
					                                       waypointSubIndex, 0);
					targetOffset = state.playerstate.car_waypoint_target;
					targetOffset.x -= (state.playerstate.car_posWorld1.lx >> 6);
					if (targetOffset.y != -1)
						targetOffset.y -= (state.playerstate.car_posWorld1.ly >> 6);
					else
						targetOffset.y = -(state.playerstate.car_posWorld1.ly >> 6);
					targetOffset.z -= (state.playerstate.car_posWorld1.lz >> 6);
					mat_mul_vector(&targetOffset, carRotationMatrix, &waypointLocalVec);

					if (waypointSubIndex == 0 ||
					    (waypointLocalVec.z < waypointDelta.z && waypointLocalVec.z > 0)) {
						bestWaypointSubIndex = waypointSubIndex;
						waypointDelta.z      = waypointLocalVec.z;
					}
					waypointSubIndex++;
				} while (!lookupComplete);

				if (state.game_flyover_state == 2) {
					/* Use path tangent to determine whether heading matches */
					if (bestWaypointSubIndex == 0) {
						track_waypoint_lookup(waypointIndex, pathPointsPrev, 0, 0);
						track_waypoint_lookup(waypointIndex, pathPointsNext, 1, 0);
					} else {
						track_waypoint_lookup(waypointIndex, pathPointsPrev, bestWaypointSubIndex - 1, 0);
						track_waypoint_lookup(waypointIndex, pathPointsNext, bestWaypointSubIndex,     0);
					}

					si = ((state.playerstate.car_rotate.x
					       - polarAngle(pathPointsPrev[0].x - pathPointsNext[0].x,
					                    pathPointsNext[0].z - pathPointsPrev[0].z))
					      & STATECAR_ANGLE_MASK) & STATECAR_ANGLE_MASK;

					if (si > STATECAR_FLYOVER_EXIT_MAX || si < STATECAR_HEADING_SNAP_MARGIN) {
						/* heading mismatch: exit flyover and lock to this sub-path */
						state.game_flyover_state = 0;
						state.game_flyover_counter = 1;
						state.playerstate.car_waypoint_seq_index    = waypointIndex;
						state.playerstate.car_track_waypoint_index = bestWaypointSubIndex;
					}
					/* si in [SNAP_MARGIN, EXIT_MAX]: heading OK — use bestWaypointSubIndex */
					do_advance = 1;
				} else {
					/* flyover_state == 0 after junction lookup: commit chosen sub-path */
					state.playerstate.car_waypoint_seq_index    = state.game_current_waypoint_index;
					state.playerstate.car_track_waypoint_index = bestWaypointSubIndex;
					do_advance = 1;
				}
			}
		}

		/* ---- advance to the next waypoint along the track index ---- */
		if (do_advance && !skip_heading) {
			if (track_waypoint_lookup(state.playerstate.car_waypoint_seq_index,
			                          &state.playerstate.car_waypoint_target,
			                          state.playerstate.car_track_waypoint_index++, 0) == 0) {
				/* reached end of this sub-path: skip heading computation */
				skip_heading = 1;
			} else {
				/* advance to next segment or reset index */
				if (track_waypoint_alt[state.game_current_waypoint_index] == -1)
					state.playerstate.car_waypoint_seq_index = track_waypoint_next[state.game_current_waypoint_index];
				else
					state.playerstate.car_waypoint_seq_index = -1;
				state.playerstate.car_track_waypoint_index = 0;
			}
		}

		/* ---- heading angle and collision-type classification ---- */
		targetOffset = state.playerstate.car_waypoint_target;
		if (!skip_heading &&
		    state.playerstate.car_waypoint_seq_index != -1 &&
		    state.game_flyover_state == 0) {

			targetOffset.x -= (state.playerstate.car_posWorld1.lx >> 6);
			if (targetOffset.y != -1)
				targetOffset.y -= (state.playerstate.car_posWorld1.ly >> 6);
			else
				targetOffset.y = 0;
			targetOffset.z -= (state.playerstate.car_posWorld1.lz >> 6);

			carRotationMatrix = mat_rot_zxy(state.playerstate.car_rotate.z,
			                                state.playerstate.car_rotate.y,
			                                state.playerstate.car_rotate.x, 1);
			mat_mul_vector(&targetOffset, carRotationMatrix, &waypointLocalVec);
			state.playerstate.car_heading_angle =
			    polarAngle(-waypointLocalVec.x, waypointLocalVec.z) & STATECAR_ANGLE_MASK;

			if (state.playerstate.car_crashBmpFlag == 0) {
				unsigned int bin = ((state.playerstate.car_heading_angle + STATECAR_HEADING_SNAP_MARGIN)
				                    & STATECAR_ANGLE_MASK) >> STATECAR_HEADING_BIN_SHIFT;
				if (bin == STATECAR_HEADING_BIN_RIGHT) {
					state.game_collision_type = 1;
				} else if (bin == STATECAR_HEADING_BIN_LEFT) {
					state.game_collision_type =
					    (state.playerstate.car_collision_contact_flag != 0) ? 0 : 2;
				} else {
					state.game_collision_type = 0;
				}
			}
		}
	}

	/* ---- wrong-way / impact detection ---- */
	if (state.playerstate.car_impact_state_counter != 0) {
		si  = multiply_and_scale(cos_fast(track_angle),
		                          trackcenterpos[startrow2]  - (state.playerstate.car_posWorld1.lz >> 6));
		si += multiply_and_scale(sin_fast(track_angle),
		                          trackcenterpos2[startcol2] - (state.playerstate.car_posWorld1.lx >> 6));
		if (si < 0) {
			int wrongway_guard_frames = (framespersec > 0) ? (framespersec >> 1) : 15;
			if ((int)state.game_frame >= wrongway_guard_frames)
				update_crash_state(3, 0);
		}
	}
}

/** @brief Update grip.
 * @param carstate Parameter value.
 * @param simd Parameter value.
 * @param isPlayerFlag Parameter value.
 */
void update_grip(struct CARSTATE* carstate, struct SIMD* simd, int isPlayerFlag) {

	int grassCount;
	int steeringSum;
	int steeringAdjusted;
	int speedshr8;
	int steeringAbs;
	int demandedGrip;
	int combinedGrip;
	int sumSliding;
	int decay;
	int delta;
	int absVz;
	unsigned char tileX;
	unsigned char tileZ;
	unsigned char tile;
	int cosval;

	if (carstate->car_sumSurfAllWheels == 0) {
		carstate->car_40MfrontWhlAngle = 0;
		carstate->car_slidingFlag = 0;
		return;
	}

	grassCount = 0;
	if (carstate->car_surfaceWhl[0] == 4) grassCount++;
	if (carstate->car_surfaceWhl[1] == 4) grassCount++;
	if (carstate->car_surfaceWhl[2] == 4) grassCount++;
	if (carstate->car_surfaceWhl[3] == 4) grassCount++;

	if (grassCount != 0) {
		int decel = carstate->car_speed2 / grassDecelDivTab[grassCount];
		carstate->car_speed2 = (unsigned short)(carstate->car_speed2 - decel);
		carstate->car_speed = carstate->car_speed2;
	}

	steeringSum = carstate->car_steeringAngle + carstate->car_36MwhlAngle;
	steeringAdjusted = steeringSum;

	speedshr8 = carstate->car_speed >> 8;
	steeringAbs = steeringAdjusted < 0 ? -steeringAdjusted : steeringAdjusted;
	steeringAbs >>= 3;
	{
		uint16_t speed16 = (uint16_t)speedshr8;
		uint16_t steer16 = (uint16_t)steeringAbs;
		uint16_t sq16 = (uint16_t)((uint32_t)speed16 * (uint32_t)speed16);
		uint16_t demand16 = (uint16_t)((uint16_t)(sq16 >> 6) * steer16);
		demandedGrip = (int)(int16_t)demand16;
	}

	combinedGrip = (int)(int16_t)((uint16_t)((unsigned short)simd->grip << 1));
	{
		signed char s0 = (signed char)carstate->car_surfaceWhl[0];
		signed char s1 = (signed char)carstate->car_surfaceWhl[1];
		signed char s2 = (signed char)carstate->car_surfaceWhl[2];
		signed char s3 = (signed char)carstate->car_surfaceWhl[3];
		const unsigned char* sliding_base = (const unsigned char*)&simd->sliding;
		short v0, v1, v2, v3;

		memcpy(&v0, sliding_base + ((int)s0 * (int)sizeof(short)), sizeof(v0));
		memcpy(&v1, sliding_base + ((int)s1 * (int)sizeof(short)), sizeof(v1));
		memcpy(&v2, sliding_base + ((int)s2 * (int)sizeof(short)), sizeof(v2));
		memcpy(&v3, sliding_base + ((int)s3 * (int)sizeof(short)), sizeof(v3));

		sumSliding = (int)v0 + (int)v1 + (int)v2 + (int)v3;
	}
	combinedGrip = (int)((int32_t)combinedGrip * (int32_t)sumSliding >> 10);
	combinedGrip = (int)(int16_t)combinedGrip;

	carstate->car_demandedGrip = (short)demandedGrip;
	carstate->car_surfacegrip_sum = (short)combinedGrip;

	if (isPlayerFlag != 0) {
		if (carstate->car_steeringAngle == 0) {
			delta = (signed char)carstate->car_rotate.x;
			if (delta != 0 && ((delta < 0 ? -delta : delta) < 8)) {
				if (delta > 0) {
					carstate->car_rotate.x--;
				} else {
					carstate->car_rotate.x++;
				}
			}
		}

		if (demandedGrip > combinedGrip) {
			long numerator;
			long denom;

			carstate->car_slidingFlag = 1;

			numerator = (long)speedshr8 * (long)speedshr8;
			denom = (long)combinedGrip << 8;
			steeringAdjusted = denom != 0 ? (int)(numerator / denom) : 0;
			if (steeringSum < 0) {
				steeringAdjusted = -steeringAdjusted;
			}
			steeringAdjusted = ((steeringAdjusted * 3) + steeringSum) >> 2;
			carstate->car_steering_residual = (short)(steeringSum - steeringAdjusted);
		} else {
			carstate->car_slidingFlag = 0;
			if (carstate->car_steering_residual != 0) {
				decay = carstate->car_steering_residual >> 4;
				carstate->car_steering_residual = (short)(carstate->car_steering_residual - decay);
				if ((carstate->car_steering_residual < 0 ? -carstate->car_steering_residual : carstate->car_steering_residual) < 16) {
					carstate->car_steering_residual >>= 1;
				}
			}
		}

		if (carstate->car_angle_z == 0 && carstate->car_crashBmpFlag != 1) {
			carstate->car_40MfrontWhlAngle = (short)steeringAdjusted;
		} else {
			carstate->car_40MfrontWhlAngle = 0;
		}

		if (carstate->car_rotate.z != 0) {
			absVz = carstate->car_rotate.z;
			if (absVz < 0) {
				absVz = -absVz;
			}
			if (absVz > 4) {
				tileX = (unsigned char)((unsigned long)carstate->car_posWorld1.lx >> 16);
				tileZ = (unsigned char)((unsigned long)carstate->car_posWorld1.lz >> 16);

				tile = track_elem_map[terrainrows[(unsigned char)tileZ] + tileX];
				if (tile == 253) {
					tileX--;
				} else if (tile == 254) {
					tileZ++;
				} else if (tile == 255) {
					tileX--;
				}

				tile = track_elem_map[terrainrows[(unsigned char)tileZ] + tileX];
				if (tile >= 52 && tile <= 55) {
					carstate->car_40MfrontWhlAngle = (short)(carstate->car_40MfrontWhlAngle + (carstate->car_rotate.z / 5));
				}
			}
		}

		if (demandedGrip > combinedGrip + 1000) {
			delta = (steeringAdjusted - steeringSum) / 14;
			carstate->car_angle_z = (short)(carstate->car_angle_z + delta);
			carstate->car_angle_z = (short)(carstate->car_angle_z / 2);
		} else {
			if (carstate->car_angle_z != 0) {
				delta = (steeringAdjusted - steeringSum) / 14;
				carstate->car_angle_z = (short)(carstate->car_angle_z + delta);
				carstate->car_angle_z = (short)(carstate->car_angle_z / 2);
				if (carstate->car_angle_z == 0) {
					cosval = cos_fast(carstate->car_36MwhlAngle);
					carstate->car_speed2 = (unsigned short)multiply_and_scale(cosval, carstate->car_speed2);
					cosval = cos_fast(carstate->car_36MwhlAngle);
					if (cosval < 0) {
						carstate->car_speed2 = 0;
					}
					carstate->car_36MwhlAngle = 0;
				}
			}
		}

	} else {
		carstate->car_40MfrontWhlAngle = (short)(carstate->car_steeringAngle << 2);
		if (carstate->car_angle_z != 0) {
			carstate->car_angle_z = (short)((carstate->car_angle_z * 15) >> 4);
		}
	}

	if (carstate->car_36MwhlAngle != 0 && carstate->car_angle_z == 0) {
		carstate->car_36MwhlAngle = (short)((carstate->car_36MwhlAngle * 15) >> 4);
	}

	if (carstate->car_angle_z != 0) {
		carstate->car_36MwhlAngle = (short)(carstate->car_36MwhlAngle - carstate->car_angle_z);
	}

	if (carstate->car_slidingFlag != 0) {
		delta = carstate->car_steering_residual;
		delta = delta < 0 ? -delta : delta;
		delta <<= 1;
		if (carstate->car_speed > delta && carstate->car_speed2 > delta) {
			carstate->car_speed = (unsigned short)(carstate->car_speed - delta);
			carstate->car_speed2 = (unsigned short)(carstate->car_speed2 - delta);
		} else {
			carstate->car_speed = 0;
			carstate->car_speed2 = 0;
		}

		if (carstate->car_crashBmpFlag == 0) {
			if (carstate->car_surfaceWhl[0] == 1 || carstate->car_surfaceWhl[1] == 1 || carstate->car_surfaceWhl[2] == 1 || carstate->car_surfaceWhl[3] == 1) {
				carstate->car_position_initialized |= 2;
			} else {
				carstate->car_position_initialized |= 4;
			}
		}
	}

	carstate->car_steering_residual = 0;
}

/** @brief State spawn debris particles.
 * @param arg0 Parameter value.
 * @param arg2 Parameter value.
 * @param arg4 Parameter value.
 */
void state_spawn_debris_particles(int arg0, int arg2, int arg4)
{
    int angle_base;
    int angle_divisor;
    int max_spawn_count;
    int shape_base_index;
    int vertical_velocity_scale;
	int freecount;
	int used;
	int si;
	int di;
	int angle;
    int initial_vertical_velocity;
    short* obstacle_vertical_velocity;

    obstacle_vertical_velocity = (short*)state.game_obstacle_metadata;

	if (arg0 < 2) {
        angle_base = arg2;
	angle_divisor = STATECAR_DEBRIS_ANGLE_DIVISOR_WIDE;
	max_spawn_count = STATECAR_DEBRIS_MAX_WIDE;
        shape_base_index = (arg0 << 2) + 4;
        vertical_velocity_scale = 6;
	} else {
        angle_base = arg2 - 96;
	angle_divisor = STATECAR_DEBRIS_ANGLE_DIVISOR_NARROW;
        max_spawn_count = 8;
        shape_base_index = 0;
        vertical_velocity_scale = 1;
	}

	state.game_obstacle_count = 1;
	freecount = 0;
	for (si = 0; si < 24; si++) {
		if (state.game_obstacle_active[si] == 0) {
			freecount++;
		}
	}
    if (freecount > max_spawn_count) {
        freecount = max_spawn_count;
	}

	used = 0;
	for (si = 0; si < 24 && used < freecount; si++) {
		if (state.game_obstacle_active[si] != 0) {
			continue;
		}

		state.game_obstacle_status[si] = (char)arg0;
        state.game_obstacle_shape[si] = (char)((used & 3) + shape_base_index);

		state.game_longs1[si] = 0;
		state.game_longs2[si] = 0;
		state.game_longs3[si] = 0;

		state.game_obstacle_rotx[si] = (short)(get_kevinrandom() * 4);
		state.game_obstacle_roty[si] = (short)(get_kevinrandom() * 4);

        angle = ((freecount * used) / angle_divisor) + angle_base;
		angle &= 1023;
		state.game_obstacle_rotz[si] = (short)angle;

		di = get_kevinrandom();
		di = ((di << 1) + di) << 1; /* *6 */
		di >>= 2; /* /4 */
		di += arg4 + 384;
		state.game_obstacle_active[si] = (short)di;

        initial_vertical_velocity = (vertical_velocity_scale * di) >> 2;
        obstacle_vertical_velocity[si] = (short)initial_vertical_velocity;

		used++;
	}
}

/** @brief Update world debris particles.
 */
void update_world_debris_particles(void)
{
    int has_active_particles = 0;
    short* obstacle_vertical_velocity = (short*)state.game_obstacle_metadata;
	int si;
	struct MATRIX* rot;
	struct VECTOR invec;
	struct VECTOR outvec;
	long worldY;

	for (si = 0; si < 24; si++) {
		if (state.game_obstacle_active[si] == 0) {
			continue;
		}

		rot = mat_rot_zxy(state.game_obstacle_rotz[si], 0, 0, 0);
		invec.x = 0;
		invec.y = 0;
		invec.z = state.game_obstacle_active[si];
		mat_mul_vector(&invec, rot, &outvec);

		state.game_longs1[si] += outvec.x;
		state.game_longs3[si] += outvec.z;

        obstacle_vertical_velocity[si] -= 19;
        state.game_longs2[si] += obstacle_vertical_velocity[si];
		if (framespersec == 10) {
            obstacle_vertical_velocity[si] -= 19;
            state.game_longs2[si] += obstacle_vertical_velocity[si];
		} else if (framespersec >= 30) {
            /* At 30fps there are 1.5× more ticks per second than 20fps.
               The per-tick gravity (-19) already accumulates faster, so
               skip the extra step — 30 ticks × 19 > 20 ticks × 19. */
		}

		worldY = state.game_longs2[si] + state.playerstate.car_posWorld1.ly;
		if (worldY >= 0) {
            has_active_particles = 1;
			state.game_obstacle_rotx[si] = (short)(state.game_obstacle_rotx[si] + 16);
			state.game_obstacle_roty[si] = (short)(state.game_obstacle_roty[si] + 16);
		} else {
			state.game_obstacle_active[si] = 0;
		}
	}

    state.game_obstacle_count = (char)has_active_particles;
}

/*
 * opponent_op - Opponent AI steering and physics update
/** @brief Opponent op.
 * @param waypoints Parameter value.
 * @param angle Parameter value.
 * @param update Parameter value.
 * @param update_opponent_car_state Parameter value.
 * @return Function return value.
 */
/** @brief Update opponent car state.
 */
void update_opponent_car_state(void) {

	short steerMax;
	short steerStep;
	char chaseFlag;
	short oppPosX, oppPosY, oppPosZ;
	short plrPosX, plrPosY, plrPosZ;
	struct VECTOR vec_3C;  /* copy of car_waypoint_target / temp */
	struct VECTOR delta;   /* var_26..var_22 */
	struct VECTOR rotDelta; /* var_3C..var_38 after mat_mul */
	struct VECTOR localDelta; /* var_6..var_4..var_2 after mat_mul */
	struct VECTOR midpoint; /* var_C..var_A..var_8 */
	short si; /* distance / general use */
	short targetAngle; /* var_2C */
	short targetSpeed; /* var_12 */
	char accelCmd; /* var_E */
	struct MATRIX* matptr; /* var_16 */
	/* FPS-dependent constants */
	if (framespersec == 20) {
		steerMax = 8;
		steerStep = 1;
	} else if (framespersec >= 30) {
		steerMax = 5;
		steerStep = 1;
	} else {
		steerMax = 16;
		steerStep = 2;
	}

	/* Determine if opponent is in steering-limited mode */
	if (state.opponentstate.car_36MwhlAngle != 0 || state.game_inputmode == 2) {
		chaseFlag = 1;
	} else {
		chaseFlag = 0;
	}

	/* Get scaled world positions (long >> 6) */
	oppPosX = (short)(state.opponentstate.car_posWorld1.lx >> 6);
	oppPosY = (short)(state.opponentstate.car_posWorld1.ly >> 6);
	oppPosZ = (short)(state.opponentstate.car_posWorld1.lz >> 6);
	plrPosX = (short)(state.playerstate.car_posWorld1.lx >> 6);
	plrPosY = (short)(state.playerstate.car_posWorld1.ly >> 6);
	plrPosZ = (short)(state.playerstate.car_posWorld1.lz >> 6);

	state.opponentstate.car_position_initialized = 0;
	state.game_flyover_check = 0;

	matptr = mat_rot_zxy(
		state.opponentstate.car_rotate.z,
		state.opponentstate.car_rotate.y,
		state.opponentstate.car_rotate.x,
		1
	);

	state.opponentstate.car_position_initialized = 1;

	/* If crashed, handle braking */
	if (state.opponentstate.car_crashBmpFlag != 0) {
		if (state.opponentstate.car_speed2 != 0) {
			goto do_physics;
		}
		state.opponentstate.car_position_initialized = 0;
		goto do_physics;
	}

	/* Copy current waypoint */
	vec_3C = state.opponentstate.car_waypoint_target;

	/* If waypoint Y != -1, compute 3D distance; otherwise 2D */
	if (vec_3C.y != -1) {
		delta.x = vec_3C.x - oppPosX;
		delta.y = vec_3C.y - oppPosY;
		delta.z = vec_3C.z - oppPosZ;
		si = polarRadius3D(&delta);
	} else {
		si = polarRadius2D(vec_3C.x - oppPosX, vec_3C.z - oppPosZ);
	}

	/* Advance waypoint if close enough */
	if (si < 200) {
advance_waypoint:
		{
			short trackval;
			short idx = state.opponentstate.car_waypoint_seq_index;
			short ce = (short)(unsigned char)state.opponentstate.car_track_waypoint_index;
			state.opponentstate.car_track_waypoint_index++;
			trackval = ((short *)track_waypoint_order)[idx];
            if (track_waypoint_lookup(trackval, &state.opponentstate.car_waypoint_target, ce, (short*)&state.game_track_lookup_temp) != 0) {
				state.opponentstate.car_waypoint_seq_index++;
				if (((short *)track_waypoint_order)[state.opponentstate.car_waypoint_seq_index] == 0) {
					state.opponentstate.car_impact_state_counter++;
					state.opponentstate.car_waypoint_seq_index = 0;
				}
				state.opponentstate.car_track_waypoint_index = 0;
			}
		}
	}

	/* Compute steering target */
	if (state.game_inputmode == 2) {
		/* Autopilot mode - follow waypoints directly */
		midpoint = state.opponentstate.car_waypoint_target;
compute_target:
		delta = midpoint;
		delta.x -= oppPosX;
		if (midpoint.y == -1) {
			delta.y = 0;
		} else {
			delta.y -= oppPosY;
		}
		delta.z -= oppPosZ;
		goto compute_angle;
	}

	/* Normal mode: steer toward player */
	delta.x = plrPosX - oppPosX;
	delta.y = plrPosY - oppPosY;
	delta.z = plrPosZ - oppPosZ;

	mat_mul_vector(&delta, matptr, &rotDelta);

	/* If player is behind, follow waypoints */
	if (rotDelta.y > STATECAR_AI_PLAYER_BEHIND_Y) {
		goto use_waypoint;
	}

	/* Check lateral distance */
	{
		short absX = rotDelta.x < 0 ? -rotDelta.x : rotDelta.x;
		if (absX > STATECAR_AI_LATERAL_MAX) goto use_waypoint;
	}
	if (rotDelta.z > STATECAR_AI_DEPTH_MAX) goto use_waypoint;
	if (rotDelta.z < STATECAR_AI_DEPTH_MIN) goto use_waypoint;

	/* Steer toward waypoint, adjusted for player proximity */
	delta.x = plrPosX - state.opponentstate.car_waypoint_target.x;
	if (state.opponentstate.car_waypoint_target.y != -1) {
		delta.y = plrPosY - state.opponentstate.car_waypoint_target.y;
	} else {
		delta.y = 0;
	}
	delta.z = plrPosZ - state.opponentstate.car_waypoint_target.z;

	mat_mul_vector(&delta, matptr, &localDelta);

	if (localDelta.x < 0) {
			/* Player is to the left of waypoint -> opponent takes right-side path (car_vec_unk5) */
		midpoint.x = (short)(((long)state.opponentstate.car_waypoint_right.x + (long)state.opponentstate.car_waypoint_target.x) >> 1);
		if (state.opponentstate.car_waypoint_target.y != -1) {
			midpoint.y = (short)(((long)state.opponentstate.car_waypoint_right.y + (long)state.opponentstate.car_waypoint_target.y) >> 1);
		} else {
			midpoint.y = -1;
		}
		midpoint.z = (short)(((long)state.opponentstate.car_waypoint_right.z + (long)state.opponentstate.car_waypoint_target.z) >> 1);

		/* If player is behind and not crashed, set field_45E=2 */
		if (rotDelta.z > STATECAR_AI_CHASE_DEPTH_TRIGGER) {
			if (state.playerstate.car_crashBmpFlag == 0) {
				state.game_flyover_check = 2;
			}
		}
		goto compute_target;
	} else {
			/* Player is to the right -> opponent takes left-side path (car_vec_unk4) */
		midpoint.x = (short)(((long)state.opponentstate.car_waypoint_left.x + (long)state.opponentstate.car_waypoint_target.x) >> 1);
		if (state.opponentstate.car_waypoint_target.y != -1) {
			midpoint.y = (short)(((long)state.opponentstate.car_waypoint_left.y + (long)state.opponentstate.car_waypoint_target.y) >> 1);
		} else {
			midpoint.y = -1;
		}
		midpoint.z = (short)(((long)state.opponentstate.car_waypoint_left.z + (long)state.opponentstate.car_waypoint_target.z) >> 1);

		if (rotDelta.z > STATECAR_AI_CHASE_DEPTH_TRIGGER) {
			if (state.playerstate.car_crashBmpFlag == 0) {
				state.game_flyover_check = 1;
			}
		}
		goto compute_target;
	}

use_waypoint:
	midpoint = state.opponentstate.car_waypoint_target;
	goto compute_target;

compute_angle:
	/* Transform delta into local coords */
	mat_mul_vector(&delta, matptr, &vec_3C);
	targetAngle = polarAngle(vec_3C.x, vec_3C.z);

	/* If not sliding and angle too large, advance waypoint */
	if (state.opponentstate.car_slidingFlag == 0) {
		short absAngle = targetAngle < 0 ? -targetAngle : targetAngle;
		if (absAngle > STATECAR_AI_WAYPOINT_ANGLE_MAX) {
			short trackval;
			short idx = state.opponentstate.car_waypoint_seq_index;
			short ce = (short)(unsigned char)state.opponentstate.car_track_waypoint_index;
			state.opponentstate.car_track_waypoint_index++;
			trackval = ((short *)track_waypoint_order)[idx];
            if (track_waypoint_lookup(trackval, &state.opponentstate.car_waypoint_target, ce, (short*)&state.game_track_lookup_temp) != 0) {
				state.opponentstate.car_waypoint_seq_index++;
				if (((short *)track_waypoint_order)[state.opponentstate.car_waypoint_seq_index] == 0) {
					state.opponentstate.car_impact_state_counter++;
					state.opponentstate.car_waypoint_seq_index = 0;
				}
				state.opponentstate.car_track_waypoint_index = 0;
			}
		}
	}

	/* Clamp target angle and handle chaseFlag */
	if (targetAngle > STATECAR_AI_STEER_TARGET_MAX) {
		if (chaseFlag == 0) {
			chaseFlag = 1;
			goto advance_waypoint;
		}
		targetAngle = STATECAR_AI_STEER_TARGET_MAX;
	} else if (targetAngle < STATECAR_AI_STEER_TARGET_MIN) {
		if (chaseFlag == 0) {
			chaseFlag = 1;
			goto advance_waypoint;
		}
		targetAngle = STATECAR_AI_STEER_TARGET_MIN;
	}

	/* Zero steering if no front-wheel surface contact */
	if (state.opponentstate.car_sumSurfFrontWheels == 0) {
		targetAngle = 0;
	}

	/* Apply steering with rate limit */
	{
		short diff = targetAngle - state.opponentstate.car_steeringAngle;
		short absDiff = diff < 0 ? -diff : diff;
		if (absDiff > steerMax) {
			if (targetAngle < state.opponentstate.car_steeringAngle) {
				state.opponentstate.car_steeringAngle -= steerMax;
			} else {
				state.opponentstate.car_steeringAngle += steerMax;
			}
		} else {
			state.opponentstate.car_steeringAngle = targetAngle;
		}
	}

do_physics:
	/* Determine acceleration command */
	accelCmd = 0;
	if (state.opponentstate.car_sumSurfRearWheels != 0) {
		if (state.opponentstate.car_crashBmpFlag != 0) {
			accelCmd = 2; /* brake */
		} else if (state.opponentstate.car_36MwhlAngle != 0) {
			/* Decelerating from collision bounce */
			{
				unsigned short decel = (unsigned short)steerStep << 9;
				if (decel >= state.opponentstate.car_speed2) {
					state.opponentstate.car_speed2 = 0;
					state.opponentstate.car_36MwhlAngle = 0;
				} else {
					state.opponentstate.car_speed2 -= decel;
				}
			}
		} else if (state.opponentstate.car_demandedGrip <= state.opponentstate.car_surfacegrip_sum) {
			if (state.game_inputmode == 2) {
				targetSpeed = STATECAR_AI_TARGET_SPEED_ARCADE;
			} else {
				targetSpeed = (short)((unsigned short)state.game_track_lookup_temp << 8);
			}
			if ((unsigned short)(targetSpeed - STATECAR_AI_SPEED_TARGET_LOWER_BAND) > state.opponentstate.car_speed) {
				accelCmd = 1; /* accelerate */
			} else if ((unsigned short)(targetSpeed + STATECAR_AI_SPEED_TARGET_UPPER_BAND) < state.opponentstate.car_speed) {
				accelCmd = 2; /* brake */
			}
		} else {
			accelCmd = 2; /* brake when grip exceeded */
		}
	}

	update_car_speed((char)accelCmd, 1, &state.opponentstate, &simd_opponent_rt);
	update_grip(&state.opponentstate, &simd_opponent_rt, 0);
	update_player_state(&state.opponentstate, &simd_opponent_rt, &state.playerstate, &simd_player, 1);

	/* Post-physics: compute heading field_48 */
	if (state.opponentstate.car_crashBmpFlag == 0) {
		delta = state.opponentstate.car_waypoint_target;
		delta.x -= (short)(state.opponentstate.car_posWorld1.lx >> 6);
		delta.y -= (short)(state.opponentstate.car_posWorld1.ly >> 6);
		delta.z -= (short)(state.opponentstate.car_posWorld1.lz >> 6);

		matptr = mat_rot_zxy(
			state.opponentstate.car_rotate.z,
			state.opponentstate.car_rotate.y,
			state.opponentstate.car_rotate.x,
			1
		);
		mat_mul_vector(&delta, matptr, &vec_3C);
		state.opponentstate.car_heading_angle = polarAngle(-vec_3C.x, vec_3C.z) & STATECAR_ANGLE_MASK;
	}

	/* Check if opponent crossed the finish line */
	if (state.opponentstate.car_impact_state_counter == 0) {
		return;
	}
	{
		short finishX, finishZ;
		short posScaledX, posScaledZ;
		short dist;

		posScaledZ = (short)(state.opponentstate.car_posWorld1.lz >> 6);
		finishZ = trackcenterpos[(signed char)startrow2];
		dist = multiply_and_scale(finishZ - posScaledZ, cos_fast(track_angle));

		posScaledX = (short)(state.opponentstate.car_posWorld1.lx >> 6);
		finishX = trackcenterpos2[(signed char)startcol2];
		dist += multiply_and_scale(finishX - posScaledX, sin_fast(track_angle));

		if (dist < 0) {
			update_crash_state(3, 1);
		}
	}
}

/* ---- car_car_speed_adjust_maybe ---- */
/* Adjusts car speeds and wheel angles during car-to-car collision */
/** @brief Car car speed adjust collision.
 * @param oState Parameter value.
 * @param pState Parameter value.
 * @return Function return value.
 */
int car_car_speed_adjust_collision(struct CARSTATE* oState, struct CARSTATE* pState) {

    uint16_t oSpeed;
    uint16_t pSpeed;
    int16_t oAngle;
    int16_t pAngle;
    int16_t oVx;
    int16_t oVz;
    int16_t pVx;
    int16_t pVz;
    int16_t relSpeed;
    int16_t speedReduce;

	oState->car_crash_impact_flag = 1;
	pState->car_crash_impact_flag = 1;

    oSpeed = (uint16_t)oState->car_speed2;
    pSpeed = (uint16_t)pState->car_speed2;
    oAngle = (int16_t)oState->car_rotate.x;
    pAngle = (int16_t)pState->car_rotate.x;

	/* Compute velocity components */
    oVx = (int16_t)multiply_and_scale(sin_fast(oAngle), (int)(oSpeed >> 8));
    pVx = (int16_t)multiply_and_scale(sin_fast(pAngle), (int)(pSpeed >> 8));
    oVz = (int16_t)multiply_and_scale(cos_fast(oAngle), (int)(oSpeed >> 8));
    pVz = (int16_t)multiply_and_scale(cos_fast(pAngle), (int)(pSpeed >> 8));

	/* Relative velocity magnitude */
    relSpeed = (int16_t)polarRadius2D((int)(pVx - oVx), (int)(pVz - oVz));
	if (relSpeed < STATECAR_COLLISION_RELSPEED_MIN)
		relSpeed = STATECAR_COLLISION_RELSPEED_MIN;

	/* Speed reduction: (STATECAR_COLLISION_REDUCE_SCALE * relSpeed) >> STATECAR_COLLISION_REDUCE_SHIFT */
    speedReduce = (int16_t)(((int)STATECAR_COLLISION_REDUCE_SCALE * (int)relSpeed) >> STATECAR_COLLISION_REDUCE_SHIFT);

	/* Decelerate oState */
    if ((uint16_t)oState->car_speed2 < (uint16_t)speedReduce)
		oState->car_speed2 = 0;
	else
        oState->car_speed2 = (uint16_t)((uint16_t)oState->car_speed2 - (uint16_t)speedReduce);

	/* Set oState wheel angle */
    oState->car_36MwhlAngle = (int16_t)(pAngle - oAngle);
	if (oState->car_36MwhlAngle >= STATECAR_ANGLE_HALF) {
		oState->car_36MwhlAngle = (int16_t)(oState->car_36MwhlAngle - STATECAR_ANGLE_FULL);
    }
	if (oState->car_36MwhlAngle <= STATECAR_ANGLE_SIGN_WRAP_NEG) {
		oState->car_36MwhlAngle = (int16_t)(oState->car_36MwhlAngle + STATECAR_ANGLE_FULL);
    }

	/* Set pState wheel angle */
    pState->car_36MwhlAngle = (int16_t)(oAngle - pAngle);
	if (pState->car_36MwhlAngle >= STATECAR_ANGLE_HALF) {
		pState->car_36MwhlAngle = (int16_t)(pState->car_36MwhlAngle - STATECAR_ANGLE_FULL);
    }
	if (pState->car_36MwhlAngle <= STATECAR_ANGLE_SIGN_WRAP_NEG) {
		pState->car_36MwhlAngle = (int16_t)(pState->car_36MwhlAngle + STATECAR_ANGLE_FULL);
    }

	/* Copy speed2 to speed */
	oState->car_speed = oState->car_speed2;
	pState->car_speed = pState->car_speed2;

		if (relSpeed > STATECAR_COLLISION_RELSPEED_MAX)
		return 1;
	return 0;
}

/* ---- carState_rc_op ---- */
/* Suspension spring/damper operation for a single wheel */
/** @brief Carstate update wheel suspension.
 * @param pState Parameter value.
 * @param suspension_delta Parameter value.
 * @param wheelIndex Parameter value.
 * @return Function return value.
 */
short carState_update_wheel_suspension(struct CARSTATE* pState, short suspension_delta, short wheelIndex) {

	short old_rc2 = pState->car_rc2[wheelIndex];
	short delta = 0;

	/* Decay rc5 toward 0 */
	if (pState->car_rc5[wheelIndex] != 0) {
		if (pState->car_rc5[wheelIndex] < 0) {
			pState->car_rc5[wheelIndex] += 4;
			if (pState->car_rc5[wheelIndex] > 0)
				pState->car_rc5[wheelIndex] = 0;
		} else {
			pState->car_rc5[wheelIndex] -= 4;
			if (pState->car_rc5[wheelIndex] < 0)
				pState->car_rc5[wheelIndex] = 0;
		}
	}

	/* If suspension_delta < 0 and rc2 is large enough, clamp suspension_delta to 0 */
	if (suspension_delta < 0) {
		if (pState->car_rc2[wheelIndex] > -suspension_delta)
			suspension_delta = 0;
	}

	if (suspension_delta == 0) {
		/* No force: decay rc2 toward rc5 */
		if (pState->car_rc2[wheelIndex] > pState->car_rc5[wheelIndex]) {
			pState->car_rc2[wheelIndex] -= 128;
			if (pState->car_rc2[wheelIndex] < pState->car_rc5[wheelIndex])
				pState->car_rc2[wheelIndex] = pState->car_rc5[wheelIndex];
			delta = old_rc2 - pState->car_rc2[wheelIndex];
		} else if (pState->car_rc2[wheelIndex] < pState->car_rc5[wheelIndex]) {
			pState->car_rc2[wheelIndex] += 128;
			if (pState->car_rc2[wheelIndex] > pState->car_rc5[wheelIndex])
				pState->car_rc2[wheelIndex] = pState->car_rc5[wheelIndex];
			/* delta stays 0 */
		}
		/* else rc2 == rc5: no change, delta stays 0 */
	} else if (suspension_delta > 0) {
		/* Compression: increment rc2, cap increment at 192, clamp at 384 */
		if (suspension_delta > 192)
			pState->car_rc2[wheelIndex] += 192;
		else
			pState->car_rc2[wheelIndex] += suspension_delta;
		if (pState->car_rc2[wheelIndex] > 384)
			pState->car_rc2[wheelIndex] = 384;
		pState->car_rc4[wheelIndex] = 0;
		/* delta stays 0 */
	} else {
		/* Extension (suspension_delta < 0): damped if too large */
		if (pState->car_rc2[wheelIndex] + suspension_delta <= (short)65248) {
			/* Large extension: apply damped force (3/4) */
			pState->car_rc2[wheelIndex] += (suspension_delta * 3) >> 2;
			if (pState->car_rc2[wheelIndex] < (short)65152)
				pState->car_rc2[wheelIndex] = (short)65152;
		} else {
			/* Small extension: apply full force */
			pState->car_rc2[wheelIndex] += suspension_delta;
		}
		delta = old_rc2 - pState->car_rc2[wheelIndex] + suspension_delta;
	}

	return old_rc2 + delta;
}

/* ---- track_waypoint_lookup ---- */
/* Track waypoint lookup for opponent AI navigation.
 * Reads track element data, looks up camera/waypoint arrays,
 * applies rotation transform, terrain height, and tile position offsets.
 * Writes: out[0..2] = midpoint VECTOR, out[3..5] = waypoint1, out[6..8] = waypoint2, out[9] = opp_flag
 * Returns 1 if field_CE == arrowType-1, else 0. */
/** @brief Track waypoint lookup.
 * @param path_index Parameter value.
 * @param waypoint_out Parameter value.
 * @param lane_index Parameter value.
 * @param opponent_speed_out Parameter value.
 * @return Function return value.
 */
short track_waypoint_lookup(short path_index, struct VECTOR* waypoint_out, short lane_index, short* opponent_speed_out) {

	extern unsigned char trkObjectList[];
	unsigned char tileElem;
	unsigned char td18subTOI;
	unsigned char td18connStatus;
	const unsigned char* trkObjBytes;
	state_trackobject_raw trkObj;
	state_trkobjinfo_raw toInfo;
	short opponentFlag; /* opp flag */
	unsigned char arrowType;
	unsigned char cameraDataIndex; /* index into camera data */
	short* dataBase;
	short* dataPtr;
	short wp1_x, wp1_y, wp1_z; /* var_C, var_A, var_8 */
	short wp2_x, wp2_y, wp2_z; /* var_22, var_20, var_1E */
	short tmp;
	unsigned char col, row;
	short* outShorts = (short*)waypoint_out;

	/* Look up track element and sub-object index */
	tileElem = track_elem_ordered[path_index];
	td18subTOI = path_conn_flags[path_index] & 15;
	td18connStatus = path_conn_flags[path_index] & 16;

	/* Decode raw 16-bit dseg-based TRACKOBJECT/TRKOBJINFO entries */
	trkObjBytes = (const unsigned char*)trkObjectList;
	if (!state_trackobject_raw_decode(trkObjBytes, (unsigned int)tileElem, &trkObj)) {
        fatal_error("track_waypoint_lookup: bad trackobject decode elem=%u", (unsigned)tileElem);
	}
/** @brief Trkobjinfo.
 * @param tile Parameter value.
 * @param trkobj_info_ofs Parameter value.
 * @return Function return value.
 */
	/* trkobj_info_ofs == 0 means this element has no TRKOBJINFO (empty tile, no waypoints).
	 * Return 0 gracefully — same as arrowType == 0. */
	if (trkObj.trkobj_info_ofs == 0) {
		return 0;
	}
	if (!state_trkobjinfo_raw_decode((const unsigned char*)state_seg_ptr16(0, trkObj.trkobj_info_ofs), (unsigned int)td18subTOI, &toInfo)) {
        fatal_error("track_waypoint_lookup: bad trkobjinfo decode elem=%u sub=%u",
			(unsigned)tileElem,
			(unsigned)td18subTOI);
	}

	opponentFlag = 0;
	arrowType = toInfo.arrow_type;
	if (arrowType == 0) {
		return 0;
	}
	if ((unsigned char)lane_index >= arrowType) {
		lane_index = (short)(arrowType - 1);
	}

	/* Compute index into camera data arrays */
	if (td18connStatus == 0) {
		cameraDataIndex = (unsigned char)lane_index << 1;
	} else {
		cameraDataIndex = (arrowType - (unsigned char)lane_index) * 2 - 2;
	}

	/* If opponent_speed_out is provided, compute opponent speed code */
	if (opponent_speed_out != 0) {
		unsigned char spedCode = toInfo.opp_sped_code;
		unsigned char surfType = trkObj.surface_type;
		/* Original ASM: oppnentSped[bx+si] where bx=spedCode, si=surfType&255 */
		*((unsigned char*)opponent_speed_out) = opponent_speed_table[(unsigned int)spedCode + (surfType & 255)];
	}

/** @brief Nonzero.
 * @param opp2 Parameter value.
 * @return Function return value.
 */
	/* Check if si_opp1 is nonzero (read as word: si_opp1 + si_opp2) */
	if (((unsigned short)toInfo.opp1 | ((unsigned short)toInfo.opp2 << 8)) != 0) {
		opponentFlag = 1;
	}

	/* Read waypoint data based on connStatus */
	if (td18connStatus != 0) {
		if (opponentFlag != 0) {
			/* Use si_opp1 as data base pointer */
			dataBase = (short*)state_seg_ptr16(0, (unsigned short)toInfo.opp1 | ((unsigned short)toInfo.opp2 << 8));
		} else {
			/* Use si_cameraDataOffset, read in reversed order */
			dataBase = (short*)state_seg_ptr16(0, toInfo.camera_data_ofs);
			dataPtr = (short*)((char*)dataBase + (unsigned char)cameraDataIndex * 6);
			/* First waypoint = data[index+6..11], second = data[index+0..5] */
			wp1_x = dataPtr[3]; wp1_y = dataPtr[4]; wp1_z = dataPtr[5];
			wp2_x = dataPtr[0]; wp2_y = dataPtr[1]; wp2_z = dataPtr[2];
			goto after_data_read;
		}
	} else {
		dataBase = (short*)state_seg_ptr16(0, toInfo.camera_data_ofs);
	}

	/* Normal order: first waypoint = data[index], second = data[index+6] */
	dataPtr = (short*)((char*)dataBase + (unsigned char)cameraDataIndex * 6);
	wp1_x = dataPtr[0]; wp1_y = dataPtr[1]; wp1_z = dataPtr[2];
	wp2_x = dataPtr[3]; wp2_y = dataPtr[4]; wp2_z = dataPtr[5];

after_data_read:
	/* Apply rotation based on si_arrowOrient */
	switch (toInfo.arrow_orient) {
	case 256: /* 90 degrees CW */
		tmp = wp1_x;
		wp1_x = wp1_z;
		wp1_z = -tmp;
		tmp = wp2_x;
		wp2_x = wp2_z;
		wp2_z = -tmp;
		break;
	case 512: /* 180 degrees */
		wp1_z = -wp1_z;
		wp1_x = -wp1_x;
		wp2_z = -wp2_z;
		wp2_x = -wp2_x;
		break;
	case 768: /* 270 degrees CW */
		tmp = wp1_x;
		wp1_x = -wp1_z;
		wp1_z = tmp;
		tmp = wp2_x;
		wp2_x = -wp2_z;
		wp2_z = tmp;
		break;
	default:
		break;
	}

	/* Get tile column and row from track data */
	col = path_col[path_index];
	row = path_row[path_index];

	/* Terrain height adjustment */
	if (wp1_y != -1) {
		short terrIdx = terrainrows[(unsigned char)row];
		if (track_terrain_map[terrIdx + (unsigned char)col] == 6) {
			wp1_y += hillHeightConsts[1];
			wp2_y += hillHeightConsts[1];
		}
	}

	
	if (trkObj.multi_tile_flag & 1) {
		/* Vertical multi-tile: use trackpos */
		wp1_z += trackpos[(unsigned char)row];
		wp2_z += trackpos[(unsigned char)row];
	} else {
		wp1_z += trackcenterpos[(unsigned char)row];
		wp2_z += trackcenterpos[(unsigned char)row];
	}

	if (trkObj.multi_tile_flag & 2) {
		/* Horizontal multi-tile: use trackpos2[1] */
		wp1_x += trackpos2[1 + (unsigned char)col];
		wp2_x += trackpos2[1 + (unsigned char)col];
	} else {
		wp1_x += trackcenterpos2[(unsigned char)col];
		wp2_x += trackcenterpos2[(unsigned char)col];
	}

	/* Output: midpoint = average of two waypoints (signed 32-bit average → 16-bit) */
	outShorts[0] = (short)(((long)wp1_x + (long)wp2_x) >> 1);
	if (wp1_y == -1) {
		outShorts[1] = -1;
	} else {
		outShorts[1] = (short)(((long)wp1_y + (long)wp2_y) >> 1);
	}
	outShorts[2] = (short)(((long)wp1_z + (long)wp2_z) >> 1);

	/* Output: first waypoint at [waypoint_out+6] */
	outShorts[3] = wp1_x;
	outShorts[4] = wp1_y;
	outShorts[5] = wp1_z;

	/* Output: second waypoint at [waypoint_out+0Ch] */
	outShorts[6] = wp2_x;
	outShorts[7] = wp2_y;
	outShorts[8] = wp2_z;

	/* Output: opp flag at [waypoint_out+12h] */
	outShorts[9] = opponentFlag;

	/* Return 1 if lane_index == arrowType - 1 */
	if ((short)(arrowType - 1) == (short)(signed char)lane_index)
		return 1;
	return 0;
}

/* ---- audio_carstate ---- */
/* Manages audio state for car engine/crash sounds each frame.
   In replay mode: plays back stored audio events.
   In normal mode: computes camera-relative positions and updates sound effects. */
/** @brief Update per-car engine and crash/skid audio state for the current frame.
 */

void audio_sync_car_audio(void) {
	short playerPos2X, playerPos2Y, playerPos2Z; /* player pos2 >> 6 */
	short playerPos1X,  playerPos1Y,  playerPos1Z;  /* player pos1 >> 6 */
	short opponentPos2X, opponentPos2Y, opponentPos2Z; /* opponent pos2 >> 6 */
	short opponentPos1X,  opponentPos1Y,  opponentPos1Z;  /* opponent pos1 >> 6 */
	short cameraPos1X, cameraPos1Y, cameraPos1Z; /* camera pos1 */
	short cameraPos2X, cameraPos2Y, cameraPos2Z; /* camera pos2 */
	short carCount; /* number of cars to process (1 or 2) */
	short carIndex; /* loop counter */
	char soundFlags;  /* sound flags copy */
	short *bx;
	char  *fp;

	if (is_in_replay) {
		/* Replay path */
		if (audio_replay_apply_state) {
			audio_frame_index = menu_selection_buffer;
            if (player_audio_state & 6) {
                audio_clear_chunk_fx(crash_sound_handle);
			}
			if (player_audio_state & 1) {
                audio_stop_engine_note(crash_sound_handle);
			}
			if (gameconfig.game_opponenttype) {
                if (opponent_audio_state & 6) {
                    audio_clear_chunk_fx(audio_engine_sound_handle);
				}
				if (opponent_audio_state & 1) {
                    audio_stop_engine_note(audio_engine_sound_handle);
				}
			}
			audio_replay_apply_state = 0;
			player_audio_state = 0;
			opponent_audio_state = 0;
		}
		if (sprite_boundary_marker != is_in_replay) {
            audio_on_replay_mode_changed();
		}
		goto end_func;
	}

	/* Normal (non-replay) path: compute shifted positions */
	playerPos2X = (short)(state.playerstate.car_posWorld2.lx >> 6);
	playerPos2Y = (short)(state.playerstate.car_posWorld2.ly >> 6);
	playerPos2Z = (short)(state.playerstate.car_posWorld2.lz >> 6);
	playerPos1X  = (short)(state.playerstate.car_posWorld1.lx >> 6);
	playerPos1Y  = (short)(state.playerstate.car_posWorld1.ly >> 6);
	playerPos1Z  = (short)(state.playerstate.car_posWorld1.lz >> 6);

	if (gameconfig.game_opponenttype) {
		opponentPos2X = (short)(state.opponentstate.car_posWorld2.lx >> 6);
		opponentPos2Y = (short)(state.opponentstate.car_posWorld2.ly >> 6);
		opponentPos2Z = (short)(state.opponentstate.car_posWorld2.lz >> 6);
		opponentPos1X  = (short)(state.opponentstate.car_posWorld1.lx >> 6);
		opponentPos1Y  = (short)(state.opponentstate.car_posWorld1.ly >> 6);
		opponentPos1Z  = (short)(state.opponentstate.car_posWorld1.lz >> 6);
	} else {
		opponentPos2X = opponentPos2Y = opponentPos2Z = 0;
		opponentPos1X = opponentPos1Y = opponentPos1Z = 0;
	}

	/* Camera mode determines which positions to use for audio */
	switch ((signed char)cameramode) {
	case 0:
	case 2:
		if (followOpponentFlag) {
			cameraPos1X = opponentPos1X;  cameraPos1Y = opponentPos1Y;  cameraPos1Z = opponentPos1Z;
			cameraPos2X = opponentPos2X; cameraPos2Y = opponentPos2Y; cameraPos2Z = opponentPos2Z;
		} else {
			cameraPos1X = playerPos1X;  cameraPos1Y = playerPos1Y;  cameraPos1Z = playerPos1Z;
			cameraPos2X = playerPos2X; cameraPos2Y = playerPos2Y; cameraPos2Z = playerPos2Z;
		}
		break;
	case 1: {
		short idx = (signed char)followOpponentFlag;
		cameraPos1X = state.game_vec1[idx].x;
		cameraPos1Y = state.game_vec1[idx].y;
		cameraPos1Z = state.game_vec1[idx].z;
		cameraPos2X = (&state.game_vec3)[idx].x;
		cameraPos2Y = (&state.game_vec3)[idx].y;
		cameraPos2Z = (&state.game_vec3)[idx].z;
		break;
	}
	case 3: {
		short idx = (signed char)state.game_track_indices[(unsigned char)followOpponentFlag];
		fp = (char *)waypoint_world_pos + idx * 6;
		cameraPos1X = *(short *)fp;
		cameraPos1Y = *(short *)(fp + 2) + camera_y_offset + 90;
		cameraPos1Z = *(short *)(fp + 4);
		cameraPos2X = cameraPos1X; cameraPos2Y = cameraPos1Y; cameraPos2Z = cameraPos1Z;
		break;
	}
	default:
		cameraPos1X = cameraPos1Y = cameraPos1Z = 0;
		cameraPos2X = cameraPos2Y = cameraPos2Z = 0;
		break;
	}

	/* Compute audio frame entry pointer: var_2 = 34 * menu_selection_buffer + &replay_audio_frame_buffer */
	bx = (short *)(replay_audio_frame_buffer + 34 * menu_selection_buffer);

	/* Store camera-relative positions for player */
	bx[3] = cameraPos2X - playerPos2X;
	bx[4] = cameraPos2Y - playerPos2Y;
	bx[5] = cameraPos2Z - playerPos2Z;
	bx[6] = cameraPos1X - playerPos1X;
	bx[7] = cameraPos1Y - playerPos1Y;
	bx[8] = cameraPos1Z - playerPos1Z;
	bx[15] = state.playerstate.car_currpm;

	if (gameconfig.game_opponenttype) {
		/* Store camera-relative positions for opponent */
		bx[9]  = cameraPos2X - opponentPos2X;
		bx[10] = cameraPos2Y - opponentPos2Y;
		bx[11] = cameraPos2Z - opponentPos2Z;
		bx[12] = cameraPos1X - opponentPos1X;
		bx[13] = cameraPos1Y - opponentPos1Y;
		bx[14] = cameraPos1Z - opponentPos1Z;
		bx[16] = state.opponentstate.car_currpm;
		carCount = 2;
	} else {
		carCount = 1;
	}

	/* Process sound effects for each car */
	for (carIndex = 0; carIndex < carCount; carIndex++) {
		struct CARSTATE *cs;
		short audioHandle;

		if (carIndex == 0) {
			cs = &state.playerstate;
			audioHandle = crash_sound_handle;
			soundFlags = player_audio_state;
		} else {
			cs = &state.opponentstate;
			audioHandle = audio_engine_sound_handle;
			soundFlags = opponent_audio_state;
		}

		/* Crash/engine sound bit 0 */
		if (cs->car_position_initialized & 1) {
			if (!(soundFlags & 1)) {
                soundFlags |= 1;
                audio_start_engine_note(audioHandle);
			}
		} else {
			if (soundFlags & 1) {
				soundFlags &= ~1;
                audio_stop_engine_note(audioHandle);
			}
		}

        /* Skid sound bits 1-2 (derived from live physics state) */
        {
            char old_bits = soundFlags & 6;
            char new_bits = 0;
            if (cs->car_crashBmpFlag == 0 && cs->car_slidingFlag != 0 && cs->car_sumSurfAllWheels != 0) {
                if (cs->car_surfaceWhl[0] == 1 || cs->car_surfaceWhl[1] == 1 ||
                    cs->car_surfaceWhl[2] == 1 || cs->car_surfaceWhl[3] == 1) {
                    new_bits = 2;
                } else {
                    new_bits = 4;
                }
            }

            if (new_bits != 0) {
                if (old_bits != new_bits) {
                    if (old_bits == 0) {
                        if (new_bits & 2) {
                        audio_select_skid1_fx(audioHandle);
                            soundFlags += 2;
                        } else {
                        audio_select_skid2_fx(audioHandle);
                            soundFlags += 4;
                        }
                    } else {
                        if (soundFlags & 2) soundFlags -= 2;
                        if (soundFlags & 4) soundFlags -= 4;
                        audio_clear_chunk_fx(audioHandle);
                    }
                }
            } else {
                if (soundFlags & 6) {
                    if (soundFlags & 2) soundFlags -= 2;
                    if (soundFlags & 4) soundFlags -= 4;
                    audio_clear_chunk_fx(audioHandle);
                }
            }
        }

		/* Store updated flags */
		if (carIndex != 0)
			opponent_audio_state = soundFlags;
		else
			player_audio_state = soundFlags;
	}

	/* Mark audio state as valid, advance frame counter */
	audio_replay_apply_state = 1;
	menu_selection_buffer++;
	if (menu_selection_buffer == 40)
		menu_selection_buffer = 0;

end_func:
	sprite_boundary_marker = is_in_replay;
}

/* ---- detect_penalty ---- */
/* Traverses track path from the last known position, tracking how many
   off-course segments the player has traversed. Returns 1 if penalty
   detected, 0 if player is at start. Uses backtracking stack for junctions. */
/** @brief Detect penalty.
 * @param extVar2ptr Parameter value.
 * @param extVar1Eptr Parameter value.
 * @return Function return value.
 */

short detect_penalty(short* extVar2ptr, short* extVar1Eptr) {
	extern unsigned char trkObjectList[];
	char visited[904];
	short pathStack[132];
	short penaltyStack[132];
	short stackDepth;
	char tileRowMax;
	char multiTileFlags;
	char tileColMax;
	short bestWaypointIndex;
	char tileRow;
	char tileCol;
	char playerTileRow;
	short branchWaypoint;
	short bestPenalty;
	char playerTileCol;
	short nextWaypoint;
	short di;
	short si;
	short i;
	unsigned char tileElem;
	state_trackobject_raw trkObj;

	/* Get current tile from player world position */
	playerTileCol = (char)(state.playerstate.car_posWorld1.lx >> 16);
	playerTileRow = 29 - (char)(state.playerstate.car_posWorld1.lz >> 16);

	/* Check if at starting position -> no penalty */
	if ((signed char)playerTileCol == state.game_startcol ||
	    (signed char)playerTileCol == state.game_startcol2) {
		if ((signed char)playerTileRow == state.game_startrow ||
		    (signed char)playerTileRow == state.game_startrow2) {
			*extVar1Eptr = 0;
			return 0;
		}
	}

	/* Bounds check */
	if (playerTileCol < 0 || playerTileCol > 29 || playerTileRow < 0 || playerTileRow > 29) {
		*extVar1Eptr = -2;
		return 1;
	}

	/* Initialize */
	bestPenalty = 0;
	stackDepth = 0;
	di = 0;
	si = 0;
	for (i = 0; i < track_pieces_counter; i++)
		visited[i] = 0;

	si = *extVar2ptr;

loop_start:
	/* Follow path: look up next piece */
	nextWaypoint = track_waypoint_next[si];

	/* Check if already visited */
	if (visited[nextWaypoint] != 0) {
		/* Backtrack */
		if (stackDepth > 0) {
			stackDepth--;
			si = pathStack[stackDepth];
			di = penaltyStack[stackDepth];
			goto loop_start;
		}
		/* Stack empty */
		if (bestPenalty != 0) {
			*extVar2ptr = bestWaypointIndex;
			*extVar1Eptr = bestPenalty;
			return 1;
		}
		/* No penalty found, set start to current */
		state.game_startcol = (signed char)playerTileCol;
		state.game_startcol2 = (signed char)playerTileCol;
		state.game_startrow = (signed char)playerTileRow;
		state.game_startrow2 = (signed char)playerTileRow;
		*extVar1Eptr = -2;
		return 1;
	}

	/* Mark as visited */
	visited[nextWaypoint] = 1;

	/* Get tile row and col from path */
	tileRow = path_row[nextWaypoint];
	tileCol = path_col[nextWaypoint];

	/* Get tile element and multi-tile flag */
	tileElem = (unsigned char)track_elem_ordered[nextWaypoint];
	if (!state_trackobject_raw_decode((const unsigned char*)trkObjectList, (unsigned int)tileElem, &trkObj)) {
		multiTileFlags = 0;
	} else {
		multiTileFlags = (char)trkObj.multi_tile_flag;
	}

	/* Compute tile row range */
	if (multiTileFlags & 1)
		tileRowMax = tileRow + 1;
	else
		tileRowMax = tileRow;

	/* Compute tile col range */
	if (multiTileFlags & 2)
		tileColMax = tileCol + 1;
	else
		tileColMax = tileCol;

	/* Check if player is on this tile */
	if ((tileCol == playerTileCol || tileColMax == playerTileCol) &&
	    (tileRow == playerTileRow || tileRowMax == playerTileRow)) {
		/* Player is on this tile */
		if (track_waypoint_alt[si] != -1)
			nextWaypoint = si;

		/* Update game start positions */
		state.game_startcol = (signed char)tileCol;
		state.game_startcol2 = (signed char)tileColMax;
		state.game_startrow = (signed char)tileRow;
		state.game_startrow2 = (signed char)tileRowMax;

		if (di <= 0) {
			/* Player found on track with no penalty */
			*extVar2ptr = nextWaypoint;
			*extVar1Eptr = di;
			return 1;
		}
		/* Check if this is the best penalty path */
		if (bestPenalty == 0 || bestPenalty > di) {
			bestWaypointIndex = nextWaypoint;
			bestPenalty = di;
		}
	}

	/* Process path branching */
	branchWaypoint = track_waypoint_alt[si];
	if (branchWaypoint != -1) {
		penaltyStack[stackDepth] = di;
		pathStack[stackDepth] = branchWaypoint;
		stackDepth++;
	}

	/* Advance along path */
	if (nextWaypoint == 0) {
		di = -1;
	} else if (di != -1) {
		di++;
	}

	si = nextWaypoint;
	goto loop_start;
}

/* ---- car_car_coll_detect_maybe ---- */
/* OBB collision detection between two cars. Checks if any corner of one
   car's collision box falls inside the other car's box (in both directions).
   Uses 4 corners per car, transform to opponent's local space for testing. */
/** @brief Car car detect collision.
 * @param pColl Parameter value.
 * @param pWorld Parameter value.
 * @param oColl Parameter value.
 * @param oWorld Parameter value.
 * @return Function return value.
 */

short car_car_detect_collision(short* pColl, short* pWorld, short* oColl, short* oWorld) {
	struct VECTOR diff;
	struct VECTOR tmp;
	struct VECTOR corners[4];
	struct MATRIX* mat;
	short combinedRadius;
	short axisDelta;
	int i;

	combinedRadius = (short)(pColl[3] + oColl[3]);

	if (pWorld[0] < oWorld[0])
		axisDelta = (short)(oWorld[0] - pWorld[0]);
	else
		axisDelta = (short)(pWorld[0] - oWorld[0]);
	if (axisDelta > combinedRadius)
		return 0;

	if (pWorld[2] < oWorld[2])
		axisDelta = (short)(oWorld[2] - pWorld[2]);
	else
		axisDelta = (short)(pWorld[2] - oWorld[2]);
	if (axisDelta > combinedRadius)
		return 0;

	if (pWorld[1] < oWorld[1])
		axisDelta = (short)(oWorld[1] - pWorld[1]);
	else
		axisDelta = (short)(pWorld[1] - oWorld[1]);
	if (axisDelta > combinedRadius)
		return 0;

	diff.x = (short)(pWorld[0] - oWorld[0]);
	diff.y = (short)(pWorld[1] - oWorld[1]);
	diff.z = (short)(pWorld[2] - oWorld[2]);
	if ((unsigned short)polarRadius3D(&diff) > (unsigned short)combinedRadius)
		return 0;

	mat = mat_rot_zxy((short)-pWorld[3], (short)-pWorld[4], (short)-pWorld[5], 0);
	for (i = 0; i < 4; i++) {
		tmp.x = position_rotation_matrix[i] ? (short)-pColl[0] : pColl[0];
		tmp.y = 0;
		tmp.z = camera_position_vector[i] ? (short)-pColl[2] : pColl[2];
		mat_mul_vector(&tmp, mat, &diff);
		corners[i].x = (short)(diff.x + pWorld[0]);
		corners[i].y = (short)(diff.y + pWorld[1]);
		corners[i].z = (short)(diff.z + pWorld[2]);
	}

	mat = mat_rot_zxy(oWorld[3], oWorld[4], oWorld[5], 1);
	for (i = 0; i < 4; i++) {
		tmp.x = (short)(oWorld[0] - corners[i].x);
		tmp.y = (short)(oWorld[1] - corners[i].y);
		tmp.z = (short)(oWorld[2] - corners[i].z);
		mat_mul_vector(&tmp, mat, &diff);
		if (diff.y < 0)
			continue;
		if (oColl[1] < diff.y)
			continue;
		if (diff.x < (short)-oColl[0])
			continue;
		if (diff.x > oColl[0])
			continue;
		if ((short)-oColl[2] > diff.z)
			continue;
		if (oColl[2] < diff.z)
			continue;
		return 1;
	}

	mat = mat_rot_zxy((short)-oWorld[3], (short)-oWorld[4], (short)-oWorld[5], 0);
	for (i = 0; i < 4; i++) {
		tmp.x = position_rotation_matrix[i] ? (short)-oColl[0] : oColl[0];
		tmp.y = 0;
		tmp.z = camera_position_vector[i] ? (short)-oColl[2] : oColl[2];
		mat_mul_vector(&tmp, mat, &diff);
		corners[i].x = (short)(diff.x + oWorld[0]);
		corners[i].y = (short)(diff.y + oWorld[1]);
		corners[i].z = (short)(diff.z + oWorld[2]);
	}

	mat = mat_rot_zxy(pWorld[3], pWorld[4], pWorld[5], 1);
	for (i = 0; i < 4; i++) {
		tmp.x = (short)(pWorld[0] - corners[i].x);
		tmp.y = (short)(pWorld[1] - corners[i].y);
		tmp.z = (short)(pWorld[2] - corners[i].z);
		mat_mul_vector(&tmp, mat, &diff);
		if (diff.y < 0)
			continue;
		if (pColl[1] < diff.y)
			continue;
		if (diff.x < (short)-pColl[0])
			continue;
		if (diff.x > pColl[0])
			continue;
		if ((short)-pColl[2] > diff.z)
			continue;
		if (pColl[2] < diff.z)
			continue;
		return 1;
	}

	return 0;
}


/* ===== END MERGED FROM src/state.c ===== */

/* ===== BEGIN MERGED FROM src/stateply.c ===== */
#include "stunts.h"
#include "math.h"
#include <stdio.h>
#include <stdlib.h>
/* Variables moved from data_game.c (private to this translation unit) */
static short gState_oEndFrame = 0;
static short nextPosAndNormalIP = 0;
static int32_t pState_lvec1_x = 0;
static int32_t pState_lvec1_y = 0;
static int32_t pState_lvec1_z = 0;
static short pState_minusRotate_x_1 = 0;
static short pState_minusRotate_y_1 = 0;
static short pState_minusRotate_z_1 = 0;


void build_track_object(struct VECTOR* a, struct VECTOR* b);


/** @brief Get wheel rating coefficients.
 * @param wheel_index Parameter value.
 * @return Function return value.
 */
static short get_wheel_rating_coefficients(int wheel_index)
{
    int base = wheel_index * 2;
    return (short)((unsigned short)wheel_rating_coefficients[base] | ((unsigned short)wheel_rating_coefficients[base + 1] << 8));
}

/** @brief Update player state.
 * @param playerState Parameter value.
 * @param playerSimd Parameter value.
 * @param opponentState Parameter value.
 * @param opponentSimd Parameter value.
 * @param isOpponentCar Parameter value.
 */
void update_player_state(struct CARSTATE* playerState, struct SIMD* playerSimd, struct CARSTATE* opponentState, struct SIMD* opponentSimd, int isOpponentCar) {
	struct MATRIX angleZRotationMatrix;
	int speed2ScaledStep;
	struct VECTOR vec_FC;
	struct VECTOR vec_1C6;
	int wheelSteerData[4];
	struct VECTORLONG* wheelPos2Ptr;
	struct VECTORLONG* wheelPos1Ptr;
	int pState_f40_sar2;
	char hasAngleZRotation;
	int wheelYAdjust;
	struct VECTOR vec_E4;
	struct VECTORLONG vecl_1C0[4];
	struct VECTORLONG vecl_176[4];
	int wheelIndex;
	char integrationPass;
	struct VECTOR vec_182, vec_1E4, prevPosLocal, vec_1C, vec_17C, planeOriginWorld;
	struct VECTOR playerWorldPoints[2], vec_18EoStateWorldCrds[2];
	struct MATRIX mat_134;
	char wallVectorSwapped;
	int remainingSpeed, consumedSpeed, wallRelativeAngle, steerKickAngle;
	unsigned int impactSpeedThreshold;
	struct MATRIX* wallAlignMatrix;
	int si;
	int wheelPlaneDepth[4];
	struct PLANE * planePtr;
	struct VECTOR vec_1DE[4];
	int impactDepth;
	char collisionFlag;
	struct VECTOR clipVertices[32];

	/* ====== Section 1: Initial state copy ====== */
	pState_lvec1_x = playerState->car_posWorld1.lx;
	pState_lvec1_y = playerState->car_posWorld1.ly;
	pState_lvec1_z = playerState->car_posWorld1.lz;
	playerState->car_posWorld2.lx = playerState->car_posWorld1.lx;
	playerState->car_posWorld2.ly = playerState->car_posWorld1.ly;
	playerState->car_posWorld2.lz = playerState->car_posWorld1.lz;
	pState_minusRotate_z_1 = playerState->car_rotate.z;
	pState_minusRotate_z_2 = playerState->car_rotate.z;
	pState_minusRotate_x_1 = playerState->car_rotate.y;
	pState_minusRotate_x_2 = playerState->car_rotate.y;
	pState_minusRotate_y_1 = playerState->car_rotate.x;
	pState_minusRotate_y_2 = playerState->car_rotate.x;

	/* ====== Section 2: Steering and speed setup ====== */
	if (playerState->car_sumSurfAllWheels != 0) {
		pState_f40_sar2 = playerState->car_40MfrontWhlAngle >> 2;
	} else {
		pState_f40_sar2 = 0;
	}

	if (framespersec == STATECAR_FPS_10) {
		speed2ScaledStep = ((long)playerState->car_speed2 * STATECAR_SPEED_STEP_SCALE_NUM) / STATECAR_SPEED_STEP_DENOM_10FPS;
	} else if (framespersec >= STATECAR_FPS_30) {
		speed2ScaledStep = ((long)playerState->car_speed2 * STATECAR_SPEED_STEP_SCALE_NUM) / STATECAR_SPEED_STEP_DENOM_30FPS;
	} else {
		speed2ScaledStep = ((long)playerState->car_speed2 * STATECAR_SPEED_STEP_SCALE_NUM) / STATECAR_SPEED_STEP_DENOM_20FPS;
	}

	/* ====== Section 3: Rotation matrix and pseudo-gravity ====== */
	mat_car_orientation = *mat_rot_zxy(-pState_minusRotate_z_1, -pState_minusRotate_x_1, -pState_minusRotate_y_1, 0);
	if (pState_minusRotate_x_1 != 0 || pState_minusRotate_z_1 != 0) {
		vec_1C6.x = 0;
		vec_1C6.y = 0;
		vec_1C6.z = STATECAR_GRAVITY_PROBE_Z;
		mat_mul_vector(&vec_1C6, &mat_car_orientation, &vec_FC);
		playerState->car_pseudoGravity = -vec_FC.y;
	} else {
		playerState->car_pseudoGravity = 0;
	}

	/* ====== Section 4: Angle-Z rotation matrix ====== */
	if ((playerState->car_angle_z & STATECAR_ANGLE_MASK) != 0) {
		hasAngleZRotation = 1;
		angleZRotationMatrix = *mat_rot_zxy(0, 0, -playerState->car_angle_z, 0);
	} else {
		hasAngleZRotation = 0;
	}

	/* ====== Section 5: Compute wheelYAdjust ====== */
	vec_1C6.x = 0;
	vec_1C6.y = STATECAR_TILT_PROBE_Y;
	vec_1C6.z = 0;
	mat_mul_vector(&vec_1C6, &mat_car_orientation, &vec_FC);

	if (playerState->car_sumSurfAllWheels != 0 && vec_FC.y < 0) {
		if (playerState->car_speed2 > STATECAR_SPEED_STEP_DENOM_10FPS) {
			wheelYAdjust = STATECAR_WHEEL_TILT_ADJUST;
			vec_1C6.y = -STATECAR_WHEEL_TILT_ADJUST;
			mat_mul_vector(&vec_1C6, &mat_car_orientation, &vec_E4);
		} else {
			wheelYAdjust = -STATECAR_WHEEL_TILT_ADJUST;
		}
	} else {
		wheelYAdjust = 0;
	}

	/* ====== Section 6: Compute initial wheel world positions ====== */
	vec_movement_local.x = 0;
	vec_movement_local.y = 0;
	planindex_copy = -1;
	wheelPos2Ptr = vecl_1C0;
	wheelPos1Ptr = vecl_176;

	for (wheelIndex = 0; wheelIndex < 4; wheelIndex++, wheelPos2Ptr++, wheelPos1Ptr++) {
		vec_1C6 = playerSimd->wheel_coords[wheelIndex];
		vec_1C6.y = -(playerState->car_rc2[wheelIndex] + STATECAR_WHEEL_BASE_HEIGHT);
		if (wheelYAdjust < 0)
			vec_1C6.y -= wheelYAdjust;

		if (hasAngleZRotation != 0) {
			mat_mul_vector(&vec_1C6, &angleZRotationMatrix, &vec_FC);
			vec_1C6 = vec_FC;
		}

		mat_mul_vector(&vec_1C6, &mat_car_orientation, &vec_FC);
		wheelPos2Ptr->lx = vec_FC.x + pState_lvec1_x;
		wheelPos2Ptr->ly = vec_FC.y + pState_lvec1_y;
		wheelPos2Ptr->lz = vec_FC.z + pState_lvec1_z;

		wheelPos1Ptr->lx = wheelPos2Ptr->lx;
		wheelPos1Ptr->ly = wheelPos2Ptr->ly;
		wheelPos1Ptr->lz = wheelPos2Ptr->lz;

		if (speed2ScaledStep != 0) {
			vec_movement_local.z = speed2ScaledStep;
			if (pState_f40_sar2 != 0 && wheelIndex < 2)
				pState_f36Mminf40sar2 = playerState->car_36MwhlAngle - pState_f40_sar2;
			else
				pState_f36Mminf40sar2 = playerState->car_36MwhlAngle;

			wheelSteerData[wheelIndex] = pState_f36Mminf40sar2;
			plane_apply_rotation_matrix();
			wheelPos2Ptr->lx += vec_planerotopresult.x;
			wheelPos2Ptr->ly += vec_planerotopresult.y;
			wheelPos2Ptr->lz += vec_planerotopresult.z;
		}
	}

	/* ====== Section 7: Integration passes (max 5, wall collision restarts) ====== */
	for (integrationPass = 1; ; integrationPass++) {
		int wallHit;

		if (integrationPass == STATECAR_MAX_INTEGRATION_PASSES) {
			playerState->car_36MwhlAngle = STATECAR_ANGLE_HALF;
			update_crash_state(1, isOpponentCar);
			break;
		}

		wallHit = 0;
		wheelPos2Ptr = vecl_1C0;
		wheelPos1Ptr = vecl_176;

		for (wheelIndex = 0; wheelIndex < 4; wheelIndex++, wheelPos2Ptr++, wheelPos1Ptr++) {
			int doRc1Check;
			int depthPass;

			/* -- Compute world position for this wheel -- */
			vec_1C6.x = wheelPos2Ptr->lx >> 6;
			vec_1C6.y = wheelPos2Ptr->ly >> 6;
			vec_1C6.z = wheelPos2Ptr->lz >> 6;

			if (state.game_inputmode == 2) {
				wallindex = -1;
				current_surf_type = 1;
				planindex = 0;
				current_planptr = planptr;
			} else {
				build_track_object(&vec_1C6, &playerState->car_whlWorldCrds1[wheelIndex]);
			}

			/* -- Surface type and collision point -- */
			playerState->car_surfaceWhl[wheelIndex] = current_surf_type;
			vec_1C6.x = wheelPos2Ptr->lx >> 6;
			vec_1C6.y = wheelPos2Ptr->ly >> 6;
			vec_1C6.z = wheelPos2Ptr->lz >> 6;

			if (state.game_inputmode == 2)
				nextPosAndNormalIP = vec_1C6.y;
			else
				nextPosAndNormalIP = plane_get_collision_point(planindex, vec_1C6.x, vec_1C6.y, vec_1C6.z);

			/* -- Wall collision check -- */
			if (wallindex != -1
				&& nextPosAndNormalIP > elRdWallRelated
				&& nextPosAndNormalIP < wallHeight)
			{
				/* Compute wall-relative vectors for old and new positions */
				vec_182.x = playerState->car_whlWorldCrds1[wheelIndex].x - wallStartX;
				vec_182.y = 0;
				vec_182.z = playerState->car_whlWorldCrds1[wheelIndex].z - wallStartZ;
				vec_1E4.x = (wheelPos2Ptr->lx >> 6) - wallStartX;
				vec_1E4.y = 0;
				vec_1E4.z = (wheelPos2Ptr->lz >> 6) - wallStartZ;

				mat_rot_y(&mat_134, -wallOrientation - STATECAR_ANGLE_QUARTER);
				mat_mul_vector(&vec_182, &mat_134, &prevPosLocal);
				mat_mul_vector(&vec_1E4, &mat_134, &vec_1C);

				/* Check for wall crossing (vectors on opposite sides) */
				if (!(vec_1C.z > 0 && prevPosLocal.z > 0) && !(vec_1C.z < 0 && prevPosLocal.z < 0)) {
					/* Sort so vec_1C.z <= prevPosLocal.z */
					if (vec_1C.z > prevPosLocal.z) {
						wallVectorSwapped = 1;
						vec_FC = vec_1C;
						vec_1C = prevPosLocal;
						prevPosLocal = vec_FC;
					} else {
						wallVectorSwapped = 0;
					}

					/* Compute consumed/remaining speed at crossing point */
					if (vec_1C.z == 0) {
						remainingSpeed = speed2ScaledStep;
						consumedSpeed = 0;
					} else if (prevPosLocal.z == 0) {
						remainingSpeed = 0;
						consumedSpeed = speed2ScaledStep;
					} else {
						vector_lerp_at_z(&vec_1C, &prevPosLocal, &vec_FC, 0);
						vec_17C.x = (vec_1C.x - vec_FC.x) << 6;
						vec_17C.y = (vec_1C.y - vec_FC.y) << 6;
						vec_17C.z = (vec_1C.z - vec_FC.z) << 6;
						consumedSpeed = polarRadius3D(&vec_17C);
						remainingSpeed = speed2ScaledStep - consumedSpeed;
					}

					/* Wall response direction */
					wallRelativeAngle = (-pState_minusRotate_y_1 - wallOrientation) & STATECAR_ANGLE_MASK;
					vec_FC.z = consumedSpeed;
					vec_FC.y = 0;
					if (wallRelativeAngle < STATECAR_ANGLE_QUARTER || wallRelativeAngle > STATECAR_ANGLE_THREE_QUARTER) {
						wallRelativeAngle = wallOrientation;
						vec_FC.x = STATECAR_WALL_DEFLECT_X;
					} else {
						wallRelativeAngle = (wallOrientation + STATECAR_ANGLE_HALF) & STATECAR_ANGLE_MASK;
						vec_FC.x = -STATECAR_WALL_DEFLECT_X;
					}
					if (wallVectorSwapped != 0)
						vec_FC.x = -vec_FC.x;

					/* Impact check and crash */
					wallAlignMatrix = mat_rot_zxy(-pState_minusRotate_z_1, -pState_minusRotate_x_1, wallRelativeAngle, 0);
					mat_mul_vector(&vec_FC, wallAlignMatrix, &vec_1C);
					si = (-pState_minusRotate_y_1 - wallRelativeAngle) & STATECAR_ANGLE_MASK;
					steerKickAngle = 0;
					if (si > STATECAR_ANGLE_QUARTER) {
						si = STATECAR_ANGLE_FULL - si;
						steerKickAngle = 1;
					}

					impactSpeedThreshold = -((si * STATECAR_WALL_IMPACT_COEFF >> 8) - STATECAR_WALL_IMPACT_BASE) << 8;
					if (playerState->car_speed2 > impactSpeedThreshold) {
						if (steerKickAngle != 0)
							steerKickAngle = -si << 1;
						else
							steerKickAngle = si << 1;
						playerState->car_36MwhlAngle = steerKickAngle;
						update_crash_state(1, isOpponentCar);
					}

					/* Adjust all 4 wheel positions for wall bounce */
					playerState->car_position_initialized |= 16;
				{
						struct VECTORLONG* wp2 = vecl_1C0;
						struct VECTORLONG* wp1 = vecl_176;
						for (si = 0; si < 4; si++, wp2++, wp1++) {
							if (remainingSpeed == 0 || speed2ScaledStep == 0) {
								prevPosLocal.x = 0;
								prevPosLocal.y = 0;
								prevPosLocal.z = 0;
							} else {
								prevPosLocal.x = ((wp2->lx - wp1->lx) * remainingSpeed) / speed2ScaledStep;
								prevPosLocal.y = ((wp2->ly - wp1->ly) * remainingSpeed) / speed2ScaledStep;
								prevPosLocal.z = ((wp2->lz - wp1->lz) * remainingSpeed) / speed2ScaledStep;
							}
							wp2->lx = prevPosLocal.x + vec_1C.x + wp1->lx;
							wp2->ly = prevPosLocal.y + vec_1C.y + wp1->ly;
							wp2->lz = prevPosLocal.z + vec_1C.z + wp1->lz;
						}
					}
					wallHit = 1;
					break; /* restart integration pass */
				}
				/* No crossing: fall through to terrain handling */
			}

			/* -- Terrain following + plane collision (max 2 depth passes) -- */
			doRc1Check = 1;
			for (depthPass = 0; depthPass < 2; depthPass++) {
				/* Terrain following */
				if (nextPosAndNormalIP > 0) {
					if (wheelYAdjust > 0 && nextPosAndNormalIP < STATECAR_TERRAIN_LIFT_THRESH) {
						/* High-speed lift correction */
						wheelPos2Ptr->lx += vec_E4.x;
						wheelPos2Ptr->ly += vec_E4.y;
						wheelPos2Ptr->lz += vec_E4.z;
					} else {
						/* Standard terrain following (gravity pull) */
						playerState->car_rc1[wheelIndex] += get_wheel_rating_coefficients(wheelIndex);
						wheelPos2Ptr->ly -= playerState->car_rc1[wheelIndex];
						if (framespersec == STATECAR_FPS_10) {
							playerState->car_rc1[wheelIndex] += get_wheel_rating_coefficients(wheelIndex);
							wheelPos2Ptr->ly -= playerState->car_rc1[wheelIndex];
						}

						vec_1C6.y = wheelPos2Ptr->ly >> 6;
						if (state.game_inputmode == 2)
							nextPosAndNormalIP = vec_1C6.y;
						else
							nextPosAndNormalIP = plane_get_collision_point(planindex, vec_1C6.x, vec_1C6.y, vec_1C6.z);

						if (nextPosAndNormalIP > STATECAR_TERRAIN_SURFACE_THRESH)
							playerState->car_surfaceWhl[wheelIndex] = 0;
					}
				}

				/* Store wheel depth */
				wheelPlaneDepth[wheelIndex] = nextPosAndNormalIP;

				if (nextPosAndNormalIP == 0) {
					break; /* on surface → rc1 checks */
				}
				if (nextPosAndNormalIP > 0) {
					doRc1Check = 0; /* above surface → skip rc1 */
					break;
				}

				/* -- Plane collision (depth < 0) -- */
				planePtr = &planptr[planindex];
				planeOriginWorld.x = planePtr->plane_origin.x + elem_xCenter;
				planeOriginWorld.y = planePtr->plane_origin.y + terrainHeight;
				planeOriginWorld.z = planePtr->plane_origin.z + elem_zCenter;

				vec_182.x = (wheelPos1Ptr->lx >> 6) - planeOriginWorld.x;
				vec_182.y = (wheelPos1Ptr->ly >> 6) - planeOriginWorld.y;
				vec_182.z = (wheelPos1Ptr->lz >> 6) - planeOriginWorld.z;

				vec_1E4.x = (wheelPos2Ptr->lx >> 6) - planeOriginWorld.x;
				vec_1E4.y = (wheelPos2Ptr->ly >> 6) - planeOriginWorld.y;
				vec_1E4.z = (wheelPos2Ptr->lz >> 6) - planeOriginWorld.z;

				mat_134 = planePtr->plane_rotation;
				mat_invert(&mat_134, &angleZRotationMatrix);
				mat_mul_vector(&vec_182, &angleZRotationMatrix, &prevPosLocal);
				mat_mul_vector(&vec_1E4, &angleZRotationMatrix, &vec_1C);

				wallVectorSwapped = 0;

				/* Check for deep penetration (falling through floor) */
				if (track_object_render_enabled == 0 && prevPosLocal.y < STATECAR_PLANE_WARN_DEPTH && vec_1C.y < STATECAR_PLANE_WARN_DEPTH) {
					if (vec_1C.y <= STATECAR_PLANE_DEEP_DEPTH) {
						/* Reset to ground plane and re-enter terrain following */
						planindex = 0;
						current_planptr = planptr;
						track_object_render_enabled = 1;
						vec_1C6.x = wheelPos2Ptr->lx >> 6;
						vec_1C6.y = wheelPos2Ptr->ly >> 6;
						vec_1C6.z = wheelPos2Ptr->lz >> 6;
						nextPosAndNormalIP = plane_get_collision_point(0, vec_1C6.x, vec_1C6.y, vec_1C6.z);
						continue; /* second depth pass */
					}
					update_crash_state(5, isOpponentCar);
					wallVectorSwapped = 1;
				}

				/* Normal plane collision handling */
				if (vec_1C.y == 0) {
					/* Exactly on surface - small correction */
					vec_movement_local.x = 0;
					vec_movement_local.y = 0;
					vec_movement_local.z = STATECAR_PLANE_SURFACE_STEP;
					planindex_copy = planindex;
					pState_f36Mminf40sar2 = wheelSteerData[wheelIndex];
					plane_apply_rotation_matrix();
					wheelPos2Ptr->lx -= vec_planerotopresult.x;
					wheelPos2Ptr->ly -= vec_planerotopresult.y;
					wheelPos2Ptr->lz -= vec_planerotopresult.z;
				} else {
					if (prevPosLocal.y <= 0 || vec_1C.y >= 0) {
						/* Surface follow: project onto plane */
						vec_movement_local.x = 0;
						vec_movement_local.y = 0;
						vec_movement_local.z = speed2ScaledStep;
						planindex_copy = planindex;
						pState_f36Mminf40sar2 = wheelSteerData[wheelIndex];
						plane_apply_rotation_matrix();
						wheelPos2Ptr->lx = wheelPos1Ptr->lx + vec_planerotopresult.x;
						wheelPos2Ptr->ly = wheelPos1Ptr->ly + vec_planerotopresult.y;
						wheelPos2Ptr->lz = wheelPos1Ptr->lz + vec_planerotopresult.z;
					} else {
						/* Surface crossing: interpolate to crossing point */
						wallRelativeAngle = prevPosLocal.z;
						prevPosLocal.z = -prevPosLocal.y;
						prevPosLocal.y = wallRelativeAngle;

						wallRelativeAngle = vec_1C.z;
						vec_1C.z = -vec_1C.y;
						vec_1C.y = wallRelativeAngle;

						vector_lerp_at_z(&vec_1C, &prevPosLocal, &vec_FC, 0);
						vec_17C.x = (vec_1C.x - vec_FC.x) << 6;
						vec_17C.y = (vec_1C.y - vec_FC.y) << 6;
						vec_17C.z = (vec_1C.z - vec_FC.z) << 6;

						wallRelativeAngle = polarRadius3D(&vec_17C);
						remainingSpeed = playerState->car_rc1[wheelIndex] + speed2ScaledStep;
						consumedSpeed = remainingSpeed - wallRelativeAngle;
						prevPosLocal.x = ((wheelPos2Ptr->lx - wheelPos1Ptr->lx) * consumedSpeed) / remainingSpeed;
						prevPosLocal.y = ((wheelPos2Ptr->ly - wheelPos1Ptr->ly) * consumedSpeed) / remainingSpeed;
						prevPosLocal.z = ((wheelPos2Ptr->lz - wheelPos1Ptr->lz) * consumedSpeed) / remainingSpeed;

						vec_movement_local.x = 0;
						vec_movement_local.y = 0;
						vec_movement_local.z = wallRelativeAngle;
						planindex_copy = planindex;
						pState_f36Mminf40sar2 = wheelSteerData[wheelIndex];
						plane_apply_rotation_matrix();
						wheelPos2Ptr->lx = wheelPos1Ptr->lx + prevPosLocal.x + vec_planerotopresult.x;
						wheelPos2Ptr->ly = wheelPos1Ptr->ly + prevPosLocal.y + vec_planerotopresult.y;
						wheelPos2Ptr->lz = wheelPos1Ptr->lz + prevPosLocal.z + vec_planerotopresult.z;
					}

					/* Recheck collision after plane adjustment */
					vec_1C6.x = wheelPos2Ptr->lx >> 6;
					vec_1C6.y = wheelPos2Ptr->ly >> 6;
					vec_1C6.z = wheelPos2Ptr->lz >> 6;
					nextPosAndNormalIP = plane_get_collision_point(planindex, vec_1C6.x, vec_1C6.y, vec_1C6.z);

					if (nextPosAndNormalIP < 0) {
						if (wallVectorSwapped != 0)
							nextPosAndNormalIP = (-nextPosAndNormalIP) + STATECAR_PLANE_PUSH_OFFSET;
						/* Push wheel away from plane surface */
						vec_1C6.z = 0;
						vec_1C6.x = 0;
						vec_1C6.y = (-nextPosAndNormalIP) << 6;
						mat_mul_vector2(&vec_1C6, &planptr[planindex].plane_rotation, &vec_FC);
						wheelPos2Ptr->lx += vec_FC.x;
						wheelPos2Ptr->ly += vec_FC.y;
						wheelPos2Ptr->lz += vec_FC.z;
					}
				}
				break; /* done with plane collision */
			} /* end depth pass loop */

			/* RC1 / suspension limit checks */
			if (doRc1Check) {
				if (playerState->car_rc1[wheelIndex] > STATECAR_SUSPENSION_EXT_WARN) {
					playerState->car_position_initialized |= 32;
			}
				if (playerState->car_rc1[wheelIndex] > STATECAR_SUSPENSION_EXT_CRASH)
					update_crash_state(1, isOpponentCar);
				playerState->car_rc1[wheelIndex] = 0;
			}
		} /* end per-wheel loop */

		if (!wallHit)
			break; /* normal completion, proceed to final update */
	} /* end integration pass loop */

	/* ====== Section 8: All-airborne check ====== */
	if (playerState->car_surfaceWhl[0] == STATECAR_SURFACE_AIRBORNE
		&& playerState->car_surfaceWhl[1] == STATECAR_SURFACE_AIRBORNE
		&& playerState->car_surfaceWhl[2] == STATECAR_SURFACE_AIRBORNE
		&& playerState->car_surfaceWhl[3] == STATECAR_SURFACE_AIRBORNE)
	{
		update_crash_state(2, isOpponentCar);
	}

	/* ====== Section 9: Final wheel suspension update ====== */
	wheelPos2Ptr = vecl_1C0;
	for (wheelIndex = 0; wheelIndex < 4; wheelIndex++, wheelPos2Ptr++) {
		playerState->car_whlWorldCrds1[wheelIndex].x = wheelPos2Ptr->lx >> 6;
		playerState->car_whlWorldCrds1[wheelIndex].y = wheelPos2Ptr->ly >> 6;
		playerState->car_whlWorldCrds1[wheelIndex].z = wheelPos2Ptr->lz >> 6;

		wallRelativeAngle = carState_update_wheel_suspension(playerState, wheelPlaneDepth[wheelIndex], wheelIndex);

		if (pState_minusRotate_z_1 == 0 && pState_minusRotate_x_1 == 0) {
			/* No rotation - simple Y adjustment */
			wheelPos2Ptr->ly += wallRelativeAngle + STATECAR_WHEEL_BASE_HEIGHT;
		} else {
			/* Rotated adjustment */
			vec_1C6.z = 0;
			vec_1C6.x = 0;
			vec_1C6.y = wallRelativeAngle + STATECAR_WHEEL_BASE_HEIGHT;
			mat_mul_vector(&vec_1C6, &mat_car_orientation, &vec_182);
			wheelPos2Ptr->lx += vec_182.x;
			wheelPos2Ptr->ly += vec_182.y;
			wheelPos2Ptr->lz += vec_182.z;
		}
	}

	/* ====== Section 10: Global position from wheel average ====== */
	pState_lvec1_x = (vecl_1C0[0].lx + vecl_1C0[1].lx + vecl_1C0[2].lx + vecl_1C0[3].lx) >> 2;
	pState_lvec1_y = (vecl_1C0[0].ly + vecl_1C0[1].ly + vecl_1C0[2].ly + vecl_1C0[3].ly) >> 2;
	pState_lvec1_z = (vecl_1C0[0].lz + vecl_1C0[1].lz + vecl_1C0[2].lz + vecl_1C0[3].lz) >> 2;

	/* Compute per-wheel offsets from center */
	wheelPos2Ptr = vecl_1C0;
	for (wheelIndex = 0; wheelIndex < 4; wheelIndex++, wheelPos2Ptr++) {
		vec_1DE[wheelIndex].x = wheelPos2Ptr->lx - pState_lvec1_x;
		vec_1DE[wheelIndex].y = wheelPos2Ptr->ly - pState_lvec1_y;
		vec_1DE[wheelIndex].z = wheelPos2Ptr->lz - pState_lvec1_z;
	}

	/* Clamp Y to positive */
	if (pState_lvec1_y < 0)
		pState_lvec1_y = 0;

	/* ====== Section 11: Clamp position to track bounds ====== */
	if (pState_lvec1_x > STATECAR_TRACK_BOUND_MAX)
		pState_lvec1_x = STATECAR_TRACK_BOUND_MAX_CLAMP;
	else if (pState_lvec1_x < STATECAR_TRACK_BOUND_MIN)
		pState_lvec1_x = STATECAR_TRACK_BOUND_MIN;

	if (pState_lvec1_z > STATECAR_TRACK_BOUND_MAX)
		pState_lvec1_z = STATECAR_TRACK_BOUND_MAX_CLAMP;
	else if (pState_lvec1_z < STATECAR_TRACK_BOUND_MIN)
		pState_lvec1_z = STATECAR_TRACK_BOUND_MIN;

	/* ====== Section 12: Y-rotation (heading) extraction ====== */
	wallRelativeAngle = vec_1DE[3].x + vec_1DE[2].x - vec_1DE[0].x - vec_1DE[1].x;
	consumedSpeed = vec_1DE[3].z + vec_1DE[2].z - vec_1DE[0].z - vec_1DE[1].z;
	pState_minusRotate_y_1 = polarAngle(wallRelativeAngle, -consumedSpeed) & 1023;
	mat_rot_y(&angleZRotationMatrix, pState_minusRotate_y_1);

	for (wheelIndex = 0; wheelIndex < 4; wheelIndex++) {
		vec_FC = vec_1DE[wheelIndex];
		mat_mul_vector(&vec_FC, &angleZRotationMatrix, &vec_1DE[wheelIndex]);
	}

	/* ====== Section 13: X-rotation (pitch) extraction ====== */
	consumedSpeed = vec_1DE[3].z + vec_1DE[2].z - vec_1DE[0].z - vec_1DE[1].z;
	remainingSpeed = vec_1DE[3].y + vec_1DE[2].y - vec_1DE[0].y - vec_1DE[1].y;

	if (remainingSpeed == 0 && consumedSpeed < 0) {
		pState_minusRotate_x_1 = 0;
	} else {
		pState_minusRotate_x_1 = polarAngle(-consumedSpeed, remainingSpeed) - 256;
		if (pState_minusRotate_x_1 >= 0) {
			if (pState_minusRotate_x_1 < 2)
				pState_minusRotate_x_1 = 0;
		} else {
			if (-pState_minusRotate_x_1 < 2)
				pState_minusRotate_x_1 = 0;
		}
	}

	if (pState_minusRotate_x_1 != 0) {
		mat_rot_x(&angleZRotationMatrix, pState_minusRotate_x_1);
		for (wheelIndex = 0; wheelIndex < 4; wheelIndex++) {
			vec_FC = vec_1DE[wheelIndex];
			mat_mul_vector(&vec_FC, &angleZRotationMatrix, &vec_1DE[wheelIndex]);
		}
	}

	/* ====== Section 14: Z-rotation (roll) extraction ====== */
	consumedSpeed = vec_1DE[1].x + vec_1DE[2].x - vec_1DE[0].x - vec_1DE[3].x;
	remainingSpeed = vec_1DE[1].y + vec_1DE[2].y - vec_1DE[0].y - vec_1DE[3].y;

	if (remainingSpeed == 0 && consumedSpeed > 0) {
		pState_minusRotate_z_1 = 0;
	} else {
		pState_minusRotate_z_1 = polarAngle(consumedSpeed, remainingSpeed) - 256;
		if (pState_minusRotate_z_1 >= 0) {
			if (pState_minusRotate_z_1 < 2)
				pState_minusRotate_z_1 = 0;
		} else {
			if (-pState_minusRotate_z_1 < 2)
				pState_minusRotate_z_1 = 0;
		}
	}

	/* ====== Section 15: Surface sums ====== */
	playerState->car_sumSurfFrontWheels = playerState->car_surfaceWhl[0] + playerState->car_surfaceWhl[1];
	playerState->car_sumSurfRearWheels = playerState->car_surfaceWhl[2] + playerState->car_surfaceWhl[3];

	if (state.game_inputmode != 2) {
		/* ====== Section 16: Audio crash flags ====== */
		if (is_in_replay == 0) {
			if (isOpponentCar == 0)
				audio_apply_crash_flags(playerState->car_position_initialized, crash_sound_handle);
			else
				audio_apply_crash_flags(playerState->car_position_initialized, audio_engine_sound_handle);
		}

		/* ====== Section 17: Secondary collision check loop ====== */
		wallAlignMatrix = mat_rot_zxy(-pState_minusRotate_z_1, -pState_minusRotate_x_1, -pState_minusRotate_y_1, 0);

		for (wheelIndex = 0; wheelIndex < 4; wheelIndex++) {
			vec_1C6 = playerSimd->wheel_coords[wheelIndex];
			vec_1C6.y = playerSimd->collide_points[0].py << 6;
			mat_mul_vector(&vec_1C6, wallAlignMatrix, &vec_FC);

			vec_1C6.x = (vec_FC.x + pState_lvec1_x) >> 6;
			vec_1C6.y = (vec_FC.y + pState_lvec1_y) >> 6;
			vec_1C6.z = (vec_FC.z + pState_lvec1_z) >> 6;

			vec_17C = vec_1C6;
			build_track_object(&vec_1C6, &playerState->car_whlWorldCrds2[wheelIndex]);
			si = plane_get_collision_point(planindex, vec_1C6.x, vec_1C6.y, vec_1C6.z);

			if (planindex < 4) {
				/* Simple terrain - crash if below or at surface */
				if (si <= 0)
					update_crash_state(5, isOpponentCar);
			} else {
				/* Complex terrain - check for surface crossing with old position */
				impactDepth = planindex;
				vec_1C6 = playerState->car_whlWorldCrds2[wheelIndex];
				build_track_object(&vec_1C6, &vec_17C);
				if (impactDepth == planindex) {
					steerKickAngle = plane_get_collision_point(planindex, vec_1C6.x, vec_1C6.y, vec_1C6.z);
					if (game_replay_mode != 1) {
						if ((si < 0 && steerKickAngle > 0) || (si > 0 && steerKickAngle < 0))
							update_crash_state(5, isOpponentCar);
					}
				}
			}

			playerState->car_whlWorldCrds2[wheelIndex] = vec_17C;
		}

		/* ====== Section 18: Jump count and surface flag ====== */
		collisionFlag = playerState->car_sumSurfFrontWheels + playerState->car_sumSurfRearWheels;
		if (isOpponentCar == 0 && collisionFlag == 0 && playerState->car_sumSurfAllWheels != 0)
			state.game_jumpCount++;

		playerState->car_sumSurfAllWheels = collisionFlag;

		/* ====== Section 19: Set up player world points for collision ====== */
		playerWorldPoints[0].x = pState_lvec1_x >> 6;
		playerWorldPoints[0].y = pState_lvec1_y >> 6;
		playerWorldPoints[0].z = pState_lvec1_z >> 6;

		playerWorldPoints[1].x = pState_minusRotate_z_1;
		playerWorldPoints[1].y = pState_minusRotate_x_1;
		playerWorldPoints[1].z = pState_minusRotate_y_1;

		/* ====== Section 20: Car-to-car collision ====== */
		if (gameconfig.game_opponenttype != 0) {
			vec_18EoStateWorldCrds[0].x = opponentState->car_posWorld1.lx >> 6;
			vec_18EoStateWorldCrds[0].y = opponentState->car_posWorld1.ly >> 6;
			vec_18EoStateWorldCrds[0].z = opponentState->car_posWorld1.lz >> 6;

			vec_18EoStateWorldCrds[1].x = opponentState->car_rotate.z;
			vec_18EoStateWorldCrds[1].y = opponentState->car_rotate.y;
			vec_18EoStateWorldCrds[1].z = opponentState->car_rotate.x;

			if (car_car_detect_collision((short *)playerSimd->collide_points, (short *)playerWorldPoints,
				(short *)opponentSimd->collide_points, (short *)vec_18EoStateWorldCrds) != 0)
			{
				if (playerState->car_crash_impact_flag != 0)
					return; /* already in crash, skip state writeback */

				if (car_car_speed_adjust_collision(playerState, opponentState) == 0)
					return; /* no speed adjustment needed, skip state writeback */

				/* Both cars crash */
				update_crash_state(1, isOpponentCar);
				update_crash_state(1, isOpponentCar ^ 1);
				return;
			}
		}

		/* ====== Section 21: World object collision checks ====== */
		vec_FC.x = playerWorldPoints[0].x >> 10;
		vec_FC.z = -((playerWorldPoints[0].z >> 10) - 29);
		vec_18EoStateWorldCrds[1].x = 0;
		vec_18EoStateWorldCrds[1].y = 0;
		vec_18EoStateWorldCrds[1].z = 0;

		if (vec_FC.x >= 0 && vec_FC.x < 30 && vec_FC.z >= 0 && vec_FC.z < 30) {
			/* Road sign / tree collision */
			hasAngleZRotation = bto_auxiliary1(vec_FC.x, vec_FC.z, clipVertices);
			for (si = 0; si < hasAngleZRotation; si++) {
				vec_18EoStateWorldCrds[0].x = clipVertices[si].x;
				vec_18EoStateWorldCrds[0].y = clipVertices[si].y;
				vec_18EoStateWorldCrds[0].z = clipVertices[si].z;
				if (car_car_detect_collision((short *)playerSimd->collide_points, (short *)playerWorldPoints,
					(short *)roadside_pole_collision_box, (short *)vec_18EoStateWorldCrds) != 0)
				{
					playerState->car_36MwhlAngle -= 512;
					update_crash_state(1, isOpponentCar);
					return; /* crash with pole/sign */
				}
			}

			/* Breakable obstacle collision */
			si = (char)tile_obstacle_map[trackrows[vec_FC.z] + vec_FC.x];
			if (si != -1 && state.game_obstacle_flags[si] == 0) {
				vec_18EoStateWorldCrds[0].x = obstacle_world_pos[si * 3 + 0];
				vec_18EoStateWorldCrds[0].y = obstacle_world_pos[si * 3 + 1];
				vec_18EoStateWorldCrds[0].z = obstacle_world_pos[si * 3 + 2];
				if (car_car_detect_collision((short *)playerSimd->collide_points, (short *)playerWorldPoints,
					(short *)breakable_obstacle_collision_box, (short *)vec_18EoStateWorldCrds) != 0)
				{
					state.game_obstacle_flags[si] = 1;
					state_spawn_debris_particles(si + 2, -playerState->car_rotate.x,
						((long)playerState->car_speed2 * 1408) / 15360);
				}
			}

			/* Start/finish pole collision */
			if (vec_FC.x == startcol2 && vec_FC.z == startrow2) {
				vec_18EoStateWorldCrds[0].x = trackcenterpos2[startcol2] + multiply_and_scale(sin_fast(track_angle + 256), 126);
				vec_18EoStateWorldCrds[0].y = hillHeightConsts[hillFlag];
				vec_18EoStateWorldCrds[0].z = trackcenterpos[startrow2] + multiply_and_scale(cos_fast(track_angle + 256), 126);

				steerKickAngle = car_car_detect_collision((short *)playerSimd->collide_points, (short *)playerWorldPoints,
					(short *)finish_pole_collision_box, (short *)vec_18EoStateWorldCrds);

				if (steerKickAngle == 0) {
					vec_18EoStateWorldCrds[0].x = trackcenterpos2[startcol2] + multiply_and_scale(sin_fast(track_angle + 768), 126);
					vec_18EoStateWorldCrds[0].z = trackcenterpos[startrow2] + multiply_and_scale(cos_fast(track_angle + 768), 126);

					steerKickAngle = car_car_detect_collision((short *)playerSimd->collide_points, (short *)playerWorldPoints,
						(short *)finish_pole_collision_box, (short *)vec_18EoStateWorldCrds);
				}

				if (steerKickAngle != 0) {
					update_crash_state(1, isOpponentCar);
					return; /* crash with start/finish pole */
				}
			}
		}
	} /* end if (game_inputmode != 2) */

	/* ====== Section 22: Write back state ====== */
	playerState->car_posWorld1.lx = pState_lvec1_x;
	playerState->car_posWorld1.ly = pState_lvec1_y;
	playerState->car_posWorld1.lz = pState_lvec1_z;
	playerState->car_rotate.z = pState_minusRotate_z_1;
	playerState->car_rotate.y = pState_minusRotate_x_1;
	playerState->car_rotate.x = pState_minusRotate_y_1;
	playerState->car_crash_impact_flag = 0;
}

/** @brief Apply crash-effect flags to the selected car audio channel.
 * @param flags Crash/impact flags generated by car physics.
 * @param sound_id Target sound channel identifier.
 */
void audio_apply_crash_flags(unsigned char flags, unsigned short sound_id) {
    if (!audio_replay_apply_state) {
        return;
    }

    if (flags & 16) {
        audio_select_crash2_fx(sound_id);
    }

    if (flags & 32) {
        audio_select_crash1_fx(sound_id);
    }
}

/** @brief Push replay-frame engine sound parameters to active audio channels.
 * @param info Replay audio state buffer for the current frame.
 * @param sound_id Target sound channel identifier.
 */
void audio_replay_update_engine_sounds(unsigned short* info, unsigned short sound_id) {

    audio_update_engine_sound(
        crash_sound_handle,
        info[15],
        info[3],
        info[4],
        info[5],
        info[6],
        info[7],
        info[8],
        sound_id);

    if (gameconfig.game_opponenttype != 0) {
        audio_update_engine_sound(
            audio_engine_sound_handle,
            info[16],
            info[9],
            info[10],
            info[11],
            info[12],
            info[13],
            info[14],
            sound_id);
    }
}

// previously set_AV_event_triggers
/** @brief Update crash state.
 * @param crash_type Parameter value.
 * @param isOpponentCar Parameter value.
 */
void update_crash_state(int crash_type, int isOpponentCar) {

	char stopVehicleOnCrash;
	struct CARSTATE *crashCarState;

	/* Select target car: opponent for isOpponentCar==1, player for all other values. */
	if (isOpponentCar == 1)
		crashCarState = &state.opponentstate;
	else
		crashCarState = &state.playerstate;

	/* Guard: already in a crash state — nothing to do. */
	if (crashCarState->car_crashBmpFlag != 0)
		return;

	stopVehicleOnCrash = 0;

	/* crash_type==5 is an alias for a stopping hard-crash (same as 1 but also
	 * zeroes speed). Normalise it here so the branches below only need to handle 1. */
	if (crash_type == 5) {
		crash_type      = 1;
		stopVehicleOnCrash = 1;
	}

	if (crash_type == 1) {
		/* Hard crash: set flag, spawn debris, optionally play crash sound. */
		crashCarState->car_crashBmpFlag = 1;
		state_spawn_debris_particles(isOpponentCar, crashCarState->car_rotate.x, 0);
		if (isOpponentCar == 0) {
			state.game_impactSpeed    = crashCarState->car_speed2;
			state.game_frames_per_sec = framespersec << 2;
		}
		if (is_in_replay == 0 && audio_replay_apply_state != 0) {
			if (isOpponentCar == 0)
				audio_select_crash2_fx_and_restart(crash_sound_handle);
			else
				audio_select_crash2_fx_and_restart(audio_engine_sound_handle);
		}
	} else if (crash_type == 2) {
		/* Suspension / scrape crash: play sound first, then set flag and stop car. */
		if (is_in_replay == 0 && audio_replay_apply_state != 0) {
			if (isOpponentCar == 0)
				audio_select_crash2_fx_and_restart(crash_sound_handle);
			else
				audio_select_crash2_fx_and_restart(audio_engine_sound_handle);
		}
		crashCarState->car_crashBmpFlag = 2;
		stopVehicleOnCrash = 1;
		if (isOpponentCar == 0) {
			state.game_impactSpeed    = crashCarState->car_speed2;
			state.game_frames_per_sec = framespersec << 2;
		}
	} else if (crash_type == 3) {
		/* Wrong-way / finish trigger: set flag and record finish time. */
		crashCarState->car_crashBmpFlag = 3;
		if (isOpponentCar == 0) {
			state.game_total_finish   = state.game_frame + state.game_penalty + elapsed_time1;
			state.game_frames_per_sec = framespersec;
		} else {
			state.game_frame_update_target = state.game_frame + elapsed_time1;
		}
	} else if (crash_type == 4) {
		/* Pause / freeze: slow the simulation to 1 fps. */
		state.game_frame_in_sec  = 1;
		state.game_frames_per_sec = 1;
	}
	/* crash_type not in {1,2,3,4,5}: no crash-type action; continue to cleanup. */

	/* Halt car if this crash type requires it. */
	if (stopVehicleOnCrash != 0) {
		crashCarState->car_speed2 = 0;
		crashCarState->car_speed  = 0;
	}

	/* Record end-frame timestamp for the relevant car. */
	if (isOpponentCar == 0)
		state.game_pEndFrame = state.game_frame;
	else
		state.game_oEndFrame = state.game_frame;

	/* Latch the first crash type into the evaluation flag (player only). */
	if (state.game_3F6autoLoadEvalFlag == 0 && isOpponentCar == 0)
		state.game_3F6autoLoadEvalFlag = crash_type;

/** @brief Screen.
 * @param replay_mode_state_flag Parameter value.
 * @return Function return value.
 */
	/* Snapshot game statistics for the evaluation screen (unless in replay). */
	if ((replay_mode_state_flag & 4) == 0) {
		/*
		 * Original DOS code copied this block as a compact struct assignment.
		 * We keep explicit field copies here because the destination is modeled
		 * as standalone globals consumed by the evaluation screen path.
		 */
		gState_travDist          = state.game_travDist;
		gState_frame             = state.game_frame;
		gState_total_finish_time = state.game_total_finish;
		gState_144               = state.game_frame_update_target;
		gState_pEndFrame         = state.game_pEndFrame;
		gState_oEndFrame         = state.game_oEndFrame;
		gState_penalty           = state.game_penalty;
		gState_impactSpeed       = state.game_impactSpeed;
		gState_topSpeed          = state.game_topSpeed;
		gState_jumpCount         = state.game_jumpCount;
	}
}
