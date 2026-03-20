#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <stdint.h>
#include "math.h"
#include "shape3d.h"

/* Size of the global path scratch buffer g_path_buf (see src/data_game.c). */
#define GAME_PATH_BUF_SIZE 94

#pragma pack(push, 1)

struct GAMEINFO {
	char game_playercarid[4];
	char game_playermaterial;
	char game_playertransmission;
	char game_opponenttype;
	char game_opponentcarid[4];
	char game_opponentmaterial;
	char game_opponenttransmission;
	char game_trackname[9];
	unsigned short game_framespersec;
	unsigned short game_recordedframes;
};

struct CARSTATE {
	struct VECTORLONG car_posWorld1;
	struct VECTORLONG car_posWorld2;
	struct VECTOR car_rotate;
	short car_pseudoGravity;
	short car_steeringAngle;
	short car_currpm;
	short car_lastrpm;
	short car_idlerpm2;
	short car_speeddiff;
	unsigned short car_speed;
	unsigned short car_speed2;
	unsigned short car_lastspeed;
	unsigned short car_gearratio;
	unsigned short car_gearratioshr8;
	short car_knob_x;
	short car_36MwhlAngle;
	short car_knob_y;
	short car_knob_x2;
	short car_knob_y2;
	short car_angle_z;
	short car_40MfrontWhlAngle;
	short car_steering_residual;
	short car_demandedGrip;
	short car_surfacegrip_sum;
	short car_heading_angle;
	short car_waypoint_seq_index;
	short car_rc1[4];
	short car_rc2[4];
	short car_rc3[4];
	short car_rc4[4];
	short car_rc5[4];
	struct VECTOR car_whlWorldCrds1[4];
	struct VECTOR car_whlWorldCrds2[4];
	struct VECTOR car_waypoint_target;
	struct VECTOR car_waypoint_left;
	struct VECTOR car_waypoint_right;
	short car_collision_contact_flag;
	char car_is_braking;
	char car_is_accelerating;
	char car_current_gear;
	char car_sumSurfFrontWheels;
	char car_sumSurfRearWheels;
	char car_sumSurfAllWheels;
	char car_surfaceWhl[4];
	char car_engineLimiterTimer;
	char car_slidingFlag;
	char car_crash_impact_flag;
	char car_crashBmpFlag;
	char car_changing_gear;
	char car_fpsmul2;
	char car_transmission;
	char car_impact_state_counter;
	char car_track_waypoint_index;
	char car_position_initialized;
};

struct GAMESTATE {
	int32_t game_longs1[24];
	int32_t game_longs2[24];
	int32_t game_longs3[24];
	struct VECTOR game_vec1[2];
	struct VECTOR game_vec3;
	struct VECTOR game_vec4;
	short game_frame_in_sec;
	short game_frames_per_sec;
	int32_t game_travDist;
	short game_frame;
	short game_total_finish; // finish time + penalty when crossed finish line
	short game_frame_update_target;
	short game_pEndFrame;
	short game_oEndFrame;
	short game_penalty;
	unsigned short game_impactSpeed;
	unsigned short game_topSpeed;
	short game_jumpCount;
	struct CARSTATE playerstate;
	struct CARSTATE opponentstate;
	short game_current_waypoint_index;
	short game_track_segment_working_index;
	short game_startcol;
	short game_startcol2;
	short game_startrow;
	short game_startrow2;
	short game_obstacle_rotx[24];
	short game_obstacle_roty[24];
	short game_obstacle_rotz[24];
	short game_obstacle_active[24];
	char game_obstacle_metadata[48];
	char kevinseed[6];
	char game_checkpoint_valid;
	char game_inputmode;
	char game_3F6autoLoadEvalFlag;
	char game_track_indices[2];
	char game_track_lookup_temp;
	char game_obstacle_flags[48];
	char game_obstacle_count;
	char game_obstacle_shape[24];
	char game_obstacle_status[24];
	char game_flyover_state;
	char game_flyover_counter;
	char game_collision_type;
	char game_flyover_check;
	char game_misc_state_counter;
};

struct SIMD {
	char num_gears;
	char simd_reserved;
	short car_mass;
	short braking_eff;
	short idle_rpm;
	short downshift_rpm;
	short upshift_rpm;
	short max_rpm;
	unsigned short gear_ratios[7];
	struct POINT2D_SIMD knob_points[7];
	short aero_resistance;
	char idle_torque;
	char torque_curve[104];
	char simd_reserved_a3;
	short grip;
	short simd_reserved_a6[7];
	short sliding;
	short surface_grip[4];
	char simd_reserved_b3[10];
	struct POINT2D_SIMD collide_points[2];
	short car_height;
	struct VECTOR wheel_coords[4];
	char steeringdots[62];
	struct POINT2D_SIMD spdcenter;
	short spdnumpoints;
	char spdpoints[208];
	struct POINT2D_SIMD revcenter;
	short revnumpoints;
	char revpoints[256];
	uintptr_t aerorestable;
};

struct TRKOBJINFO {
	char  si_noOfBlocks;      // How many shapeInfo pieces compose the element.
	char  si_entryPoint;      // Connectivity of the track element regarding tiles.
	char  si_exitPoint;
	char  si_entryType;        // Connectivity of the track element regarding element types.
	char  si_exitType;
	char  si_arrowType;        // Type of the element for determining penalty-arrow behaviour.
	short si_arrowOrient;      // Orientation angle for penalty-arrow purposes
	short* si_cameraDataOffset; // offset (0003B770)
	char  si_opp1;             // Appears to affect how the opponent AI approaches an element.
	char  si_opp2;
	char  si_opp3;
	char  si_oppSpedCode;
};

struct TRACKOBJECT {
	struct TRKOBJINFO* ss_trkObjInfoPtr; // offset (0003B770)
	short ss_rotY;           // Horizontal orientation of the element.
	struct SHAPE3D* ss_shapePtr;       // offset (0003B770)
	struct SHAPE3D* ss_loShapePtr;     // offset (0003B770)
	unsigned char  ss_ssOvelay;       // Renders additional sceneShapes over the current one.
	char  ss_surfaceType;    // Paintjob. FF will induce alternating paintjobs.
	char  ss_ignoreZBias;    // Appears to be Z-bias override flag, mostly used for roads and corners.
	char  ss_multiTileFlag;  // 0 = one-tile, 1 = two-tile vertical, 2 = two-tile horizontal, 3 = four-tile.
	char  ss_physicalModel;  // sets the physical model in build_track_object
	char  scene_reserved5;        // always zero.
};

#define STATE_TRACKOBJECT_RAW_SIZE 14u
#define STATE_TRKOBJINFO_RAW_SIZE 14u

typedef struct state_trackobject_raw {
	uint16_t trkobj_info_ofs;
	int16_t rot_y;
	uint16_t shape_ofs;
	uint16_t lo_shape_ofs;
	uint8_t overlay;
	uint8_t surface_type;
	uint8_t ignore_z_bias;
	uint8_t multi_tile_flag;
	uint8_t physical_model;
	uint8_t scene_reserved5;
} state_trackobject_raw;

typedef struct state_trkobjinfo_raw {
	uint8_t no_of_blocks;
	uint8_t entry_point;
	uint8_t exit_point;
	uint8_t entry_type;
	uint8_t exit_type;
	uint8_t arrow_type;
	int16_t arrow_orient;
	uint16_t camera_data_ofs;
	uint8_t opp1;
	uint8_t opp2;
	uint8_t opp3;
	uint8_t opp_sped_code;
} state_trkobjinfo_raw;

int state_trackobject_raw_decode(const unsigned char* table, unsigned int index, state_trackobject_raw* out);
int state_trkobjinfo_raw_decode(const unsigned char* table, unsigned int index, state_trkobjinfo_raw* out);

#pragma pack(pop)

#endif
