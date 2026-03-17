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
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define STUNTS_IMPL
#include "stunts.h"
#include "game_timing.h"
#include "ressources.h"
#include "keyboard.h"
#include "math.h"
#include "memmgr.h"
#include "shape2d.h"
#include "shape3d.h"
#include <string.h>
#include "timer.h"
#include "font.h"
#include "mouse.h"
#include "highscore.h"
#include "menu.h"
#include "ui_keys.h"
#include "ui_widgets.h"

/* Variables moved from data_game.c */
static void (*exitlistfuncs[11])(void) = { 0 };
static char dashb_toggle_copy = 0;
static void * eng1ptr = 0;
static void * engptr = 0;
static char game_replay_mode_copy = 0;
static int roofbmpheight_copy = 0;
static short sprite_animation_frame_counter = 0;

/* Variables moved from data_game.c (private to this translation unit) */
static unsigned char audio_adlib_config[12] = { 244, 1, 16, 39, 40, 35, 0, 0, 24, 48, 0, 0 };
static unsigned char audio_mt32_config[48] = { 244, 1, 16, 39, 40, 35, 0, 0, 74, 48, 0, 0, 79, 48, 0, 0, 84, 48, 0, 0, 89, 48, 0, 0, 94, 48, 0, 0, 99, 48, 0, 0, 104, 48, 0, 0, 109, 48, 0, 0, 114, 48, 0, 0, 119, 48, 0, 0 };
static int dashbmp_y_copy = 0;
static char followOpponentFlag_copy = 0;
static short fps_times_thirty = 0;
static unsigned char g_kevinrandom_seed[] = { 0, 0, 0, 0, 0, 0 };
static char game_input_keyboard_state = 0;
static struct GAMEINFO gameconfigcopy = { 0 };
static struct RECTANGLE rect_windshield = { 0, 0, 0, 0 };
static char replaybar_toggle_copy = 0;
static unsigned short someZeroVideoConst = 0;
static char sprite_buffer_x_coords[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static char sprite_buffer_y_coords[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static char * track_file_append = 0;


/* file-local data (moved from data_global.c) */
static char input_status_stack_index = 0;

struct SIMD simd_opponent_rt = {0};

void call_exitlist(void);

// Forward declarations
void do_mer_restext(void);
void draw_beveled_border(unsigned short x, unsigned short y, unsigned short w, unsigned short h, unsigned short col1, unsigned short col2, unsigned short col3);

/* POINT2D_DOS: 16-bit DOS resource layout (2 bytes per int, total 4 bytes) */
struct POINT2D_DOS {
	int16_t px, py;
};

/* SIMD_RESOURCE: Matches the exact binary layout in the .RES resource file.
   Uses 16-bit POINT2D_DOS and 16-bit VECTOR for proper alignment. */
struct SIMD_RESOURCE {
	char num_gears;
	char simd_reserved;
	int16_t car_mass;
	int16_t braking_eff;
	int16_t idle_rpm;
	int16_t downshift_rpm;
	int16_t upshift_rpm;
	int16_t max_rpm;
	uint16_t gear_ratios[7];
	struct POINT2D_DOS knob_points[7];
	int16_t aero_resistance;
	char idle_torque;
	char torque_curve[104];
	char simd_reserved_a3;
	int16_t grip;
	int16_t simd_reserved_a6[7];
	int16_t sliding;
	int16_t surface_grip[4];
	char simd_reserved_b3[10];
	struct POINT2D_DOS collide_points[2];
	int16_t car_height;
	struct VECTOR wheel_coords[4];
	char steeringdots[62];
	struct POINT2D_DOS spdcenter;
	int16_t spdnumpoints;
	char spdpoints[208];
	struct POINT2D_DOS revcenter;
	int16_t revnumpoints;
	char revpoints[256];
	uint32_t aerorestable_dos;
};

/** @brief Copy SIMD car data from on-disk resource format
 * @param dst Destination SIMD structure
 * @param dst_capacity Size of destination buffer
 * @param src Source resource data pointer
 * @return 0 on success, non-zero on error
 */
static int simd_copy_from_resource(struct SIMD* dst, size_t dst_capacity, const void* src)
{
	const struct SIMD_RESOURCE* res;
	int i;

	if (dst == 0 || src == 0) {
		return 0;
	}
	if (dst_capacity == 0) {
		dst_capacity = sizeof(*dst);
	}

	res = (const struct SIMD_RESOURCE*)src;

	dst->num_gears = res->num_gears;
	dst->simd_reserved = res->simd_reserved;
	dst->car_mass = res->car_mass;
	dst->braking_eff = res->braking_eff;
	dst->idle_rpm = res->idle_rpm;
	dst->downshift_rpm = res->downshift_rpm;
	dst->upshift_rpm = res->upshift_rpm;
	dst->max_rpm = res->max_rpm;

	for (i = 0; i < 7; i++) {
		dst->gear_ratios[i] = res->gear_ratios[i];
	}

	for (i = 0; i < 7; i++) {
		dst->knob_points[i].px = res->knob_points[i].px;
		dst->knob_points[i].py = res->knob_points[i].py;
	}

	dst->aero_resistance = res->aero_resistance;
	dst->idle_torque = res->idle_torque;
	memcpy(dst->torque_curve, res->torque_curve, sizeof(dst->torque_curve));
	dst->simd_reserved_a3 = res->simd_reserved_a3;
	dst->grip = res->grip;

	for (i = 0; i < 7; i++) {
		dst->simd_reserved_a6[i] = res->simd_reserved_a6[i];
	}

	dst->sliding = res->sliding;

	for (i = 0; i < 4; i++) {
		dst->surface_grip[i] = res->surface_grip[i];
	}

	memcpy(dst->simd_reserved_b3, res->simd_reserved_b3, sizeof(dst->simd_reserved_b3));

	for (i = 0; i < 2; i++) {
		dst->collide_points[i].px = res->collide_points[i].px;
		dst->collide_points[i].py = res->collide_points[i].py;
	}

	dst->car_height = res->car_height;

	for (i = 0; i < 4; i++) {
		dst->wheel_coords[i] = res->wheel_coords[i];
	}

	memcpy(dst->steeringdots, res->steeringdots, sizeof(dst->steeringdots));

	dst->spdcenter.px = res->spdcenter.px;
	dst->spdcenter.py = res->spdcenter.py;
	dst->spdnumpoints = res->spdnumpoints;
	memcpy(dst->spdpoints, res->spdpoints, sizeof(dst->spdpoints));

	dst->revcenter.px = res->revcenter.px;
	dst->revcenter.py = res->revcenter.py;
	dst->revnumpoints = res->revnumpoints;
	{
		size_t rev_off = (size_t)((const char*)dst->revpoints - (const char*)dst);
		size_t rev_cap = 0;
		size_t rev_copy;
		if (dst_capacity > rev_off) {
			rev_cap = dst_capacity - rev_off;
		}
		rev_copy = sizeof(dst->revpoints);
		if (rev_copy > rev_cap) {
			rev_copy = rev_cap;
		}
		if (rev_copy > 0) {
			memcpy(dst->revpoints, res->revpoints, rev_copy);
		}
		if (rev_copy < sizeof(dst->revpoints)) {
			short max_points = (short)(rev_copy / 2);
			if (dst->revnumpoints > max_points) {
				dst->revnumpoints = max_points;
			}
		}
	}

	if (dst_capacity >= sizeof(*dst)) {
		dst->aerorestable = 0;
	}

	return 1;
}

// Entries in the CVX gamestate buffer.
#define RST_CVX_NUM 20

// ASCII code properties map.
#define RST_ASC_CHAR_UPPER 1
#define RST_ASC_CHAR_LOWER 2
#define RST_ASC_NUMBER     4
#define RST_ASC_WHITESPACE 8
#define RST_ASC_PUNCTATION 16
#define RST_ASC_CONTROL    32
#define RST_ASC_SPACE      64
#define RST_ASC_HEX        128

enum {
	STN_TRACK_GRID_SIZE = 30,
	STN_TRACK_LAST_INDEX = 29,
	STN_TRACK_CELL_SHIFT = 10,
	STN_TRACK_CELL_CENTER_OFFSET = 512,
	STN_TRACK_POS_SENTINEL = -1024,
	STN_TRACK_OPPONENT_AUTOSELECT = 255,
	STN_TRACKDATA_TOTAL_SIZE = 27635,
	STN_TRACKDATA_BLOCK_MAIN = 1802,
	STN_TRACKDATA_BLOCK_SMALL = 128,
	STN_TRACKDATA_BLOCK_PATH = 901,
	STN_TRACKDATA_TRACKNAME_LEN = 81,
	STN_TRACKDATA_PATH_BASE = 1802,
	STN_TRACKDATA_REPLAY_BASE = 1883,
	STN_SCREEN_WIDTH = 320,
	STN_SCREEN_HEIGHT = 200,
	STN_SCREEN_DEPTH = 15,
	STN_CLIPRECT_BOTTOM = 95,
	STN_DASH_REPLAYBAR_Y = 151,
	STN_WAIT_DIALOG_Y = 100,
	STN_PALETTE_BYTES = 768,
	STN_PALETTE_COLORS = 256,
	STN_FPS_DEFAULT = 10,
	STN_FPS_ALT = 30,
	STN_FPS_TEXT_MULT = 30,
	STN_TRACK_START_OFFSET_NEAR = 210,
	STN_TRACK_START_OFFSET_SIDE = 36,
	STN_TRACK_DIR_BIAS_SMALL = 512,
	STN_TRACK_DIR_BIAS_LARGE = 4096,
	STN_TRACK_SPAWN_Y_OFFSET = 960,
	STN_WORLD_SCALE_SHIFT = 6,
	STN_CAR_START_Y_LIFT = 1408,
	STN_PLANE_NORMAL_Y = 8192,
	STN_PLAN_START_ROW = 28,
	STN_PLAN_SPEED_INIT = 200
};

enum {
	STN_PERSIST_PATH_LEN = 82,
	STN_PERSIST_TRACKNAME_LEN = 9,
	STN_PERSIST_VERSION = 1
};

static const char STN_PERSIST_CONFIG_FILE[] = "stunts.cfg";
static const char STN_DEFAULT_TRACK_NAME[] = "DEFAULT";
static char aMisc_1[] = "misc";
static unsigned char steerWhlRespTable_20fps[64] = { 0, 8, 248, 0, 0, 7, 249, 0, 0, 6, 250, 0, 0, 5, 251, 0, 0, 4, 252, 0, 0, 4, 252, 0, 0, 3, 253, 0, 0, 3, 253, 0, 0, 2, 254, 0, 0, 2, 254, 0, 0, 2, 254, 0, 0, 1, 255, 0, 0, 1, 255, 0, 0, 1, 255, 0, 0, 1, 255, 0, 0, 1, 255, 0 };
static unsigned char steerWhlRespTable_10fps[62] = { 0, 16, 240, 0, 0, 14, 242, 0, 0, 12, 244, 0, 0, 10, 246, 0, 0, 8, 248, 0, 0, 8, 248, 0, 0, 6, 250, 0, 0, 6, 250, 0, 0, 4, 252, 0, 0, 4, 252, 0, 0, 4, 252, 0, 0, 2, 254, 0, 0, 2, 254, 0, 0, 1, 255, 0, 0, 1, 255, 0, 0, 1 };

char track_highscore_path_buffer[82] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0
};

char replay_file_path_buffer[82] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0
};

char aDefault_1[10] = "DEFAULT";

#pragma pack(push, 1)
struct STN_PERSIST_CONFIG {
	char magic[8];
	unsigned short version;
	struct GAMEINFO gameconfig;
	char track_path[STN_PERSIST_PATH_LEN];
	char replay_path[STN_PERSIST_PATH_LEN];
	char mouse_mode;
	char joystick_mode;
};
#pragma pack(pop)

/** @brief Check if the saved track file exists on disk
 * @return Non-zero if available, 0 otherwise
 */
static int stn_persist_track_is_available(void)
{
	file_build_path(track_highscore_path_buffer, gameconfig.game_trackname, ".trk", g_path_buf);
	return file_paras_nofatal(g_path_buf) != 0;
}

/** @brief Validate and fix persisted game configuration state
 */
static void stn_persist_sanitize_runtime_state(void)
{
	gameconfig.game_trackname[STN_PERSIST_TRACKNAME_LEN - 1] = '\0';
	track_highscore_path_buffer[STN_PERSIST_PATH_LEN - 1] = '\0';
	replay_file_path_buffer[STN_PERSIST_PATH_LEN - 1] = '\0';

	if (gameconfig.game_trackname[0] == '\0') {
		strcpy(gameconfig.game_trackname, STN_DEFAULT_TRACK_NAME);
	}

	if (!stn_persist_track_is_available()) {
		strcpy(gameconfig.game_trackname, STN_DEFAULT_TRACK_NAME);
		if (!stn_persist_track_is_available()) {
			gameconfig.game_trackname[0] = '\0';
		}
	}

	if ((unsigned char)gameconfig.game_opponenttype > 6) {
		gameconfig.game_opponenttype = 1;
	}
}

/** @brief Load persistent configuration from stunts.cfg
 * @return Non-zero on success, 0 on failure
 */
static int stn_persist_load(void)
{
	struct STN_PERSIST_CONFIG cfg;
	FILE* fp = fopen(STN_PERSIST_CONFIG_FILE, "rb");

	if (fp == NULL) {
		return 0;
	}

	if (fread(&cfg, sizeof(cfg), 1, fp) != 1) {
		fclose(fp);
		return 0;
	}

	fclose(fp);

	if (memcmp(cfg.magic, "STNCFG01", 8) != 0 || cfg.version != STN_PERSIST_VERSION) {
		return 0;
	}

	gameconfig = cfg.gameconfig;
	memcpy(track_highscore_path_buffer, cfg.track_path, STN_PERSIST_PATH_LEN);
	memcpy(replay_file_path_buffer, cfg.replay_path, STN_PERSIST_PATH_LEN);
	mouse_motion_state_flag = cfg.mouse_mode;
	joystick_assigned_flags = cfg.joystick_mode;

	stn_persist_sanitize_runtime_state();
	return 1;
}

/** @brief Save persistent configuration to stunts.cfg
 */
static void stn_persist_save(void)
{
	struct STN_PERSIST_CONFIG cfg;
	FILE* fp;

	stn_persist_sanitize_runtime_state();

	memcpy(cfg.magic, "STNCFG01", 8);
	cfg.version = STN_PERSIST_VERSION;
	cfg.gameconfig = gameconfig;
	memcpy(cfg.track_path, track_highscore_path_buffer, STN_PERSIST_PATH_LEN);
	memcpy(cfg.replay_path, replay_file_path_buffer, STN_PERSIST_PATH_LEN);
	cfg.mouse_mode = mouse_motion_state_flag;
	cfg.joystick_mode = joystick_assigned_flags;

	fp = fopen(STN_PERSIST_CONFIG_FILE, "wb");
	if (fp == NULL) {
		return;
	}

	(void)fwrite(&cfg, sizeof(cfg), 1, fp);
	fclose(fp);
}

/* Exit handler list - array of 10  function pointers
 * Declared in dseg.asm as exitlistfuncs
 */
typedef void ( *exit_handler_func)(void);

/**
 * @brief Register a cleanup function to be called on exit
 * 
 * @param handler Far pointer to the exit handler function
 * 
 * Searches the exitlistfuncs array for an empty slot and adds the handler.
 * If the handler is already registered, does nothing.
 * Calls fatal_error if no free slots are available.
 */
void add_exit_handler(exit_handler_func handler) {
	int i;
	
	for (i = 0; i < 10; i++) {
		/* Check if already registered */
		if (exitlistfuncs[i] == handler) {
			return;
		}
/** @brief Slot.
 * @param exit_handler_func Parameter `exit_handler_func`.
 * @return Function result.
 */
		/* Check for empty slot (NULL pointer) */
		if (exitlistfuncs[i] == (exit_handler_func)0) {
			exitlistfuncs[i] = handler;
			return;
		}
	}
	
	/* No free slots - this is fatal */
	fatal_error("EXIT LIST OVERFLOW\r\n");
}

/**
 * @brief Call all registered exit handlers
 * 
/** @brief Order.
 * @param call_exitlist Parameter `call_exitlist`.
 * @return Function result.
 */
void call_exitlist(void) {
	int i;
	
	for (i = 10; i >= 0; i--) {
		if (exitlistfuncs[i] != (exit_handler_func)0) {
			exitlistfuncs[i]();
		}
	}
}

/** @brief Call all exit handlers and terminate the program
 */
void call_exitlist2(void)
{
	call_exitlist();
	exit(0);
}

#ifndef _WIN32
/** @brief Custom toupper implementation for Linux builds
 * @param ch Character to convert
 * @return Uppercase version of ch, or ch if not lowercase
 */
int toupper(int ch)
{
	if (ch >= 'a' && ch <= 'z') {
		ch -= ' ';
	}

	return ch;
}
#endif

void copy_string(char* dest, char * src);

/** @brief Initialize track grid row and column lookup tables
 */
void init_row_tables(void) {
	int i;
	for (i = 0; i < STN_TRACK_GRID_SIZE; i++) {
		trackrows[i] = STN_TRACK_GRID_SIZE * (STN_TRACK_LAST_INDEX - i);
		terrainrows[i] = STN_TRACK_GRID_SIZE * i;
		trackpos[i] = (STN_TRACK_LAST_INDEX - i) << STN_TRACK_CELL_SHIFT;
		trackpos2[i] = i << STN_TRACK_CELL_SHIFT;
		trackcenterpos[i] = ((STN_TRACK_LAST_INDEX - i) << STN_TRACK_CELL_SHIFT) + STN_TRACK_CELL_CENTER_OFFSET;
		terrainpos[i] = i << STN_TRACK_CELL_SHIFT;
		terraincenterpos[i] = (i << STN_TRACK_CELL_SHIFT) + STN_TRACK_CELL_CENTER_OFFSET;
		trackcenterpos2[i] = (i << STN_TRACK_CELL_SHIFT) + STN_TRACK_CELL_CENTER_OFFSET;
	}

	terrainrows[STN_TRACK_GRID_SIZE] = STN_TRACK_GRID_SIZE * STN_TRACK_GRID_SIZE;
	trackrows[STN_TRACK_GRID_SIZE] = STN_TRACK_GRID_SIZE * (STN_TRACK_LAST_INDEX - STN_TRACK_GRID_SIZE);
	trackpos[STN_TRACK_GRID_SIZE] = STN_TRACK_POS_SENTINEL;
	trackpos2[STN_TRACK_GRID_SIZE] = STN_TRACK_GRID_SIZE << STN_TRACK_CELL_SHIFT;
	terrainpos[STN_TRACK_GRID_SIZE] = STN_TRACK_GRID_SIZE << STN_TRACK_CELL_SHIFT;
}

/** @brief Set default player and opponent car configuration
 */
void set_default_car(void) {
	gameconfig.game_playercarid[0]     = 'C';
	gameconfig.game_playercarid[1]     = 'O';
	gameconfig.game_playercarid[2]     = 'U';
	gameconfig.game_playercarid[3]     = 'N';
	gameconfig.game_playermaterial     = 0;
	gameconfig.game_playertransmission = 1;
	gameconfig.game_opponenttype       = 0; /* default opponent: Clock */
	gameconfig.game_opponentmaterial   = 1; /* alternate material from player */
	gameconfig.game_opponentcarid[0]   = STN_TRACK_OPPONENT_AUTOSELECT; /* auto-select at race start */
	gameconfig.game_opponenttransmission = 0;
}

/** @brief Allocate and partition the track data buffer
 */
void init_trackdata(void) {
	unsigned char * trkptr;
	trkptr = (unsigned char *)mmgr_alloc_resbytes("trakdata", STN_TRACKDATA_TOTAL_SIZE);

	track_waypoint_next = (short *)trkptr;

	trkptr += STN_TRACKDATA_BLOCK_MAIN;
	track_waypoint_alt = (short *)trkptr;

	trkptr += STN_TRACKDATA_BLOCK_MAIN;
	track_waypoint_order = (char *)trkptr;

	trkptr += STN_TRACKDATA_BLOCK_MAIN;
	aero_table_player = (short *)trkptr;

	trkptr += STN_TRACKDATA_BLOCK_SMALL;
	aero_table_opponent = (short *)trkptr;

	trkptr += STN_TRACKDATA_BLOCK_SMALL;
	track_cam_height_base = (char *)trkptr;

	trkptr += STN_TRACKDATA_BLOCK_SMALL;
	track_element_height_ofs = (char *)trkptr;

	trkptr += STN_TRACKDATA_BLOCK_SMALL;
	obstacle_rot_z = (short *)trkptr;

	trkptr += 96;
	waypoint_world_pos = (short *)trkptr;

	trkptr += 384;
	obstacle_world_pos = (short *)trkptr;

	trkptr += 288;
	highscore_data = (char *)trkptr;

	trkptr += 364;
	window_save_buf = (char *)trkptr;

	trkptr += 240;
	replay_header = (char *)trkptr;

	trkptr += 26;
	track_elem_map = trkptr;

	trkptr += STN_TRACKDATA_BLOCK_PATH;
	track_terrain_map = trkptr;

	trkptr += STN_TRACKDATA_BLOCK_PATH;
	replay_buffer = (char *)trkptr;

	trkptr += 12000;
	track_elem_ordered = (char *)trkptr;

	trkptr += STN_TRACKDATA_BLOCK_PATH;
	path_conn_flags = (char *)trkptr;

	trkptr += STN_TRACKDATA_BLOCK_PATH;
	tile_obstacle_map = trkptr;

	trkptr += STN_TRACKDATA_BLOCK_PATH;
	track_file_append = (char *)trkptr;

	trkptr += 1964;
	path_col = (char *)trkptr;

	trkptr += STN_TRACKDATA_BLOCK_PATH;
	path_row = (char *)trkptr;

	trkptr += 901;
	obstacle_scene_index = trkptr;

	trkptr += 48;

}

struct RECTANGLE shaperect = { 0, STN_SCREEN_WIDTH, 0, STN_SCREEN_HEIGHT };
struct TRANSFORMEDSHAPE3D transshape;
struct RECTANGLE cliprect = { 0, STN_SCREEN_WIDTH, 0, STN_CLIPRECT_BOTTOM };
struct VECTOR carpos = { 0, 64696, 2880 }; // from the original

/** @brief Reset replay-related runtime variables to defaults
 */
void reset_replay_runtime_state(void)
{
	replay_fps_inverse_scale = 1;
	replay_capture_frame_skip_counter = 2;
	replay_frame_counter = 0;
	game_finish_state = 0;
	game_pause_counter = 0;
	current_rotation_angle_value = 0;
	replay_autoplay_active = 0;
}

/** @brief Initialize car physics state from SIMD data
 * @param playerstate Car state structure to initialize
 * @param simd SIMD car data source
 * @param transmission Transmission type
 * @param posX Initial X position
 * @param posY Initial Y position
 * @param posZ Initial Z position
 * @param track_angle Initial track heading angle
 */
void init_carstate_from_simd(struct CARSTATE* playerstate, struct SIMD* simd, char transmission, long posX, long posY, long posZ, short track_angle)
{
	int i;
	struct VECTOR whlPos;

	playerstate->car_posWorld1.lx = posX;
	playerstate->car_posWorld2.lx = posX;
	playerstate->car_posWorld1.ly = posY + 512;
	playerstate->car_posWorld2.ly = posY;
	playerstate->car_posWorld1.lz = posZ;
	playerstate->car_posWorld2.lz = posZ;
	
	playerstate->car_rotate.x = track_angle;
	playerstate->car_rotate.y = 0;
	playerstate->car_rotate.z = 0;
	playerstate->car_36MwhlAngle = 0;
	playerstate->car_pseudoGravity = 0;
	playerstate->car_steeringAngle = 0;
	playerstate->car_is_braking = 0;
	playerstate->car_is_accelerating = 0;
	playerstate->car_currpm = simd->idle_rpm;
	playerstate->car_lastrpm = playerstate->car_currpm;
	playerstate->car_idlerpm2 = playerstate->car_currpm;
	playerstate->car_current_gear = 1;
	playerstate->car_speeddiff = 0;
	playerstate->car_speed = 0;
	playerstate->car_speed2 = 0;
	playerstate->car_lastspeed = 0;
	playerstate->car_gearratio = simd->gear_ratios[1];
	playerstate->car_gearratioshr8 = playerstate->car_gearratio >> 8;
	playerstate->car_knob_x = simd->knob_points[1].px;
	playerstate->car_knob_x2 = playerstate->car_knob_x;
	playerstate->car_knob_y = simd->knob_points[1].py;
	playerstate->car_knob_y2 = playerstate->car_knob_y;
	playerstate->car_angle_z = 0;
	playerstate->car_40MfrontWhlAngle = 0;
	playerstate->car_steering_residual = 0;
	playerstate->car_heading_angle = 0;
	playerstate->car_waypoint_seq_index = 0;
	playerstate->car_sumSurfFrontWheels = 2;
	playerstate->car_sumSurfRearWheels = 2;
	playerstate->car_sumSurfAllWheels = 4;
	playerstate->car_demandedGrip = 0;
	playerstate->car_surfacegrip_sum = 1000;

	whlPos.x = posX / 64;
	whlPos.y = posY / 64;
	whlPos.z = posZ / 64;

	for (i = 0; i < 4; ++i) {
		playerstate->car_surfaceWhl[i] = 1;
		playerstate->car_rc1[i] = 0;
		playerstate->car_rc2[i] = 0;
		playerstate->car_rc3[i] = 0;
		playerstate->car_rc4[i] = 0;
		playerstate->car_rc5[i] = 0;

		playerstate->car_whlWorldCrds1[i] = whlPos;
		playerstate->car_whlWorldCrds2[i] = whlPos;
	}

	playerstate->car_engineLimiterTimer = 0;
	playerstate->car_slidingFlag = 0;
	playerstate->car_crash_impact_flag = 0;
	playerstate->car_crashBmpFlag = 0;
	playerstate->car_changing_gear = 0;
	playerstate->car_fpsmul2 = 0;
	playerstate->car_transmission = transmission;
	playerstate->car_impact_state_counter = 0;
	playerstate->car_track_waypoint_index = 0;
	playerstate->car_position_initialized = 1;
}

/** @brief Initialize or reset the full game state
/** @brief Mode.
 * @param init Parameter `init`.
 * @param init Parameter `init`.
 * @param arg Parameter `arg`.
 * @return Function result.
 */
void init_game_state(short arg)
{
	int i, tmpcol, tmprow;

	if (arg == -1) {
		elapsed_time1 = 0;

		for (i = 0; i < RST_CVX_NUM; ++i) {
			((struct GAMESTATE *)cvxptr)[i].game_checkpoint_valid = 0;
		}
	}
	
	if (framespersec == STN_FPS_DEFAULT) {
		steerWhlRespTable_ptr = &steerWhlRespTable_10fps;
	}
	else {
		steerWhlRespTable_ptr = &steerWhlRespTable_20fps;
	}
	
	   fps_times_thirty = framespersec * STN_FPS_TEXT_MULT;
	   if (framespersec == 0) {
		   framespersec = STN_FPS_DEFAULT; // Default to 10 if not set
	   }
	   inverse_fps_hundredths = 100 / framespersec;

	if (arg != -3) {
		reset_replay_runtime_state();

		state.game_checkpoint_valid = 1;
		state.game_frames_per_sec = 1;
		state.game_inputmode = 0;
		state.game_3F6autoLoadEvalFlag = 0;
		state.game_frame_in_sec = 0;
		state.game_track_segment_working_index = 0;
		state.game_track_indices[0] = 0;
		state.game_track_indices[1] = 0;

		for (i = 0; i < 48; ++i) {
			state.game_obstacle_flags[i] = 0;
		}
				
		for (i = 0; i < 24; ++i) {
			state.game_obstacle_active[i] = 0;
		}

		state.game_vec1[0].x =
			  multiply_and_scale(sin_fast(track_angle + 768),  STN_TRACK_DIR_BIAS_SMALL)
			+ multiply_and_scale(sin_fast(track_angle + 512), STN_TRACK_DIR_BIAS_LARGE)
			+ ((short)startcol2 << STN_TRACK_CELL_SHIFT);

		state.game_vec1[0].y = hillHeightConsts[hillFlag] + STN_TRACK_SPAWN_Y_OFFSET;

		state.game_vec1[0].z =
			  multiply_and_scale(cos_fast(track_angle + 768),  STN_TRACK_DIR_BIAS_SMALL)
			+ multiply_and_scale(cos_fast(track_angle + 512), STN_TRACK_DIR_BIAS_LARGE)
			+ trackpos[startrow2];

		state.game_vec1[1] = state.game_vec1[0];
		state.game_vec3 = state.game_vec1[0];
		state.game_vec4 = state.game_vec1[0];
		
		state.game_travDist = 0;
		state.game_frame = 0;
		state.game_total_finish = 0;
		state.game_frame_update_target = 0;
		state.game_pEndFrame = 0;
		state.game_oEndFrame = 0;
		state.game_penalty = 0;
		state.game_impactSpeed = 0;
		state.game_topSpeed = 0;
		state.game_jumpCount = 0;

		// Init player car.
		tmpcol =
			  multiply_and_scale(sin_fast(track_angle + 512), STN_TRACK_START_OFFSET_NEAR)
			+ multiply_and_scale(sin_fast(track_angle + 256),  STN_TRACK_START_OFFSET_SIDE);
		
		tmprow =
			  multiply_and_scale(cos_fast(track_angle + 512), STN_TRACK_START_OFFSET_NEAR)
			+ multiply_and_scale(cos_fast(track_angle + 256),  STN_TRACK_START_OFFSET_SIDE);

		init_carstate_from_simd(
			&state.playerstate,
			&simd_player,
			gameconfig.game_playertransmission,
			(long)(trackcenterpos2[startcol2] + tmpcol) * (1L << STN_WORLD_SCALE_SHIFT),
			(long)hillHeightConsts[hillFlag] * (1L << STN_WORLD_SCALE_SHIFT),
			(long)(trackcenterpos[startrow2] + tmprow) * (1L << STN_WORLD_SCALE_SHIFT),
			-track_angle);

		state.game_current_waypoint_index = 0;
		state.game_collision_type = 0;
		state.game_flyover_check = 0;
		state.game_flyover_state = 0;
		state.game_flyover_counter = 0;
		
		state.game_startcol  = startcol2;
		state.game_startcol2 = startcol2;
		state.game_startrow  = startrow2;
		state.game_startrow2 = startrow2;

		if (arg != -2) {
			track_waypoint_lookup(
				state.playerstate.car_waypoint_seq_index,
				&state.playerstate.car_waypoint_target,
				state.playerstate.car_track_waypoint_index,
				0);
			
			state.playerstate.car_track_waypoint_index++;
		}

		// Init opponent car.
		// Keep longitudinal offset identical, and force the lateral offset to be
		// the opposite of the player's lane offset (restunts-equivalent intent).
		// This avoids startup overlap when angle quantization/drift causes both
		// cars to pick the same side.
		{
			short nearCol = multiply_and_scale(sin_fast(track_angle + 512), STN_TRACK_START_OFFSET_NEAR);
			short nearRow = multiply_and_scale(cos_fast(track_angle + 512), STN_TRACK_START_OFFSET_NEAR);
			short playerSideCol = multiply_and_scale(sin_fast(track_angle + 256), STN_TRACK_START_OFFSET_SIDE);
			short playerSideRow = multiply_and_scale(cos_fast(track_angle + 256), STN_TRACK_START_OFFSET_SIDE);

			tmpcol = nearCol - playerSideCol;
			tmprow = nearRow - playerSideRow;
		}

		init_carstate_from_simd(
			&state.opponentstate,
			&simd_opponent_rt,
			1,
			(long)(trackcenterpos2[startcol2] + tmpcol) * (1L << STN_WORLD_SCALE_SHIFT),
			(long)hillHeightConsts[hillFlag] * (1L << STN_WORLD_SCALE_SHIFT),
			(long)(trackcenterpos[startrow2] + tmprow) * (1L << STN_WORLD_SCALE_SHIFT),
			-track_angle);

		if (gameconfig.game_opponenttype && arg != -2) {
			track_waypoint_lookup(
				((short*)track_waypoint_order)[state.opponentstate.car_waypoint_seq_index],
				&state.opponentstate.car_waypoint_target,
				state.opponentstate.car_track_waypoint_index,
				(short*)&state.game_track_lookup_temp);
		
			state.opponentstate.car_track_waypoint_index++;
		}

		state.game_obstacle_count = 0;
	}
}

/** @brief Restore game state from CVX checkpoint buffer
 * @param frame Frame number to restore from
 */
void restore_gamestate(unsigned short frame)
{
	unsigned short curframe;

	if (frame == 0 && elapsed_time1 == 0) {
		init_game_state(0);
	}
	
	curframe = frame / fps_times_thirty;
	
	if (curframe == RST_CVX_NUM) {
		curframe--;
	}
	
	// Find last gamestate in cvx.
	if (frame >= state.game_frame) {
		while (1) {
			if (curframe * fps_times_thirty <= state.game_frame) {
				return;
			}
			else if (((struct GAMESTATE *)cvxptr)[curframe].game_checkpoint_valid != 0) {
				break;
			}
			
			curframe--;
		}
	}
	
	// Copy last gamestate from cvx.
	state = ((struct GAMESTATE *)cvxptr)[curframe];

	init_kevinrandom(state.kevinseed);
	replay_frame_counter = state.game_frame;
}

/** @brief Per-frame game state update handling physics and input
 */
void update_gamestate() {
	char var_carInputByte;

	var_carInputByte = replay_buffer[state.game_frame];
	if (var_carInputByte != 0) {
		state.game_inputmode = 1;
	}
	
	if ((state.game_frame % fps_times_thirty) == 0) {
		get_kevinrandom_seed(state.kevinseed);
	
		memcpy(&cvxptr[state.game_frame / fps_times_thirty], &state, sizeof(struct GAMESTATE));
	}

	state.game_frame++;
	if (state.game_3F6autoLoadEvalFlag != 0 && state.game_frame_in_sec < state.game_frames_per_sec) {
		state.game_frame_in_sec++;
		if (state.game_frame_in_sec == state.game_frames_per_sec && game_finish_state == 0) {
			if (state.playerstate.car_crashBmpFlag == 1 && state.playerstate.car_speed2 != 0) {
				state.game_frames_per_sec++;
			} else if (game_replay_mode == 0) {
				game_finish_state = 1;
			}
		}
	}

	if (state.game_inputmode != 0) {
		
		update_player_car_state(var_carInputByte);
		
		if (gameconfig.game_opponenttype != 0) {
			update_opponent_car_state();
		}

		update_follow_camera_vectors();
		if (state.game_obstacle_count != 0) {
				update_world_debris_particles();
		}

		audio_sync_car_audio();

	} else if (game_replay_mode == 1) {
		// if paused
		audio_sync_car_audio();
		if (game_pause_counter != 0) {
			if (current_rotation_angle_value < 450) {
				current_rotation_angle_value += 8;
			}

			if (game_pause_counter == 1 && current_rotation_angle_value > 384) {
				game_pause_counter++;
			}

			if (game_pause_counter == 2) {
				if (  multiply_and_scale(cos_fast(track_angle), trackcenterpos[startrow2] - (state.playerstate.car_posWorld1.lz >> 6)) 
					+ multiply_and_scale(sin_fast(track_angle), trackcenterpos2[startcol2] - (state.playerstate.car_posWorld1.lx >> 6)) <= 228) {
					if (state.playerstate.car_speed != 0) {
						update_player_car_state(2);
					} else {
						game_pause_counter = 0;
					}
				} else 
				if (state.playerstate.car_speed < 1280) {
					update_player_car_state(1);
				} else {
					update_player_car_state(0);
				}
			}
		}
	}
}

static struct PLANE  plan_memres[1];
unsigned short  unused_state_word_3B1E2;
unsigned short  unused_state_word_3B1E4;
unsigned short  unused_state_word_3B1E6;
unsigned short  unused_state_word_3B1E8;
unsigned short  unused_state_word_3B1EA;
unsigned short  unused_state_word_3B1EC;
unsigned short  unused_state_word_3B1EE;

/** @brief Copy null-terminated string from source to destination
 * @param dest Destination buffer
 * @param src Source string pointer
 */
void copy_string(char* dest, char * src) {
	while (*src != '\0') {
		*dest++ = *src++;
	}
	*dest = '\0';
}

/** @brief Extract filename without extension from a full path
 * @param dest Output buffer for the extracted filename
 * @param source Full file path input
 */
void parse_filepath_separators(char* dest, char * source) {
	int len = 0;
	int destidx;
	
	// Calculate length of source string
	while (source[len] != '\0') {
		len++;
	}
	
	// Find last path separator (\ or :) by walking backwards
	while (len > 0) {
		char c = source[len - 1];
		if (c == '\\' || c == ':') {
			break;
		}
		len--;
	}
	
	// Copy from after separator until we hit '.'
	destidx = 0;
	while (1) {
		char c = source[len];
		dest[destidx] = c;
		destidx++;
		len++;
		if (c == '.') {
			break;
		}
	}
	
	// Replace the '.' with null terminator
	dest[destidx - 1] = '\0';
}

// Display "Please wait..." dialog
/** @brief Display a "Please wait..." dialog on screen
 */
void show_waiting(void) {
	char * textresptr;
	unsigned short prev_menu_pause;

	prev_menu_pause = audio_set_menu_music_paused(1);
	
	// Locate the "wai" text resource from main resource file
	textresptr = locate_text_res(mainresptr, "wai");
	
	// Show the dialog with the wait message
	if (textresptr != 0) {
		ui_dialog_display_only(textresptr, UI_DIALOG_AUTO_POS, waitflag, 0);
	}

	audio_set_menu_music_paused(prev_menu_pause);
	
	// Update mouse cursor display
	mouse_draw_opaque_check();
}

// Unload audio resources (songs and voice files).
// Mirrors the original ASM: audio_driver_func3F(2) stops the driver first,
// then frees the song/voice memory. This ensures menu KMS music is silenced
// before gameplay begins.
/** @brief Unload audio song and voice resources
 */
void audio_unload(void) {
	if (is_audioloaded == 0) {
		return;
	}
	audio_driver_func3F(2);
	mmgr_free((char *)songfileptr);
	mmgr_free((char *)voicefileptr);
	songfileptr = 0;
	voicefileptr = 0;
	is_audioloaded = 0;
}

// Push current input status onto stack
/** @brief Push current input state onto the input stack
 */
void input_push_status(void) {
	int idx = input_status_stack_index;
	sprite_buffer_x_coords[idx] = mouse_motion_detected_flag;
	sprite_buffer_y_coords[idx] = kbormouse;
	input_status_stack_index++;
}

/** @brief Pop input state from the input stack
 */
void input_pop_status(void) {
	int idx;
	if (input_status_stack_index == 0) {
		return;
	}
	input_status_stack_index--;
	idx = input_status_stack_index;
	mouse_motion_detected_flag = sprite_buffer_x_coords[idx];
	kbormouse = sprite_buffer_y_coords[idx];
	if (kbormouse == 0) {
		mouse_draw_opaque_check();
	}
}

/** @brief Convert an integer to a padded string representation
 * @param dest Destination buffer
 * @param value Integer value to convert
 * @param zeroPadFlag If non-zero, pad with '0' instead of spaces
 * @param width Minimum field width
 */
void print_int_as_string_maybe(char* dest, int value, int zeroPadFlag, int width) {
	int len = 0;
	int i;
	
	// Convert integer to string (base 10)
	stunts_itoa(value, dest, 10);
	
	// Get current length
	while (dest[len] != 0) {
		len++;
	}
	
	// Adjust to match target width
	while (width != len) {
		if (width < len) {
			// Shift left to remove chars
			for (i = 0; i < len; i++) {
				dest[i] = dest[i + 1];
			}
			len--;
		} else {
			// Shift right to add space
			for (i = len; i >= 0; i--) {
				dest[i + 1] = dest[i];
			}
			dest[0] = ' ';
			len++;
		}
	}
	
	// Replace leading spaces with zeros if flag is set
	if (zeroPadFlag != 0) {
		for (i = 0; dest[i] == ' '; i++) {
			dest[i] = '0';
		}
	}
}

/** @brief Format a frame count as a MM:SS[.FF] time string
 * @param dest Destination buffer
 * @param frames Frame count to format
 * @param showFractions If non-zero, include fractional frames
 */
void format_frame_as_string(char* dest, unsigned short frames, unsigned short showFractions) {
	unsigned short framesPerMinute;
	unsigned short minutes;
	unsigned short seconds;
	unsigned short remainingFrames;
	unsigned short fps;
	char tempBuf[24];
	
	fps = framespersec;
	if (fps == 0) {
		fps = 20;
	}

	// Calculate frames per minute (60 * framespersec)
	framesPerMinute = 60 * fps;
	
	// Calculate minutes
	minutes = frames / framesPerMinute;
	remainingFrames = frames - (minutes * framesPerMinute);
	
	// Calculate seconds
	seconds = remainingFrames / fps;
	remainingFrames = remainingFrames - (seconds * fps);
	
	// Format minutes (2 digits, no zero-pad)
	print_int_as_string_maybe(tempBuf, minutes, 0, 2);
	strcpy(dest, tempBuf);
	
	// Add colon separator
	strcat(dest, ":");
	
	// Format seconds (2 digits, zero-padded)
	print_int_as_string_maybe(tempBuf, seconds, 1, 2);
	strcat(dest, tempBuf);
	
	// Optionally add fractions
	if (showFractions != 0) {
		unsigned short fraction;
		strcat(dest, ".");
		
		// Calculate fraction as hundredths
		fraction = (100 / fps) * remainingFrames;
		print_int_as_string_maybe(tempBuf, fraction, 1, 2);
		strcat(dest, tempBuf);
	}
}

// Wait for input to be released
/** @brief Spin-wait until all keys and buttons are released
 */
void check_input(void) {
	char done;
	do {
		short flags;
		/* Pump SDL events so kbinput key-release events are processed.
		 * Without this, kbinput[ENTER] stays 1 forever when the first
		 * branch is taken and SDL_KEYUP is never seen. */
		kb_poll_sdl_input();
		flags = get_kb_or_joy_flags();
		if ((flags & 48) != 0) {
			done = 1;
		} else {
			unsigned long delta = timer_get_delta_alt();
			unsigned short result = input_checking((unsigned short)delta);
			if (result != 0) {
				done = 1;
			} else if (kbormouse != 0 && (mouse_butstate & 3) != 0) {
				done = 1;
			} else {
				done = 0;
			}
		}
	} while (done != 0);
}

/** @brief Check input with processing - wrapper around input_checking
 * @param delta Time delta for input processing
 * @return Input code received
 */
int input_do_checking(int delta) {
	return input_checking(delta);
}

/** @brief Wait for input with a timeout period
 * @param timeout Timeout value in ticks
 * @return Input code if received, 0 if timeout
 */
unsigned short input_repeat_check(unsigned short timeout) {
	unsigned short totalTime = 0;
	unsigned short result;
	unsigned short delta;
	unsigned short spin_guard = 0;
	
	// Initial timer call to reset
	timer_get_delta_alt();
	
	while (timeout > totalTime) {
		delta = (unsigned short)timer_get_delta_alt();
		if (delta == 0) {
			spin_guard++;
			if (spin_guard >= 2048) {
				delta = 1;
				spin_guard = 0;
			}
		} else {
			spin_guard = 0;
		}

		totalTime += delta;
		result = input_do_checking(delta);
		if (result != 0) {
			return result;
		}
	}

	return 0;
}

/** @brief Blit sprite to screen with dithered transition animation
 * @param sprite Sprite to display
/** @brief Mode.
 * @param immediate Parameter `immediate`.
 * @param interrupted Parameter `interrupted`.
 * @param mode Parameter `mode`.
 * @return Function result.
 */
unsigned short sprite_blit_to_video(struct SPRITE * sprite, unsigned short mode) {
	short i;
	unsigned short delta;
	unsigned short input;
	const unsigned short transition_present_repeats = 4;
	if (sprite == 0 || sprite->sprite_bitmapptr == 0) {
		return 0;
	}

	sprite_copy_2_to_1();
	mouse_draw_opaque_check();

	if (mode == 65534) {
		sprite_putimage(sprite->sprite_bitmapptr);
		mouse_draw_transparent_check();
		video_refresh();
		return 0;
	}

	#ifdef _WIN32
	for (i = 0; i < 4; i++) {
		sprite_draw_dithered_pass(i, sprite);
		mouse_draw_transparent_check();
		video_refresh();
		if (i != 3) {
			mouse_draw_opaque_check();
		}
	}
	return 0;
	#endif

	// Animation loop
	for (i = 0; i < 4; i++) {
		unsigned short hold;

		sprite_draw_dithered_pass(i, sprite);

		for (hold = 0; hold < transition_present_repeats; hold++) {
			mouse_draw_transparent_check();
			video_refresh();

			delta = timer_get_delta_alt();
			if (delta == 0) {
				delta = 1;
			}
			input = input_do_checking(delta);
			if (input != 0) {
				sprite_copy_2_to_1();
				sprite_putimage(sprite->sprite_bitmapptr);
				mouse_draw_transparent_check();
				video_refresh();
				return input;
			}

			if (hold + 1 < transition_present_repeats || i != 3) {
				mouse_draw_opaque_check();
			}
		}
	}
	return 0;
}

// --- Dialog functions ---

// Show key configuration dialog
/** @brief Show keyboard configuration dialog
 */
void do_key_restext(void) {
	void * textptr;

	input_push_status();
	game_startup_flag = 1;
	audio_disable_flag2();

	textptr = locate_text_res(mainresptr, "key");
	ui_dialog_timed(textptr);

	joystick_assigned_flags = 0;
	mouse_motion_state_flag = 0;
	game_startup_flag = 0;
	audio_enable_flag2();
	input_pop_status();
}

// Show mouse configuration dialog
/** @brief Show mouse configuration dialog
 */
void do_mou_restext(void) {
	void * textptr;

	input_push_status();
	game_startup_flag = 1;
	audio_disable_flag2();
	mouse_motion_state_flag = 1;

	textptr = locate_text_res(mainresptr, "mou");
	ui_dialog_timed(textptr);

	game_startup_flag = 0;
	audio_enable_flag2();
	input_pop_status();
}

// Show pause text resource
/** @brief Show pause text dialog
 */
void do_pau_restext(void) {
	void * textptr;

	input_push_status();
	game_startup_flag = 1;
	audio_disable_flag2();

	textptr = locate_text_res(mainresptr, "pau");
	ui_dialog_timed(textptr);

	game_startup_flag = 0;
	audio_enable_flag2();
	input_pop_status();
}

// Show music on/off dialog
/** @brief Show music on/off toggle dialog
 */
void do_mof_restext(void) {
	void * textptr;

	input_push_status();
	game_startup_flag = 1;

	if (audio_toggle_flag2() != 0) {
		textptr = locate_text_res(mainresptr, "mon");
	} else {
		textptr = locate_text_res(mainresptr, "mof");
	}

	ui_dialog_timed(textptr);

	game_startup_flag = 0;
	input_pop_status();
}

// Show sons of dialog
/** @brief Show sound on/off toggle dialog
 */
void do_sonsof_restext(void) {
	void * textptr;

	input_push_status();
	game_startup_flag = 1;
	audio_disable_flag2();

	textptr = locate_text_res(mainresptr, "son");
	ui_dialog_timed(textptr);

	game_startup_flag = 0;
	audio_enable_flag2();
	input_pop_status();
}

// Show DOS text resource dialog
/** @brief Show DOS quit confirmation dialog
 */
void do_dos_restext(void) {
	void * textptr;

	input_push_status();
	game_startup_flag = 1;
	audio_disable_flag2();

	textptr = locate_text_res(mainresptr, "dos");
	ui_dialog_timed(textptr);

	game_startup_flag = 0;
	audio_enable_flag2();
	input_pop_status();
}

/** @brief Show memory error text dialog
 */
void do_mer_restext(void) {
	void * textptr;

	input_push_status();
	game_startup_flag = 1;
	audio_disable_flag2();

	textptr = locate_text_res(mainresptr, "mer");
	ui_dialog_timed(textptr);

	game_startup_flag = 0;
	audio_enable_flag2();
	input_pop_status();
}

/** @brief Show disk error dialog with abort/retry option
 * @return 0 = cancel/abort, 1 = retry, 2 = special abort
 */
short  do_dea_textres(void) {
	void * textptr;
	short result;

	input_push_status();

	if (g_is_busy != 0) {
		// Show "disk error, abort/retry" dialog ("dea" resource)
		textptr = locate_text_res(mainresptr, "dea");
		result = ui_dialog_confirm_restext(textptr, 0);
		if (result != 0) {
			result = 0;  // User hit retry - return 0
		}
		// result = 0 means abort (user chose cancel)
	} else {
		// Show "disk error" info dialog ("der" resource)
		textptr = locate_text_res(mainresptr, "der");
		ui_dialog_info_restext(textptr);
		result = 1;
	}

	input_pop_status();
	return result;
}

/** @brief Read a file with disk error retry support
/** @brief Code.
 * @param retry Parameter `retry`.
 * @param data Parameter `data`.
 * @param buffer Parameter `buffer`.
 * @return Function result.
 */
void * file_read_with_mode(unsigned short op, const char* filename, void * buffer) {
	void * result;
	
	while (1) {
		if (op == 9) {
			// Read with retry support
			result = file_read_nofatal(filename, buffer);
			if (result != NULL) {
				return result;
			}
			// File read failed - show disk error dialog
			if (do_dea_textres() == 2) {
				// User chose abort
				return NULL;
			}
			// User chose retry - loop again
		} else if (op == 10) {
			// Direct read without retry
			return file_read_nofatal(filename, buffer);
		} else {
			// Invalid operation - fall through to error check
			break;
		}
	}
	
	return NULL;
}

/** @brief Draw a 3-color beveled rectangle border
 * @param x Left coordinate
 * @param y Top coordinate
 * @param w Width
 * @param h Height
 * @param col1 Top/left highlight color
 * @param col2 Middle border color
 * @param col3 Bottom/right shadow color
 */
void draw_beveled_border(unsigned short x, unsigned short y, unsigned short w, unsigned short h, unsigned short col1, unsigned short col2, unsigned short col3) {
	unsigned short right = x + w;
	unsigned short bottom = y + h;
	
	// Top horizontal lines
	preRender_line(x, y, right, y, col1);
	preRender_line(x + 1, y + 1, right - 1, y + 1, col1);
	preRender_line(x + 2, y + 2, right - 2, y + 2, col2);
	
	// Left vertical lines
	preRender_line(x, y, x, bottom, col1);
	preRender_line(x + 1, y + 1, x + 1, bottom - 1, col1);
	preRender_line(x + 2, y + 2, x + 2, bottom - 2, col2);
	
	// Bottom horizontal lines
	preRender_line(x, bottom, right, bottom, col3);
	preRender_line(x + 1, bottom - 1, right - 1, bottom - 1, col3);
	preRender_line(x + 2, bottom - 2, right - 2, bottom - 2, col2);
	
	// Right vertical lines
	preRender_line(right, y, right, bottom, col3);
	preRender_line(right - 1, y + 1, right - 1, bottom - 1, col3);
	preRender_line(right - 2, y + 2, right - 2, bottom - 2, col2);
}

/** @brief Load car aerodynamic data and compute drag table
 * @param carresptr Car resource data pointer
 * @param is_opponent Non-zero if setting up opponent car
 */
void setup_aero_trackdata(void * carresptr, int is_opponent) {
	int i;
	if (is_opponent == 0) {
		if (!simd_copy_from_resource(&simd_player, sizeof(simd_player), locate_shape_alt(carresptr, "simd"))) {
			fatal_error("setup_aero_trackdata: missing player simd resource");
		}
		simd_player.aerorestable = (uintptr_t)aero_table_player;
		// Maximum speed is 40h
		// Division by 2^9.
		// 2^8 shifts one fullbyte, and it is known there is a 1/2 factor in FDrag.
		for (i = 0; i < 64; i++) {
			aero_table_player[i] = ((long)simd_player.aero_resistance * (long)i * (long)i) >> 9;
		}

		copy_string(gnam_string, locate_shape_alt(carresptr, "gnam"));
	} else {
		if (!simd_copy_from_resource(&simd_opponent_rt, sizeof(simd_opponent_rt), locate_shape_alt(carresptr, "simd"))) {
			fatal_error("setup_aero_trackdata: missing opponent simd resource");
		}
		simd_opponent_rt.aerorestable = (uintptr_t)aero_table_opponent;

		for (i = 0; i < 64; i++) {
			aero_table_opponent[i] = ((long)simd_opponent_rt.aero_resistance * (long)i * (long)i) >> 9;
		}
		copy_string(gsna_string, locate_shape_alt(carresptr, "gsna"));
	}
}

// Initialize opponent track plan data and car state (ported from seg001 init_plantrak)
/** @brief Initialize opponent track plan and car state
 */
void init_plantrak_(void) {
	unsigned short * td3 = (unsigned short *)track_waypoint_order;
	unsigned char * td17 = (unsigned char *)track_elem_ordered;
	unsigned char * td18 = (unsigned char *)path_conn_flags;
	unsigned char * td21 = (unsigned char *)path_col;
	unsigned char * td22 = (unsigned char *)path_row;
	long posX;
	long posY;
	long posZ;
	unsigned short idx;
	unsigned short td3_entry;

	// Reset game state and point plan tables to resident memory copy.
	init_game_state(-3);
	state.game_inputmode = 2;
	plan_memres[0].plane_normal.y = STN_PLANE_NORMAL_Y;
	planptr = (struct PLANE *)plan_memres;
	current_planptr = plan_memres;

	startcol2 = 1;
	startrow2 = STN_PLAN_START_ROW;

	// Seed path arrays and path_conn_flags the same way as the original asm.
	td17[0] = 7; td21[0] = 1; td22[0] = (unsigned char)startrow2; td18[0] = 0;
	td17[1] = 6; td21[1] = 0; td22[1] = (unsigned char)startrow2; td18[1] = 0;
	td17[2] = 8; td21[2] = 0; td22[2] = (unsigned char)(startrow2 + 1); td18[2] = 0;
	td17[3] = 9; td21[3] = 1; td22[3] = (unsigned char)(startrow2 + 1); td18[3] = 0;
	td17[4] = 7; td21[4] = 1; td22[4] = (unsigned char)startrow2; td18[4] = 0;

	// Build initial track_waypoint_order table (word entries)
	td3[0]  = 0; td3[1]  = 1; td3[2]  = 2; td3[3]  = 3; td3[4]  = 4;
	td3[5]  = 1; td3[6]  = 2; td3[7]  = 3; td3[8]  = 4;
	td3[9]  = 1; td3[10] = 2; td3[11] = 3; td3[12] = 4;
	td3[13] = 0; td3[14] = 1; td3[15] = 2; td3[16] = 3; td3[17] = 0;

	opponent_speed_table[0] = STN_PLAN_SPEED_INIT;

	// Position and orientation setup for opponent car
	posZ = ((long)(trackpos[28] + 302)) << 6;
	posY = 0;
	posX = 96000L;
	init_carstate_from_simd(&state.opponentstate, &simd_opponent_rt, 1, posX, posY, posZ, 0);

	idx = (unsigned short)state.opponentstate.car_waypoint_seq_index;
	td3_entry = td3[idx];
	track_waypoint_lookup(td3_entry, &state.opponentstate.car_waypoint_target, state.opponentstate.car_track_waypoint_index, (short*)&state.game_track_lookup_temp);
	state.opponentstate.car_track_waypoint_index++;
}

/** @brief Full car setup for gameplay including shapes, audio and physics
 * @return 0 on success, non-zero on error
 */
int setup_player_cars(void) {
	void * carresptr;
	unsigned long var_8;

	/* If opponent is enabled but no car was selected, use the player's car
	   (matching original DOS behaviour and the opponent menu "Done" fallback). */
	if (gameconfig.game_opponenttype != 0 && (unsigned char)gameconfig.game_opponentcarid[0] == STN_TRACK_OPPONENT_AUTOSELECT) {
		memcpy(gameconfig.game_opponentcarid, gameconfig.game_playercarid, 4);
		gameconfig.game_opponentmaterial = (char)((gameconfig.game_playermaterial & 1) ^ 1);
		gameconfig.game_opponenttransmission = 0;
	}

	wndsprite = 0;
	ensure_file_exists(2);
	shape3d_load_car_shapes(gameconfig.game_playercarid, gameconfig.game_opponentcarid);
	{
		char carname[8] = "carcoun";
		carname[3] = gameconfig.game_playercarid[0];
		carname[4] = gameconfig.game_playercarid[1];
		carname[5] = gameconfig.game_playercarid[2];
		carname[6] = gameconfig.game_playercarid[3];
		carresptr = file_load_resfile(carname);
		setup_aero_trackdata(carresptr, 0);
		unload_resource(carresptr);

		if (gameconfig.game_opponenttype != 0) {
			carname[3] = gameconfig.game_opponentcarid[0];
			carname[4] = gameconfig.game_opponentcarid[1];
			carname[5] = gameconfig.game_opponentcarid[2];
			carname[6] = gameconfig.game_opponentcarid[3];
			carresptr = file_load_resfile(carname);
			setup_aero_trackdata(carresptr, 1);
			unload_resource(carresptr);

			ensure_file_exists(4);
			load_opponent_data();
		}
	}

	ensure_file_exists(3);
	eng1ptr = file_load_resource(5, "eng1");//aEng1); // "eng1"
	engptr = file_load_resource(6, "eng");//aEng); // "eng"
	audio_add_driver_timer();
	crash_sound_handle = audio_init_engine(33, &audio_adlib_config, eng1ptr, engptr);

	audio_replay_apply_state = 0;
	player_audio_state = 0;
	opponent_audio_state = 0;
	if (gameconfig.game_opponenttype != 0) {
		audio_engine_sound_handle = audio_init_engine(32, &audio_mt32_config, eng1ptr, engptr);
	}

	audio_frame_index = 0;
	menu_selection_buffer = 0;
	sprite_row_offset_table = 0;
	fontledresptr = file_load_resource(0, "fontled.fnt");//aFontled_fnt); // "fontled.fnt"
	timertestflag_copy = timertestflag;
	init_rect_arrays();
	if (idle_expired == 0) {
		setup_car_shapes(0);
	}

	if (idle_expired == 0) {
		sdgameresptr = file_load_resource(3, "sdgame");//aSdgame); // "sdgame"
		loop_game(0, 0, 0);
	}

	gameresptr = file_load_resfile("game");
	planptr = (struct PLANE *)locate_shape_alt(gameresptr, "plan");//aPlan); // "plan"
	wallptr = (short *)locate_shape_alt(gameresptr, "wall");//aWall); // "wall"
	load_sdgame2_shapes();
	load_skybox(track_elem_map[900]);
	if (shape3d_load_all() != 0) {
		return 1;
	}

	if (video_flag5_is0 == 0) {
		
		var_8 = 64000 / (video_flag1_is1 * video_flag4_is1);
		if (mmgr_get_res_ofs_diff_scaled() <= var_8) {
			return 1;
		}
		wndsprite = sprite_make_wnd(STN_SCREEN_WIDTH, STN_SCREEN_HEIGHT, STN_SCREEN_DEPTH);
	}

	followOpponentFlag = 0;
	is_in_replay_copy = -1;
	return 0;
}

/** @brief Free all car-related resources
 */
void free_player_cars(void) {
	if (video_flag5_is0 == 0) {
		if (wndsprite != 0) {
			sprite_free_wnd(wndsprite);
		}
	}

	shape3d_free_all();
	unload_skybox();
	free_sdgame2();
	unload_resource(gameresptr);
	if (idle_expired == 0) {
		mmgr_free(sdgameresptr);
		setup_car_shapes(3);
	}

	mmgr_free(fontledresptr);
	audio_remove_driver_timer();
	mmgr_free(engptr);
	mmgr_free(eng1ptr);
	shape3d_free_car_shapes();
}

void shape2d_render_bmp_as_mask(void * data);

/** @brief Main game loop handling driving, replay and pause
 */
void run_game(void) {
	unsigned short var_16[2];
	int var_12, var_E;
	struct RECTANGLE var_rect;
	int var_2;
	int regsi;
	rect_windshield.left = 0;
	rect_windshield.right = 320;
	var_2 = -1;
	timer_tick_counter = -1;
	var_E = -1;
	run_game_random = get_kevinrandom() << 3;
	replaybar_toggle = 1;
	is_in_replay = 0;
	if (idle_expired == 0) {
		if (gameconfig.game_recordedframes != 0) {
			cameramode = 0;
			game_replay_mode = 2;
			is_in_replay = 1;
		} else {
			cameramode = 0;
			game_replay_mode = 1;
		}
	} else {
		cameramode++;
		if (cameramode == 4) {
			cameramode = 0;
		}

		game_replay_mode = 2;		
		if (file_load_replay(0, "default") != 0) {
			return ;
		}
		track_setup();
	}

	if (setup_player_cars() != 0) {
		free_player_cars();
		do_mer_restext();
	} else {
		kbormouse = 0;
		screen_shake_intensity = 0;
		game_finish_state = 1;
		set_frame_callback();
		game_replay_mode_copy = -1;
		current_screen_buffer_selector = 0;
		screen_display_toggle_flag = 0;
		checkpoint_lap_trigger = 0;
		dashb_toggle = 0;

		if (idle_expired != 0) {
			framespersec = gameconfig.game_framespersec;

			init_game_state(-1);
		} else {
			if (is_in_replay == 0) {
				cameramode = 0;
				dashb_toggle = 1;
				show_penalty_counter = 0;
				framespersec = framespersec2;
				gameconfig.game_framespersec = framespersec2;
				init_game_state(-1);
				sprite_animation_frame_counter = 0;
				*(char*)&lap_completion_trigger_flag = 0; // byte ptr!
				game_pause_counter = 1;
				mouse_minmax_position(mouse_motion_state_flag);
				game_replay_mode = 1;
				
				state.playerstate.car_posWorld1.lx += multiply_and_scale(sin_fast(track_angle), -240) << STN_WORLD_SCALE_SHIFT;
				state.playerstate.car_posWorld1.lz += multiply_and_scale(cos_fast(track_angle), -240) << STN_WORLD_SCALE_SHIFT;
				state.playerstate.car_posWorld1.ly += STN_CAR_START_Y_LIFT;
				replay_mode_state_flag = 1;
			} else {
				cameramode = 0;
				game_replay_mode = 2;
				current_rotation_angle_value = 500;
				framespersec = gameconfig.game_framespersec;
				restore_gamestate(0);
				restore_gamestate(gameconfig.game_recordedframes);

				while (gameconfig.game_recordedframes != state.game_frame) {
					if (input_do_checking(1) == 27)
						break;
					update_gamestate();
				}

				replay_frame_counter = gameconfig.game_recordedframes;

				/* Rewind to frame 0 and enable auto-play so the user
				 * sees the replay animate from the beginning.
				 * is_in_replay stays 1 so the replay bar remains
				 * interactive; replay_autoplay_active lets
				 * frame_callback bypass the is_in_replay gate. */
				restore_gamestate(0);
				replay_frame_counter = 0;
				replay_autoplay_active = 1;
			}
		}

		while (1) {

			timer_get_counter(); /* drive timer ISR: frame_callback → replay_capture_frame_input → replay_frame_counter++ */

			if (state.game_frame != replay_frame_counter) {
				if ((mouse_motion_state_flag != 0 || joystick_assigned_flags != 0) && game_replay_mode == 0) {
					replay_apply_steering_correction();
				}
				update_gamestate();
				/* Safety valve: if physics is significantly behind rendering,
				 * yield briefly so the render path is not starved. */
				if ((int)(replay_frame_counter - state.game_frame) > 2) {
					struct timespec ts_yield = {0, GAME_YIELD_NS};
					nanosleep(&ts_yield, NULL);
				}
				continue;
			}
			
			
			if (state.game_inputmode == 0 && game_replay_mode == 0) {
				replay_frame_counter = 0;
				gameconfig.game_recordedframes = 0;
				state.game_frame = 0;
			}

			if (timertestflag_copy != timertestflag) {
				timertestflag_copy = timertestflag;
				init_rect_arrays();
			}

			if (checkpoint_lap_trigger != 0) {
				input_push_status();
				audio_disable_flag2();
				regsi = ui_dialog_confirm_restext(locate_text_res(gameresptr, "rbf"), 0);
				if (regsi == -1)
					regsi = 0;

				audio_enable_flag2();
				game_startup_flag = 0;
				input_pop_status();
				if (regsi != 0) {
					update_crash_state(4, 0);
					game_finish_state = 1;
				}

				checkpoint_lap_trigger = 0;
			}

			if (video_flag5_is0 != 0) {
				setup_mcgawnd2();
				screen_display_toggle_flag = current_screen_buffer_selector;
			} else {
				sprite_select_wnd_as_sprite1();
			}

			if (game_replay_mode != game_replay_mode_copy || dashb_toggle != dashb_toggle_copy || replaybar_toggle != replaybar_toggle_copy || is_in_replay != is_in_replay_copy || followOpponentFlag != followOpponentFlag_copy) {
				game_replay_mode_copy = game_replay_mode;
				dashb_toggle_copy = dashb_toggle;
				replaybar_toggle_copy = replaybar_toggle;
				is_in_replay_copy = is_in_replay;
				followOpponentFlag_copy = followOpponentFlag;
				roofbmpheight_copy = 0;
				game_input_keyboard_state = 0;

				if (game_replay_mode != 2 || idle_expired != 0 || (replaybar_toggle == 0 && is_in_replay == 0)) {
					replaybar_enabled = 0;
			} else {
				replaybar_enabled = 1;
			}
				if (dashb_toggle == 0 || followOpponentFlag != 0) {
					if (game_replay_mode == 2) {
						if (replaybar_enabled != 0) {
							dashbmp_y_copy = STN_DASH_REPLAYBAR_Y;
						} else {
							dashbmp_y_copy = STN_SCREEN_HEIGHT;
						}
					} else {
						dashbmp_y_copy = STN_SCREEN_HEIGHT;
					}
				} else {
					if (game_replay_mode != 2 || replaybar_enabled == 0) {
						height_above_replaybar = 200;
					} else {
						height_above_replaybar = 151;
					}

					game_input_keyboard_state = 1;
					roofbmpheight_copy = roofbmpheight;
					dashbmp_y_copy = dashbmp_y;
				}

				if (var_2 != roofbmpheight_copy || dashbmp_y_copy != timer_tick_counter || var_E != height_above_replaybar) {
					race_condition_state_flag = video_flag6_is1;
					set_projection(35, dashbmp_y_copy / 6, STN_SCREEN_WIDTH, dashbmp_y_copy);
					rect_windshield.top = roofbmpheight_copy;
					rect_windshield.bottom = dashbmp_y_copy;
					var_2 = roofbmpheight_copy;
					timer_tick_counter = dashbmp_y_copy;
					var_E = height_above_replaybar;
				}
			}

			if (race_condition_state_flag != 0) {
				mouse_button_press_state[screen_display_toggle_flag] = 0;
				if (game_input_keyboard_state != 0) {
					sprite_set_1_size(0, STN_SCREEN_WIDTH, dashbmp_y_copy, height_above_replaybar);
					setup_car_shapes(1);
				}

				if (replaybar_enabled != 0) {
					sprite_set_1_size(0, STN_SCREEN_WIDTH, 0, STN_SCREEN_HEIGHT);
					loop_game(1, state.game_frame, state.game_frame);
				}
			} else {
				if (replaybar_enabled == 0) {
					mouse_button_press_state[screen_display_toggle_flag] = 0;
				}
			}

			update_frame(current_screen_buffer_selector, &rect_windshield);

			if (dastbmp_y != 0 && game_input_keyboard_state != 0) {			
				if (timertestflag_copy != 0) {
					var_rect.left = 0;
					var_rect.right = STN_SCREEN_WIDTH;
					var_rect.top = dastbmp_y;
					var_rect.bottom = dashbmp_y_copy;
					if (rect_buffer_primary != 0) {
						rect_union(rect_buffer_primary, &var_rect, rect_buffer_primary);
					}
				}

				shape2d_render_bmp_as_mask(dasmshapeptr);
				shape2d_draw_rle_or((struct SHAPE2D *)g_dast_shape_ptr);
			}

			render_present_ingame_view(&rect_windshield);
			if (game_input_keyboard_state != 0) {
				sprite_set_1_size(0, STN_SCREEN_WIDTH, dashbmp_y_copy, height_above_replaybar);
				setup_car_shapes(2);
				sprite_set_1_size(0, STN_SCREEN_WIDTH, 0, STN_SCREEN_HEIGHT);
			}

			if (race_condition_state_flag != 0) {
				race_condition_state_flag--;
			}

			if (video_flag5_is0 != 0) {
				mouse_draw_opaque_check();
				setup_mcgawnd1();
				current_screen_buffer_selector ^= 1;
				screen_display_toggle_flag = current_screen_buffer_selector;
				mouse_draw_transparent_check();
			}

			if (game_replay_mode == 1 && game_pause_counter == 0) {
				game_replay_mode = 0;
				framespersec = framespersec2;
				gameconfig.game_framespersec = framespersec2;
				init_game_state(-1);
			}

			if (idle_expired == 0) {
				if (game_finish_state != 0) {

					if ((game_replay_mode != 0 || state.game_3F6autoLoadEvalFlag == 4) && game_finish_state != 2) {
						game_finish_state = 0;
						game_replay_mode = 2;
						mouse_minmax_position(0);
						loop_game(0, 0, 0);
						loop_game(2, 4, 0);
						is_in_replay = 1;
						audio_sync_car_audio();
					} else {
						break;
					}
				}

				if (game_replay_mode == 2) {
					loop_game(3, 0, 0);
					continue;
				}

				do {
					var_12 = kb_get_char();
					if (var_12 != 0 && var_12 != UI_KEY_ESCAPE_EXT && var_12 != UI_KEY_ESCAPE) {
						handle_ingame_kb_shortcuts(var_12);
					}
					if (var_12 == UI_KEY_ESCAPE_EXT || var_12 == UI_KEY_ESCAPE) {
						/* ESC during driving: switch to replay mode.
						 * loop_game(3,...) will handle the menu on next iteration. */
						game_replay_mode = 2;
						mouse_minmax_position(0);
						loop_game(0, 0, 0);
						loop_game(2, 4, 0);
						is_in_replay = 1;
						audio_sync_car_audio();
						/* Re-queue ESC so loop_game(3,...) sees it */
						kb_sdl_requeue_key(UI_KEY_ESCAPE_EXT);
						break;
					}

				} while (var_12 == UI_KEY_UP || var_12 == UI_KEY_LEFT || var_12 == UI_KEY_RIGHT || var_12 == UI_KEY_DOWN);

				if (game_replay_mode == 1) {
					mouse_get_state(&mouse_butstate, &mouse_xpos, &mouse_ypos);
					if (((mouse_butstate & 3) != 0) || ((get_kb_or_joy_flags() & 48) != 0)) {
						game_replay_mode = 0;
						game_pause_counter = 0;
						framespersec = framespersec2;
						gameconfig.game_framespersec = framespersec2;
						init_game_state(-1);
					}
				}

			} else {
				if (kb_get_char() != 0 || game_finish_state != 0 || get_kb_or_joy_flags() != 0) {
					break;
				}
			}
		}

		if (video_flag5_is0 != 0) {
			mouse_draw_opaque_check();
			setup_mcgawnd2();
			sprite_copy_rect_2_to_1(0, 0, 320, 200, 0);
			setup_mcgawnd1();
			mouse_draw_transparent_check();
		}

		sprite_copy_2_to_1();
		is_in_replay = 1;
		audio_sync_car_audio();
		audio_remove_driver_timer();
		if (game_replay_mode == 0 && gameconfig.game_opponenttype != 0 && state.opponentstate.car_crashBmpFlag == 0) {
			ui_dialog_show_restext(3, 0, locate_text_res(gameresptr, "cop"), UI_DIALOG_AUTO_POS, 80, performGraphColor, var_16, 0);
			*(char*)&lap_completion_trigger_flag = 1;
			regsi = framespersec;
			regsi--;

			while (1) {
				replay_capture_frame_input(1);
				update_gamestate();
				regsi++;
				if (regsi == framespersec) {
					regsi = 0;
					format_frame_as_string(resID_byte1, state.game_frame + elapsed_time1, 1);
					mouse_draw_opaque_check();
					sprite_draw_text_opaque(resID_byte1, font_get_centered_x(resID_byte1), var_16[1]);
					mouse_draw_transparent_check();
				}

				if (input_do_checking(1) == 27)
					break;
				if (state.opponentstate.car_crashBmpFlag != 0)
					break;
				if (1500 * framespersec == state.game_frame + elapsed_time1)
					break;
			}
		}

		*(char*)&lap_completion_trigger_flag = 0; // byte ptr 
		mouse_minmax_position(0);
		remove_frame_callback();
		free_player_cars();
	}

	waitflag = STN_WAIT_DIALOG_Y;
	check_input();
	show_waiting();

	return ;
}

/* Storage for the address where divide-by-zero occurred */

/** @brief Copy material colour and pattern list pointers
 * @param clrlist Destination color list pointer
 * @param clrlist2 Destination color list 2 pointer
 * @param patlist Destination pattern list pointer
 * @param patlist2 Destination pattern list 2 pointer
 * @param direction Copy direction flag
 */
void copy_material_list_pointers(void* clrlist, void* clrlist2, void* patlist, void* patlist2, unsigned short videoConst)
{
	material_clrlist_ptr_cpy = clrlist;
	material_clrlist2_ptr_cpy = clrlist2;
	material_patlist_ptr_cpy = patlist;
	material_patlist2_ptr_cpy = patlist2;
	someZeroVideoConst = videoConst;
}

/**
 * @brief Load palette and mouse cursor sprites
 * 
 * Loads the "sdmain" resource file, extracts the palette and
/** @brief Sprites.
 * @param smou Parameter `smou`.
 * @param load_palandcursor Parameter `load_palandcursor`.
 * @return Function result.
 */
void load_palandcursor(void)
{
	unsigned char palette[STN_PALETTE_BYTES];
	char * shapedata;
	char * palptr;
	struct SHAPE2D * palshape;
	unsigned char * paldata;
	unsigned long palbytes;
	char * smou_shape;
	unsigned short sprite_width;
	unsigned short sprite_height;
	unsigned short i;
	
	/* Load sdmain shape file for palette */
	shapedata = file_load_shape2d_fatal_thunk("sdmain");
	
	/* Find palette shape and copy its data */
	palptr = locate_shape_fatal(shapedata, "!pal");
	palshape = (struct SHAPE2D *)palptr;
	paldata = (unsigned char *)palptr + sizeof(struct SHAPE2D);
	palbytes = (unsigned long)palshape->s2d_width * (unsigned long)palshape->s2d_height;
	if (palbytes > STN_PALETTE_BYTES) {
		palbytes = STN_PALETTE_BYTES;
	}
	memset(palette, 0, sizeof(palette));
	for (i = 0; i < (unsigned short)palbytes; i++) {
		palette[i] = paldata[i];
	}
	
	/* Set the VGA palette */
	video_set_palette(0, STN_PALETTE_COLORS, palette);
	
	/* Find smou (small mouse cursor) shape to get dimensions */
	smou_shape = locate_shape_fatal(shapedata, "smou");
	sprite_width = *(unsigned short *)smou_shape * video_flag2_is1;
	sprite_height = *(unsigned short *)(smou_shape + 2);
	
	/* Free the shape file before allocating sprite buffers */
	mmgr_free(shapedata);
	
	/* Create sprite buffers for mouse cursors */
	smouspriteptr = sprite_make_wnd(sprite_width, sprite_height, STN_SCREEN_DEPTH);
	mmouspriteptr = sprite_make_wnd(sprite_width, sprite_height, STN_SCREEN_DEPTH);
	mouseunkspriteptr = sprite_make_wnd(sprite_width + video_flag2_is1, sprite_height, STN_SCREEN_DEPTH);
	
	/* Reload sdmain to render cursor shapes */
	shapedata = file_load_shape2d_fatal_thunk("sdmain");
	
	/* Render smou shape to smouspriteptr */
	sprite_set_1_from_argptr(smouspriteptr);
	smou_shape = locate_shape_fatal(shapedata, "smou");
	sprite_shape_to_1((struct SHAPE2D *)smou_shape, 0, 0);
	
	/* Render mmou shape to mmouspriteptr */
	sprite_set_1_from_argptr(mmouspriteptr);
	smou_shape = locate_shape_fatal(shapedata, "mmou");
	sprite_shape_to_1((struct SHAPE2D *)smou_shape, 0, 0);
	
	/* Free shape file and restore sprite state */
	mmgr_free(shapedata);
	sprite_copy_2_to_1();
}

/** @brief Minimal palette-only loader without cursor sprites
 */
static void __attribute__((unused)) load_palandcursor_minimal(void)
{
	unsigned char palette[STN_PALETTE_BYTES];
	char * shapedata;
	char * palptr;
	struct SHAPE2D * palshape;
	unsigned char * paldata;
	unsigned long palbytes;
	unsigned short i;

	shapedata = file_load_shape2d_fatal_thunk("sdmain");
	palptr = locate_shape_fatal(shapedata, "!pal");
	palshape = (struct SHAPE2D *)palptr;
	paldata = (unsigned char *)palptr + sizeof(struct SHAPE2D);
	palbytes = (unsigned long)palshape->s2d_width * (unsigned long)palshape->s2d_height;
	if (palbytes > STN_PALETTE_BYTES) {
		palbytes = STN_PALETTE_BYTES;
	}
	memset(palette, 0, sizeof(palette));
	for (i = 0; i < (unsigned short)palbytes; i++) {
		palette[i] = paldata[i];
	}

	video_set_palette(0, STN_PALETTE_COLORS, palette);
	mmgr_free(shapedata);
}

/**
/** @brief Functions.
 * @param nullsub_1 Parameter `nullsub_1`.
 * @param nullsub_1 Parameter `nullsub_1`.
 * @return Function result.
 */
void nullsub_1(void) { }
void nullsub_2(void * resptr, unsigned short idx) { (void)resptr; (void)idx; }

/** @brief Main initialization for keyboard, video, audio and timing
 * @param argc Argument count
 * @param argv Argument vector
 */
void init_main(int argc, char* argv[])
{
	unsigned int i;
	unsigned char argmode4, argnosound, argnounknown;
	unsigned long timerdelta1, timerdelta2, timerdelta3;

	// Keyboard
	kb_init_interrupt();
	kb_shift_checking2();
	kb_call_readchar_callback();
	kb_reg_callback(7, &do_mrl_textres);
	kb_reg_callback(10, &do_joy_restext);
	kb_reg_callback(11, &do_key_restext);
	kb_reg_callback(12800, &do_mof_restext);
	kb_reg_callback(16, &do_pau_restext);
	kb_reg_callback('p', &do_pau_restext);
	kb_reg_callback(17, &do_dos_restext);
	kb_reg_callback(19, &do_sonsof_restext);
	kb_reg_callback(24, &do_dos_restext);
	
	// Video
	video_flag1_is1 = 1;
	video_flag2_is1 = 1;
	video_flag3_isFFFF = -1;
	video_flag4_is1 = 1;

	mmgr_alloc_a000();
	
	video_flag5_is0 = 0;
	video_flag6_is1 = 1;
	
	textresprefix = 'e';
	
	// Parse arguments.
	argmode4 = 0;
	argnosound = 0;
	argnounknown = 0;
	
	for (i = 1; i < (unsigned int)argc; ++i) {
		if (argv[i][0] == '/') {
			switch (argv[i][1]) {
				case 'h':
					argmode4 = 4;
					break;

				case 'n':
					if (argv[i][2] == 's') {
						argnosound = 1;
					}
					else if (argv[i][2] == 'd') {
						argnounknown = 1;
					}
					break;

				case 's':
					if ((((g_ascii_props[(unsigned char)argv[i][2]] & RST_ASC_CHAR_UPPER) ? (argv[i][2] + ' ') : (argv[i][2])) == 's')
					 && (((g_ascii_props[(unsigned char)argv[i][3]] & RST_ASC_CHAR_UPPER) ? (argv[i][3] + ' ') : (argv[i][3])) == 'b')) {
						audiodriverstring[0] = argv[i][2];
						audiodriverstring[1] = argv[i][3];
					}
					else {
						audiodriverstring[0] = 'a';
						audiodriverstring[1] = 'd';
					}
					break;
			}
		}
	}
	
	video_sdl_init();
	(void)argmode4;
	(void)argnounknown;
	
	sprite_copy_2_to_1_clear();

	mouse_init(320, 200);

	// Audio driver.
	if (audio_load_driver(audiodriverstring, 0, 0)) {
		timer_stop_dispatch();
		argnosound = 1;
	}
	
	if (argnosound) {
		audio_toggle_flag2();
	}

	load_palandcursor();
	
	// Timing measures.
	sprite_copy_2_to_1();
	sprite_set_1_size(0, 320, 0, 120);

	timer_get_delta_alt();
	/*
	 * Keep DOS startup stable: skip sprite clear stress operation here because
	 * it currently causes immediate termination in this port.
	 */
	timerdelta1 = timer_get_delta_alt();

	/*
	 * Timing calibration fallback:
	 * the original stress loops are unstable in the current DOS port runtime.
	 * Keep startup deterministic and continue with sane timing defaults.
	 */
	timerdelta2 = timerdelta1 + 1;
	timerdelta3 = 34;
	
	timertestflag = (timerdelta2 <= timerdelta1);
	framespersec2 = (timerdelta3 >= 75) ? 10 : 20;

	if (timerdelta3 < 35) {
		timertestflag2 = 0;
	}
	else if (timerdelta3 < 55) {
		timertestflag2 = 1;
	}
	else if (timerdelta3 < 75) {
		timertestflag2 = 2;
	}
	else if (timerdelta3 < 100) {
		timertestflag2 = 3;
	}
	else if (timertestflag) {
		timertestflag2 = 4;
	}
	else {
		timertestflag2 = 3;
	}

	framespersec = framespersec2;
	timertestflag_copy = timertestflag;

	random_wait();
	
	copy_material_list_pointers(material_clrlist_ptr, material_clrlist2_ptr, material_patlist_ptr, material_patlist2_ptr, 0);
}

/** @brief Program entry point
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 */
int main(int argc, char* argv[]) {

	int i, result;
	int regsi;
	char var_A;
	
	//return stuntsmain_(argc, argv);
	init_main(argc, argv);
	init_row_tables();
	
	mainresptr = file_load_resfile("main");
	
	fontdefptr = file_load_resource(0, "fontdef.fnt");
	fontnptr = file_load_resource(0, "fontn.fnt");

	font_set_fontdef();
	init_polyinfo();
	
	init_trackdata();

	reset_replay_runtime_state();
	
	init_kevinrandom("kevin");
	
	strcpy(gameconfig.game_trackname, STN_DEFAULT_TRACK_NAME);
	
	input_do_checking(1);
	input_do_checking(1);
	mouse_draw_opaque_check();
	
	kbormouse = 0;
	passed_security = 1;  // set to 0 for the original copy protection
	set_default_car();
	if (!stn_persist_load()) {
		stn_persist_save();
	}

	regsi = 1;

	while (1) {
		ensure_file_exists(2);
		
		if (regsi != 0) {
			file_build_path(track_highscore_path_buffer, gameconfig.game_trackname, ".trk", g_path_buf);
			file_read_fatal(g_path_buf, track_elem_map);
		}
		
		idle_expired = 0;
		
		result = run_intro_looped_();

		if (result == 27) {
			/*
			 * Avoid immediate DOS exit on spurious early ESC during unstable
			 * startup/input state; continue to menu flow instead.
			 */
			regsi = 0;
			continue;
		}

		while (1) {
			/* Clear latched input codes before entering menu polling to avoid click-through */
			mousebutinputcode = 0;
			joyinputcode = 0;
			newjoyflags = 0;
			mouse_oldbut = mouse_butstate;
			ensure_file_exists(2);
			if (is_audioloaded == 0) {
				file_load_audiores("skidslct", "skidms", "SLCT");
			}
			result = run_menu_();
			if (((unsigned char)result) == 255)  {
				audio_unload();
				regsi = 0;
				break;
			} else if (result == 0) {
				var_A = 0;
			} else if (result == 1) {
				check_input();
				show_waiting();
				run_car_menu(
					&gameconfig.game_playercarid[0],
					(unsigned char*)&gameconfig.game_playermaterial,
					(unsigned char*)&gameconfig.game_playertransmission,
					0
				);
				stn_persist_save();
				check_input();
				continue;
			} else if (result == 2) {
				check_input();
				show_waiting();
				run_opponent_menu();
				stn_persist_save();
				check_input();
				continue;
			} else if (result == 3) {
				check_input();
				run_tracks_menu(0);
				stn_persist_save();
				check_input();
				continue;
			} else if (result == 4) {
				check_input();
				show_waiting();
				result = run_option_menu_();
				stn_persist_save();
				check_input();
				if (result == 0) {
					continue;
				} else {
					// goto replay-mode if option-menu-result != 0
					var_A = 1;
				}
			} else {
				continue;
			}

			memcpy(&gameconfigcopy, &gameconfig, sizeof(struct GAMEINFO));
			for (i = 0; i < STN_TRACKDATA_BLOCK_MAIN; i++) {
				track_file_append[i] = track_elem_map[i];
			}
			for (i = 0; i < STN_TRACKDATA_TRACKNAME_LEN; i++) {
				track_file_append[i + STN_TRACKDATA_PATH_BASE] = track_highscore_path_buffer[i];
				track_file_append[i + STN_TRACKDATA_REPLAY_BASE] = replay_file_path_buffer[i];
			}
			
			if (idle_expired == 0) {
				result = track_setup();
				//result = setup_track();
				if (result != 0) {
					run_tracks_menu(1);
					continue;
				}
				random_wait();
				if (passed_security == 0) {
					fatal_error("security check");
					//get_super_random();
					//security_check();
				}
			} else if (file_find("tedit.*") == 0) {
				audio_unload();
				regsi = 0;
				break;
			}

			audio_unload();

			cvxptr = mmgr_alloc_resbytes("cvx", sizeof(struct GAMESTATE) * RST_CVX_NUM);
			init_game_state(-1);
			
			if (var_A != 0) {
				replay_mode_state_flag = 0;
 			} else {

				gameconfig.game_recordedframes = 0;
			}

			while (1) {
				show_waiting();
				stn_persist_save();
				run_game();
				stn_persist_save();
				if (idle_expired == 0 && replay_mode_state_flag != 0) {
					result = end_hiscore();
					if (result == 0) {
						// view replay
						replay_mode_state_flag = 4;
						continue;
					} else if (result == 1) {
						// drive
						gameconfig.game_recordedframes = 0;
						continue;
					}
				}
				// main menu
				break;
			}

			memcpy(&gameconfigcopy, &gameconfig, sizeof(struct GAMEINFO));
			for (i = 0; i < STN_TRACKDATA_BLOCK_MAIN; i++) {
				track_elem_map[i] = track_file_append[i];
			}
			for (i = 0; i < STN_TRACKDATA_TRACKNAME_LEN; i++) {
				track_highscore_path_buffer[i] = track_file_append[i + STN_TRACKDATA_PATH_BASE];
				replay_file_path_buffer[i] = track_file_append[i + STN_TRACKDATA_REPLAY_BASE];
			}
			mmgr_release((char *)cvxptr);
			
			if (idle_expired != 0) {
				stn_persist_save();
				regsi = 0;
				break;
			}
		}

	}

}


/** @brief Functions.
 * @param fmt Parameter `fmt`.
 * @return Function result.
 */
/* ── Utility functions (previously in utils.c) ───────────────────────────── */

void fatal_error(const char* fmt, ...)
{
	va_list args;
	fprintf(stderr, "FATAL: ");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
	fflush(stderr);
	abort();
}

/** @brief Stunts itoa.
 * @param value Parameter `value`.
 * @param str Parameter `str`.
 * @param radix Parameter `radix`.
 * @return Function result.
 */
char* stunts_itoa(int value, char* str, int radix)
{
	unsigned int uvalue, digit;
	char tmp[34];
	int pos = 0, out = 0, negative = 0;

	if (str == NULL)
		return NULL;
	if (radix < 2 || radix > 36) {
		str[0] = '\0';
		return str;
	}
	if (value < 0 && radix == 10) {
		negative = 1;
		uvalue = (unsigned int)(-value);
	} else {
		uvalue = (unsigned int)value;
	}
	do {
		digit = uvalue % (unsigned int)radix;
		tmp[pos++] = (char)(digit < 10 ? '0' + digit : 'a' + digit - 10);
		uvalue /= (unsigned int)radix;
	} while (uvalue != 0);
	if (negative) str[out++] = '-';
	while (pos > 0) str[out++] = tmp[--pos];
	str[out] = '\0';
	return str;
}

/** @brief Initialize kevinrandom.
 * @param seed Parameter `seed`.
 */
void init_kevinrandom(const char* seed)
{
	int i;
	for (i = 0; i < 6; ++i)
		g_kevinrandom_seed[i] = (unsigned char)seed[i];
}

/** @brief Get kevinrandom seed.
 * @param seed Parameter `seed`.
 */
void get_kevinrandom_seed(char* seed)
{
	int i;
	for (i = 0; i < 6; ++i)
		seed[i] = (char)g_kevinrandom_seed[i];
}

/** @brief Get kevinrandom.
 * @return Function result.
 */
int get_kevinrandom(void)
{
	g_kevinrandom_seed[4] += g_kevinrandom_seed[5];
	g_kevinrandom_seed[3] += g_kevinrandom_seed[4];
	g_kevinrandom_seed[2] += g_kevinrandom_seed[3];
	g_kevinrandom_seed[1] += g_kevinrandom_seed[2];
	g_kevinrandom_seed[0] += g_kevinrandom_seed[1];
	g_kevinrandom_seed[5]++;
	if (g_kevinrandom_seed[5] == 0) {
		g_kevinrandom_seed[4]++;
		if (g_kevinrandom_seed[4] == 0) {
			g_kevinrandom_seed[3]++;
			if (g_kevinrandom_seed[3] == 0) {
				g_kevinrandom_seed[2]++;
				if (g_kevinrandom_seed[2] == 0) {
					g_kevinrandom_seed[1]++;
					if (g_kevinrandom_seed[1] == 0)
						g_kevinrandom_seed[0]++;
				}
			}
		}
	}
	return g_kevinrandom_seed[0];
}

/** @brief Get super random.
 * @return Function result.
 */
int get_super_random(void)
{
	int val = rand() + get_kevinrandom() + (int)timer_get_counter() + gState_frame;
	return val < 0 ? -val : val;
}

/** @brief Random wait.
 * @return Function result.
 */
int random_wait(void)
{
	int i = (int)(unsigned char)aMisc_1[0];
	while (i--) { rand(); get_kevinrandom(); }
	i &= 255;
	while (i--) { get_kevinrandom(); rand(); }
	return 0;
}
