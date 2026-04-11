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

/*
 * build_track_object - Complete C translation from seg004.asm lines 61-2755
 * 16-bit Borland C medium model (int=short=16-bit)
 */
#include <stddef.h>
#include <stdlib.h>
#include "math.h"
#include "stunts.h"
#include "shape3d.h"
#include "ressources.h"
#include "memmgr.h"

/* Variables moved from data_game.c (private to this translation unit) */
static bool corkFlag = false;


/* file-local data (moved from data_global.c) */
static short loopSurface_maxZ = 449;
static short loopSurface_ZBounds0[6] = { 0, 224, 389, 449, 389, 224 };
static short loopSurface_ZBounds1[6] = { 224, 389, 449, 389, 224, 0 };
static short loopSurface_XBounds0[6] = { -400, -400, -352, -304, -270, -235 };
static short loopSurface_XBounds1[6] = { -400, -352, -304, -270, -235, -200 };
static short loopBase_ZBounds0[6] = { 0, 178, 360, 536, 704, 868 };
static short loopBase_ZBounds1[6] = { 178, 360, 536, 704, 868, 2000 };
static short loopBae_InnXBounds0[6] = { 0, -20, -40, -60, -80, -100 };
static short loopBase_InnXBounds1[6] = { -20, -40, -60, -80, -100, -120 };
static short loopBase_OutXBounds0[6] = { 400, 361, 320, 276, 226, 174 };
static short loopBase_OutXBounds1[6] = { 361, 320, 276, 226, 174, 120 };
static short bkRdEntr_triang_zAdjust[4] = { -251, -84, 84, 251 };
static short corkLR_negZBound[12] = { 0,    -94,  -187, -280, -373, -466,
                                      -559, -652, -745, -838, -931, -1024 };
static short corkLR_posZBound[12] = { 0, 1024, 931, 838, 745, 652, 559, 466, 373, 280, 187, 94 };
static short highEntrZBounds0[6] = { -512, -334, -168, 0, 168, 334 };
static short highEntrZBounds1[6] = { -334, -168, 0, 168, 334, 1000 };
static short highEntrXInnBounds0[6] = { 0, 0, 0, 0, 0, 120 };
static short highEntrXInnBounds1[6] = { 0, 0, 0, 0, 120, 120 };
static short highEntrXOutBounds0[6] = { 120, 168, 216, 264, 312, 360 };
static short highEntrXOutBounds1[6] = { 168, 216, 264, 312, 360, 360 };
static unsigned char phys_model_0B_points[6] = { 0, 0, 0, 0, 0, 0 };
static unsigned char phys_model_0x12_points[48]
    = { 136, 255, 0,   0,   231, 254, 136, 255, 0,   0, 25,  255, 136, 255, 0,   0,
        25,  1,   136, 255, 0,   0,   231, 0,   120, 0, 0,   0,   231, 254, 120, 0,
        0,   0,   25,  255, 120, 0,   0,   0,   25,  1, 120, 0,   0,   0,   231, 0 };
static unsigned char phys_model_0x23_points[12] = { 196, 255, 0, 0, 0, 254, 60, 0, 0, 0, 0, 2 };
static unsigned char phys_model_0x20_points[12] = { 120, 254, 0, 0, 0, 0, 136, 253, 0, 0, 0, 0 };
static unsigned char phys_model_0x21_points[12] = { 136, 1, 0, 0, 0, 0, 120, 2, 0, 0, 0, 0 };
static unsigned char phys_model_0x22_points[24]
    = { 23, 0, 0, 0, 1, 255, 97, 0, 0, 0, 1, 255, 159, 255, 0, 0, 255, 0, 233, 255, 0, 0, 255, 0 };

typedef struct {
    unsigned char source_element;
    unsigned char hill_element;
} BtoHillRoadSubstitution;

static const BtoHillRoadSubstitution hillroad_substitutions[][6] = {
    { { 4, 182 }, { 14, 186 }, { 24, 190 }, { 39, 194 }, { 59, 194 }, { 98, 194 } },
    { { 5, 183 }, { 15, 187 }, { 25, 191 }, { 36, 195 }, { 56, 195 }, { 95, 195 } },
    { { 4, 184 }, { 14, 188 }, { 24, 192 }, { 38, 196 }, { 58, 196 }, { 97, 196 } },
    { { 5, 185 }, { 15, 189 }, { 25, 193 }, { 37, 197 }, { 57, 197 }, { 96, 197 } },
};

/* Grass=4, Water=5, from structs.inc */
#define SURF_GRASS 4
#define SURF_WATER 5

enum {
    BTO_TRACK_SIZE = 30,
    BTO_TRACK_LAST_INDEX = BTO_TRACK_SIZE - 1,
    BTO_TRACKDATA_PATH_COUNT = 901,
    BTO_TRACK_WAYPOINT_ORDER_CAPACITY = 901,
    BTO_WORLD_TO_TILE_SHIFT = 10,
    BTO_MARKER_CORNER = 253,
    BTO_MARKER_VERTICAL = 254,
    BTO_MARKER_HORIZONTAL = 255,
    BTO_ORIENT_0 = 0,
    BTO_ORIENT_90 = 256,
    BTO_ORIENT_180 = 512,
    BTO_ORIENT_270 = 768,
    BTO_ORIENT_MASK = 1023,
    BTO_ORIENT_QUADRANT_SHIFT = 8,
    BTO_PHYS_MODEL_MAX = 74,
    BTO_PHYSMODEL_START_FINISH = 0,
    BTO_PHYSMODEL_ROAD = 1,
    BTO_PHYSMODEL_SHARP_CORNER = 2,
    BTO_PHYSMODEL_LARGE_CORNER = 3,
    BTO_PHYSMODEL_CHICANE_RL = 4,
    BTO_PHYSMODEL_CHICANE_LR = 5,
    BTO_PHYSMODEL_SHARP_SPLIT_A = 6,
    BTO_PHYSMODEL_SHARP_SPLIT_B = 7,
    BTO_PHYSMODEL_LARGE_SPLIT_A = 8,
    BTO_PHYSMODEL_LARGE_SPLIT_B = 9,
    BTO_PHYSMODEL_HIGHWAY_ENTRANCE = 10,
    BTO_PHYSMODEL_HIGHWAY = 11,
    BTO_PHYSMODEL_CROSSROAD = 12,
    BTO_PHYSMODEL_UNUSED_13 = 13,
    BTO_PHYSMODEL_UNUSED_14 = 14,
    BTO_PHYSMODEL_UNUSED_15 = 15,
    BTO_PHYSMODEL_RAMP = 16,
    BTO_PHYSMODEL_SOLID_RAMP = 17,
    BTO_PHYSMODEL_ELEVATED_ROAD = 18,
    BTO_PHYSMODEL_ELEVATED_SPAN = 19,
    BTO_PHYSMODEL_SOLID_ROAD = 20,
    BTO_PHYSMODEL_ELEVATED_CORNER = 21,
    BTO_PHYSMODEL_OVERPASS = 22,
    BTO_PHYSMODEL_BANK_ENTRANCE_B = 23,
    BTO_PHYSMODEL_BANK_ENTRANCE_A = 24,
    BTO_PHYSMODEL_BANK_ROAD = 25,
    BTO_PHYSMODEL_BANK_CORNER = 26,
    BTO_PHYSMODEL_LOOP = 27,
    BTO_PHYSMODEL_TUNNEL = 28,
    BTO_PHYSMODEL_PIPE_ENTRANCE = 29,
    BTO_PHYSMODEL_PIPE = 30,
    BTO_PHYSMODEL_HALFPIPE = 31,
    BTO_PHYSMODEL_CORKSCREW_UD_LH = 32,
    BTO_PHYSMODEL_CORKSCREW_UD_RH = 33,
    BTO_PHYSMODEL_SLALOM = 34,
    BTO_PHYSMODEL_CORKSCREW_LR = 35,
    BTO_PHYSMODEL_UNUSED_36 = 36,
    BTO_PHYSMODEL_UNUSED_37 = 37,
    BTO_PHYSMODEL_UNUSED_38 = 38,
    BTO_PHYSMODEL_UNUSED_39 = 39,
    BTO_PHYSMODEL_UNUSED_40 = 40,
    BTO_PHYSMODEL_UNUSED_41 = 41,
    BTO_PHYSMODEL_UNUSED_42 = 42,
    BTO_PHYSMODEL_UNUSED_43 = 43,
    BTO_PHYSMODEL_UNUSED_44 = 44,
    BTO_PHYSMODEL_UNUSED_45 = 45,
    BTO_PHYSMODEL_UNUSED_46 = 46,
    BTO_PHYSMODEL_UNUSED_47 = 47,
    BTO_PHYSMODEL_UNUSED_48 = 48,
    BTO_PHYSMODEL_UNUSED_49 = 49,
    BTO_PHYSMODEL_UNUSED_50 = 50,
    BTO_PHYSMODEL_UNUSED_51 = 51,
    BTO_PHYSMODEL_UNUSED_52 = 52,
    BTO_PHYSMODEL_UNUSED_53 = 53,
    BTO_PHYSMODEL_UNUSED_54 = 54,
    BTO_PHYSMODEL_UNUSED_55 = 55,
    BTO_PHYSMODEL_UNUSED_56 = 56,
    BTO_PHYSMODEL_UNUSED_57 = 57,
    BTO_PHYSMODEL_UNUSED_58 = 58,
    BTO_PHYSMODEL_UNUSED_59 = 59,
    BTO_PHYSMODEL_UNUSED_60 = 60,
    BTO_PHYSMODEL_UNUSED_61 = 61,
    BTO_PHYSMODEL_UNUSED_62 = 62,
    BTO_PHYSMODEL_UNUSED_63 = 63,
    BTO_PHYSMODEL_BARN = 64,
    BTO_PHYSMODEL_GAS_STATION = 65,
    BTO_PHYSMODEL_JOES_DINER = 66,
    BTO_PHYSMODEL_OFFICE = 67,
    BTO_PHYSMODEL_WINDMILL = 68,
    BTO_PHYSMODEL_SHIP = 69,
    BTO_PHYSMODEL_PINE_TREE = 70,
    BTO_PHYSMODEL_CACTUS = 71,
    BTO_PHYSMODEL_TENNIS_COURT = 72,
    BTO_PHYSMODEL_PALM_TREE = 73,
    BTO_PHYSMODEL_EXTRA = 74,
    BTO_ROAD_HALF_WIDTH = 120,
    BTO_TURN_OFFSET_SMALL = 512,
    BTO_TURN_OFFSET_LARGE = 1024,
    BTO_TURN_RADIUS_BASE_LARGE = 1536,
    BTO_TURN_SMALL_INNER = 392,
    BTO_TURN_SMALL_OUTER = 632,
    BTO_TURN_LARGE_INNER = 1416,
    BTO_TURN_LARGE_OUTER = 1656,
    BTO_WALL_HEIGHT_RAIL = 42,
    BTO_WALL_ORIENT_OFFSET = 512,
    BTO_TERRAIN_HILL_MIN = 7,
    BTO_TERRAIN_HILL_MAX = 10,
    BTO_TERRAIN_HILL_MAX_EXCL = 11,
    BTO_HILL_HEIGHT = 450,
    BTO_PLANINDEX_SHIFT = 2,
    BTO_GRASS_HEIGHT_BIAS = 2,
    BTO_SURFACE_TYPE_OFFSET = 1,
    BTO_SURFACE_TYPE_MIN = 1,
    BTO_PLAN_ROAD = 1,
    BTO_PLAN_SOLID_ROAD = 2,
    BTO_PLAN_RAMP = 3,
    BTO_PLAN_BANK_ROAD = 6,
    BTO_PLAN_HIGHWAY_RIGHT = 131,
    BTO_PLAN_HIGHWAY_LEFT = 132,
    BTO_PLAN_TUNNEL_TOP = 133,
    BTO_STARTFINISH_Z_SPLIT_1 = (short)65156,
    BTO_STARTFINISH_Z_SPLIT_2 = (short)65236,
    BTO_NEG_ROAD_HALF_WIDTH = (short)65416,
    BTO_HIGHWAY_HALF_WIDTH = 360,
    BTO_HIGHWAY_SPLIT_Z = 334,
    BTO_RAMP_FRONT_WALL_Z = 476,
    BTO_SOLIDROAD_REAR_WALL_Z = (short)65060,
    BTO_ELEVATED_MIN_CLEARANCE = 390,
    BTO_ELEVCORNER_RADIUS_MIN = (short)65386,
    BTO_ELEVCORNER_RADIUS_MAX = 150,
    BTO_ELEVCORNER_WALL_FREE_MIN = (short)65428,
    BTO_ELEVCORNER_WALL_FREE_MAX = 108,
    BTO_POLAR_ANGLE_MASK_LOW = 255,
    BTO_POLAR_ANGLE_STEP_MULT = 18,
    BTO_POLAR_ANGLE_STEP_BASE = 17,
    BTO_WALLIDX_ELEVCORNER_INNER = 105,
    BTO_WALLIDX_ELEVCORNER_OUTER = 123,
    BTO_BANK_ENTRY_PLAN_BASE_B = 25,
    BTO_BANK_ENTRY_PLAN_BASE_A = 35,
    BTO_BANK_ENTRY_SEGMENT_LEFT = 0,
    BTO_BANK_ENTRY_SEGMENT_RIGHT_LOWER = 1,
    BTO_BANK_ENTRY_SEGMENT_RIGHT_MIDDLE = 2,
    BTO_BANK_ENTRY_SEGMENT_RIGHT_UPPER = 3,
    BTO_BANK_ENTRY_ANGLE_B = 160,
    BTO_BANK_ENTRY_ANGLE_A = (short)64864,
    BTO_BANK_ENTRY_Z_MIN = (short)65202,
    BTO_BANK_ENTRY_Z_SEG_1 = (short)65368,
    BTO_BANK_ENTRY_Z_SEG_2 = 0,
    BTO_BANK_ENTRY_Z_SEG_3 = 168,
    BTO_BANK_ENTRY_Z_SEG_4 = 334,
    BTO_BANK_ENTRY_PLAN_OFFSET_FLAT = 0,
    BTO_BANK_ENTRY_PLAN_OFFSET_LOWER = 1,
    BTO_BANK_ENTRY_PLAN_OFFSET_LOWER_MID = 3,
    BTO_BANK_ENTRY_PLAN_OFFSET_UPPER_MID = 5,
    BTO_BANK_ENTRY_PLAN_OFFSET_UPPER = 7,
    BTO_BANK_ENTRY_PLAN_OFFSET_EXIT = 9,
    BTO_BANK_CORNER_RADIUS_MIN = (short)65416,
    BTO_BANK_CORNER_RADIUS_MAX = 126,
    BTO_BANK_CORNER_WALL_MIN = 102,
    BTO_LOOP_PLAN_FORWARD = 45,
    BTO_LOOP_PLAN_BACKWARD = 51,
    BTO_LOOP_Z_CLAMP_MARGIN = 100,
    BTO_LOOP_X_WIDTH = 400,
    BTO_LOOP_UPSIDE_Y = 524,
    BTO_TUNNEL_INTERIOR_Y = 144,
    BTO_TUNNEL_WALL_MIN_X = 120,
    BTO_TUNNEL_WALL_MAX_X = 270,
    BTO_TUNNEL_WALL_MIN_NEG_X = (short)65266,
    BTO_TUNNEL_FRONT_WALL_Z = (short)65024,
    BTO_PIPE_ENTR_SIDE_MIN_X = 115,
    BTO_PIPE_ENTR_SIDE_MAX_X = 164,
    BTO_PIPE_ENTR_MAX_Y = 171,
    BTO_PIPE_NEAR_CENTER_X = 31,
    BTO_HALFPIPE_MAX_Y = 265,
    BTO_HALFPIPE_SURF_LIMIT_X = 130,
    BTO_HALFPIPE_FLOOR_MAX_X = 84,
    BTO_HALFPIPE_FLOOR_MAX_Z = 75,
    BTO_HALFPIPE_FLOOR_FRONT_Z = (short)65461,
    BTO_HALFPIPE_FLOOR_REAR_Z = 75,
    BTO_HALFPIPE_LOWER_Y = 88,
    BTO_HALFPIPE_UPPER_Y = 151,
    BTO_CORK_EXIT_MIN_Y = 350,
    BTO_CORK_RADIUS_MIN = 332,
    BTO_CORK_RADIUS_MAX = 692,
    BTO_CORK_RADIUS_MID = 512,
    BTO_CORK_INNER_WALL_DELTA = 90,
    BTO_CORK_OUTER_WALL_DELTA = (short)65446,
    BTO_CORK_SEGMENT_COUNT = 24,
    BTO_HILL_TERRAIN_MIN_CONCAVE = 11,
    BTO_HILL_TERRAIN_MAX_CONCAVE = 14,
    BTO_HILL_TERRAIN_MIN_CONVEX = 15,
    BTO_HILL_TERRAIN_MAX_CONVEX = 18,
    BTO_HILL_COAST_ANGLE = (short)65408,
    BTO_HILL_TILE_MIN = 182,
    BTO_HILL_TILE_MAX = 197,
    BTO_HILL_PLAN_MIN = 12,
    BTO_HILL_PLAN_MAX = 15,
    BTO_HILLROUTE_FORCED_HEIGHT = 24,
    BTO_WALL_ENTRY_STRIDE = 3,
    BTO_TERRAIN_WATER = 1,
    BTO_TERRAIN_COAST_A = 2,
    BTO_TERRAIN_COAST_B = 3,
    BTO_TERRAIN_COAST_C = 4,
    BTO_TERRAIN_COAST_D = 5,
    BTO_TERRAIN_HILL_RAISED = 6,
    BTO_HILL_SUBSTITUTION_TERRAIN_COUNT = BTO_TERRAIN_HILL_MAX_EXCL - BTO_TERRAIN_HILL_MIN,
    BTO_HILL_SUBSTITUTION_ENTRIES_PER_TERRAIN = 6,
    BTO_HILL_HEIGHT_INDEX = 1,
    BTO_COAST_ANGLE_A = 128,
    BTO_COAST_ANGLE_B = (short)64896,
    BTO_COAST_ANGLE_C = (short)65152,
    BTO_COAST_ANGLE_D = (short)65408,
    BTO_LOOP_UPSIDE_INDEX_MAX = 5,
    BTO_SLOPE_ORIENT_TABLE_COUNT = 12,
    BTO_SLOPE_PLAN_DEFAULT = 3,
    BTO_CORK_ANGLE_BASE = 256,
    BTO_CORK_PLAN_SEGMENT_OFFSET = 1,
    BTO_CORK_PLAN_UD_LH = 79,
    BTO_CORK_PLAN_UD_RH = 105,
    BTO_CORK_WALL_BASE_INNER_LH = 50,
    BTO_CORK_WALL_BASE_OUTER_LH = 75,
    BTO_CORK_WALL_BASE_OUTER_RH = 25,
    BTO_CORK_EXIT_WALL_OFFSET = 24,
    BTO_CORK_EXIT_PLAN_OFFSET = 25,
    BTO_SLALOM_BLOCK1_MIN_X = 23,
    BTO_SLALOM_BLOCK1_MAX_X = 97,
    BTO_SLALOM_BLOCK1_MIN_Z = (short)65265,
    BTO_SLALOM_BLOCK1_MAX_Z = (short)65295,
    BTO_SLALOM_BLOCK2_MAX_X = (short)65513,
    BTO_SLALOM_BLOCK2_MIN_X = (short)65439,
    BTO_SLALOM_BLOCK2_MAX_Z = 271,
    BTO_SLALOM_BLOCK2_MIN_Z = 241,
    BTO_CORKLR_MAX_X = 150,
    BTO_CORKLR_WALL_HEIGHT = 117,
    BTO_CORKLR_PLAN_BASE = 57,
    BTO_CORKLR_WALL_INDEX = 185,
    BTO_BUILDING_BARN_HALF_SIZE = 150,
    BTO_BUILDING_BARN_HEIGHT = 425,
    BTO_BUILDING_BARN_WALL_NZ = (short)65386,
    BTO_BUILDING_GAS_MIN_X = (short)65336,
    BTO_BUILDING_GAS_MAX_X = 260,
    BTO_BUILDING_GAS_HALF_Z = 80,
    BTO_BUILDING_GAS_HEIGHT = 230,
    BTO_BUILDING_GAS_NZ = (short)65456,
    BTO_BUILDING_JOES_HALF_X = 180,
    BTO_BUILDING_JOES_HALF_Z = 100,
    BTO_BUILDING_JOES_HEIGHT = 248,
    BTO_BUILDING_JOES_NZ = (short)65436,
    BTO_BUILDING_JOES_NX = (short)65356,
    BTO_BUILDING_OFFICE_HALF_SIZE = 200,
    BTO_BUILDING_OFFICE_HEIGHT = 550,
    BTO_BUILDING_OFFICE_NZ = (short)65336,
    BTO_BUILDING_WINDMILL_HALF_SIZE = 114,
    BTO_BUILDING_WINDMILL_HEIGHT = 495,
    BTO_BUILDING_WINDMILL_NZ = (short)65422,
    BTO_BUILDING_SHIP_MIN_X = (short)65366,
    BTO_BUILDING_SHIP_MAX_X = 260,
    BTO_BUILDING_SHIP_HALF_Z = 110,
    BTO_BUILDING_SHIP_HEIGHT = 230,
    BTO_BUILDING_SHIP_NZ = (short)65426,
    BTO_WALLIDX_HIGHWAY_LEFT = 187,
    BTO_WALLIDX_HIGHWAY_RIGHT = 189,
    BTO_WALLIDX_HIGHWAY_EXIT = 186,
    BTO_WALLIDX_HIGHWAY_CENTER = 188,
    BTO_WALLIDX_SIDE_LEFT = 100,
    BTO_WALLIDX_SIDE_RIGHT = 101,
    BTO_WALLIDX_RAMP_REAR = 102,
    BTO_WALLIDX_RAMP_FRONT = 103,
    BTO_WALLIDX_SOLIDROAD_BACK = 104,
    BTO_WALLIDX_TUNNEL_FRONT = 154,
    BTO_WALLIDX_TUNNEL_REAR = 153,
    BTO_WALLIDX_TUNNEL_RIGHT_INNER = 152,
    BTO_WALLIDX_TUNNEL_RIGHT_OUTER = 150,
    BTO_WALLIDX_TUNNEL_LEFT_INNER = 151,
    BTO_WALLIDX_TUNNEL_LEFT_OUTER = 149,
    BTO_WALLIDX_PIPE_ENTR_RIGHT = 159,
    BTO_WALLIDX_PIPE_ENTR_LEFT = 160,
    BTO_WALLIDX_PIPE_RIGHT = 155,
    BTO_WALLIDX_PIPE_LEFT = 156,
    BTO_WALLIDX_HALFPIPE_FRONT = 157,
    BTO_WALLIDX_HALFPIPE_REAR = 158,
    BTO_PLAN_PIPE_ENTR_CENTER = 70,
    BTO_PLAN_PIPE_ENTR_LEFT_OUTER = 73,
    BTO_PLAN_PIPE_ENTR_LEFT_INNER = 71,
    BTO_PLAN_PIPE_ENTR_RIGHT_OUTER = 77,
    BTO_PLAN_PIPE_ENTR_RIGHT_INNER = 75,
    BTO_PIPE_TRI_CENTER_LEFT = (short)65452,
    BTO_PIPE_TRI_LEFT_OUTER_X = (short)65436,
    BTO_PIPE_TRI_LEFT_OUTER_ANGLE = (short)65531,
    BTO_PIPE_TRI_LEFT_INNER_X = (short)65479,
    BTO_PIPE_TRI_LEFT_INNER_ANGLE = (short)65528,
    BTO_PIPE_TRI_RIGHT_OUTER_X = 100,
    BTO_PIPE_TRI_RIGHT_OUTER_ANGLE = 5,
    BTO_PIPE_TRI_RIGHT_INNER_X = 57,
    BTO_PIPE_TRI_RIGHT_INNER_ANGLE = 8,
    BTO_PLAN_HALFPIPE_FLOOR = 69,
    BTO_PLAN_PIPE_LOWER_LEFT = 60,
    BTO_PLAN_PIPE_LOWER_RIGHT = 66,
    BTO_PLAN_PIPE_TOP_CENTER = 63,
    BTO_PLAN_PIPE_BOTTOM_CENTER = 57,
    BTO_PLAN_PIPE_TOP_LEFT_OUTER = 61,
    BTO_PLAN_PIPE_BOTTOM_LEFT_OUTER = 59,
    BTO_PLAN_PIPE_TOP_LEFT_INNER = 62,
    BTO_PLAN_PIPE_BOTTOM_LEFT_INNER = 58,
    BTO_PLAN_PIPE_TOP_RIGHT_OUTER = 65,
    BTO_PLAN_PIPE_BOTTOM_RIGHT_OUTER = 67,
    BTO_PLAN_PIPE_TOP_RIGHT_INNER = 64,
    BTO_PLAN_PIPE_BOTTOM_RIGHT_INNER = 68,
    BTO_WALLIDX_SLALOM_B1_NZ = 145,
    BTO_WALLIDX_SLALOM_B1_PZ = 146,
    BTO_WALLIDX_SLALOM_B1_NX = 148,
    BTO_WALLIDX_SLALOM_B1_PX = 147,
    BTO_WALLIDX_SLALOM_B2_PZ = 141,
    BTO_WALLIDX_SLALOM_B2_NZ = 142,
    BTO_WALLIDX_SLALOM_B2_PX = 143,
    BTO_WALLIDX_SLALOM_B2_NX = 144,
    BTO_CORKLR_BUCKET_RIGHT_OUTER = 10,
    BTO_CORKLR_BUCKET_RIGHT_INNER = 11,
    BTO_OPP_FILENAME_SIZE = 5,
    BTO_OPP_SPEED_TABLE_SIZE = 16,
    BTO_OPP_SPEED_TABLE_LAST_INDEX = BTO_OPP_SPEED_TABLE_SIZE - 1,
    BTO_OPP_SEARCH_STACK_CAPACITY = 256,
    BTO_OPP_SEARCH_INITIAL_COST = 999999,
    BTO_OPP_PATH_RESERVED_TAIL = 2,
    BTO_PATH_NODE_DEAD_END = -1,
    BTO_PATH_NODE_FINISH = 0,
    BTO_PATH_NODE_TERMINATOR = 1,
    BTO_PATH_BRANCH_NONE = -1,
    BTO_WALLIDX_BARN_NZ = 161,
    BTO_WALLIDX_BARN_PZ = 162,
    BTO_WALLIDX_BARN_PX = 163,
    BTO_WALLIDX_BARN_NX = 164,
    BTO_WALLIDX_GAS_NZ = 165,
    BTO_WALLIDX_GAS_PZ = 168,
    BTO_WALLIDX_GAS_NX = 166,
    BTO_WALLIDX_GAS_PX = 167,
    BTO_WALLIDX_JOES_NZ = 169,
    BTO_WALLIDX_JOES_PZ = 172,
    BTO_WALLIDX_JOES_NX = 171,
    BTO_WALLIDX_JOES_PX = 170,
    BTO_WALLIDX_OFFICE_NZ = 173,
    BTO_WALLIDX_OFFICE_PZ = 174,
    BTO_WALLIDX_OFFICE_NX = 175,
    BTO_WALLIDX_OFFICE_PX = 176,
    BTO_WALLIDX_WINDMILL_NZ = 180,
    BTO_WALLIDX_WINDMILL_PZ = 178,
    BTO_WALLIDX_WINDMILL_NX = 177,
    BTO_WALLIDX_WINDMILL_PX = 179,
    BTO_WALLIDX_SHIP_NZ = 181,
    BTO_WALLIDX_SHIP_PZ = 184,
    BTO_WALLIDX_SHIP_NX = 183,
    BTO_WALLIDX_SHIP_PX = 182
};

#define BTO_WALL_NONE        (-1)
#define BTO_WALL_HEIGHT_INIT (-12)
#define BTO_ELRD_WALL_INIT   (-1000)
#define BTO_ELRD_WALL_SHORT  ((short)65524)

/* Externs for globals written/read by this function */


/**
 * @brief Decode a raw track-object table entry.
 *
 * @param elem Track element id.
 * @param out Decoded output structure.
 * @return Non-zero on success.
 */
static int
bto_trackobj_decode(unsigned char elem, state_trackobject_raw *out) {
    return state_trackobject_raw_decode((const unsigned char *)trkObjectList, (unsigned int)elem,
                                        out);
}

/**
 * @brief Return the absolute value of an integer.
 *
 * @param value Input value.
 * @return Absolute value.
 */
static int
bto_abs_int(int value) {
    return value < 0 ? -value : value;
}

/**
 * @brief Return the multi-tile flags for a track element.
 *
 * @param elem Track element id.
 * @return Multi-tile bitmask, or 0 when decoding fails.
 */
static unsigned char
bto_trackobj_multi(unsigned char elem) {
    state_trackobject_raw obj;

    if (!bto_trackobj_decode(elem, &obj)) {
        return 0;
    }

    return obj.multi_tile_flag;
}

/**
 * @brief Return the surface type for a track element.
 *
 * @param elem Track element id.
 * @return Signed surface type value, or 0 when decoding fails.
 */
static signed char
bto_trackobj_surface(unsigned char elem) {
    state_trackobject_raw obj;

    if (!bto_trackobj_decode(elem, &obj)) {
        return 0;
    }

    return (signed char)obj.surface_type;
}

/**
 * @brief Return the physical model for a track element.
 *
 * @param elem Track element id.
 * @return Signed physical model value, or 0 when decoding fails.
 */
static signed char
bto_trackobj_phys(unsigned char elem) {
    state_trackobject_raw obj;

    if (!bto_trackobj_decode(elem, &obj)) {
        return 0;
    }

    return (signed char)obj.physical_model;
}

/**
 * @brief Return the Y rotation for a track element.
 *
 * @param elem Track element id.
 * @return Rotation in the engine's angle units, or 0 when decoding fails.
 */
static short
bto_trackobj_roty(unsigned char elem) {
    state_trackobject_raw obj;

    if (!bto_trackobj_decode(elem, &obj)) {
        return 0;
    }

    return (short)obj.rot_y;
}

static void
bto_initialize_waypoint_order(void) {
    short *waypointOrder = (short *)track_waypoint_order;
    int waypointIndex;

    waypointOrder[0] = BTO_PATH_NODE_FINISH;
    waypointOrder[1] = BTO_PATH_NODE_TERMINATOR;
    for (waypointIndex = BTO_OPP_PATH_RESERVED_TAIL;
         waypointIndex < BTO_TRACK_WAYPOINT_ORDER_CAPACITY;
         waypointIndex++) {
        waypointOrder[waypointIndex] = 0;
    }
}

static void
bto_store_waypoint_path(const short *pathNodes, int nodeCount) {
    short *waypointOrder = (short *)track_waypoint_order;
    int waypointIndex;

    for (waypointIndex = 0; waypointIndex < nodeCount; waypointIndex++) {
        waypointOrder[waypointIndex] = pathNodes[waypointIndex];
    }
    waypointOrder[nodeCount] = BTO_PATH_NODE_FINISH;
    waypointOrder[nodeCount + 1] = BTO_PATH_NODE_TERMINATOR;
}

static bool
bto_restore_search_branch(int *stackDepth, int *tileIndex, int *nodeCount, long *runningCost,
                          const short tileIndexStack[BTO_OPP_SEARCH_STACK_CAPACITY],
                          const short nodeCountStack[BTO_OPP_SEARCH_STACK_CAPACITY],
                          const long costStack[BTO_OPP_SEARCH_STACK_CAPACITY]) {
    if (*stackDepth == 0) {
        return false;
    }

    (*stackDepth)--;
    *tileIndex = tileIndexStack[*stackDepth];
    *nodeCount = nodeCountStack[*stackDepth];
    *runningCost = costStack[*stackDepth];
    return true;
}

static void
bto_rotate_local_vector(struct VECTOR *vector, int orientation) {
    int tmp;

    switch (orientation) {
    case BTO_ORIENT_270:
        tmp = vector->x;
        vector->x = vector->z;
        vector->z = (short)-tmp;
        break;
    case BTO_ORIENT_180:
        vector->x = (short)-vector->x;
        vector->z = (short)-vector->z;
        break;
    case BTO_ORIENT_90:
        tmp = vector->x;
        vector->x = (short)-vector->z;
        vector->z = (short)tmp;
        break;
    default:
        break;
    }
}

static int
bto_apply_plan_orientation(int basePlanIndex, int orientation) {
    if (basePlanIndex <= 0) {
        return basePlanIndex;
    }

    return (basePlanIndex << BTO_PLANINDEX_SHIFT)
           + ((4 - (orientation >> BTO_ORIENT_QUADRANT_SHIFT)) & 3);
}

static void
bto_get_rotated_wall_start(const short *wallEntry, int orientation, int *wallStartXOut,
                           int *wallStartZOut) {
    switch (orientation) {
    case BTO_ORIENT_270:
        *wallStartXOut = -wallEntry[2];
        *wallStartZOut = wallEntry[1];
        break;
    case BTO_ORIENT_180:
        *wallStartXOut = -wallEntry[1];
        *wallStartZOut = -wallEntry[2];
        break;
    case BTO_ORIENT_90:
        *wallStartXOut = wallEntry[2];
        *wallStartZOut = -wallEntry[1];
        break;
    default:
        *wallStartXOut = wallEntry[1];
        *wallStartZOut = wallEntry[2];
        break;
    }
}

/* Lookup tables */


/* Functions */

/**
 * @brief Build collision and surface state for the current track object.
 *
 * @param world_pos Current object-relative world position.
 * @param next_world_pos Predicted next world position used for wall selection.
 */
void
build_track_object(struct VECTOR *world_pos, struct VECTOR *next_world_pos) {
    int wallOrientationOffset;
    char terrainTile = 0;
    int absElemX;
    int absElemZ;
    struct VECTOR elemPos;
    int physModel;
    char tileRow;
    int highwayInnerBoundX;
    int highwayLateralDistance;
    int bankEntryPlanBase;
    int bankEntrySegmentIndex;
    int loopPlanBase;
    int loopSurfaceClampedZ;
    int loopBaseInnerBoundX;
    int pipeTriangleCenterX;
    int corkLateralCoord;
    char tileCol;
    struct VECTOR nextElemPos;
    char surfaceType;
    int elementOrientation;
    int corkOuterWallBase;
    int corkInnerWallBase;
    int turnRadius;
    char tileElement = 0;
    int effectiveX;
    int effectiveZ;

    int si;
    int di;
    int ax;

    planindex = 0;
    wallindex = BTO_WALL_NONE;
    wallHeight = BTO_WALL_HEIGHT_INIT;    /* 65524 */
    elRdWallRelated = BTO_ELRD_WALL_INIT; /* 64536 */
    corkFlag = false;
    current_surf_type = SURF_GRASS;
    track_object_render_enabled = true;
    wallOrientationOffset = 0;
    elementOrientation = 0;
    terrainHeight = 0;

    /* Compute tile column and row from world coords (divide by 1024) */
    tileCol = (char)(world_pos->x >> BTO_WORLD_TO_TILE_SHIFT);
    tileRow = (char)(world_pos->z >> BTO_WORLD_TO_TILE_SHIFT);

    do { /* single exit to finalization (replaces goto exit_func) */
        /* Bounds check: col must be 0..29, row must be 0..29 */
        if (tileCol < 0)
            break;
        if (tileCol > BTO_TRACK_LAST_INDEX)
            break;
        if (tileRow < 0)
            break;
        if (tileRow > BTO_TRACK_LAST_INDEX)
            break;

        /* Set element center from lookup tables */
        di = (int)(signed char)tileCol;
        elem_xCenter = trackcenterpos2[di];

        int tileRowDoubled = (int)(signed char)tileRow * 2;
        elem_zCenter = terraincenterpos[tileRowDoubled / 2];

        /* Look up terrain tile */
        {
            int rowOffset = trackrows[tileRowDoubled / 2];
            terrainTile = (char)track_terrain_map[rowOffset + di];
        }

        if (terrainTile != 0) {
            unsigned int terrVal = (unsigned char)terrainTile;
            if (terrVal == BTO_TERRAIN_WATER) {
                current_surf_type = SURF_WATER;
            }
            else if (terrVal >= BTO_TERRAIN_COAST_A && terrVal <= BTO_TERRAIN_COAST_D) {
                /* coast boundary check: rotate position by coast angle; if behind boundary, set water */
                switch (terrVal) {
                case BTO_TERRAIN_COAST_A:
                    si = BTO_COAST_ANGLE_A;
                    break;
                case BTO_TERRAIN_COAST_B:
                    si = BTO_COAST_ANGLE_B;
                    break;
                case BTO_TERRAIN_COAST_C:
                    si = BTO_COAST_ANGLE_C;
                    break;
                default:
                    si = BTO_COAST_ANGLE_D;
                    break;
                }
                {
                    int pxElem = world_pos->x - elem_xCenter;
                    int pzElem = world_pos->z - elem_zCenter;
                    short sinVal, cosVal;
                    elemPos.x = (short)pxElem;
                    elemPos.z = (short)pzElem;
                    sinVal = sin_fast((unsigned short)si);
                    di = multiply_and_scale((short)pzElem, sinVal);
                    cosVal = cos_fast((unsigned short)si);
                    int coastRotatedX = multiply_and_scale((short)pxElem, cosVal) + di;
                    if (coastRotatedX < 0)
                        current_surf_type = SURF_WATER;
                }
            }
            else if (terrVal == BTO_TERRAIN_HILL_RAISED) {
                terrainHeight = hillHeightConsts[BTO_HILL_HEIGHT_INDEX];
            }
        }

        /* loc_1E276: look up element tile */
        {
            int rowIdx = (int)(signed char)tileRow;
            int colIdx = (int)(signed char)tileCol;
            int bx = terrainrows[rowIdx];
            tileElement = (char)track_elem_map[bx + colIdx];
        }

        if (tileElement == 0) {
            /* skip element processing, go straight to terrain */
        }
        else {
            /* Tile center adjustment for multi-tile and marker tile elements */
            if ((unsigned char)tileElement < BTO_MARKER_CORNER) {
                /* Normal tile: adjust centers using multi-tile flags */
                unsigned char te = (unsigned char)tileElement;
                unsigned char mtf = (unsigned char)bto_trackobj_multi(te);
                if (mtf != 0) {
                    if ((int)mtf & 1) {
                        int rowIdx = (int)(signed char)tileRow;
                        elem_zCenter = terrainpos[rowIdx];
                    }
                    if (mtf & 2) {
                        int colIdx = (int)(signed char)tileCol;
                        elem_xCenter = trackpos2[colIdx + 1];
                    }
                }
            }
            else {
                /* Marker tile (253=CORNER, 254=VERTICAL, 255=HORIZONTAL): look up the actual tile */
                unsigned int tev = (unsigned char)tileElement;
                int rowIdx = (int)(signed char)tileRow;
                int colIdx = (int)(signed char)tileCol;

                if (tev == BTO_MARKER_CORNER) {
                    /* Look at tile to the left (col-1) in the row above (row+1) */
                    int bx = terrainrows[rowIdx + 1];
                    tileElement = (char)track_elem_map[bx + colIdx - 1];
                    {
                        unsigned char multiFlag = bto_trackobj_multi((unsigned char)tileElement);
                        if (multiFlag & 1)
                            elem_zCenter = terrainpos[rowIdx + 1];
                        if (multiFlag & 2)
                            elem_xCenter = trackpos2[colIdx];
                    }
                }
                else if (tev == BTO_MARKER_VERTICAL) {
                    /* Look at tile at (col, row+1) */
                    int bx = terrainrows[rowIdx + 1];
                    tileElement = (char)track_elem_map[bx + colIdx];
                    {
                        unsigned char multiFlag = bto_trackobj_multi((unsigned char)tileElement);
                        if (multiFlag & 1)
                            elem_zCenter = terrainpos[rowIdx + 1];
                        if (multiFlag & 2)
                            elem_xCenter = trackpos2[colIdx + 1];
                    }
                }
                else if (tev == BTO_MARKER_HORIZONTAL) {
                    /* Look at tile at (col-1, row) */
                    int bx = terrainrows[rowIdx];
                    tileElement = (char)track_elem_map[bx + colIdx - 1];
                    {
                        unsigned char multiFlag = bto_trackobj_multi((unsigned char)tileElement);
                        if (multiFlag & 1)
                            elem_zCenter = terrainpos[rowIdx];
                        if (multiFlag & 2)
                            elem_xCenter = trackpos2[colIdx];
                    }
                }
                /* else: unknown marker type — no center adjustment */
            }

            /* loc_1E464: compute element-relative coordinates */
            elemPos.x = (short)world_pos->x - elem_xCenter;
            elemPos.z = (short)world_pos->z - elem_zCenter;
            nextElemPos.x = (short)next_world_pos->x - elem_xCenter;
            nextElemPos.z = (short)next_world_pos->z - elem_zCenter;

            /* Substitute hill road track if terrain is 7..10 */
            if (tileElement != 0 && (unsigned char)terrainTile >= BTO_TERRAIN_HILL_MIN
                && (unsigned char)terrainTile < BTO_TERRAIN_HILL_MAX_EXCL) {
                tileElement = subst_hillroad_track((unsigned char)terrainTile,
                                                   (unsigned char)tileElement);
            }

            /* Load TRACKOBJECT data */
            {
                unsigned char te = (unsigned char)tileElement;
                physModel = (int)bto_trackobj_phys(te);
                elementOrientation = bto_trackobj_roty(te);
            }

            /* Rotate element coordinates based on orientation */
            bto_rotate_local_vector(&elemPos, elementOrientation);
            bto_rotate_local_vector(&nextElemPos, elementOrientation);

            /* Compute surface type and absolute element coordinates */
            {
                unsigned char te = (unsigned char)tileElement;
                surfaceType = (char)(bto_trackobj_surface(te) + BTO_SURFACE_TYPE_OFFSET);
                if (surfaceType < BTO_SURFACE_TYPE_MIN)
                    surfaceType = BTO_SURFACE_TYPE_MIN;
            }

            /* Compute absolute values of element coordinates */
            absElemX = bto_abs_int(elemPos.x);
            absElemZ = bto_abs_int(elemPos.z);

            /* Dispatch on physical model (0..74 = 74) */
            if (physModel <= BTO_PHYS_MODEL_MAX) {
                switch (physModel) {

                case BTO_PHYSMODEL_START_FINISH: /* code_bto_sfLine: start/finish line */
                    if (state.game_inputmode == 0) {
                        if (elemPos.x > 0) {
                            if (elemPos.z < BTO_STARTFINISH_Z_SPLIT_1) {
                                planindex = BTO_PLAN_HIGHWAY_RIGHT;
                            }
                            else if (elemPos.z < BTO_STARTFINISH_Z_SPLIT_2) {
                                planindex = BTO_PLAN_HIGHWAY_LEFT;
                            }
                        }
                    }
                    /* fall through to ROAD check */
                    /* fall through */
                case BTO_PHYSMODEL_ROAD: /* code_bto_road */
                    if (absElemX < BTO_ROAD_HALF_WIDTH) {
                        {
                            current_surf_type = surfaceType;
                            break;
                        }
                    }
                    break;

                case BTO_PHYSMODEL_SHARP_CORNER: /* code_bto_sCorner: sharp corner */
                {
                    int pz_adj = elemPos.z + BTO_TURN_OFFSET_SMALL;
                    int px_adj = elemPos.x + BTO_TURN_OFFSET_SMALL;
                    turnRadius = polarRadius2D(pz_adj, px_adj);
                    if (turnRadius > BTO_TURN_SMALL_INNER) {
                        if (turnRadius < BTO_TURN_SMALL_OUTER) {
                            current_surf_type = surfaceType;
                            break;
                        }
                    }
                    break;
                }

                case BTO_PHYSMODEL_LARGE_CORNER: /* code_bto_lCorner: large corner */
                {
                    int pz_adj = elemPos.z + BTO_TURN_OFFSET_LARGE;
                    int px_adj = elemPos.x + BTO_TURN_OFFSET_LARGE;
                    turnRadius = polarRadius2D(pz_adj, px_adj);
                    if (turnRadius > BTO_TURN_LARGE_INNER) {
                        if (turnRadius < BTO_TURN_LARGE_OUTER) {
                            current_surf_type = surfaceType;
                            break;
                        }
                    }
                    break;
                }

                case BTO_PHYSMODEL_CHICANE_RL: /* code_bto_chicaneRL */
                {
                    char surfT = surfaceType;
                    current_surf_type = surfT;
                    if (elemPos.x > 0) {
                        elemPos.z = (short)-elemPos.z;
                        elemPos.x = (short)-elemPos.x;
                    }
                    {
                        int pz_adj = elemPos.z + BTO_TURN_OFFSET_LARGE;
                        int px_adj = elemPos.x + BTO_TURN_OFFSET_LARGE;
                        turnRadius = polarRadius2D(pz_adj, px_adj);
                        if (turnRadius > BTO_TURN_LARGE_INNER) {
                            if (turnRadius < BTO_TURN_LARGE_OUTER) {
                                current_surf_type = surfaceType;
                                break;
                            }
                        }
                        break;
                    }
                }

                case BTO_PHYSMODEL_CHICANE_LR: /* code_bto_chicaneLR */
                    elemPos.x = (short)-elemPos.x;
                    {
                        char surfT = surfaceType;
                        current_surf_type = surfT;
                        if (elemPos.x > 0) {
                            elemPos.z = (short)-elemPos.z;
                            elemPos.x = (short)-elemPos.x;
                        }
                        {
                            int pz_adj = elemPos.z + BTO_TURN_OFFSET_LARGE;
                            int px_adj = elemPos.x + BTO_TURN_OFFSET_LARGE;
                            turnRadius = polarRadius2D(pz_adj, px_adj);
                            if (turnRadius > BTO_TURN_LARGE_INNER) {
                                if (turnRadius < BTO_TURN_LARGE_OUTER) {
                                    current_surf_type = surfaceType;
                                    break;
                                }
                            }
                            break;
                        }
                    }

                case BTO_PHYSMODEL_SHARP_SPLIT_A: /* code_bto_sSplitA: sharp split A */
                    if (absElemX < BTO_ROAD_HALF_WIDTH) {
                        current_surf_type = surfaceType;
                        break;
                    }
                    {
                        int pz_adj = elemPos.z + BTO_TURN_OFFSET_SMALL;
                        int px_adj = elemPos.x + BTO_TURN_OFFSET_SMALL;
                        turnRadius = polarRadius2D(pz_adj, px_adj);
                        if (turnRadius > BTO_TURN_SMALL_INNER) {
                            if (turnRadius < BTO_TURN_SMALL_OUTER) {
                                current_surf_type = surfaceType;
                                break;
                            }
                        }
                        break;
                    }

                case BTO_PHYSMODEL_SHARP_SPLIT_B: /* code_bto_sSplitB: sharp split B */
                    if (absElemX < BTO_ROAD_HALF_WIDTH) {
                        current_surf_type = surfaceType;
                        break;
                    }
                    {
                        int pz_adj = elemPos.z + BTO_TURN_OFFSET_SMALL;
                        int px_adj = BTO_TURN_OFFSET_SMALL - elemPos.x;
                        turnRadius = polarRadius2D(pz_adj, px_adj);
                        if (turnRadius > BTO_TURN_SMALL_INNER) {
                            if (turnRadius < BTO_TURN_SMALL_OUTER) {
                                current_surf_type = surfaceType;
                                break;
                            }
                        }
                        break;
                    }

                case BTO_PHYSMODEL_LARGE_SPLIT_A: /* code_bto_lSplitA: large split A */
                    if (elemPos.x >= BTO_TURN_SMALL_INNER && elemPos.x <= BTO_TURN_SMALL_OUTER) {
                        {
                            current_surf_type = surfaceType;
                            break;
                        }
                    }
                    {
                        int pz_adj = elemPos.z + BTO_TURN_OFFSET_LARGE;
                        int px_adj = elemPos.x + BTO_TURN_OFFSET_LARGE;
                        turnRadius = polarRadius2D(pz_adj, px_adj);
                        if (turnRadius > BTO_TURN_LARGE_INNER) {
                            if (turnRadius < BTO_TURN_LARGE_OUTER) {
                                current_surf_type = surfaceType;
                                break;
                            }
                        }
                        break;
                    }

                case BTO_PHYSMODEL_LARGE_SPLIT_B: /* code_bto_lSplitB: large split B */
                    if (elemPos.x >= -BTO_TURN_SMALL_OUTER && elemPos.x <= -BTO_TURN_SMALL_INNER) {
                        {
                            current_surf_type = surfaceType;
                            break;
                        }
                    }
                    {
                        int pz_adj = elemPos.z + BTO_TURN_OFFSET_LARGE;
                        int px_adj = BTO_TURN_OFFSET_LARGE - elemPos.x;
                        turnRadius = polarRadius2D(pz_adj, px_adj);
                        if (turnRadius > BTO_TURN_LARGE_INNER) {
                            if (turnRadius < BTO_TURN_LARGE_OUTER) {
                                current_surf_type = surfaceType;
                                break;
                            }
                        }
                        break;
                    }

                case BTO_PHYSMODEL_HIGHWAY_ENTRANCE: /* code_bto_highEntrance: highway entrance */
                {
                    int absX = bto_abs_int(elemPos.x);
                    highwayLateralDistance = absX;

                    si = 0;
                    while (highEntrZBounds1[si] < elemPos.z) {
                        si++;
                    }

                    /* Interpolate inner X bound */
                    di = si;
                    ax = highEntrXInnBounds0[di];
                    if (highEntrXInnBounds1[di] != ax) {
                        long divisor = (long)(highEntrZBounds1[di] - highEntrZBounds0[di]);
                        long dividend = (long)(elemPos.z - highEntrZBounds0[di]);
                        long range = (long)(highEntrXInnBounds1[di] - highEntrXInnBounds0[di]);
                        ax = (int)((dividend * range) / divisor) + highEntrXInnBounds0[di];
                    }
                    highwayInnerBoundX = ax;

                    /* Interpolate outer X bound */
                    ax = highEntrXOutBounds0[si];
                    if (highEntrXOutBounds1[si] != ax) {
                        long divisor = (long)(highEntrZBounds1[si] - highEntrZBounds0[si]);
                        long dividend = (long)(elemPos.z - highEntrZBounds0[si]);
                        long range = (long)(highEntrXOutBounds1[si] - highEntrXOutBounds0[si]);
                        ax = (int)((dividend * range) / divisor) + highEntrXOutBounds0[si];
                    }
                    int highwayOuterBoundX = ax;

                    /* Check if position is between inner and outer bounds */
                    if (highwayLateralDistance > highwayInnerBoundX
                        && highwayLateralDistance < highwayOuterBoundX) {
                        {
                            current_surf_type = surfaceType;
                            break;
                        }
                    }

                    if (elemPos.z < 0)
                        break;
                    if (highwayLateralDistance > BTO_ROAD_HALF_WIDTH)
                        break;

                    planindex = BTO_PLAN_ROAD;
                    if (elemPos.z >= BTO_HIGHWAY_SPLIT_Z) {
                        if (nextElemPos.x > BTO_NEG_ROAD_HALF_WIDTH) {
                            if (nextElemPos.x >= BTO_ROAD_HALF_WIDTH) {
                                wallindex = BTO_WALLIDX_HIGHWAY_EXIT;
                            }
                        }
                        else {
                            wallindex = BTO_WALLIDX_HIGHWAY_CENTER;
                        }
                    }
                    else {
                        if (nextElemPos.x >= 0) {
                            wallindex = BTO_WALLIDX_HIGHWAY_LEFT;
                        }
                        else {
                            wallindex = BTO_WALLIDX_HIGHWAY_RIGHT;
                        }
                    }
                    break;
                }

                case BTO_PHYSMODEL_HIGHWAY: /* code_bto_highway */
                    if (absElemX > BTO_HIGHWAY_HALF_WIDTH)
                        break;
                    if (absElemX > BTO_ROAD_HALF_WIDTH) {
                        current_surf_type = surfaceType;
                        break;
                    }

                    planindex = BTO_PLAN_ROAD;
                    if (nextElemPos.x <= BTO_NEG_ROAD_HALF_WIDTH) {
                        wallindex = BTO_WALLIDX_HIGHWAY_CENTER;
                    }
                    else {
                        /* z >= split: nextElemPos.x already > NEG_ROAD_HALF_WIDTH here */
                        if (elemPos.z >= BTO_HIGHWAY_SPLIT_Z) {
                            if (nextElemPos.x >= BTO_ROAD_HALF_WIDTH) {
                                wallindex = BTO_WALLIDX_HIGHWAY_EXIT;
                            }
                        }
                        else {
                            if (nextElemPos.x >= 0) {
                                wallindex = BTO_WALLIDX_HIGHWAY_LEFT;
                            }
                            else {
                                wallindex = BTO_WALLIDX_HIGHWAY_RIGHT;
                            }
                        }
                    }
                    break;

                case BTO_PHYSMODEL_CROSSROAD: /* code_bto_crossroad */
                    if (absElemX < BTO_ROAD_HALF_WIDTH) {
                        current_surf_type = surfaceType;
                        break;
                    }
                    if (absElemZ < BTO_ROAD_HALF_WIDTH) {
                        current_surf_type = surfaceType;
                        break;
                    }
                    break;

                case BTO_PHYSMODEL_UNUSED_13: /* blank */
                case BTO_PHYSMODEL_UNUSED_14: /* blank */
                case BTO_PHYSMODEL_UNUSED_15: /* blank */
                    break;

                case BTO_PHYSMODEL_RAMP:       /* code_bto_ramp */
                case BTO_PHYSMODEL_SOLID_RAMP: /* code_bto_solidRamp */
                    if (physModel == BTO_PHYSMODEL_RAMP) {
                        if (elemPos.z > 0) {
                            track_object_render_enabled = false;
                        }
                        else {
                            if (nextElemPos.z >= 0) {
                                wallindex = BTO_WALLIDX_RAMP_REAR;
                            }
                        }
                    }
                    else {
                        if (nextElemPos.z >= BTO_RAMP_FRONT_WALL_Z) {
                            wallindex = BTO_WALLIDX_RAMP_FRONT;
                        }
                    }
                    {
                        int absNextX = bto_abs_int(nextElemPos.x);

                        if (absNextX < BTO_ROAD_HALF_WIDTH) {
                            planindex = BTO_PLAN_RAMP;
                            current_surf_type = surfaceType;
                            if (wallindex != BTO_WALL_NONE)
                                break;

                            if (elemPos.z < 0)
                                break;
                            if (absElemX < BTO_ROAD_HALF_WIDTH)
                                break;

                            wallHeight = BTO_WALL_HEIGHT_RAIL;
                            elRdWallRelated = BTO_ELRD_WALL_SHORT;
                            if (elemPos.x < 0) {
                                wallindex = BTO_WALLIDX_SIDE_LEFT;
                                break;
                            }
                            wallindex = BTO_WALLIDX_SIDE_RIGHT;
                            break;
                        }

                        /* absNextX >= road half-width */
                        if (!track_object_render_enabled)
                            break;
                        if (absElemX > BTO_ROAD_HALF_WIDTH)
                            break;

                        planindex = BTO_PLAN_RAMP;
                        if (wallindex != BTO_WALL_NONE)
                            break;

                        wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;
                        if (elemPos.x < 0) {
                            wallindex = BTO_WALLIDX_SIDE_LEFT;
                            break;
                        }
                        wallindex = BTO_WALLIDX_SIDE_RIGHT;
                        break;
                    }

                case BTO_PHYSMODEL_ELEVATED_ROAD: /* code_bto_elevRoad */
                case BTO_PHYSMODEL_ELEVATED_SPAN: /* code_bto_elevRoad (elevated span) */
                    if (world_pos->y - terrainHeight <= BTO_ELEVATED_MIN_CLEARANCE) {
                        break;
                    }
                    track_object_render_enabled = false;
                    /* fall through */
                case BTO_PHYSMODEL_SOLID_ROAD: /* code_bto_solidRoad */
                {
                    int absNextX = bto_abs_int(nextElemPos.x);

                    if (absNextX <= BTO_ROAD_HALF_WIDTH) {
                        planindex = BTO_PLAN_SOLID_ROAD;
                        current_surf_type = surfaceType;
                        if (track_object_render_enabled) {
                            if (nextElemPos.z >= BTO_RAMP_FRONT_WALL_Z) {
                                wallindex = BTO_WALLIDX_RAMP_FRONT;
                            }
                            else if (nextElemPos.z <= BTO_SOLIDROAD_REAR_WALL_Z) {
                                wallindex = BTO_WALLIDX_SOLIDROAD_BACK;
                            }
                        }
                        if (absElemX < BTO_ROAD_HALF_WIDTH)
                            break;
                        wallHeight = BTO_WALL_HEIGHT_RAIL;
                        if (elemPos.x < 0) {
                            wallindex = BTO_WALLIDX_SIDE_LEFT;
                            break;
                        }
                        wallindex = BTO_WALLIDX_SIDE_RIGHT;
                        break;
                    }

                    /* absNextX > road half-width */
                    if (!track_object_render_enabled)
                        break;
                    if (absElemX > BTO_ROAD_HALF_WIDTH)
                        break;

                    planindex = BTO_PLAN_SOLID_ROAD;
                    wallHeight = BTO_WALL_HEIGHT_RAIL;
                    wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;
                    if (nextElemPos.x >= 0) {
                        wallindex = BTO_WALLIDX_SIDE_RIGHT;
                        break;
                    }
                    wallindex = BTO_WALLIDX_SIDE_LEFT;
                    break;
                }

                case BTO_PHYSMODEL_ELEVATED_CORNER: /* code_bto_elevCorner */
                    if (world_pos->y - terrainHeight <= BTO_ELEVATED_MIN_CLEARANCE) {
                        break;
                    }
                    {
                        int currentPzAdj = elemPos.z + BTO_TURN_OFFSET_LARGE;
                        int currentPxAdj = elemPos.x + BTO_TURN_OFFSET_LARGE;
                        int currentTurnRadius = polarRadius2D(currentPxAdj, currentPzAdj)
                                                - BTO_TURN_RADIUS_BASE_LARGE;
                        if (currentTurnRadius <= BTO_ELEVCORNER_RADIUS_MIN)
                            break;
                        if (currentTurnRadius >= BTO_ELEVCORNER_RADIUS_MAX)
                            break;

                        current_surf_type = surfaceType;
                        planindex = BTO_PLAN_SOLID_ROAD;
                        track_object_render_enabled = false;

                        if (currentTurnRadius >= BTO_ELEVCORNER_WALL_FREE_MIN
                            && currentTurnRadius <= BTO_ELEVCORNER_WALL_FREE_MAX) {
                            break;
                        }

                        /* Compute wall index from polar angle */
                        {
                            int angle = polarAngle(currentPxAdj, currentPzAdj);
                            angle &= BTO_POLAR_ANGLE_MASK_LOW; /* sub ah,ah equivalent */
                            angle = angle * BTO_POLAR_ANGLE_STEP_MULT;
                            angle = angle >> 8;
                            angle = -(angle - BTO_POLAR_ANGLE_STEP_BASE);

                            wallHeight = BTO_WALL_HEIGHT_RAIL;
                            wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;

                            if (currentTurnRadius < 0) {
                                wallindex = angle + BTO_WALLIDX_ELEVCORNER_INNER;
                            }
                            else {
                                wallindex = angle + BTO_WALLIDX_ELEVCORNER_OUTER;
                            }
                            break;
                        }
                    }

                case BTO_PHYSMODEL_OVERPASS: /* code_bto_overpass */
                    if (world_pos->y - terrainHeight > BTO_ELEVATED_MIN_CLEARANCE) {
                        int absNextX = bto_abs_int(nextElemPos.x);
                        track_object_render_enabled = false;
                        if (absNextX <= BTO_ROAD_HALF_WIDTH) {
                            planindex = BTO_PLAN_SOLID_ROAD;
                            current_surf_type = surfaceType;
                            if (track_object_render_enabled) {
                                if (nextElemPos.z >= BTO_RAMP_FRONT_WALL_Z) {
                                    wallindex = BTO_WALLIDX_RAMP_FRONT;
                                }
                                else if (nextElemPos.z <= BTO_SOLIDROAD_REAR_WALL_Z) {
                                    wallindex = BTO_WALLIDX_SOLIDROAD_BACK;
                                }
                            }
                            if (absElemX < BTO_ROAD_HALF_WIDTH)
                                break;
                            wallHeight = BTO_WALL_HEIGHT_RAIL;
                            if (elemPos.x < 0) {
                                wallindex = BTO_WALLIDX_SIDE_LEFT;
                                break;
                            }
                            wallindex = BTO_WALLIDX_SIDE_RIGHT;
                        }
                        else {
                            if (!track_object_render_enabled)
                                break;
                            if (absElemX > BTO_ROAD_HALF_WIDTH)
                                break;
                            planindex = BTO_PLAN_SOLID_ROAD;
                            wallHeight = BTO_WALL_HEIGHT_RAIL;
                            wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;
                            if (nextElemPos.x >= 0) {
                                wallindex = BTO_WALLIDX_SIDE_RIGHT;
                                break;
                            }
                            wallindex = BTO_WALLIDX_SIDE_LEFT;
                        }
                        break;
                    }
                    if (absElemZ <= BTO_ROAD_HALF_WIDTH) {
                        current_surf_type = surfaceType;
                        break;
                    }
                    break;

                case BTO_PHYSMODEL_BANK_ENTRANCE_B: /* code_bto_bankEntranceB */
                case BTO_PHYSMODEL_BANK_ENTRANCE_A: /* code_bto_bankEntranceA */
                    if (physModel == BTO_PHYSMODEL_BANK_ENTRANCE_B) {
                        bankEntryPlanBase = BTO_BANK_ENTRY_PLAN_BASE_B;
                        bankEntrySegmentIndex = BTO_BANK_ENTRY_SEGMENT_RIGHT_LOWER;
                        si = BTO_BANK_ENTRY_ANGLE_B;
                    }
                    else {
                        bankEntryPlanBase = BTO_BANK_ENTRY_PLAN_BASE_A;
                        bankEntrySegmentIndex = BTO_BANK_ENTRY_SEGMENT_LEFT;
                        si = BTO_BANK_ENTRY_ANGLE_A;
                    }
                    if (absElemX > BTO_ROAD_HALF_WIDTH)
                        break;

                    if (bankEntrySegmentIndex == BTO_BANK_ENTRY_SEGMENT_LEFT) {
                        if (nextElemPos.x <= BTO_NEG_ROAD_HALF_WIDTH) {
                            wallHeight = BTO_WALL_HEIGHT_RAIL;
                            elRdWallRelated = BTO_ELRD_WALL_SHORT;
                            wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;
                            wallindex = BTO_WALLIDX_SIDE_LEFT;
                        }
                    }

                    if (bankEntrySegmentIndex != BTO_BANK_ENTRY_SEGMENT_LEFT) {
                        if (nextElemPos.x >= BTO_ROAD_HALF_WIDTH) {
                            wallHeight = BTO_WALL_HEIGHT_RAIL;
                            elRdWallRelated = BTO_ELRD_WALL_SHORT;
                            wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;
                            wallindex = BTO_WALLIDX_SIDE_RIGHT;
                        }
                    }

                    current_surf_type = surfaceType;

                    if (elemPos.z < BTO_BANK_ENTRY_Z_MIN) {
                        planindex = (short)bankEntryPlanBase + BTO_BANK_ENTRY_PLAN_OFFSET_FLAT;
                        break;
                    }
                    if (elemPos.z >= BTO_BANK_ENTRY_Z_SEG_4) {
                        planindex = (short)bankEntryPlanBase + BTO_BANK_ENTRY_PLAN_OFFSET_EXIT;
                        break;
                    }

                    if (elemPos.z < BTO_BANK_ENTRY_Z_SEG_1) {
                        planindex = (short)bankEntryPlanBase + BTO_BANK_ENTRY_PLAN_OFFSET_LOWER;
                        bankEntrySegmentIndex = BTO_BANK_ENTRY_SEGMENT_LEFT;
                    }
                    else if (elemPos.z < BTO_BANK_ENTRY_Z_SEG_2) {
                        planindex = (short)bankEntryPlanBase + BTO_BANK_ENTRY_PLAN_OFFSET_LOWER_MID;
                        bankEntrySegmentIndex = BTO_BANK_ENTRY_SEGMENT_RIGHT_LOWER;
                    }
                    else if (elemPos.z < BTO_BANK_ENTRY_Z_SEG_3) {
                        planindex = (short)bankEntryPlanBase + BTO_BANK_ENTRY_PLAN_OFFSET_UPPER_MID;
                        bankEntrySegmentIndex = BTO_BANK_ENTRY_SEGMENT_RIGHT_MIDDLE;
                    }
                    else if (elemPos.z < BTO_BANK_ENTRY_Z_SEG_4) {
                        planindex = (short)bankEntryPlanBase + BTO_BANK_ENTRY_PLAN_OFFSET_UPPER;
                        bankEntrySegmentIndex = BTO_BANK_ENTRY_SEGMENT_RIGHT_UPPER;
                    }
                    int zAdj = elemPos.z - bkRdEntr_triang_zAdjust[bankEntrySegmentIndex];
                    short sinVal = sin_fast((unsigned short)si);
                    di = multiply_and_scale((short)zAdj, sinVal);
                    {
                        short cosVal = cos_fast((unsigned short)si);
                        int result = multiply_and_scale(elemPos.x, cosVal);
                        int bankEntrSideCheck = result + di;
                        if (bankEntrSideCheck > 0) {
                            planindex++;
                        }
                    }
                    break;

                case BTO_PHYSMODEL_BANK_ROAD: /* code_bto_bankRoad */
                    if (absElemX > BTO_ROAD_HALF_WIDTH)
                        break;
                    current_surf_type = surfaceType;
                    planindex = BTO_PLAN_BANK_ROAD;
                    if (nextElemPos.x < BTO_ROAD_HALF_WIDTH)
                        break;
                    wallHeight = BTO_WALL_HEIGHT_RAIL;
                    elRdWallRelated = BTO_ELRD_WALL_SHORT;
                    wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;
                    wallindex = BTO_WALLIDX_SIDE_RIGHT;
                    break;

                case BTO_PHYSMODEL_BANK_CORNER: /* code_bto_bankCorner */
                {
                    int pz_adj = elemPos.z + BTO_TURN_OFFSET_LARGE;
                    int px_adj = elemPos.x + BTO_TURN_OFFSET_LARGE;
                    turnRadius = polarRadius2D(px_adj, pz_adj) - BTO_TURN_RADIUS_BASE_LARGE;
                    if (turnRadius <= BTO_BANK_CORNER_RADIUS_MIN)
                        break;
                    if (turnRadius >= BTO_BANK_CORNER_RADIUS_MAX)
                        break;

                    {
                        int angle = polarAngle(px_adj, pz_adj);
                        int wallStep;
                        angle &= BTO_POLAR_ANGLE_MASK_LOW;
                        angle = angle * BTO_POLAR_ANGLE_STEP_MULT;
                        angle = angle >> 8;
                        wallStep
                            = -(angle - BTO_POLAR_ANGLE_STEP_BASE); /* reversed for wall index */

                        /* Original bank-corner plane selection uses the reversed arc step.
                 * Keeping planindex aligned with the DOS code avoids gaps at tile seams. */
                        planindex = (short)wallStep + 7;
                        current_surf_type = surfaceType;

                        if (turnRadius <= BTO_BANK_CORNER_WALL_MIN)
                            break;

                        wallHeight = BTO_WALL_HEIGHT_RAIL;
                        elRdWallRelated = BTO_ELRD_WALL_SHORT;
                        wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;
                        wallindex = wallStep + BTO_WALLIDX_ELEVCORNER_OUTER;
                    }
                }
                    track_object_render_enabled = false;
                    break;

                case BTO_PHYSMODEL_LOOP: /* code_bto_loop */
                {
                    int do_loop_base = 0;
                    int effX, effZ;

                    if (elemPos.z < 0) {
                        loopPlanBase = BTO_LOOP_PLAN_BACKWARD;
                        effX = -elemPos.x;
                        effZ = -elemPos.z;
                    }
                    else {
                        loopPlanBase = BTO_LOOP_PLAN_FORWARD;
                        effX = elemPos.x;
                        effZ = elemPos.z;
                    }
                    effectiveX = effX;
                    effectiveZ = effZ;

                    do {
                        /* Clamp effZ to loopSurface_maxZ */
                        {
                            int maxZ = loopSurface_maxZ;
                            int clampedZ;
                            if (effectiveZ > maxZ - BTO_PLAN_ROAD) {
                                if (effectiveZ > maxZ + BTO_LOOP_Z_CLAMP_MARGIN) {
                                    do_loop_base = 1;
                                    break;
                                }
                                clampedZ = maxZ - BTO_PLAN_ROAD;
                            }
                            else {
                                clampedZ = effectiveZ;
                            }
                            loopSurfaceClampedZ = clampedZ;
                        }

                        /* Find Z slice */
                        si = 0;
                        while (loopSurface_ZBounds1[si] < loopSurfaceClampedZ) {
                            si++;
                        }

                        /* Check if player is above or below */
                        if (world_pos->y - terrainHeight > BTO_LOOP_UPSIDE_Y) {
                            /* Upside-down: invert slice index */
                            si = BTO_LOOP_UPSIDE_INDEX_MAX - si;
                            di = si;

                            if (loopSurface_XBounds0[di] > effectiveX)
                                break;
                            if (loopSurface_XBounds1[di] + BTO_LOOP_X_WIDTH < effectiveX)
                                break;

                            if (loopSurface_XBounds1[di] < effectiveX) {
                                if (loopSurface_XBounds0[di] + BTO_LOOP_X_WIDTH > effectiveX) {
                                    planindex = (short)loopPlanBase + si;
                                    current_surf_type = surfaceType;
                                    track_object_render_enabled = false;
                                    break;
                                }
                            }

                            /* Interpolate */
                            int loopXInterpOffset, loopInterpolatedX;
                            {
                                long divisor
                                    = (long)(loopSurface_ZBounds1[di] - loopSurface_ZBounds0[di]);
                                long dividend
                                    = (long)(loopSurface_ZBounds0[di] - loopSurfaceClampedZ);
                                long range
                                    = (long)(loopSurface_XBounds0[di] - loopSurface_XBounds1[di]);
                                loopXInterpOffset = (int)((dividend * range) / divisor);
                                loopInterpolatedX = loopSurface_XBounds0[di] + loopXInterpOffset;
                            }

                            if (loopInterpolatedX >= effectiveX)
                                break;
                            if (loopInterpolatedX + BTO_LOOP_X_WIDTH <= effectiveX)
                                break;

                            planindex = (short)loopPlanBase + si;
                            current_surf_type = surfaceType;
                            track_object_render_enabled = false;
                            break;
                        }

                        /* Below the top (right-side up) */
                        if (si > 1) {
                            if (world_pos->y - terrainHeight < BTO_LOOP_Z_CLAMP_MARGIN) {
                                do_loop_base = 1;
                                break;
                            }
                        }

                        di = si;
                        if (loopSurface_XBounds0[di] > effectiveX) {
                            do_loop_base = 1;
                            break;
                        }
                        if (loopSurface_XBounds1[di] + BTO_LOOP_X_WIDTH < effectiveX) {
                            do_loop_base = 1;
                            break;
                        }

                        if (loopSurface_XBounds1[di] < effectiveX) {
                            if (loopSurface_XBounds0[di] + BTO_LOOP_X_WIDTH > effectiveX) {
                                planindex = (short)loopPlanBase + si;
                                current_surf_type = surfaceType;
                                track_object_render_enabled = false;
                                break;
                            }
                        }

                        /* Check if bounds are same (no interpolation needed) */
                        if (loopSurface_XBounds0[di] == loopSurface_XBounds1[di]) {
                            do_loop_base = 1;
                            break;
                        }

                        /* Interpolate for right-side up */
                        int loopXInterpOffset2, loopInterpolatedX2;
                        {
                            long divisor
                                = (long)(loopSurface_ZBounds1[di] - loopSurface_ZBounds0[di]);
                            long dividend = (long)(loopSurface_ZBounds0[di] - loopSurfaceClampedZ);
                            long range
                                = (long)(loopSurface_XBounds0[di] - loopSurface_XBounds1[di]);
                            loopXInterpOffset2 = (int)((dividend * range) / divisor);
                            loopInterpolatedX2 = loopSurface_XBounds0[di] + loopXInterpOffset2;
                        }

                        if (loopInterpolatedX2 >= effectiveX) {
                            do_loop_base = 1;
                            break;
                        }
                        if (loopInterpolatedX2 + BTO_LOOP_X_WIDTH <= effectiveX) {
                            do_loop_base = 1;
                            break;
                        }

                        planindex = (short)loopPlanBase + si;
                        current_surf_type = surfaceType;
                        track_object_render_enabled = false;
                    } while (0);

                    if (do_loop_base) {
                        si = 0;
                        while (loopBase_ZBounds1[si] < effectiveZ) {
                            si++;
                        }

                        /* Interpolate inner X bound */
                        {
                            long divisor = (long)(loopBase_ZBounds1[si] - loopBase_ZBounds0[si]);
                            long dividend = (long)(effectiveZ - loopBase_ZBounds0[si]);
                            long rangeInn
                                = (long)(loopBase_InnXBounds1[si] - loopBae_InnXBounds0[si]);
                            loopBaseInnerBoundX = (int)((dividend * rangeInn) / divisor)
                                                  + loopBae_InnXBounds0[si];
                        }

                        /* Interpolate outer X bound */
                        int loopBaseOuterX;
                        {
                            long divisor = (long)(loopBase_ZBounds1[si] - loopBase_ZBounds0[si]);
                            long dividend = (long)(effectiveZ - loopBase_ZBounds0[si]);
                            long rangeOut
                                = (long)(loopBase_OutXBounds1[si] - loopBase_OutXBounds0[si]);
                            loopBaseOuterX = (int)((dividend * rangeOut) / divisor)
                                             + loopBase_OutXBounds0[si];
                        }

                        if (effectiveX >= loopBaseInnerBoundX && effectiveX <= loopBaseOuterX) {
                            current_surf_type = surfaceType;
                        }
                    }
                    break;
                }

                case BTO_PHYSMODEL_TUNNEL: /* code_bto_tunnel */
                {
                    int posY = world_pos->y - terrainHeight;
                    int nextY = next_world_pos->y - terrainHeight;

                    if (posY >= BTO_TUNNEL_INTERIOR_Y || nextY >= BTO_TUNNEL_INTERIOR_Y) {
                        /* Above tunnel */
                        if (absElemX >= BTO_TUNNEL_WALL_MAX_X)
                            break;
                        current_surf_type = surfaceType;
                        planindex = BTO_PLAN_TUNNEL_TOP;
                        break;
                    }

                    /* Inside tunnel */
                    if (absElemX < BTO_TUNNEL_WALL_MIN_X) {
                        current_surf_type = surfaceType;
                    }

                    if (elemPos.x >= BTO_TUNNEL_WALL_MIN_X && elemPos.x <= BTO_TUNNEL_WALL_MAX_X) {
                        /* Right wall zone */
                        wallHeight = BTO_TUNNEL_INTERIOR_Y;
                        if (nextElemPos.z <= BTO_TUNNEL_FRONT_WALL_Z) {
                            wallindex = BTO_WALLIDX_TUNNEL_FRONT;
                            break;
                        }
                        if (nextElemPos.z >= BTO_TURN_OFFSET_SMALL) {
                            wallindex = BTO_WALLIDX_TUNNEL_REAR;
                            break;
                        }
                        if (nextElemPos.x <= BTO_TUNNEL_WALL_MIN_X) {
                            wallindex = BTO_WALLIDX_TUNNEL_RIGHT_INNER;
                            break;
                        }
                        if (nextElemPos.x >= BTO_TUNNEL_WALL_MAX_X) {
                            wallindex = BTO_WALLIDX_TUNNEL_RIGHT_OUTER;
                            break;
                        }
                        break;
                    }

                    if (elemPos.x > BTO_NEG_ROAD_HALF_WIDTH)
                        break;
                    if (elemPos.x < BTO_TUNNEL_WALL_MIN_NEG_X)
                        break;

                    /* Left wall zone */
                    wallHeight = BTO_TUNNEL_INTERIOR_Y;
                    if (nextElemPos.z <= BTO_TUNNEL_FRONT_WALL_Z) {
                        wallindex = BTO_WALLIDX_TUNNEL_FRONT;
                        break;
                    }
                    if (nextElemPos.z >= BTO_TURN_OFFSET_SMALL) {
                        wallindex = BTO_WALLIDX_TUNNEL_REAR;
                        break;
                    }
                    if (nextElemPos.x >= BTO_NEG_ROAD_HALF_WIDTH) {
                        wallindex = BTO_WALLIDX_TUNNEL_LEFT_INNER;
                        break;
                    }
                    if (nextElemPos.x <= BTO_TUNNEL_WALL_MIN_NEG_X) {
                        wallindex = BTO_WALLIDX_TUNNEL_LEFT_OUTER;
                        break;
                    }
                    break;
                }

                case BTO_PHYSMODEL_PIPE_ENTRANCE: /* code_bto_pipeEntrance */
                {
                    int absNextX = bto_abs_int(nextElemPos.x);

                    if (absNextX >= BTO_PIPE_ENTR_SIDE_MIN_X
                        && absElemX <= BTO_PIPE_ENTR_SIDE_MAX_X) {
                        wallHeight = BTO_HALFPIPE_UPPER_Y;
                        if (nextElemPos.x > 0) {
                            wallindex = BTO_WALLIDX_PIPE_ENTR_RIGHT;
                            break;
                        }
                        wallindex = BTO_WALLIDX_PIPE_ENTR_LEFT;
                        break;
                    }

                    if (absElemX >= BTO_PIPE_ENTR_SIDE_MIN_X)
                        break;

                    if (world_pos->y - terrainHeight >= BTO_PIPE_ENTR_MAX_Y)
                        break;

                    current_surf_type = surfaceType;

                    if (absElemX < BTO_PIPE_NEAR_CENTER_X) {
                        planindex = BTO_PLAN_PIPE_ENTR_CENTER;
                        break;
                    }

                    if (elemPos.x < BTO_PIPE_TRI_CENTER_LEFT) {
                        planindex = BTO_PLAN_PIPE_ENTR_LEFT_OUTER;
                        pipeTriangleCenterX = BTO_PIPE_TRI_LEFT_OUTER_X;
                        si = BTO_PIPE_TRI_LEFT_OUTER_ANGLE;
                    }
                    else if (elemPos.x < 0) {
                        planindex = BTO_PLAN_PIPE_ENTR_LEFT_INNER;
                        pipeTriangleCenterX = BTO_PIPE_TRI_LEFT_INNER_X;
                        si = BTO_PIPE_TRI_LEFT_INNER_ANGLE;
                    }
                    else if (elemPos.x > BTO_HALFPIPE_FLOOR_MAX_X) {
                        planindex = BTO_PLAN_PIPE_ENTR_RIGHT_OUTER;
                        pipeTriangleCenterX = BTO_PIPE_TRI_RIGHT_OUTER_X;
                        si = BTO_PIPE_TRI_RIGHT_OUTER_ANGLE;
                    }
                    else {
                        planindex = BTO_PLAN_PIPE_ENTR_RIGHT_INNER;
                        pipeTriangleCenterX = BTO_PIPE_TRI_RIGHT_INNER_X;
                        si = BTO_PIPE_TRI_RIGHT_INNER_ANGLE;
                    }
                    {
                        short sinVal = sin_fast((unsigned short)si);
                        di = multiply_and_scale(elemPos.z, sinVal);
                        int pipeEntrSideCheck;
                        {
                            int cx = elemPos.x - pipeTriangleCenterX;
                            short cosVal = cos_fast((unsigned short)si);
                            int result = multiply_and_scale((short)cx, cosVal);
                            pipeEntrSideCheck = result + di;
                        }
                        if (pipeEntrSideCheck < 0) {
                            planindex++;
                            break;
                        }
                        break;
                    }
                }

                case BTO_PHYSMODEL_PIPE:     /* code_bto_pipe */
                case BTO_PHYSMODEL_HALFPIPE: /* code_bto_halfPipe */
                {
                    int isHalfpipe = (physModel == BTO_PHYSMODEL_HALFPIPE) ? 1 : 0;
                    int absNextX = bto_abs_int(nextElemPos.x);
                    int pipeUpperSectionFlag;

                    if (absNextX >= BTO_PIPE_ENTR_SIDE_MAX_X
                        && absElemX <= BTO_PIPE_ENTR_SIDE_MAX_X) {
                        wallHeight = BTO_HALFPIPE_UPPER_Y;
                        if (nextElemPos.x > 0) {
                            wallindex = BTO_WALLIDX_PIPE_RIGHT;
                            break;
                        }
                        wallindex = BTO_WALLIDX_PIPE_LEFT;
                        break;
                    }

                    if (absElemX >= BTO_PIPE_ENTR_SIDE_MAX_X)
                        break;

                    if (world_pos->y - terrainHeight >= BTO_HALFPIPE_MAX_Y)
                        break;

                    if (absElemX < BTO_HALFPIPE_SURF_LIMIT_X) {
                        current_surf_type = surfaceType;
                    }

                    /* Determine if above or below pipe center */
                    if (world_pos->y - terrainHeight > BTO_HALFPIPE_UPPER_Y) {
                        pipeUpperSectionFlag = BTO_PLAN_ROAD;
                    }
                    else {
                        pipeUpperSectionFlag = 0;
                    }

                    /* Half-pipe special floor case */
                    if (isHalfpipe != 0 && pipeUpperSectionFlag == 0
                        && absElemX <= BTO_HALFPIPE_FLOOR_MAX_X
                        && absElemZ <= BTO_HALFPIPE_FLOOR_MAX_Z) {
                        planindex = BTO_PLAN_HALFPIPE_FLOOR;
                        if (nextElemPos.z < BTO_HALFPIPE_FLOOR_FRONT_Z) {
                            wallindex = BTO_WALLIDX_HALFPIPE_FRONT;
                            break;
                        }
                        if (nextElemPos.z >= BTO_HALFPIPE_FLOOR_REAR_Z) {
                            wallindex = BTO_WALLIDX_HALFPIPE_REAR;
                            break;
                        }
                        break;
                    }

                    /* Height > 88 and below center? */
                    if (world_pos->y - terrainHeight > BTO_HALFPIPE_LOWER_Y
                        && pipeUpperSectionFlag == 0) {
                        if (elemPos.x < 0) {
                            planindex = BTO_PLAN_PIPE_LOWER_LEFT;
                            break;
                        }
                        planindex = BTO_PLAN_PIPE_LOWER_RIGHT;
                        break;
                    }

                    /* Pipe section selection */
                    if (absElemX < BTO_PIPE_NEAR_CENTER_X) {
                        if (pipeUpperSectionFlag != 0) {
                            planindex = BTO_PLAN_PIPE_TOP_CENTER;
                        }
                        else {
                            planindex = BTO_PLAN_PIPE_BOTTOM_CENTER;
                        }
                        break;
                    }

                    if (elemPos.x < BTO_PIPE_TRI_CENTER_LEFT) {
                        if (pipeUpperSectionFlag != 0) {
                            planindex = BTO_PLAN_PIPE_TOP_LEFT_OUTER;
                        }
                        else {
                            planindex = BTO_PLAN_PIPE_BOTTOM_LEFT_OUTER;
                        }
                        break;
                    }

                    if (elemPos.x < 0) {
                        if (pipeUpperSectionFlag != 0) {
                            planindex = BTO_PLAN_PIPE_TOP_LEFT_INNER;
                        }
                        else {
                            planindex = BTO_PLAN_PIPE_BOTTOM_LEFT_INNER;
                        }
                        break;
                    }

                    if (elemPos.x > BTO_HALFPIPE_FLOOR_MAX_X) {
                        if (pipeUpperSectionFlag != 0) {
                            planindex = BTO_PLAN_PIPE_TOP_RIGHT_OUTER;
                        }
                        else {
                            planindex = BTO_PLAN_PIPE_BOTTOM_RIGHT_OUTER;
                        }
                        break;
                    }

                    if (pipeUpperSectionFlag != 0) {
                        planindex = BTO_PLAN_PIPE_TOP_RIGHT_INNER;
                    }
                    else {
                        planindex = BTO_PLAN_PIPE_BOTTOM_RIGHT_INNER;
                    }
                    break;
                }

                case BTO_PHYSMODEL_CORKSCREW_UD_LH: /* code_bto_corkUdLH: cork u/d A */
                case BTO_PHYSMODEL_CORKSCREW_UD_RH: /* code_bto_corkUdRH: cork u/d B */
                {
                    int corkUDPlanBase;
                    if (physModel == BTO_PHYSMODEL_CORKSCREW_UD_LH) {
                        corkLateralCoord = -elemPos.x;
                        corkUDPlanBase = BTO_CORK_PLAN_UD_LH;
                        corkInnerWallBase = BTO_CORK_WALL_BASE_INNER_LH;
                        corkOuterWallBase = BTO_CORK_WALL_BASE_OUTER_LH;
                    }
                    else {
                        corkLateralCoord = elemPos.x;
                        corkUDPlanBase = BTO_CORK_PLAN_UD_RH;
                        corkInnerWallBase = 0;
                        corkOuterWallBase = BTO_CORK_WALL_BASE_OUTER_RH;
                    }
                    corkFlag = true;

                    /* Cork descending entry ramp check */
                    if (elemPos.z < 0) {
                        if (world_pos->y - terrainHeight < BTO_LOOP_Z_CLAMP_MARGIN) {
                            if (corkLateralCoord > 0) {
                                if (corkLateralCoord >= BTO_TURN_SMALL_OUTER)
                                    break;
                                if (corkLateralCoord <= BTO_TURN_SMALL_INNER)
                                    break;
                                current_surf_type = surfaceType;
                                planindex = (short)corkUDPlanBase;
                                break;
                            }
                        }
                    }

                    /* Cork ascending exit ramp check */
                    if (elemPos.z > 0) {
                        if (world_pos->y - terrainHeight > BTO_CORK_EXIT_MIN_Y) {
                            if (corkLateralCoord < BTO_CORK_RADIUS_MAX
                                && corkLateralCoord > BTO_CORK_RADIUS_MIN) {
                                wallHeight = BTO_WALL_HEIGHT_RAIL;
                                elRdWallRelated = BTO_ELRD_WALL_SHORT;
                                if (corkLateralCoord > BTO_CORK_RADIUS_MID)
                                    ax = corkInnerWallBase;
                                else
                                    ax = corkOuterWallBase;
                                wallindex = ax + BTO_CORK_EXIT_WALL_OFFSET;
                                current_surf_type = surfaceType;
                                planindex = (short)corkUDPlanBase + BTO_CORK_EXIT_PLAN_OFFSET;
                                track_object_render_enabled = false;
                                break;
                            }
                        }
                    }

                    /* Main cork spiral region */
                    turnRadius = polarRadius2D(elemPos.z, corkLateralCoord);
                    if (turnRadius <= BTO_CORK_RADIUS_MIN)
                        break;
                    if (turnRadius >= BTO_CORK_RADIUS_MAX)
                        break;

                    {
                        int angle = polarAngle(elemPos.z, corkLateralCoord);
                        angle = -(angle - BTO_CORK_ANGLE_BASE);
                        angle &= BTO_ORIENT_MASK; /* and ah, 3 */
                        si = (angle * BTO_CORK_SEGMENT_COUNT) >> BTO_WORLD_TO_TILE_SHIFT;

                        planindex = (short)corkUDPlanBase + si + BTO_CORK_PLAN_SEGMENT_OFFSET;
                        current_surf_type = surfaceType;
                        track_object_render_enabled = false;

                        wallHeight = BTO_WALL_HEIGHT_RAIL;
                        elRdWallRelated = BTO_ELRD_WALL_SHORT;

                        if (turnRadius - BTO_CORK_RADIUS_MID > BTO_CORK_INNER_WALL_DELTA) {
                            ax = corkInnerWallBase;
                            wallindex = ax + si;
                            break;
                        }
                        if (turnRadius - BTO_CORK_RADIUS_MID < BTO_CORK_OUTER_WALL_DELTA) {
                            ax = corkOuterWallBase;
                            wallindex = ax + si;
                            break;
                        }
                        break;
                    }
                } /* end corkscrew UD case */

                case BTO_PHYSMODEL_SLALOM: /* code_bto_slalom */
                    if (absElemX < BTO_ROAD_HALF_WIDTH) {
                        current_surf_type = surfaceType;
                    }

                    /* First slalom block test */
                    if (elemPos.x >= BTO_SLALOM_BLOCK1_MIN_X && elemPos.x <= BTO_SLALOM_BLOCK1_MAX_X
                        && elemPos.z > BTO_SLALOM_BLOCK1_MIN_Z
                        && elemPos.z < BTO_SLALOM_BLOCK1_MAX_Z) {
                        wallHeight = BTO_WALL_HEIGHT_RAIL;
                        if (nextElemPos.z <= BTO_SLALOM_BLOCK1_MIN_Z) {
                            wallindex = BTO_WALLIDX_SLALOM_B1_NZ;
                            break;
                        }
                        if (nextElemPos.z > BTO_SLALOM_BLOCK1_MAX_Z) {
                            wallindex = BTO_WALLIDX_SLALOM_B1_PZ;
                            break;
                        }
                        if (nextElemPos.x < BTO_SLALOM_BLOCK1_MIN_X) {
                            wallindex = BTO_WALLIDX_SLALOM_B1_NX;
                            break;
                        }
                        if (nextElemPos.x > BTO_SLALOM_BLOCK1_MAX_X) {
                            wallindex = BTO_WALLIDX_SLALOM_B1_PX;
                            break;
                        }
                        break;
                    }

                    /* Second slalom block test: x in [-97, -23], z in (241, 271) */
                    if (elemPos.x > BTO_SLALOM_BLOCK2_MAX_X)
                        break;
                    if (elemPos.x < BTO_SLALOM_BLOCK2_MIN_X)
                        break;
                    if (elemPos.z >= BTO_SLALOM_BLOCK2_MAX_Z)
                        break;
                    if (elemPos.z <= BTO_SLALOM_BLOCK2_MIN_Z)
                        break;

                    wallHeight = BTO_WALL_HEIGHT_RAIL;
                    if (nextElemPos.z > BTO_SLALOM_BLOCK2_MAX_Z) {
                        wallindex = BTO_WALLIDX_SLALOM_B2_PZ;
                        break;
                    }
                    if (nextElemPos.z < (short)BTO_SLALOM_BLOCK2_MIN_Z) {
                        wallindex = BTO_WALLIDX_SLALOM_B2_NZ;
                        break;
                    }
                    if (nextElemPos.x > BTO_SLALOM_BLOCK2_MAX_X) {
                        wallindex = BTO_WALLIDX_SLALOM_B2_PX;
                        break;
                    }
                    if (nextElemPos.x < BTO_SLALOM_BLOCK2_MIN_X) {
                        wallindex = BTO_WALLIDX_SLALOM_B2_NX;
                        break;
                    }
                    break;

                case BTO_PHYSMODEL_CORKSCREW_LR: /* code_bto_corkLr: cork left/right */
                {
                    int corkUpperSectionFlag;

                    if (absElemX >= BTO_CORKLR_MAX_X)
                        break;

                    if (world_pos->y - terrainHeight >= BTO_HALFPIPE_MAX_Y)
                        break;

                    current_surf_type = surfaceType;

                    if (world_pos->y - terrainHeight > BTO_HALFPIPE_UPPER_Y) {
                        corkUpperSectionFlag = BTO_PLAN_ROAD;
                    }
                    else {
                        corkUpperSectionFlag = 0;
                    }

                    int corkLRBucketIdx = 0;

                    if (world_pos->y - terrainHeight > BTO_HALFPIPE_LOWER_Y
                        && corkUpperSectionFlag == 0) {
                        corkLRBucketIdx = (elemPos.x < 0) ? 3 : 9;
                    }
                    else if (absElemX < BTO_PIPE_NEAR_CENTER_X) {
                        if (corkUpperSectionFlag != 0) {
                            corkLRBucketIdx = 6;
                        }
                    }
                    else if (elemPos.x < BTO_PIPE_TRI_CENTER_LEFT) {
                        corkLRBucketIdx = (corkUpperSectionFlag != 0) ? 4 : 2;
                    }
                    else if (elemPos.x < 0) {
                        corkLRBucketIdx = (corkUpperSectionFlag != 0) ? 5 : 1;
                    }
                    else if (elemPos.x > BTO_HALFPIPE_FLOOR_MAX_X) {
                        corkLRBucketIdx
                            = (corkUpperSectionFlag != 0) ? 8 : BTO_CORKLR_BUCKET_RIGHT_OUTER;
                    }
                    else {
                        corkLRBucketIdx
                            = (corkUpperSectionFlag != 0) ? 7 : BTO_CORKLR_BUCKET_RIGHT_INNER;
                    }

                    if (corkLRBucketIdx != 0) {
                        di = corkLRBucketIdx;
                        if (corkLR_negZBound[di] < elemPos.z && corkLR_posZBound[di] > elemPos.z) {
                            planindex = (short)corkLRBucketIdx + BTO_CORKLR_PLAN_BASE;
                        }
                    }

                    if (planindex != 0)
                        break;

                    if (absElemZ >= BTO_TURN_OFFSET_SMALL)
                        break;

                    wallindex = BTO_CORKLR_WALL_INDEX;
                    corkFlag = true;
                    wallHeight = BTO_CORKLR_WALL_HEIGHT;
                    break;
                }

                case BTO_PHYSMODEL_UNUSED_36: /* blank */
                case BTO_PHYSMODEL_UNUSED_37: /* blank */
                case BTO_PHYSMODEL_UNUSED_38: /* blank */
                case BTO_PHYSMODEL_UNUSED_39: /* blank */
                case BTO_PHYSMODEL_UNUSED_40: /* blank */
                case BTO_PHYSMODEL_UNUSED_41: /* blank */
                case BTO_PHYSMODEL_UNUSED_42: /* blank */
                case BTO_PHYSMODEL_UNUSED_43: /* blank */
                case BTO_PHYSMODEL_UNUSED_44: /* blank */
                case BTO_PHYSMODEL_UNUSED_45: /* blank */
                case BTO_PHYSMODEL_UNUSED_46: /* blank */
                case BTO_PHYSMODEL_UNUSED_47: /* blank */
                case BTO_PHYSMODEL_UNUSED_48: /* blank */
                case BTO_PHYSMODEL_UNUSED_49: /* blank */
                case BTO_PHYSMODEL_UNUSED_50: /* blank */
                case BTO_PHYSMODEL_UNUSED_51: /* blank */
                case BTO_PHYSMODEL_UNUSED_52: /* blank */
                case BTO_PHYSMODEL_UNUSED_53: /* blank */
                case BTO_PHYSMODEL_UNUSED_54: /* blank */
                case BTO_PHYSMODEL_UNUSED_55: /* blank */
                case BTO_PHYSMODEL_UNUSED_56: /* blank */
                case BTO_PHYSMODEL_UNUSED_57: /* blank */
                case BTO_PHYSMODEL_UNUSED_58: /* blank */
                case BTO_PHYSMODEL_UNUSED_59: /* blank */
                case BTO_PHYSMODEL_UNUSED_60: /* blank */
                case BTO_PHYSMODEL_UNUSED_61: /* blank */
                case BTO_PHYSMODEL_UNUSED_62: /* blank */
                case BTO_PHYSMODEL_UNUSED_63: /* blank (empty) */
                    break;

                case BTO_PHYSMODEL_BARN: /* code_bto_barn */
                    if (absElemX > BTO_BUILDING_BARN_HALF_SIZE)
                        break;
                    if (absElemZ > BTO_BUILDING_BARN_HALF_SIZE)
                        break;
                    wallHeight = BTO_BUILDING_BARN_HEIGHT;
                    if (nextElemPos.z <= BTO_BUILDING_BARN_WALL_NZ) {
                        wallindex = BTO_WALLIDX_BARN_NZ;
                        break;
                    }
                    if (nextElemPos.z >= BTO_BUILDING_BARN_HALF_SIZE) {
                        wallindex = BTO_WALLIDX_BARN_PZ;
                        break;
                    }
                    if (nextElemPos.x >= BTO_BUILDING_BARN_HALF_SIZE) {
                        wallindex = BTO_WALLIDX_BARN_PX;
                        break;
                    }
                    if (nextElemPos.x <= BTO_BUILDING_BARN_WALL_NZ) {
                        wallindex = BTO_WALLIDX_BARN_NX;
                        break;
                    }
                    break;

                case BTO_PHYSMODEL_GAS_STATION: /* code_bto_gasStation */
                    if (elemPos.x < BTO_BUILDING_GAS_MIN_X)
                        break;
                    if (elemPos.x > BTO_BUILDING_GAS_MAX_X)
                        break;
                    if (absElemZ > BTO_BUILDING_GAS_HALF_Z)
                        break;
                    wallHeight = BTO_BUILDING_GAS_HEIGHT;
                    if (nextElemPos.z <= BTO_BUILDING_GAS_NZ) {
                        wallindex = BTO_WALLIDX_GAS_NZ;
                        break;
                    }
                    if (nextElemPos.z >= BTO_BUILDING_GAS_HALF_Z) {
                        wallindex = BTO_WALLIDX_GAS_PZ;
                        break;
                    }
                    if (nextElemPos.x <= BTO_BUILDING_GAS_MIN_X) {
                        wallindex = BTO_WALLIDX_GAS_NX;
                        break;
                    }
                    if (nextElemPos.x >= BTO_BUILDING_GAS_MAX_X) {
                        wallindex = BTO_WALLIDX_GAS_PX;
                        break;
                    }
                    break;

                case BTO_PHYSMODEL_JOES_DINER: /* code_bto_joes */
                    if (absElemX > BTO_BUILDING_JOES_HALF_X)
                        break;
                    if (absElemZ > BTO_BUILDING_JOES_HALF_Z)
                        break;
                    wallHeight = BTO_BUILDING_JOES_HEIGHT;
                    if (nextElemPos.z <= BTO_BUILDING_JOES_NZ) {
                        wallindex = BTO_WALLIDX_JOES_NZ;
                        break;
                    }
                    if (nextElemPos.z >= BTO_BUILDING_JOES_HALF_Z) {
                        wallindex = BTO_WALLIDX_JOES_PZ;
                        break;
                    }
                    if (nextElemPos.x <= BTO_BUILDING_JOES_NX) {
                        wallindex = BTO_WALLIDX_JOES_NX;
                        break;
                    }
                    if (nextElemPos.x >= BTO_BUILDING_JOES_HALF_X) {
                        wallindex = BTO_WALLIDX_JOES_PX;
                        break;
                    }
                    break;

                case BTO_PHYSMODEL_OFFICE: /* code_bto_office */
                    if (absElemX > BTO_BUILDING_OFFICE_HALF_SIZE)
                        break;
                    if (absElemZ > BTO_BUILDING_OFFICE_HALF_SIZE)
                        break;
                    wallHeight = BTO_BUILDING_OFFICE_HEIGHT;
                    if (nextElemPos.z <= BTO_BUILDING_OFFICE_NZ) {
                        wallindex = BTO_WALLIDX_OFFICE_NZ;
                        break;
                    }
                    if (nextElemPos.z >= BTO_BUILDING_OFFICE_HALF_SIZE) {
                        wallindex = BTO_WALLIDX_OFFICE_PZ;
                        break;
                    }
                    if (nextElemPos.x <= BTO_BUILDING_OFFICE_NZ) {
                        wallindex = BTO_WALLIDX_OFFICE_NX;
                        break;
                    }
                    if (nextElemPos.x >= BTO_BUILDING_OFFICE_HALF_SIZE) {
                        wallindex = BTO_WALLIDX_OFFICE_PX;
                        break;
                    }
                    break;

                case BTO_PHYSMODEL_WINDMILL: /* code_bto_windmill */
                    if (absElemX > BTO_BUILDING_WINDMILL_HALF_SIZE)
                        break;
                    if (absElemZ > BTO_BUILDING_WINDMILL_HALF_SIZE)
                        break;
                    wallHeight = BTO_BUILDING_WINDMILL_HEIGHT;
                    if (nextElemPos.z <= BTO_BUILDING_WINDMILL_NZ) {
                        wallindex = BTO_WALLIDX_WINDMILL_NZ;
                        break;
                    }
                    if (nextElemPos.z >= BTO_BUILDING_WINDMILL_HALF_SIZE) {
                        wallindex = BTO_WALLIDX_WINDMILL_PZ;
                        break;
                    }
                    if (nextElemPos.x <= BTO_BUILDING_WINDMILL_NZ) {
                        wallindex = BTO_WALLIDX_WINDMILL_NX;
                        break;
                    }
                    if (nextElemPos.x >= BTO_BUILDING_WINDMILL_HALF_SIZE) {
                        wallindex = BTO_WALLIDX_WINDMILL_PX;
                        break;
                    }
                    break;

                case BTO_PHYSMODEL_SHIP: /* code_bto_ship */
                    if (elemPos.x < BTO_BUILDING_SHIP_MIN_X)
                        break;
                    if (elemPos.x > BTO_BUILDING_SHIP_MAX_X)
                        break;
                    if (absElemZ > BTO_BUILDING_SHIP_HALF_Z)
                        break;
                    wallHeight = BTO_BUILDING_SHIP_HEIGHT;
                    if (nextElemPos.z <= BTO_BUILDING_SHIP_NZ) {
                        wallindex = BTO_WALLIDX_SHIP_NZ;
                        break;
                    }
                    if (nextElemPos.z >= BTO_BUILDING_SHIP_HALF_Z) {
                        wallindex = BTO_WALLIDX_SHIP_PZ;
                        break;
                    }
                    if (nextElemPos.x <= BTO_BUILDING_SHIP_MIN_X) {
                        wallindex = BTO_WALLIDX_SHIP_NX;
                        break;
                    }
                    if (nextElemPos.x >= BTO_BUILDING_SHIP_MAX_X) {
                        wallindex = BTO_WALLIDX_SHIP_PX;
                        break;
                    }
                    break;

                case BTO_PHYSMODEL_PINE_TREE:    /* pine - blank */
                case BTO_PHYSMODEL_CACTUS:       /* cactus - blank */
                case BTO_PHYSMODEL_TENNIS_COURT: /* tennis - blank */
                case BTO_PHYSMODEL_PALM_TREE:    /* palm - blank */
                case BTO_PHYSMODEL_EXTRA:        /* extra */
                    break;

                default:
                    break;
                } /* end of main switch */
            } /* end if physModel <= MAX */
        } /* end if tileElement != 0 */

        /* ===== Hill slope parsing ===== */
        if ((unsigned char)terrainTile < BTO_TERRAIN_HILL_MIN)
            break;

        /* Recalculate element coords relative to standard tile center */
        {
            int colIdx = (int)(signed char)tileCol;
            int rowIdx = (int)(signed char)tileRow;
            elemPos.x = (short)world_pos->x - trackcenterpos2[colIdx];
            elemPos.z = (short)world_pos->z - terraincenterpos[rowIdx];
        }

        /* Hill slope orientation dispatch (terrain types 7..18) */
        {
            unsigned int terrIdx = (unsigned char)terrainTile - BTO_TERRAIN_HILL_MIN;
            if (terrIdx >= BTO_SLOPE_ORIENT_TABLE_COUNT) {
                /* no orientation adjustment */
            }
            else {
                /* off_1F87E jump table: 12 entries, pattern 0,1,2,3 repeated 3 times. */
                switch (terrIdx & 3U) {
                case 0:
                    elementOrientation = BTO_ORIENT_0;
                    break;
                case 1:
                    elementOrientation = BTO_ORIENT_270;
                    {
                        int tmp = elemPos.x;
                        elemPos.x = elemPos.z;
                        elemPos.z = (short)-tmp;
                    }
                    break;
                case 2:
                    elementOrientation = BTO_ORIENT_180;
                    elemPos.z = (short)-elemPos.z;
                    elemPos.x = (short)-elemPos.x;
                    break;
                case 3:
                    elementOrientation = BTO_ORIENT_90;
                    {
                        int tmp = elemPos.x;
                        elemPos.x = (short)-elemPos.z;
                        elemPos.z = (short)tmp;
                    }
                    break;
                }
            } /* end if terrIdx < count */
        }

        /* Hill terrain type handling (was after_hillOrient:) */
        {
            unsigned int terrVal = (unsigned char)terrainTile;

            if (terrVal < BTO_TERRAIN_HILL_MIN)
                break;

            if (terrVal <= BTO_TERRAIN_HILL_MAX) {
                /* Simple hill slope (terrain 7-10) */
                if (planindex == 0) {
                    planindex = BTO_SLOPE_PLAN_DEFAULT;
                }
                break;
            }

            if (terrVal < BTO_HILL_TERRAIN_MIN_CONCAVE)
                break;

            if (terrVal <= BTO_HILL_TERRAIN_MAX_CONCAVE) {
                /* Terrain 11..14: concave hill (coast-like test) */
                {
                    short sinVal = sin_fast((unsigned short)BTO_HILL_COAST_ANGLE);
                    di = multiply_and_scale(elemPos.z, sinVal);
                    {
                        short cosVal = cos_fast((unsigned short)BTO_HILL_COAST_ANGLE);
                        int result = multiply_and_scale(elemPos.x, cosVal);
                        int hillRotatedX = result + di;
                        if (hillRotatedX < 0) {
                            planindex = 4;
                        }
                    }
                }
                break;
            }

            if (terrVal < BTO_HILL_TERRAIN_MIN_CONVEX)
                break;

            if (terrVal <= BTO_HILL_TERRAIN_MAX_CONVEX) {
                /* Terrain 15..18: convex hill */
                {
                    short sinVal = sin_fast((unsigned short)BTO_HILL_COAST_ANGLE);
                    di = multiply_and_scale(elemPos.z, sinVal);
                    {
                        short cosVal = cos_fast((unsigned short)BTO_HILL_COAST_ANGLE);
                        int result = multiply_and_scale(elemPos.x, cosVal);
                        int hillRotatedX = result + di;
                        if (hillRotatedX > 0) {
                            planindex = 5;
                            break;
                        }
                    }
                    terrainHeight = BTO_HILL_HEIGHT;
                }
                break;
            }
            /* terrVal > 18 */
            break;
        }
    } while (0);

    /* ===== Finalize planindex with orientation offset ===== */
    {
        planindex = bto_apply_plan_orientation(planindex, elementOrientation);
    }

    /* Compute current_planptr = planptr + planindex * sizeof(PLANE) */
    /* sizeof(PLANE) = 34 = 34 bytes */
    current_planptr = planptr + planindex; /* pointer arithmetic handles stride */

    /* Grass wobble: terrain height += 2 for non-grass, or a hash-based 0/1 for grass */
    if (current_surf_type == SURF_GRASS) {
        int hashBit;
        hashBit = (world_pos->z ^ world_pos->x) >> 8;
        hashBit &= 1;
        terrainHeight += hashBit;
    }
    else {
        terrainHeight += BTO_GRASS_HEIGHT_BIAS;
    }

    /* ===== Wall position computation ===== */
    if (wallindex < 0)
        return;

    {
        /* wallptr is an array of 6-byte entries: [orientation:2][startX:2][startZ:2] */
        short *wEntry = wallptr + (ptrdiff_t)(wallindex * BTO_WALL_ENTRY_STRIDE);

        /* Compute wall orientation */
        {
            int wallRot = wEntry[0]; /* original wall orientation */
            wallRot = -wallRot + elementOrientation + wallOrientationOffset;
            wallRot &= BTO_ORIENT_MASK; /* and ah, 3 */
            wallOrientation = (short)wallRot;
        }

        /* Rotate wall position based on element orientation */
        bto_get_rotated_wall_start(wEntry, elementOrientation, &wallStartX, &wallStartZ);

        /* Convert from element-relative to world coordinates */
        wallStartX += elem_xCenter;
        wallStartZ += elem_zCenter;
    }
}

/*
 * subst_hillroad_track - Substitutes hill-road track elements
 * Ported from seg004.asm lines 6109-6278
 *
 * Given a terrain direction and track element ID, returns the
 * substituted hill-road element ID, or 0 if no substitution.
 */
/**
 * @brief Replace flat-road elements with hill-road variants.
 *
 * @param terr Terrain id.
 * @param elem Base track element id.
 * @return Substituted hill-road element id, or 0 when no substitution exists.
 */
char
subst_hillroad_track(int terr, int elem) {
    int terrainIndex;
    int substitutionIndex;

    terrainIndex = terr - BTO_TERRAIN_HILL_MIN;
    if (terrainIndex < 0 || terrainIndex >= BTO_HILL_SUBSTITUTION_TERRAIN_COUNT) {
        return 0;
    }

    for (substitutionIndex = 0;
         substitutionIndex < BTO_HILL_SUBSTITUTION_ENTRIES_PER_TERRAIN;
         substitutionIndex++) {
        if (hillroad_substitutions[terrainIndex][substitutionIndex].source_element
            == (unsigned char)elem) {
            return (char)hillroad_substitutions[terrainIndex][substitutionIndex].hill_element;
        }
    }

    return 0;
}

/*
 * bto_auxiliary1 - Track dependency point lookup
 * Ported from seg004.asm lines 2756-3182.
/* Data tables in dseg - arrays of VECTOR (3 shorts = 6 bytes per entry) */

/**
 * @brief Return dependency points for a track tile.
 *
 * @param tile_col Track tile column.
 * @param tile_row Track tile row.
 * @param out_points Output point buffer.
 * @return Number of dependency points written.
 */
int
bto_auxiliary1(int tile_col, int tile_row, struct VECTOR *out_points) {
    unsigned char tileElement;
    int tileCenterX;      /* x center */
    int tileCenterZ;      /* z center */
    int hillHeightOffset; /* hill height offset */
    int elementOrientation;
    struct VECTOR *dependencyTable;
    unsigned char multiTileFlags;
    unsigned char terrainByte;
    int physModel;
    int pointCount; /* point count */
    int ptIdx;      /* loop index */

    /* Look up tile element at (col, row) */
    tileElement = ((unsigned char *)track_elem_map)[trackrows[tile_row] + tile_col];
    if (tileElement == 0)
        return 0;

    /* Get center positions */
    tileCenterX = trackcenterpos2[tile_col];
    tileCenterZ = trackcenterpos[tile_row];

    /* Handle multi-tile filler elements (253, 254, 255) */
    if (tileElement >= BTO_MARKER_CORNER) {
        switch (tileElement) {
        case BTO_MARKER_CORNER:
            /* Filler: look up tile at (col-1, row-1) using trackrows[row-1] */
            tileElement = ((unsigned char *)track_elem_map)[trackrows[tile_row - 1] + tile_col - 1];
            multiTileFlags = bto_trackobj_multi(tileElement);
            if (multiTileFlags & 1) {
                tileCenterZ = trackpos[tile_row + 1];
            }
            if (multiTileFlags & 2) {
                tileCenterX = trackpos2[tile_col];
            }
            break;

        case BTO_MARKER_VERTICAL:
            /* Filler: look up tile at (col, row-1) using trackrows[row-1] */
            tileElement = ((unsigned char *)track_elem_map)[trackrows[tile_row - 1] + tile_col];
            multiTileFlags = bto_trackobj_multi(tileElement);
            if (multiTileFlags & 1) {
                tileCenterZ = trackpos[tile_row + 1];
            }
            if (multiTileFlags & 2) {
                tileCenterX = trackpos2[tile_col + 1];
            }
            break;

        case BTO_MARKER_HORIZONTAL:
            /* Filler: look up tile at (col-1, row) */
            tileElement = ((unsigned char *)track_elem_map)[trackrows[tile_row] + tile_col - 1];
            multiTileFlags = bto_trackobj_multi(tileElement);
            if (multiTileFlags & 1) {
                tileCenterZ = trackpos[tile_row];
            }
            if (multiTileFlags & 2) {
                tileCenterX = trackpos2[tile_col];
            }
            break;

        default:
            break;
        }
    }
    else {
        /* Normal element: check multi-tile flags */
        multiTileFlags = bto_trackobj_multi(tileElement);
        if (multiTileFlags != 0) {
            if (multiTileFlags & 1) {
                tileCenterZ = trackpos[tile_row];
            }
            if (multiTileFlags & 2) {
                tileCenterX = trackpos2[tile_col + 1];
            }
        }
    }
    /* Dispatch on physicalModel to get point count and table pointer */
    pointCount = 0;
    dependencyTable = 0;

    physModel = (int)bto_trackobj_phys(tileElement);

    switch (physModel) {
    case BTO_PHYSMODEL_HIGHWAY:
        pointCount = 1;
        dependencyTable = (struct VECTOR *)phys_model_0B_points;
        break;
    case BTO_PHYSMODEL_ELEVATED_ROAD:
        pointCount = 8;
        dependencyTable = (struct VECTOR *)phys_model_0x12_points;
        break;
    case BTO_PHYSMODEL_CORKSCREW_UD_LH:
        pointCount = 2;
        dependencyTable = (struct VECTOR *)phys_model_0x20_points;
        break;
    case BTO_PHYSMODEL_CORKSCREW_UD_RH:
        pointCount = 2;
        dependencyTable = (struct VECTOR *)phys_model_0x21_points;
        break;
    case BTO_PHYSMODEL_SLALOM:
        pointCount = 4;
        dependencyTable = (struct VECTOR *)phys_model_0x22_points;
        break;
    case BTO_PHYSMODEL_CORKSCREW_LR:
        pointCount = 2;
        dependencyTable = (struct VECTOR *)phys_model_0x23_points;
        break;
    default:
        if (physModel >= BTO_PHYSMODEL_CACTUS && physModel <= BTO_PHYSMODEL_EXTRA) {
            pointCount = 1;
            dependencyTable = (struct VECTOR *)phys_model_0B_points;
        }
        break;
    }

    if (pointCount == 0)
        return 0;

    /* Get terrain type and hill height */
    terrainByte = ((unsigned char *)track_terrain_map)[terrainrows[tile_row] + tile_col];
    if (terrainByte == BTO_TERRAIN_HILL_RAISED) {
        hillHeightOffset = hillHeightConsts[BTO_HILL_HEIGHT_INDEX];
    }
    else {
        hillHeightOffset = 0;
    }

    /* Get element orientation */
    elementOrientation = bto_trackobj_roty(tileElement);

    /* Output rotated points */
    for (ptIdx = 0; ptIdx < pointCount; ptIdx++) {
        switch (elementOrientation) {
        case BTO_ORIENT_0:
            /* No rotation: (x, y, z) → (x + tileCenterX, y + hh, z + tileCenterZ) */
            out_points[ptIdx].x = (short)dependencyTable[ptIdx].x + tileCenterX;
            out_points[ptIdx].y = (short)dependencyTable[ptIdx].y + hillHeightOffset;
            out_points[ptIdx].z = (short)dependencyTable[ptIdx].z + tileCenterZ;
            break;
        case BTO_ORIENT_90:
            /* 90° CW: (x,y,z) → (z + tileCenterX, y + hh, -x + tileCenterZ) */
            out_points[ptIdx].x = (short)dependencyTable[ptIdx].z + tileCenterX;
            out_points[ptIdx].y = (short)dependencyTable[ptIdx].y + hillHeightOffset;
            out_points[ptIdx].z = (short)-dependencyTable[ptIdx].x + tileCenterZ;
            break;
        case BTO_ORIENT_180:
            /* 180°: (x,y,z) → (-x + tileCenterX, y + hh, -z + tileCenterZ) */
            out_points[ptIdx].x = (short)-dependencyTable[ptIdx].x + tileCenterX;
            out_points[ptIdx].y = (short)dependencyTable[ptIdx].y + hillHeightOffset;
            out_points[ptIdx].z = (short)-dependencyTable[ptIdx].z + tileCenterZ;
            break;
        case BTO_ORIENT_270:
            /* 270° CW: (x,y,z) → (-z + tileCenterX, y + hh, x + tileCenterZ) */
            out_points[ptIdx].x = (short)-dependencyTable[ptIdx].z + tileCenterX;
            out_points[ptIdx].y = (short)dependencyTable[ptIdx].y + hillHeightOffset;
            out_points[ptIdx].z = (short)dependencyTable[ptIdx].x + tileCenterZ;
            break;
        }
    }

    return pointCount;
}

/*
 * load_opponent_data - Load AI opponent path data
 * Ported from seg004.asm lines 5842-6108
 *
 * Constructs "oppN" filename from opponent type, loads the resource,
 * extracts name/path/speed data, and performs branch-and-bound
 * shortest path search through the track for the opponent AI.
 */
/**
 * @brief Load AI opponent speed data and build its waypoint order.
 */
void
load_opponent_data(void) {
    /* Stack arrays matching ASM layout */
    short pathNodes[BTO_TRACK_WAYPOINT_ORDER_CAPACITY]; /* bp-2848: path node indices */
    short siArr[BTO_OPP_SEARCH_STACK_CAPACITY];  /* bp-522: branch si (tile index) stack */
    short cntArr[BTO_OPP_SEARCH_STACK_CAPACITY]; /* bp-1038: branch node count stack */
    long costArr[BTO_OPP_SEARCH_STACK_CAPACITY]; /* bp-3886: branch running cost stack */

    char *resourcePtr;
    char *speedDataPtr;
    int stackDepth;    /* branch stack depth */
    int nodeCount;     /* current path node count */
    short currentNode; /* current graph node */
    bool terminalPath;
    bool reachedFinish;
    long runningCost;  /* running path cost (long) */
    short branchNode;  /* branch target node */
    long bestCost;

    int si;
    int visitIdx; /* inner loop index for visited-node duplicate check */

    /* Build "oppN" filename from opponent type */
    char oppname[BTO_OPP_FILENAME_SIZE] = "opp1";
    oppname[3] = (char)gameconfig.game_opponenttype + '0';

    /* Default to a minimal valid route so stale memory is never consumed if
       the search fails to produce a better path. */
    bto_initialize_waypoint_order();

    /* Load resource file */
    resourcePtr = (char *)file_load_resfile(oppname);
    if (resourcePtr == 0) {
        return;
    }

    /* Extract opponent name */
    copy_string(player_name_buffer, locate_text_res(resourcePtr, "nam"));

    /* Extract speed data pointer */
    speedDataPtr = locate_shape_alt(resourcePtr, "sped");
    if (speedDataPtr == 0) {
        unload_resource(resourcePtr);
        return;
    }

    /* Copy speed table. */
    for (si = 0; si < BTO_OPP_SPEED_TABLE_SIZE; si++) {
        opponent_speed_table[si] = ((unsigned char *)speedDataPtr)[si];
    }

    /* Initialize shortest path search */
    bestCost = BTO_OPP_SEARCH_INITIAL_COST;
    nodeCount = 0;
    runningCost = 0L;
    stackDepth = 0;
    si = 0;

    /* Branch-and-bound path search loop */
    for (;;) {
        if (si < 0 || si >= BTO_TRACKDATA_PATH_COUNT) {
            if (!bto_restore_search_branch(&stackDepth, &si, &nodeCount, &runningCost, siArr,
                                           cntArr, costArr)) {
                unload_resource(resourcePtr);
                return;
            }
            continue;
        }

        if (nodeCount >= BTO_TRACK_WAYPOINT_ORDER_CAPACITY - BTO_OPP_PATH_RESERVED_TAIL) {
            if (!bto_restore_search_branch(&stackDepth, &si, &nodeCount, &runningCost, siArr,
                                           cntArr, costArr)) {
                unload_resource(resourcePtr);
                return;
            }
            continue;
        }

        terminalPath = false;
        reachedFinish = false;
        currentNode = track_waypoint_next[si];

        if (currentNode == BTO_PATH_NODE_FINISH) {
            /* End of track: finish node */
            reachedFinish = true;
            terminalPath = true;
        }
        else if (currentNode == BTO_PATH_NODE_DEAD_END) {
            /* Dead end */
            terminalPath = true;
        }
        else {
            /* Check if this tile was already visited in current path */
            if (nodeCount > 0) {
                for (visitIdx = 0; visitIdx < nodeCount; visitIdx++) {
                    if (pathNodes[visitIdx] == si) {
                        terminalPath = true;
                        break;
                    }
                }
            }
        }

        /* Push current tile onto path */
        pathNodes[nodeCount] = (short)si;
        nodeCount++;

        /* Add cost for this tile from speed table */
        {
            unsigned char elemIdx = ((unsigned char *)track_elem_ordered)[si];
            /* sped chunk has 16 entries; clamp elemIdx to stay in bounds.
             * Raw track element codes > 15 (jumps, loops, etc.) are mapped
             * to the last speed category, matching DOS flat-memory behaviour. */
            if (elemIdx >= BTO_OPP_SPEED_TABLE_SIZE)
                elemIdx = BTO_OPP_SPEED_TABLE_LAST_INDEX;
            unsigned char speed = opponent_speed_table[elemIdx];
            runningCost += (long)((unsigned short)speed + 1);
        }

        if (!terminalPath) {
            /* Not at end: check for branch point */
            branchNode = track_waypoint_alt[si];
            if (branchNode != BTO_PATH_BRANCH_NONE && branchNode >= 0
                && branchNode < BTO_TRACKDATA_PATH_COUNT
                && stackDepth < BTO_OPP_SEARCH_STACK_CAPACITY) {
                /* Push branch state onto stack */
                siArr[stackDepth] = branchNode;
                cntArr[stackDepth] = (short)nodeCount;
                costArr[stackDepth] = runningCost;
                stackDepth++;
            }
            /* Follow main track: next tile */
            si = currentNode;
            continue;
        }

        /* At end: check if this path is the best */
        if (reachedFinish) {
            if (runningCost < bestCost
                && nodeCount + BTO_OPP_PATH_RESERVED_TAIL <= BTO_TRACK_WAYPOINT_ORDER_CAPACITY) {
                /* Record termination marker */
                pathNodes[nodeCount] = BTO_PATH_NODE_FINISH;
                nodeCount++;

                /* Save best cost */
                bestCost = runningCost;

                /* Copy path to track_waypoint_order */
                bto_store_waypoint_path(pathNodes, nodeCount);
            }
        }

        /* Backtrack: pop from branch stack */
        if (!bto_restore_search_branch(&stackDepth, &si, &nodeCount, &runningCost, siArr,
                                       cntArr, costArr)) {
            /* No more branches: done */
            unload_resource(resourcePtr);
            return;
        }
    }
}
