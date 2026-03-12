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
#include <stdio.h>
#include <stdlib.h>
#include "math.h"
#include "stunts.h"
#include "shape3d.h"
#include "ressources.h"
#include "memmgr.h"

/* Variables moved from data_game.c (private to this translation unit) */
static char corkFlag = 0;


/* file-local data (moved from data_global.c) */
static short loopSurface_maxZ = 449;
static short loopSurface_ZBounds0[6] = {   0, 224, 389, 449, 389, 224 };
static short loopSurface_ZBounds1[6] = { 224, 389, 449, 389, 224,   0 };
static short loopSurface_XBounds0[6] = { -400, -400, -352, -304, -270, -235 };
static short loopSurface_XBounds1[6] = { -400, -352, -304, -270, -235, -200 };
static short loopBase_ZBounds0[6] = {    0,  178,  360,  536,  704,  868 };
static short loopBase_ZBounds1[6] = {  178,  360,  536,  704,  868, 2000 };
static short loopBae_InnXBounds0[6] = {    0,  -20,  -40,  -60,  -80, -100 };
static short loopBase_InnXBounds1[6] = {  -20,  -40,  -60,  -80, -100, -120 };
static short loopBase_OutXBounds0[6] = {  400,  361,  320,  276,  226,  174 };
static short loopBase_OutXBounds1[6] = {  361,  320,  276,  226,  174,  120 };
static short bkRdEntr_triang_zAdjust[4] = { -251, -84, 84, 251 };
static short corkLR_negZBound[12]  = {    0,  -94, -187, -280, -373, -466, -559, -652, -745, -838,  -931, -1024 };
static short corkLR_posZBound[12]  = {    0, 1024,  931,  838,  745,  652,  559,  466,  373,  280,   187,    94 };
static short highEntrZBounds0[6] = { -512, -334, -168,   0, 168, 334 };
static short highEntrZBounds1[6] = { -334, -168,    0, 168, 334, 1000 };
static short highEntrXInnBounds0[6] = {   0,   0,   0,   0,   0, 120 };
static short highEntrXInnBounds1[6] = {   0,   0,   0,   0, 120, 120 };
static short highEntrXOutBounds0[6] = { 120, 168, 216, 264, 312, 360 };
static short highEntrXOutBounds1[6] = { 168, 216, 264, 312, 360, 360 };
static unsigned char phys_model_0B_points[6] = { 0, 0, 0, 0, 0, 0 };
static unsigned char phys_model_0x12_points[48] = { 136, 255, 0, 0, 231, 254, 136, 255, 0, 0, 25, 255, 136, 255, 0, 0, 25, 1, 136, 255, 0, 0, 231, 0, 120, 0, 0, 0, 231, 254, 120, 0, 0, 0, 25, 255, 120, 0, 0, 0, 25, 1, 120, 0, 0, 0, 231, 0 };
static unsigned char phys_model_0x23_points[12] = { 196, 255, 0, 0, 0, 254, 60, 0, 0, 0, 0, 2 };
static unsigned char phys_model_0x20_points[12] = { 120, 254, 0, 0, 0, 0, 136, 253, 0, 0, 0, 0 };
static unsigned char phys_model_0x21_points[12] = { 136, 1, 0, 0, 0, 0, 120, 2, 0, 0, 0, 0 };
static unsigned char phys_model_0x22_points[24] = { 23, 0, 0, 0, 1, 255, 97, 0, 0, 0, 1, 255, 159, 255, 0, 0, 255, 0, 233, 255, 0, 0, 255, 0 };

/* Grass=4, Water=5, from structs.inc */
#define SURF_GRASS 4
#define SURF_WATER 5

enum {
    BTO_TRACK_SIZE = 30,
    BTO_TRACK_LAST_INDEX = BTO_TRACK_SIZE - 1,
    BTO_WORLD_TO_TILE_SHIFT = 10,
    BTO_MARKER_CORNER = 253,
    BTO_MARKER_VERTICAL = 254,
    BTO_MARKER_HORIZONTAL = 255,
    BTO_ORIENT_0 = 0,
    BTO_ORIENT_90 = 256,
    BTO_ORIENT_180 = 512,
    BTO_ORIENT_270 = 768,
    BTO_ORIENT_MASK = 1023,
    BTO_PHYS_MODEL_MAX = 74,
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
    BTO_BANK_ENTRY_ANGLE_B = 160,
    BTO_BANK_ENTRY_ANGLE_A = (short)64864,
    BTO_BANK_ENTRY_Z_MIN = (short)65202,
    BTO_BANK_ENTRY_Z_SEG_1 = (short)65368,
    BTO_BANK_ENTRY_Z_SEG_2 = 0,
    BTO_BANK_ENTRY_Z_SEG_3 = 168,
    BTO_BANK_ENTRY_Z_SEG_4 = 334,
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
    BTO_HILL_HEIGHT_INDEX = 1,
    BTO_COAST_ANGLE_A = 128,
    BTO_COAST_ANGLE_B = (short)64896,
    BTO_COAST_ANGLE_C = (short)65152,
    BTO_COAST_ANGLE_D = (short)65408,
    BTO_LOOP_UPSIDE_INDEX_MAX = 5,
    BTO_SLOPE_ORIENT_TABLE_COUNT = 12,
    BTO_SLOPE_PLAN_DEFAULT = 3,
    BTO_CORK_ANGLE_BASE = 256,
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

#define BTO_WALL_NONE (-1)
#define BTO_WALL_HEIGHT_INIT (-12)
#define BTO_ELRD_WALL_INIT (-1000)
#define BTO_ELRD_WALL_SHORT ((short)65524)

/* Externs for globals written/read by this function */


/** @brief Bto trackobj decode.
 * @param elem Parameter `elem`.
 * @param out Parameter `out`.
 * @return Function result.
 */
static int bto_trackobj_decode(unsigned char elem, state_trackobject_raw* out)
{
    return state_trackobject_raw_decode((const unsigned char*)trkObjectList, (unsigned int)elem, out);
}

/** @brief Bto abs int.
 * @param value Parameter `value`.
 * @return Function result.
 */
static int bto_abs_int(int value)
{
    return value < 0 ? -value : value;
}

/** @brief Bto trackobj multi.
 * @param elem Parameter `elem`.
 * @return Function result.
 */
static unsigned char bto_trackobj_multi(unsigned char elem)
{
    state_trackobject_raw obj;

    if (!bto_trackobj_decode(elem, &obj)) {
        return 0;
    }

    return obj.multi_tile_flag;
}

/** @brief Bto trackobj surface.
 * @param elem Parameter `elem`.
 * @return Function result.
 */
static signed char bto_trackobj_surface(unsigned char elem)
{
    state_trackobject_raw obj;

    if (!bto_trackobj_decode(elem, &obj)) {
        return 0;
    }

    return (signed char)obj.surface_type;
}

/** @brief Bto trackobj phys.
 * @param elem Parameter `elem`.
 * @return Function result.
 */
static signed char bto_trackobj_phys(unsigned char elem)
{
    state_trackobject_raw obj;

    if (!bto_trackobj_decode(elem, &obj)) {
        return 0;
    }

    return (signed char)obj.physical_model;
}

/** @brief Bto trackobj roty.
 * @param elem Parameter `elem`.
 * @return Function result.
 */
static short bto_trackobj_roty(unsigned char elem)
{
    state_trackobject_raw obj;

    if (!bto_trackobj_decode(elem, &obj)) {
        return 0;
    }

    return (short)obj.rot_y;
}

/* Lookup tables */


/* Functions */

/** @brief Build track object.
 * @param world_pos Parameter `world_pos`.
 * @param next_world_pos Parameter `next_world_pos`.
 */
void build_track_object(struct VECTOR* world_pos, struct VECTOR* next_world_pos)
{
    short * currentWallPtrUnused;
    int tempValue3C;
    int wallOrientationOffset;
    char terrainTile = 0;
    int absElemX;
    int absElemZ;
    struct VECTOR elemPos;
    int physModel;
    int tempValue22;
    char tileRow;
    int tempValue1E;
    int tempValue1C;
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

    (void)currentWallPtrUnused;

    planindex = 0;
    wallindex = BTO_WALL_NONE;
    wallHeight = BTO_WALL_HEIGHT_INIT; /* 65524 */
    elRdWallRelated = BTO_ELRD_WALL_INIT; /* 64536 */
    corkFlag = 0;
    current_surf_type = SURF_GRASS;
    track_object_render_enabled = 1;
    si = 0;
    wallOrientationOffset = 0;
    elementOrientation = 0;
    terrainHeight = 0;

    /* Compute tile column and row from world coords (divide by 1024) */
    tileCol = (char)(world_pos->x >> BTO_WORLD_TO_TILE_SHIFT);
    tileRow = (char)(world_pos->z >> BTO_WORLD_TO_TILE_SHIFT);
    physModel = -1;
    
    /* Bounds check: col must be 0..29, row must be 0..29 */
    if (tileCol < 0) goto exit_func;
    if (tileCol > BTO_TRACK_LAST_INDEX) goto exit_func;
    if (tileRow < 0) goto exit_func;
    if (tileRow > BTO_TRACK_LAST_INDEX) goto exit_func;

    /* Set element center from lookup tables */
    di = (int)(signed char)tileCol;
    elem_xCenter = trackcenterpos2[di];

    tempValue3C = (int)(signed char)tileRow * 2;
    elem_zCenter = terraincenterpos[tempValue3C / 2];

    /* Look up terrain tile */
    {
        int rowOffset = trackrows[tempValue3C / 2];
        terrainTile = track_terrain_map[rowOffset + di];
    }

    if (terrainTile != 0) {
        unsigned int terrVal = (unsigned char)terrainTile;
        if (terrVal == BTO_TERRAIN_WATER) {
            goto check_coast_done; /* jump to loc_1E2FB = set water */
        }
        if (terrVal == BTO_TERRAIN_COAST_A) {
            si = BTO_COAST_ANGLE_A;
            goto do_coast_calc;
        }
        if (terrVal == BTO_TERRAIN_COAST_B) {
            si = BTO_COAST_ANGLE_B;
            goto do_coast_calc;
        }
        if (terrVal == BTO_TERRAIN_COAST_C) {
            si = BTO_COAST_ANGLE_C;
            goto do_coast_calc;
        }
        if (terrVal == BTO_TERRAIN_COAST_D) {
            si = BTO_COAST_ANGLE_D;
            goto do_coast_calc;
        }
        if (terrVal == BTO_TERRAIN_HILL_RAISED) {
            /* code_addHillHeight */
            terrainHeight = hillHeightConsts[BTO_HILL_HEIGHT_INDEX];
        }
        /* fall through to loc_1E276 */
        goto after_terrain;
    }
    goto after_terrain;

do_coast_calc:
    {
        /* loc_1E2AF */
        int pxElem, pzElem;
        pxElem = world_pos->x - elem_xCenter;
        pzElem = world_pos->z - elem_zCenter;
        elemPos.x = pxElem;
        elemPos.z = pzElem;

        {
            short sinVal, cosVal;
            int result;
            sinVal = sin_fast((unsigned short)si);
            di = multiply_and_scale(pzElem, sinVal);
            cosVal = cos_fast((unsigned short)si);
            result = multiply_and_scale(pxElem, cosVal);
            tempValue22 = result + di;
        }
        if (tempValue22 < 0) {
            goto check_coast_done;
        }
        goto after_terrain;
    }

check_coast_done:
    current_surf_type = SURF_WATER;
    goto after_terrain;

after_terrain:
    /* loc_1E276: look up element tile */
    {
        int rowIdx = (int)(signed char)tileRow;
        int colIdx = (int)(signed char)tileCol;
        int bx = terrainrows[rowIdx];
        tileElement = track_elem_map[bx + colIdx];
    }
    
    if (tileElement == 0) {
        goto code_bto_blank;
    }

/** @brief Handling.
 * @param BTO_MARKER_CORNER Parameter `BTO_MARKER_CORNER`.
 * @return Function result.
 */
    /* Filler tile handling (253, 254, 255) */
    if ((unsigned char)tileElement < BTO_MARKER_CORNER) {
        goto normal_tile;
    }

    /* loc_1E30D */
    {
        unsigned int tev = (unsigned char)tileElement;

        if (tev == BTO_MARKER_CORNER) {
            /* Look at tile to the left (col-1) in the row above (row+1) */
            int rowIdx = (int)(signed char)tileRow;
            int colIdx = (int)(signed char)tileCol;
            int bx = terrainrows[rowIdx + 1];
            tileElement = track_elem_map[bx + colIdx - 1];

            {
                unsigned char te = (unsigned char)tileElement;
                unsigned char multiFlag = bto_trackobj_multi(te);
                if (multiFlag & 1) {
                    elem_zCenter = terrainpos[rowIdx + 1];
                }
                if (multiFlag & 2) {
                    elem_xCenter = trackpos2[colIdx];
                }
            }

            goto compute_elem_crds;
        }

        if (tev == BTO_MARKER_VERTICAL) {
            /* Look at tile at (col, row+1) */
            int rowIdx = (int)(signed char)tileRow;
            int colIdx = (int)(signed char)tileCol;
            int bx = terrainrows[rowIdx + 1];
            tileElement = track_elem_map[bx + colIdx];

            {
                unsigned char te = (unsigned char)tileElement;
                unsigned char multiFlag = bto_trackobj_multi(te);
                if (multiFlag & 1) {
                    elem_zCenter = terrainpos[rowIdx + 1];
                }
                if (multiFlag & 2) {
                    elem_xCenter = trackpos2[colIdx + 1];
                }
            }

            goto compute_elem_crds;
        }

        if (tev == BTO_MARKER_HORIZONTAL) {
            /* Look at tile at (col-1, row) */
            int rowIdx = (int)(signed char)tileRow;
            int colIdx = (int)(signed char)tileCol;
            int bx = terrainrows[rowIdx];
            tileElement = track_elem_map[bx + colIdx - 1];

            {
                unsigned char te = (unsigned char)tileElement;
                unsigned char multiFlag = bto_trackobj_multi(te);
                if (multiFlag & 1) {
                    elem_zCenter = terrainpos[rowIdx];
                }
                if (multiFlag & 2) {
                    elem_xCenter = trackpos2[colIdx];
                }
            }

            goto compute_elem_crds;
        }

        goto compute_elem_crds;
    }

normal_tile:
    /* loc_1E40C */
    {
        unsigned char te = (unsigned char)tileElement;
        char mtf = (char)bto_trackobj_multi(te);
        tempValue3C = (int)(unsigned char)mtf;
        if (mtf == 0) goto compute_elem_crds;

        if (tempValue3C & 1) {
            int rowIdx = (int)(signed char)tileRow;
            elem_zCenter = terrainpos[rowIdx];
        }
    }

    {
        unsigned char te = (unsigned char)tileElement;
        unsigned char multiFlag = bto_trackobj_multi(te);
        if (multiFlag & 2) {
            int colIdx = (int)(signed char)tileCol;
            elem_xCenter = trackpos2[colIdx + 1];
            goto compute_elem_crds;
        }
    }
    goto compute_elem_crds;

compute_elem_crds:
    /* loc_1E464: compute element-relative coordinates */
    elemPos.x = world_pos->x - elem_xCenter;
    elemPos.z = world_pos->z - elem_zCenter;
    nextElemPos.x = next_world_pos->x - elem_xCenter;
    nextElemPos.z = next_world_pos->z - elem_zCenter;

    /* Substitute hill road track if terrain is 7..10 */
    if (tileElement != 0 &&
        (unsigned char)terrainTile >= BTO_TERRAIN_HILL_MIN &&
        (unsigned char)terrainTile < BTO_TERRAIN_HILL_MAX_EXCL) {
        tileElement = subst_hillroad_track((unsigned char)terrainTile, (unsigned char)tileElement);
    }

    /* Load TRACKOBJECT data */
    {
        unsigned char te = (unsigned char)tileElement;
        physModel = (int)bto_trackobj_phys(te);
        elementOrientation = bto_trackobj_roty(te);
    }

    /* Rotate element coordinates based on orientation */
/** @brief Degrees.
 * @param x Parameter `x`.
 * @param BTO_ORIENT_270 Parameter `BTO_ORIENT_270`.
 * @return Function result.
 */
    /* Orientation 768: rotate 270 degrees (x,z) -> (z, -x) */
    if (elementOrientation == BTO_ORIENT_270) {
        int tmp;
        tmp = elemPos.x;
        elemPos.x = elemPos.z;
        elemPos.z = -tmp;
        tmp = nextElemPos.x;
        nextElemPos.x = nextElemPos.z;
        nextElemPos.z = -tmp;
    }
    else if (elementOrientation == BTO_ORIENT_180) {
        /* Rotate 180 degrees: negate both */
        elemPos.z = -elemPos.z;
        elemPos.x = -elemPos.x;
        nextElemPos.z = -nextElemPos.z;
        nextElemPos.x = -nextElemPos.x;
    }
    else if (elementOrientation == BTO_ORIENT_90) {
        /* Rotate 90 degrees: (x,z) -> (-z, x) */
        int tmp;
        tmp = elemPos.x;
        elemPos.x = -elemPos.z;
        elemPos.z = tmp;
        tmp = nextElemPos.x;
        nextElemPos.x = -nextElemPos.z;
        nextElemPos.z = tmp;
    }
    /* else orientation 0: no rotation */

    /* Compute surface type and absolute element coordinates */
    {
        unsigned char te = (unsigned char)tileElement;
        surfaceType = (char)(bto_trackobj_surface(te) + BTO_SURFACE_TYPE_OFFSET);
        if (surfaceType < BTO_SURFACE_TYPE_MIN) surfaceType = BTO_SURFACE_TYPE_MIN;
    }

    /* Compute absolute values of element coordinates */
    absElemX = bto_abs_int(elemPos.x);
    absElemZ = bto_abs_int(elemPos.z);

    /* Dispatch on physical model (0..74 = 74) */
    if (physModel > BTO_PHYS_MODEL_MAX) goto code_bto_blank;

    switch (physModel) {

    case 0: /* code_bto_sfLine: start/finish line */
        if (state.game_inputmode == 0) {
            if (elemPos.x > 0) {
                if (elemPos.z < BTO_STARTFINISH_Z_SPLIT_1) {
                    planindex = BTO_PLAN_HIGHWAY_RIGHT;
                    goto case_1_road;
                }
                if (elemPos.z < BTO_STARTFINISH_Z_SPLIT_2) {
                    planindex = BTO_PLAN_HIGHWAY_LEFT;
                }
            }
        }
        goto case_1_road;
    case_1_road:
    case 1: /* code_bto_road */
        if (absElemX < BTO_ROAD_HALF_WIDTH) {
            goto set_pavement;
        }
        goto code_bto_blank;

    case 2: /* code_bto_sCorner: sharp corner */
        {
            int pz_adj = elemPos.z + BTO_TURN_OFFSET_SMALL;
            int px_adj = elemPos.x + BTO_TURN_OFFSET_SMALL;
            turnRadius = polarRadius2D(pz_adj, px_adj);
            if (turnRadius > BTO_TURN_SMALL_INNER) {
                if (turnRadius < BTO_TURN_SMALL_OUTER) goto set_pavement;
            }
            goto code_bto_blank;
        }

    case 3: /* code_bto_lCorner: large corner */
        {
            int pz_adj = elemPos.z + BTO_TURN_OFFSET_LARGE;
            int px_adj = elemPos.x + BTO_TURN_OFFSET_LARGE;
            turnRadius = polarRadius2D(pz_adj, px_adj);
            if (turnRadius > BTO_TURN_LARGE_INNER) {
                if (turnRadius < BTO_TURN_LARGE_OUTER) goto set_pavement;
            }
            goto code_bto_blank;
        }

    case 4: /* code_bto_chicaneRL */
        {
            char surfT = surfaceType;
            current_surf_type = surfT;
            if (elemPos.x > 0) {
                elemPos.z = -elemPos.z;
                elemPos.x = -elemPos.x;
            }
            goto do_lCorner;
        }

    case 5: /* code_bto_chicaneLR */
        elemPos.x = -elemPos.x;
        {
            char surfT = surfaceType;
            current_surf_type = surfT;
            if (elemPos.x > 0) {
                elemPos.z = -elemPos.z;
                elemPos.x = -elemPos.x;
            }
            goto do_lCorner;
        }

    case 6: /* code_bto_sSplitA: sharp split A */
        if (absElemX < BTO_ROAD_HALF_WIDTH) goto set_pavement;
        /* fall through to sCorner */
        goto do_sCorner;

    case 7: /* code_bto_sSplitB: sharp split B */
        if (absElemX < BTO_ROAD_HALF_WIDTH) goto set_pavement;
        {
            int pz_adj = elemPos.z + BTO_TURN_OFFSET_SMALL;
            int px_adj = BTO_TURN_OFFSET_SMALL - elemPos.x;
            turnRadius = polarRadius2D(pz_adj, px_adj);
            if (turnRadius > BTO_TURN_SMALL_INNER) {
                if (turnRadius < BTO_TURN_SMALL_OUTER) goto set_pavement;
            }
            goto code_bto_blank;
        }

    case 8: /* code_bto_lSplitA: large split A */
        if (elemPos.x >= BTO_TURN_SMALL_INNER && elemPos.x <= BTO_TURN_SMALL_OUTER) {
            goto set_pavement;
        }
        goto do_lCorner;

    case 9: /* code_bto_lSplitB: large split B */
        if (elemPos.x >= -BTO_TURN_SMALL_OUTER && elemPos.x <= -BTO_TURN_SMALL_INNER) {
            goto set_pavement;
        }
        {
            int pz_adj = elemPos.z + BTO_TURN_OFFSET_LARGE;
            int px_adj = BTO_TURN_OFFSET_LARGE - elemPos.x;
            turnRadius = polarRadius2D(pz_adj, px_adj);
            if (turnRadius > BTO_TURN_LARGE_INNER) {
                if (turnRadius < BTO_TURN_LARGE_OUTER) goto set_pavement;
            }
            goto code_bto_blank;
        }

    case 10: /* code_bto_highEntrance: highway entrance */
    {
        int absX = bto_abs_int(elemPos.x);
        tempValue1C = absX;

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
        tempValue1E = ax;

        /* Interpolate outer X bound */
        ax = highEntrXOutBounds0[si];
        if (highEntrXOutBounds1[si] != ax) {
            long divisor = (long)(highEntrZBounds1[si] - highEntrZBounds0[si]);
            long dividend = (long)(elemPos.z - highEntrZBounds0[si]);
            long range = (long)(highEntrXOutBounds1[si] - highEntrXOutBounds0[si]);
            ax = (int)((dividend * range) / divisor) + highEntrXOutBounds0[si];
        }
        tempValue22 = ax;

        /* Check if position is between inner and outer bounds */
        if (tempValue1C > tempValue1E && tempValue1C < tempValue22) {
            goto set_pavement;
        }

        if (elemPos.z < 0) goto code_bto_blank;
        if (tempValue1C > BTO_ROAD_HALF_WIDTH) goto code_bto_blank;

        planindex = BTO_PLAN_ROAD;
        goto highEntrance_wallCheck;
    }

    case 11: /* code_bto_highway */
        if (absElemX > BTO_HIGHWAY_HALF_WIDTH) goto code_bto_blank;
        if (absElemX > BTO_ROAD_HALF_WIDTH) goto set_pavement;

        planindex = BTO_PLAN_ROAD;
        if (nextElemPos.x <= BTO_NEG_ROAD_HALF_WIDTH) goto highway_wallBC;

    highEntrance_wallCheck:
        if (elemPos.z >= BTO_HIGHWAY_SPLIT_Z) {
            if (nextElemPos.x > BTO_NEG_ROAD_HALF_WIDTH) {
                goto highEntr_checkRight;
            }
            goto highway_wallBC;
        }
        /* z < 334 */
        if (nextElemPos.x >= 0) {
            wallindex = BTO_WALLIDX_HIGHWAY_LEFT;
            goto code_bto_blank;
        }
        wallindex = BTO_WALLIDX_HIGHWAY_RIGHT;
        goto code_bto_blank;

    highEntr_checkRight:
        if (nextElemPos.x < BTO_ROAD_HALF_WIDTH) goto code_bto_blank;
        wallindex = BTO_WALLIDX_HIGHWAY_EXIT;
        goto code_bto_blank;

    highway_wallBC:
        wallindex = BTO_WALLIDX_HIGHWAY_CENTER;
        goto code_bto_blank;

    case 12: /* code_bto_crossroad */
        if (absElemX < BTO_ROAD_HALF_WIDTH) goto set_pavement;
        if (absElemZ < BTO_ROAD_HALF_WIDTH) goto set_pavement;
        goto code_bto_blank;

    case 13: /* blank */
    case 14: /* blank */
    case 15: /* blank */
        goto code_bto_blank;

    case 16: /* code_bto_ramp */
        if (elemPos.z > 0) {
            track_object_render_enabled = 0;
        } else {
            if (nextElemPos.z >= 0) {
                wallindex = BTO_WALLIDX_RAMP_REAR;
            }
        }
        goto ramp_common;

    case 17: /* code_bto_solidRamp */
        if (nextElemPos.z >= BTO_RAMP_FRONT_WALL_Z) {
            wallindex = BTO_WALLIDX_RAMP_FRONT;
        }
    ramp_common:
        {
            int absNextX = bto_abs_int(nextElemPos.x);

            if (absNextX < BTO_ROAD_HALF_WIDTH) {
                planindex = BTO_PLAN_RAMP;
                current_surf_type = surfaceType;
                if (wallindex != BTO_WALL_NONE) goto code_bto_blank;

                if (elemPos.z < 0) goto code_bto_blank;
                if (absElemX < BTO_ROAD_HALF_WIDTH) goto code_bto_blank;

                wallHeight = BTO_WALL_HEIGHT_RAIL;
                elRdWallRelated = BTO_ELRD_WALL_SHORT;
                if (elemPos.x < 0) {
                    wallindex = BTO_WALLIDX_SIDE_LEFT;
                    goto code_bto_blank;
                }
                wallindex = BTO_WALLIDX_SIDE_RIGHT;
                goto code_bto_blank;
            }

            /* absNextX >= road half-width */
            if (track_object_render_enabled == 0) goto code_bto_blank;
            if (absElemX > BTO_ROAD_HALF_WIDTH) goto code_bto_blank;

            planindex = BTO_PLAN_RAMP;
            if (wallindex != BTO_WALL_NONE) goto code_bto_blank;

            wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;
            if (elemPos.x < 0) {
                wallindex = BTO_WALLIDX_SIDE_LEFT;
                goto code_bto_blank;
            }
            wallindex = BTO_WALLIDX_SIDE_RIGHT;
            goto code_bto_blank;
        }

    case 18: /* code_bto_elevRoad */
    case 19: /* code_bto_elevRoad (elevated span) */
        if (world_pos->y - terrainHeight <= BTO_ELEVATED_MIN_CLEARANCE) {
            goto code_bto_blank;
        }
        track_object_render_enabled = 0;
        goto solidRoad_entry;

    case 20: /* code_bto_solidRoad */
    solidRoad_entry:
        {
            int absNextX = bto_abs_int(nextElemPos.x);

            if (absNextX <= BTO_ROAD_HALF_WIDTH) {
                planindex = BTO_PLAN_SOLID_ROAD;
                current_surf_type = surfaceType;
                if (track_object_render_enabled != 0) {
                    if (nextElemPos.z >= BTO_RAMP_FRONT_WALL_Z) {
                        wallindex = BTO_WALLIDX_RAMP_FRONT;
                    } else if (nextElemPos.z <= BTO_SOLIDROAD_REAR_WALL_Z) {
                        wallindex = BTO_WALLIDX_SOLIDROAD_BACK;
                    }
                }
                if (absElemX < BTO_ROAD_HALF_WIDTH) goto code_bto_blank;
                wallHeight = BTO_WALL_HEIGHT_RAIL;
                if (elemPos.x < 0) {
                    wallindex = BTO_WALLIDX_SIDE_LEFT;
                    goto code_bto_blank;
                }
                wallindex = BTO_WALLIDX_SIDE_RIGHT;
                goto code_bto_blank;
            }

            /* absNextX > road half-width */
            if (track_object_render_enabled == 0) goto code_bto_blank;
            if (absElemX > BTO_ROAD_HALF_WIDTH) goto code_bto_blank;

            planindex = BTO_PLAN_SOLID_ROAD;
            wallHeight = BTO_WALL_HEIGHT_RAIL;
            wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;
            if (nextElemPos.x >= 0) {
                wallindex = BTO_WALLIDX_SIDE_RIGHT;
                goto code_bto_blank;
            }
            wallindex = BTO_WALLIDX_SIDE_LEFT;
            goto code_bto_blank;
        }

    case 21: /* code_bto_elevCorner */
        if (world_pos->y - terrainHeight <= BTO_ELEVATED_MIN_CLEARANCE) {
            goto code_bto_blank;
        }
        {
            int pz_adj = elemPos.z + BTO_TURN_OFFSET_LARGE;
            int px_adj = elemPos.x + BTO_TURN_OFFSET_LARGE;
            turnRadius = polarRadius2D(pz_adj, px_adj) - BTO_TURN_RADIUS_BASE_LARGE;
            if (turnRadius <= BTO_ELEVCORNER_RADIUS_MIN) goto code_bto_blank;
            if (turnRadius >= BTO_ELEVCORNER_RADIUS_MAX) goto code_bto_blank;

            current_surf_type = surfaceType;
            planindex = BTO_PLAN_SOLID_ROAD;
            track_object_render_enabled = 0;

            if (turnRadius >= BTO_ELEVCORNER_WALL_FREE_MIN && turnRadius <= BTO_ELEVCORNER_WALL_FREE_MAX) {
                goto code_bto_blank;
            }

            /* Compute wall index from polar angle */
            {
                int angle = polarAngle(pz_adj, px_adj);
                angle &= BTO_POLAR_ANGLE_MASK_LOW; /* sub ah,ah equivalent */
                angle = angle * BTO_POLAR_ANGLE_STEP_MULT;
                tempValue22 = angle;
                angle = angle >> 8;
                angle = -(angle - BTO_POLAR_ANGLE_STEP_BASE);
                tempValue1E = angle;

                wallHeight = BTO_WALL_HEIGHT_RAIL;
                elRdWallRelated = BTO_ELRD_WALL_SHORT;

                if (turnRadius < 0) {
                    wallindex = angle + BTO_WALLIDX_ELEVCORNER_INNER;
                } else {
                    wallindex = angle + BTO_WALLIDX_ELEVCORNER_OUTER;
                }
                goto code_bto_blank;
            }
        }

    case 22: /* code_bto_overpass */
        if (world_pos->y - terrainHeight > BTO_ELEVATED_MIN_CLEARANCE) {
            track_object_render_enabled = 0;
            goto solidRoad_entry;
        }
        if (absElemZ <= BTO_ROAD_HALF_WIDTH) goto set_pavement;
        goto code_bto_blank;

    case 23: /* code_bto_bankEntranceB */
        tempValue1C = BTO_BANK_ENTRY_PLAN_BASE_B;
        tempValue1E = BTO_PLAN_ROAD;
        si = BTO_BANK_ENTRY_ANGLE_B;
        goto bankEntrance_common;

    case 24: /* code_bto_bankEntranceA */
        tempValue1C = BTO_BANK_ENTRY_PLAN_BASE_A;
        tempValue1E = 0;
        si = BTO_BANK_ENTRY_ANGLE_A;

    bankEntrance_common:
        if (absElemX > BTO_ROAD_HALF_WIDTH) goto code_bto_blank;

        if (tempValue1E == 0) {
            if (nextElemPos.x <= BTO_NEG_ROAD_HALF_WIDTH) {
                wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;
                wallindex = BTO_WALLIDX_SIDE_LEFT;
            }
        }

        if (tempValue1E != 0) {
            if (nextElemPos.x >= BTO_ROAD_HALF_WIDTH) {
                wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;
                wallindex = BTO_WALLIDX_SIDE_RIGHT;
            }
        }

        current_surf_type = surfaceType;

        if (elemPos.z < BTO_BANK_ENTRY_Z_MIN) {
            planindex = tempValue1C;
            goto code_bto_blank;
        }
        if (elemPos.z >= BTO_BANK_ENTRY_Z_SEG_4) {
            planindex = tempValue1C + 9;
            goto code_bto_blank;
        }

        if (elemPos.z < BTO_BANK_ENTRY_Z_SEG_1) {
            planindex = tempValue1C + 1;
            tempValue1E = 0;
            goto bankEntrance_triCalc;
        }
        if (elemPos.z < BTO_BANK_ENTRY_Z_SEG_2) {
            planindex = tempValue1C + 3;
            tempValue1E = BTO_PLAN_ROAD;
            goto bankEntrance_triCalc;
        }
        if (elemPos.z < BTO_BANK_ENTRY_Z_SEG_3) {
            planindex = tempValue1C + 5;
            tempValue1E = 2;
            goto bankEntrance_triCalc;
        }
        if (elemPos.z < BTO_BANK_ENTRY_Z_SEG_4) {
            planindex = tempValue1C + 7;
            tempValue1E = 3;
        }

    bankEntrance_triCalc:
        {
            int zAdj = elemPos.z - bkRdEntr_triang_zAdjust[tempValue1E];
            short sinVal = sin_fast((unsigned short)si);
            di = multiply_and_scale(zAdj, sinVal);
            {
                short cosVal = cos_fast((unsigned short)si);
                int result = multiply_and_scale(elemPos.x, cosVal);
                tempValue22 = result + di;
            }
            if (tempValue22 > 0) {
                planindex++;
            }
            goto code_bto_blank;
        }

    case 25: /* code_bto_bankRoad */
        if (absElemX > BTO_ROAD_HALF_WIDTH) goto code_bto_blank;
        current_surf_type = surfaceType;
        planindex = BTO_PLAN_BANK_ROAD;
        if (nextElemPos.x < BTO_ROAD_HALF_WIDTH) goto code_bto_blank;
        wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;
        wallindex = BTO_WALLIDX_SIDE_RIGHT;
        goto code_bto_blank;

    case 26: /* code_bto_bankCorner */
        {
            int pz_adj = elemPos.z + BTO_TURN_OFFSET_LARGE;
            int px_adj = elemPos.x + BTO_TURN_OFFSET_LARGE;
            turnRadius = polarRadius2D(pz_adj, px_adj) - BTO_TURN_RADIUS_BASE_LARGE;
            if (turnRadius <= BTO_BANK_CORNER_RADIUS_MIN) goto code_bto_blank;
            if (turnRadius >= BTO_BANK_CORNER_RADIUS_MAX) goto code_bto_blank;

            {
                int angle = polarAngle(pz_adj, px_adj);
                int step_raw;
                angle &= BTO_POLAR_ANGLE_MASK_LOW;
                angle = angle * BTO_POLAR_ANGLE_STEP_MULT;
                tempValue22 = angle;
                angle = angle >> 8;
                step_raw = angle;              /* raw arc step 0..17 */
                angle = -(angle - BTO_POLAR_ANGLE_STEP_BASE);       /* reversed for wall index */
                tempValue1E = angle;

                /* FIX: use raw step for planindex instead of reversed angle.
                 * The original asm had: planindex = -(step-17) + 7 = 24 - step
                 * But the plane resource data is ordered step_raw + 7, so the
                 * negation maps south-end cars to east-end planes (ip ~700)
                 * instead of the correct south-end planes (ip ~0).
                 */
                planindex = step_raw + 7;
                current_surf_type = surfaceType;

                if (turnRadius <= BTO_BANK_CORNER_WALL_MIN) goto code_bto_blank;

                wallOrientationOffset = BTO_WALL_ORIENT_OFFSET;
                wallindex = tempValue1E + BTO_WALLIDX_ELEVCORNER_OUTER;
            }
        }
        track_object_render_enabled = 0;
        goto code_bto_blank;

    case 27: /* code_bto_loop */
    {
        int effX, effZ;

        if (elemPos.z < 0) {
            tempValue1C = BTO_LOOP_PLAN_BACKWARD;
            effX = -elemPos.x;
            effZ = -elemPos.z;
        } else {
            tempValue1C = BTO_LOOP_PLAN_FORWARD;
            effX = elemPos.x;
            effZ = elemPos.z;
        }
        effectiveX = effX;
        effectiveZ = effZ;

        /* Clamp effZ to loopSurface_maxZ */
        {
            int maxZ = loopSurface_maxZ;
            int clampedZ;
            if (effectiveZ > maxZ - BTO_PLAN_ROAD) {
                if (effectiveZ > maxZ + BTO_LOOP_Z_CLAMP_MARGIN) {
                    goto code_bto_loopBase;
                }
                clampedZ = maxZ - BTO_PLAN_ROAD;
            } else {
                clampedZ = effectiveZ;
            }
            tempValue1E = clampedZ;
        }

        /* Find Z slice */
        si = 0;
        while (loopSurface_ZBounds1[si] < tempValue1E) {
            si++;
        }

        /* Check if player is above or below */
        if (world_pos->y - terrainHeight > BTO_LOOP_UPSIDE_Y) {
            /* Upside-down: invert slice index */
            si = BTO_LOOP_UPSIDE_INDEX_MAX - si;
            di = si;

            if (loopSurface_XBounds0[di] > effectiveX) goto code_bto_blank;
            if (loopSurface_XBounds1[di] + BTO_LOOP_X_WIDTH < effectiveX) goto code_bto_blank;

            if (loopSurface_XBounds1[di] < effectiveX) {
                if (loopSurface_XBounds0[di] + BTO_LOOP_X_WIDTH > effectiveX) {
                    goto loopSurface_setplan;
                }
            }

            /* Interpolate */
            {
                long divisor = (long)(loopSurface_ZBounds1[di] - loopSurface_ZBounds0[di]);
                long dividend = (long)(loopSurface_ZBounds0[di] - tempValue1E);
                long range = (long)(loopSurface_XBounds0[di] - loopSurface_XBounds1[di]);
                tempValue22 = (int)((dividend * range) / divisor);
                tempValue3C = loopSurface_XBounds0[di] + tempValue22;
            }

            if (tempValue3C >= effectiveX) goto code_bto_blank;
            if (tempValue3C + BTO_LOOP_X_WIDTH <= effectiveX) goto code_bto_blank;

        loopSurface_setplan:
            planindex = tempValue1C + si;
            current_surf_type = surfaceType;
            track_object_render_enabled = 0;
            goto code_bto_blank;
        }

/** @brief Top.
 * @param si Parameter `si`.
 * @return Function result.
 */
        /* Below the top (right-side up) */
        if (si > 1) {
            if (world_pos->y - terrainHeight < BTO_LOOP_Z_CLAMP_MARGIN) {
                goto code_bto_loopBase;
            }
        }

        di = si;
        if (loopSurface_XBounds0[di] > effectiveX) goto code_bto_loopBase;
        if (loopSurface_XBounds1[di] + BTO_LOOP_X_WIDTH < effectiveX) goto code_bto_loopBase;

        if (loopSurface_XBounds1[di] < effectiveX) {
            if (loopSurface_XBounds0[di] + BTO_LOOP_X_WIDTH > effectiveX) {
                goto loopSurface_setplan;
            }
        }

        /* Check if bounds are same (no interpolation needed) */
        if (loopSurface_XBounds0[di] == loopSurface_XBounds1[di]) goto code_bto_loopBase;

        /* Interpolate for right-side up */
        {
            long divisor = (long)(loopSurface_ZBounds1[di] - loopSurface_ZBounds0[di]);
            long dividend = (long)(loopSurface_ZBounds0[di] - tempValue1E);
            long range = (long)(loopSurface_XBounds0[di] - loopSurface_XBounds1[di]);
            tempValue22 = (int)((dividend * range) / divisor);
            tempValue3C = loopSurface_XBounds0[di] + tempValue22;
        }

        if (tempValue3C >= effectiveX) goto code_bto_loopBase;
        if (tempValue3C + BTO_LOOP_X_WIDTH <= effectiveX) goto code_bto_loopBase;
        goto loopSurface_setplan;
    }

    code_bto_loopBase:
        si = 0;
        while (loopBase_ZBounds1[si] < effectiveZ) {
            si++;
        }

        /* Interpolate inner X bound */
        {
            long divisor = (long)(loopBase_ZBounds1[si] - loopBase_ZBounds0[si]);
            long dividend = (long)(effectiveZ - loopBase_ZBounds0[si]);
            long rangeInn = (long)(loopBase_InnXBounds1[si] - loopBae_InnXBounds0[si]);
            tempValue1E = (int)((dividend * rangeInn) / divisor) + loopBae_InnXBounds0[si];
        }

        /* Interpolate outer X bound */
        {
            long divisor = (long)(loopBase_ZBounds1[si] - loopBase_ZBounds0[si]);
            long dividend = (long)(effectiveZ - loopBase_ZBounds0[si]);
            long rangeOut = (long)(loopBase_OutXBounds1[si] - loopBase_OutXBounds0[si]);
            tempValue22 = (int)((dividend * rangeOut) / divisor) + loopBase_OutXBounds0[si];
        }

        if (effectiveX < tempValue1E) goto code_bto_blank;
        if (effectiveX <= tempValue22) goto set_pavement;
        goto code_bto_blank;

    case 28: /* code_bto_tunnel */
    {
        int posY = world_pos->y - terrainHeight;
        int nextY = next_world_pos->y - terrainHeight;

        if (posY >= BTO_TUNNEL_INTERIOR_Y || nextY >= BTO_TUNNEL_INTERIOR_Y) {
            /* Above tunnel */
            if (absElemX >= BTO_TUNNEL_WALL_MAX_X) goto code_bto_blank;
            current_surf_type = surfaceType;
            planindex = BTO_PLAN_TUNNEL_TOP;
            goto code_bto_blank;
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
                goto code_bto_blank;
            }
            if (nextElemPos.z >= BTO_TURN_OFFSET_SMALL) {
                wallindex = BTO_WALLIDX_TUNNEL_REAR;
                goto code_bto_blank;
            }
            if (nextElemPos.x <= BTO_TUNNEL_WALL_MIN_X) {
                wallindex = BTO_WALLIDX_TUNNEL_RIGHT_INNER;
                goto code_bto_blank;
            }
            if (nextElemPos.x >= BTO_TUNNEL_WALL_MAX_X) {
                wallindex = BTO_WALLIDX_TUNNEL_RIGHT_OUTER;
                goto code_bto_blank;
            }
            goto code_bto_blank;
        }

        if (elemPos.x > BTO_NEG_ROAD_HALF_WIDTH) goto code_bto_blank;
        if (elemPos.x < BTO_TUNNEL_WALL_MIN_NEG_X) goto code_bto_blank;

        /* Left wall zone */
        wallHeight = BTO_TUNNEL_INTERIOR_Y;
        if (nextElemPos.z <= BTO_TUNNEL_FRONT_WALL_Z) {
            wallindex = BTO_WALLIDX_TUNNEL_FRONT;
            goto code_bto_blank;
        }
        if (nextElemPos.z >= BTO_TURN_OFFSET_SMALL) {
            wallindex = BTO_WALLIDX_TUNNEL_REAR;
            goto code_bto_blank;
        }
        if (nextElemPos.x >= BTO_NEG_ROAD_HALF_WIDTH) {
            wallindex = BTO_WALLIDX_TUNNEL_LEFT_INNER;
            goto code_bto_blank;
        }
        if (nextElemPos.x <= BTO_TUNNEL_WALL_MIN_NEG_X) {
            wallindex = BTO_WALLIDX_TUNNEL_LEFT_OUTER;
            goto code_bto_blank;
        }
        goto code_bto_blank;
    }

    case 29: /* code_bto_pipeEntrance */
    {
        int absNextX = bto_abs_int(nextElemPos.x);

        if (absNextX >= BTO_PIPE_ENTR_SIDE_MIN_X && absElemX <= BTO_PIPE_ENTR_SIDE_MAX_X) {
            wallHeight = BTO_HALFPIPE_UPPER_Y;
            if (nextElemPos.x > 0) {
                wallindex = BTO_WALLIDX_PIPE_ENTR_RIGHT;
                goto code_bto_blank;
            }
            wallindex = BTO_WALLIDX_PIPE_ENTR_LEFT;
            goto code_bto_blank;
        }

        if (absElemX >= BTO_PIPE_ENTR_SIDE_MIN_X) goto code_bto_blank;

        if (world_pos->y - terrainHeight >= BTO_PIPE_ENTR_MAX_Y) goto code_bto_blank;

        current_surf_type = surfaceType;

        if (absElemX < BTO_PIPE_NEAR_CENTER_X) {
            planindex = BTO_PLAN_PIPE_ENTR_CENTER;
            goto code_bto_blank;
        }

        if (elemPos.x < BTO_PIPE_TRI_CENTER_LEFT) {
            planindex = BTO_PLAN_PIPE_ENTR_LEFT_OUTER;
            tempValue1E = BTO_PIPE_TRI_LEFT_OUTER_X;
            si = BTO_PIPE_TRI_LEFT_OUTER_ANGLE;
            goto pipeEntrance_triCalc;
        }
        if (elemPos.x < 0) {
            planindex = BTO_PLAN_PIPE_ENTR_LEFT_INNER;
            tempValue1E = BTO_PIPE_TRI_LEFT_INNER_X;
            si = BTO_PIPE_TRI_LEFT_INNER_ANGLE;
            goto pipeEntrance_triCalc;
        }
        if (elemPos.x > BTO_HALFPIPE_FLOOR_MAX_X) {
            planindex = BTO_PLAN_PIPE_ENTR_RIGHT_OUTER;
            tempValue1E = BTO_PIPE_TRI_RIGHT_OUTER_X;
            si = BTO_PIPE_TRI_RIGHT_OUTER_ANGLE;
            goto pipeEntrance_triCalc;
        }
        planindex = BTO_PLAN_PIPE_ENTR_RIGHT_INNER;
        tempValue1E = BTO_PIPE_TRI_RIGHT_INNER_X;
        si = BTO_PIPE_TRI_RIGHT_INNER_ANGLE;

    pipeEntrance_triCalc:
        {
            short sinVal = sin_fast((unsigned short)si);
            di = multiply_and_scale(elemPos.z, sinVal);
            {
                int cx = elemPos.x - tempValue1E;
                short cosVal = cos_fast((unsigned short)si);
                int result = multiply_and_scale(cx, cosVal);
                tempValue22 = result + di;
            }
            if (tempValue22 < 0) {
                planindex++;
                goto code_bto_blank;
            }
            goto code_bto_blank;
        }
    }

    case 30: /* code_bto_pipe */
        tempValue22 = 0;
        goto pipe_common;

    case 31: /* code_bto_halfPipe */
        tempValue22 = 1;

    pipe_common:
    {
        int absNextX = bto_abs_int(nextElemPos.x);

        if (absNextX >= BTO_PIPE_ENTR_SIDE_MAX_X && absElemX <= BTO_PIPE_ENTR_SIDE_MAX_X) {
            wallHeight = BTO_HALFPIPE_UPPER_Y;
            if (nextElemPos.x > 0) {
                wallindex = BTO_WALLIDX_PIPE_RIGHT;
                goto code_bto_blank;
            }
            wallindex = BTO_WALLIDX_PIPE_LEFT;
            goto code_bto_blank;
        }

        if (absElemX >= BTO_PIPE_ENTR_SIDE_MAX_X) goto code_bto_blank;

        if (world_pos->y - terrainHeight >= BTO_HALFPIPE_MAX_Y) goto code_bto_blank;

        if (absElemX < BTO_HALFPIPE_SURF_LIMIT_X) {
            current_surf_type = surfaceType;
        }

        /* Determine if above or below pipe center */
        if (world_pos->y - terrainHeight > BTO_HALFPIPE_UPPER_Y) {
            tempValue1E = BTO_PLAN_ROAD;
        } else {
            tempValue1E = 0;
        }

        /* Half-pipe special floor case */
        if (tempValue22 != 0 && tempValue1E == 0 &&
            absElemX <= BTO_HALFPIPE_FLOOR_MAX_X && absElemZ <= BTO_HALFPIPE_FLOOR_MAX_Z) {
            planindex = BTO_PLAN_HALFPIPE_FLOOR;
            if (nextElemPos.z < BTO_HALFPIPE_FLOOR_FRONT_Z) {
                wallindex = BTO_WALLIDX_HALFPIPE_FRONT;
                goto code_bto_blank;
            }
            if (nextElemPos.z >= BTO_HALFPIPE_FLOOR_REAR_Z) {
                wallindex = BTO_WALLIDX_HALFPIPE_REAR;
                goto code_bto_blank;
            }
            goto code_bto_blank;
        }

        /* Height > 88 and below center? */
        if (world_pos->y - terrainHeight > BTO_HALFPIPE_LOWER_Y && tempValue1E == 0) {
            if (elemPos.x < 0) {
                planindex = BTO_PLAN_PIPE_LOWER_LEFT;
                goto code_bto_blank;
            }
            planindex = BTO_PLAN_PIPE_LOWER_RIGHT;
            goto code_bto_blank;
        }

        /* Pipe section selection */
        if (absElemX < BTO_PIPE_NEAR_CENTER_X) {
            if (tempValue1E != 0) {
                planindex = BTO_PLAN_PIPE_TOP_CENTER;
            } else {
                planindex = BTO_PLAN_PIPE_BOTTOM_CENTER;
            }
            goto code_bto_blank;
        }

        if (elemPos.x < BTO_PIPE_TRI_CENTER_LEFT) {
            if (tempValue1E != 0) {
                planindex = BTO_PLAN_PIPE_TOP_LEFT_OUTER;
            } else {
                planindex = BTO_PLAN_PIPE_BOTTOM_LEFT_OUTER;
            }
            goto code_bto_blank;
        }

        if (elemPos.x < 0) {
            if (tempValue1E != 0) {
                planindex = BTO_PLAN_PIPE_TOP_LEFT_INNER;
            } else {
                planindex = BTO_PLAN_PIPE_BOTTOM_LEFT_INNER;
            }
            goto code_bto_blank;
        }

        if (elemPos.x > BTO_HALFPIPE_FLOOR_MAX_X) {
            if (tempValue1E != 0) {
                planindex = BTO_PLAN_PIPE_TOP_RIGHT_OUTER;
            } else {
                planindex = BTO_PLAN_PIPE_BOTTOM_RIGHT_OUTER;
            }
            goto code_bto_blank;
        }

        if (tempValue1E != 0) {
            planindex = BTO_PLAN_PIPE_TOP_RIGHT_INNER;
        } else {
            planindex = BTO_PLAN_PIPE_BOTTOM_RIGHT_INNER;
        }
        goto code_bto_blank;
    }

    case 32: /* code_bto_corkUdLH: cork u/d A */
        tempValue1E = -elemPos.x;
        tempValue22 = BTO_CORK_PLAN_UD_LH;
        corkInnerWallBase = BTO_CORK_WALL_BASE_INNER_LH;
        corkOuterWallBase = BTO_CORK_WALL_BASE_OUTER_LH;
        goto corkUd_common;

    case 33: /* code_bto_corkUdRH: cork u/d B */
        tempValue1E = elemPos.x;
        tempValue22 = BTO_CORK_PLAN_UD_RH;
        corkInnerWallBase = 0;
        corkOuterWallBase = BTO_CORK_WALL_BASE_OUTER_RH;

    corkUd_common:
        corkFlag = 1;

        /* Cork descending entry ramp check */
        if (elemPos.z < 0) {
            if (world_pos->y - terrainHeight < BTO_LOOP_Z_CLAMP_MARGIN) {
                if (tempValue1E > 0) {
                    if (tempValue1E >= BTO_TURN_SMALL_OUTER) goto code_bto_blank;
                    if (tempValue1E <= BTO_TURN_SMALL_INNER) goto code_bto_blank;
                    current_surf_type = surfaceType;
                    planindex = tempValue22;
                    goto code_bto_blank;
                }
            }
        }

        /* Cork ascending exit ramp check */
        if (elemPos.z > 0) {
            if (world_pos->y - terrainHeight > BTO_CORK_EXIT_MIN_Y) {
                if (tempValue1E < BTO_CORK_RADIUS_MAX && tempValue1E > BTO_CORK_RADIUS_MIN) {
                    wallHeight = BTO_WALL_HEIGHT_RAIL;
                    elRdWallRelated = BTO_ELRD_WALL_SHORT;
                    if (tempValue1E > BTO_CORK_RADIUS_MID)
                        ax = corkInnerWallBase;
                    else
                        ax = corkOuterWallBase;
                    wallindex = ax + BTO_CORK_EXIT_WALL_OFFSET;
                    current_surf_type = surfaceType;
                    planindex = tempValue22 + BTO_CORK_EXIT_PLAN_OFFSET;
                    track_object_render_enabled = 0;
                    goto code_bto_blank;
                }
            }
        }

        /* Main cork spiral region */
        turnRadius = polarRadius2D(elemPos.z, tempValue1E);
        if (turnRadius <= BTO_CORK_RADIUS_MIN) goto code_bto_blank;
        if (turnRadius >= BTO_CORK_RADIUS_MAX) goto code_bto_blank;

        {
            int angle = polarAngle(elemPos.z, tempValue1E);
            angle = -(angle - BTO_CORK_ANGLE_BASE);
            angle &= BTO_ORIENT_MASK; /* and ah, 3 */
            si = (angle * BTO_CORK_SEGMENT_COUNT) >> BTO_WORLD_TO_TILE_SHIFT;

            planindex = tempValue22 + si + 1;
            current_surf_type = surfaceType;
            track_object_render_enabled = 0;

            wallHeight = BTO_WALL_HEIGHT_RAIL;
            elRdWallRelated = BTO_ELRD_WALL_SHORT;

            if (turnRadius - BTO_CORK_RADIUS_MID > BTO_CORK_INNER_WALL_DELTA) {
                ax = corkInnerWallBase;
                wallindex = ax + si;
                goto code_bto_blank;
            }
            if (turnRadius - BTO_CORK_RADIUS_MID < BTO_CORK_OUTER_WALL_DELTA) {
                ax = corkOuterWallBase;
                wallindex = ax + si;
                goto code_bto_blank;
            }
            goto code_bto_blank;
        }

    case 34: /* code_bto_slalom */
        if (absElemX < BTO_ROAD_HALF_WIDTH) {
            current_surf_type = surfaceType;
        }

        /* First slalom block test */
        if (elemPos.x >= BTO_SLALOM_BLOCK1_MIN_X && elemPos.x <= BTO_SLALOM_BLOCK1_MAX_X &&
            elemPos.z > BTO_SLALOM_BLOCK1_MIN_Z && elemPos.z < BTO_SLALOM_BLOCK1_MAX_Z) {
            wallHeight = BTO_WALL_HEIGHT_RAIL;
            if (nextElemPos.z <= BTO_SLALOM_BLOCK1_MIN_Z) {
                wallindex = BTO_WALLIDX_SLALOM_B1_NZ;
                goto code_bto_blank;
            }
            if (nextElemPos.z > BTO_SLALOM_BLOCK1_MAX_Z) {
                wallindex = BTO_WALLIDX_SLALOM_B1_PZ;
                goto code_bto_blank;
            }
            if (nextElemPos.x < BTO_SLALOM_BLOCK1_MIN_X) {
                wallindex = BTO_WALLIDX_SLALOM_B1_NX;
                goto code_bto_blank;
            }
            if (nextElemPos.x > BTO_SLALOM_BLOCK1_MAX_X) {
                wallindex = BTO_WALLIDX_SLALOM_B1_PX;
                goto code_bto_blank;
            }
            goto code_bto_blank;
        }

        /* Second slalom block test: x in [-97, -23], z in (241, 271) */
        if (elemPos.x > BTO_SLALOM_BLOCK2_MAX_X) goto code_bto_blank;
        if (elemPos.x < BTO_SLALOM_BLOCK2_MIN_X) goto code_bto_blank;
        if (elemPos.z >= BTO_SLALOM_BLOCK2_MAX_Z) goto code_bto_blank;
        if (elemPos.z <= BTO_SLALOM_BLOCK2_MIN_Z) goto code_bto_blank;

        wallHeight = BTO_WALL_HEIGHT_RAIL;
        if (nextElemPos.z > BTO_SLALOM_BLOCK2_MAX_Z) {
            wallindex = BTO_WALLIDX_SLALOM_B2_PZ;
            goto code_bto_blank;
        }
        if (nextElemPos.z < (short)BTO_SLALOM_BLOCK2_MIN_Z) {
            wallindex = BTO_WALLIDX_SLALOM_B2_NZ;
            goto code_bto_blank;
        }
        if (nextElemPos.x > BTO_SLALOM_BLOCK2_MAX_X) {
            wallindex = BTO_WALLIDX_SLALOM_B2_PX;
            goto code_bto_blank;
        }
        if (nextElemPos.x < BTO_SLALOM_BLOCK2_MIN_X) {
            wallindex = BTO_WALLIDX_SLALOM_B2_NX;
            goto code_bto_blank;
        }
        goto code_bto_blank;

    case 35: /* code_bto_corkLr: cork left/right */
        if (absElemX >= BTO_CORKLR_MAX_X) goto code_bto_blank;

        if (world_pos->y - terrainHeight >= BTO_HALFPIPE_MAX_Y) goto code_bto_blank;

        current_surf_type = surfaceType;

        if (world_pos->y - terrainHeight > BTO_HALFPIPE_UPPER_Y) {
            tempValue1E = BTO_PLAN_ROAD;
        } else {
            tempValue1E = 0;
        }

        tempValue22 = 0;

        if (world_pos->y - terrainHeight > BTO_HALFPIPE_LOWER_Y && tempValue1E == 0) {
            if (elemPos.x < 0) {
                tempValue22 = 3;
            } else {
                tempValue22 = 9;
            }
            goto corkLr_planCheck;
        }

        if (absElemX < BTO_PIPE_NEAR_CENTER_X) {
            if (tempValue1E != 0) {
                tempValue22 = 6;
            }
            goto corkLr_planCheck;
        }

        if (elemPos.x < BTO_PIPE_TRI_CENTER_LEFT) {
            if (tempValue1E != 0) {
                tempValue22 = 4;
            } else {
                tempValue22 = 2;
            }
            goto corkLr_planCheck;
        }

        if (elemPos.x < 0) {
            if (tempValue1E != 0) {
                tempValue22 = 5;
            } else {
                tempValue22 = 1;
            }
            goto corkLr_planCheck;
        }

        if (elemPos.x > BTO_HALFPIPE_FLOOR_MAX_X) {
            if (tempValue1E != 0) {
                tempValue22 = 8;
            } else {
                tempValue22 = BTO_CORKLR_BUCKET_RIGHT_OUTER;
            }
            goto corkLr_planCheck;
        }

        if (tempValue1E != 0) {
            tempValue22 = 7;
        } else {
            tempValue22 = BTO_CORKLR_BUCKET_RIGHT_INNER;
        }

    corkLr_planCheck:
        if (tempValue22 != 0) {
            di = tempValue22;
            if (corkLR_negZBound[di] >= elemPos.z) goto corkLr_noPlan;
            if (corkLR_posZBound[di] <= elemPos.z) goto corkLr_noPlan;
            planindex = tempValue22 + BTO_CORKLR_PLAN_BASE;
        }
    corkLr_noPlan:

        if (planindex != 0) goto code_bto_blank;

        if (absElemZ >= BTO_TURN_OFFSET_SMALL) goto code_bto_blank;

        wallindex = BTO_CORKLR_WALL_INDEX;
        corkFlag = 1;
        wallHeight = BTO_CORKLR_WALL_HEIGHT;
        goto code_bto_blank;

    case 36: /* blank */
    case 37: /* blank */
    case 38: /* blank */
    case 39: /* blank */
    case 40: /* blank */
    case 41: /* blank */
    case 42: /* blank */
    case 43: /* blank */
    case 44: /* blank */
    case 45: /* blank */
    case 46: /* blank */
    case 47: /* blank */
    case 48: /* blank */
    case 49: /* blank */
    case 50: /* blank */
    case 51: /* blank */
    case 52: /* blank */
    case 53: /* blank */
    case 54: /* blank */
    case 55: /* blank */
    case 56: /* blank */
    case 57: /* blank */
    case 58: /* blank */
    case 59: /* blank */
    case 60: /* blank */
    case 61: /* blank */
    case 62: /* blank */
    case 63: /* blank (empty) */
        goto code_bto_blank;

    case 64: /* code_bto_barn */
        if (absElemX > BTO_BUILDING_BARN_HALF_SIZE) goto code_bto_blank;
        if (absElemZ > BTO_BUILDING_BARN_HALF_SIZE) goto code_bto_blank;
        wallHeight = BTO_BUILDING_BARN_HEIGHT;
        if (nextElemPos.z <= BTO_BUILDING_BARN_WALL_NZ) {
            wallindex = BTO_WALLIDX_BARN_NZ;
            goto code_bto_blank;
        }
        if (nextElemPos.z >= BTO_BUILDING_BARN_HALF_SIZE) {
            wallindex = BTO_WALLIDX_BARN_PZ;
            goto code_bto_blank;
        }
        if (nextElemPos.x >= BTO_BUILDING_BARN_HALF_SIZE) {
            wallindex = BTO_WALLIDX_BARN_PX;
            goto code_bto_blank;
        }
        if (nextElemPos.x <= BTO_BUILDING_BARN_WALL_NZ) {
            wallindex = BTO_WALLIDX_BARN_NX;
            goto code_bto_blank;
        }
        goto code_bto_blank;

    case 65: /* code_bto_gasStation */
        if (elemPos.x < BTO_BUILDING_GAS_MIN_X) goto code_bto_blank;
        if (elemPos.x > BTO_BUILDING_GAS_MAX_X) goto code_bto_blank;
        if (absElemZ > BTO_BUILDING_GAS_HALF_Z) goto code_bto_blank;
        wallHeight = BTO_BUILDING_GAS_HEIGHT;
        if (nextElemPos.z <= BTO_BUILDING_GAS_NZ) {
            wallindex = BTO_WALLIDX_GAS_NZ;
            goto code_bto_blank;
        }
        if (nextElemPos.z >= BTO_BUILDING_GAS_HALF_Z) {
            wallindex = BTO_WALLIDX_GAS_PZ;
            goto code_bto_blank;
        }
        if (nextElemPos.x <= BTO_BUILDING_GAS_MIN_X) {
            wallindex = BTO_WALLIDX_GAS_NX;
            goto code_bto_blank;
        }
        if (nextElemPos.x >= BTO_BUILDING_GAS_MAX_X) {
            wallindex = BTO_WALLIDX_GAS_PX;
            goto code_bto_blank;
        }
        goto code_bto_blank;

    case 66: /* code_bto_joes */
        if (absElemX > BTO_BUILDING_JOES_HALF_X) goto code_bto_blank;
        if (absElemZ > BTO_BUILDING_JOES_HALF_Z) goto code_bto_blank;
        wallHeight = BTO_BUILDING_JOES_HEIGHT;
        if (nextElemPos.z <= BTO_BUILDING_JOES_NZ) {
            wallindex = BTO_WALLIDX_JOES_NZ;
            goto code_bto_blank;
        }
        if (nextElemPos.z >= BTO_BUILDING_JOES_HALF_Z) {
            wallindex = BTO_WALLIDX_JOES_PZ;
            goto code_bto_blank;
        }
        if (nextElemPos.x <= BTO_BUILDING_JOES_NX) {
            wallindex = BTO_WALLIDX_JOES_NX;
            goto code_bto_blank;
        }
        if (nextElemPos.x >= BTO_BUILDING_JOES_HALF_X) {
            wallindex = BTO_WALLIDX_JOES_PX;
            goto code_bto_blank;
        }
        goto code_bto_blank;

    case 67: /* code_bto_office */
        if (absElemX > BTO_BUILDING_OFFICE_HALF_SIZE) goto code_bto_blank;
        if (absElemZ > BTO_BUILDING_OFFICE_HALF_SIZE) goto code_bto_blank;
        wallHeight = BTO_BUILDING_OFFICE_HEIGHT;
        if (nextElemPos.z <= BTO_BUILDING_OFFICE_NZ) {
            wallindex = BTO_WALLIDX_OFFICE_NZ;
            goto code_bto_blank;
        }
        if (nextElemPos.z >= BTO_BUILDING_OFFICE_HALF_SIZE) {
            wallindex = BTO_WALLIDX_OFFICE_PZ;
            goto code_bto_blank;
        }
        if (nextElemPos.x <= BTO_BUILDING_OFFICE_NZ) {
            wallindex = BTO_WALLIDX_OFFICE_NX;
            goto code_bto_blank;
        }
        if (nextElemPos.x >= BTO_BUILDING_OFFICE_HALF_SIZE) {
            wallindex = BTO_WALLIDX_OFFICE_PX;
            goto code_bto_blank;
        }
        goto code_bto_blank;

    case 68: /* code_bto_windmill */
        if (absElemX > BTO_BUILDING_WINDMILL_HALF_SIZE) goto code_bto_blank;
        if (absElemZ > BTO_BUILDING_WINDMILL_HALF_SIZE) goto code_bto_blank;
        wallHeight = BTO_BUILDING_WINDMILL_HEIGHT;
        if (nextElemPos.z <= BTO_BUILDING_WINDMILL_NZ) {
            wallindex = BTO_WALLIDX_WINDMILL_NZ;
            goto code_bto_blank;
        }
        if (nextElemPos.z >= BTO_BUILDING_WINDMILL_HALF_SIZE) {
            wallindex = BTO_WALLIDX_WINDMILL_PZ;
            goto code_bto_blank;
        }
        if (nextElemPos.x <= BTO_BUILDING_WINDMILL_NZ) {
            wallindex = BTO_WALLIDX_WINDMILL_NX;
            goto code_bto_blank;
        }
        if (nextElemPos.x >= BTO_BUILDING_WINDMILL_HALF_SIZE) {
            wallindex = BTO_WALLIDX_WINDMILL_PX;
            goto code_bto_blank;
        }
        goto code_bto_blank;

    case 69: /* code_bto_ship */
        if (elemPos.x < BTO_BUILDING_SHIP_MIN_X) goto code_bto_blank;
        if (elemPos.x > BTO_BUILDING_SHIP_MAX_X) goto code_bto_blank;
        if (absElemZ > BTO_BUILDING_SHIP_HALF_Z) goto code_bto_blank;
        wallHeight = BTO_BUILDING_SHIP_HEIGHT;
        if (nextElemPos.z <= BTO_BUILDING_SHIP_NZ) {
            wallindex = BTO_WALLIDX_SHIP_NZ;
            goto code_bto_blank;
        }
        if (nextElemPos.z >= BTO_BUILDING_SHIP_HALF_Z) {
            wallindex = BTO_WALLIDX_SHIP_PZ;
            goto code_bto_blank;
        }
        if (nextElemPos.x <= BTO_BUILDING_SHIP_MIN_X) {
            wallindex = BTO_WALLIDX_SHIP_NX;
            goto code_bto_blank;
        }
        if (nextElemPos.x >= BTO_BUILDING_SHIP_MAX_X) {
            wallindex = BTO_WALLIDX_SHIP_PX;
            goto code_bto_blank;
        }
        goto code_bto_blank;

    case 70: /* pine - blank */
    case 71: /* cactus - blank */
    case 72: /* tennis - blank */
    case 73: /* palm - blank */
    case 74: /* extra */
        goto code_bto_blank;

    default:
        goto code_bto_blank;
    } /* end of main switch */

    /* ---- Inline targets for goto ---- */
set_pavement:
    current_surf_type = surfaceType;
    goto code_bto_blank;

do_sCorner:
    {
        int pz_adj = elemPos.z + BTO_TURN_OFFSET_SMALL;
        int px_adj = elemPos.x + BTO_TURN_OFFSET_SMALL;
        turnRadius = polarRadius2D(pz_adj, px_adj);
        if (turnRadius > BTO_TURN_SMALL_INNER) {
            if (turnRadius < BTO_TURN_SMALL_OUTER) goto set_pavement;
        }
        goto code_bto_blank;
    }

do_lCorner:
    {
        int pz_adj = elemPos.z + BTO_TURN_OFFSET_LARGE;
        int px_adj = elemPos.x + BTO_TURN_OFFSET_LARGE;
        turnRadius = polarRadius2D(pz_adj, px_adj);
        if (turnRadius > BTO_TURN_LARGE_INNER) {
            if (turnRadius < BTO_TURN_LARGE_OUTER) goto set_pavement;
        }
        goto code_bto_blank;
    }

code_bto_blank:
    /* ===== Hill slope parsing ===== */
    if ((unsigned char)terrainTile < BTO_TERRAIN_HILL_MIN) goto exit_func;

    /* Recalculate element coords relative to standard tile center */
    {
        int colIdx = (int)(signed char)tileCol;
        int rowIdx = (int)(signed char)tileRow;
        elemPos.x = world_pos->x - trackcenterpos2[colIdx];
        elemPos.z = world_pos->z - terraincenterpos[rowIdx];
    }

    /* Hill slope orientation dispatch (terrain types 7..18) */
    {
        unsigned int terrIdx = (unsigned char)terrainTile - BTO_TERRAIN_HILL_MIN;
        if (terrIdx >= BTO_SLOPE_ORIENT_TABLE_COUNT) goto after_hillOrient;

        /* off_1F87E jump table: 12 entries, pattern: 0,1,2,3 repeated 3 times */
        switch (terrIdx) {
        case 0: /* loc_1F82A */
        case 4:
        case 8:
            elementOrientation = 0;
            break;
        case 1: /* loc_1F832 */
        case 5:
        case 9:
            elementOrientation = BTO_ORIENT_270;
            {
                int tmp = elemPos.x;
                elemPos.x = elemPos.z;
                elemPos.z = -tmp;
            }
            break;
        case 2: /* loc_1F84E */
        case 6:
        case 10:
            elementOrientation = BTO_ORIENT_180;
            elemPos.z = -elemPos.z;
            elemPos.x = -elemPos.x;
            break;
        case 3: /* loc_1F866 */
        case 7:
        case 11:
            elementOrientation = BTO_ORIENT_90;
            {
                int tmp = elemPos.x;
                elemPos.x = -elemPos.z;
                elemPos.z = tmp;
            }
            break;
        }
    }

after_hillOrient:
    /* Hill terrain type handling */
    {
        unsigned int terrVal = (unsigned char)terrainTile;

        if (terrVal < BTO_TERRAIN_HILL_MIN) goto exit_func;

        if (terrVal <= BTO_TERRAIN_HILL_MAX) {
/** @brief Slope.
 * @param planindex Parameter `planindex`.
 * @return Function result.
 */
            /* Simple hill slope (terrain 7-10) */
            if (planindex == 0) {
                planindex = BTO_SLOPE_PLAN_DEFAULT;
            }
            goto exit_func;
        }

        if (terrVal < BTO_HILL_TERRAIN_MIN_CONCAVE) goto exit_func;

        if (terrVal <= BTO_HILL_TERRAIN_MAX_CONCAVE) {
            /* Terrain 11..14: concave hill (coast-like test) */
            {
                short sinVal = sin_fast((unsigned short)BTO_HILL_COAST_ANGLE);
                di = multiply_and_scale(elemPos.z, sinVal);
                {
                    short cosVal = cos_fast((unsigned short)BTO_HILL_COAST_ANGLE);
                    int result = multiply_and_scale(elemPos.x, cosVal);
                    tempValue22 = result + di;
                }
                if (tempValue22 < 0) {
                    planindex = 4;
                }
            }
            goto exit_func;
        }

        if (terrVal < BTO_HILL_TERRAIN_MIN_CONVEX) goto exit_func;

        if (terrVal <= BTO_HILL_TERRAIN_MAX_CONVEX) {
            /* Terrain 15..18: convex hill */
            {
                short sinVal = sin_fast((unsigned short)BTO_HILL_COAST_ANGLE);
                di = multiply_and_scale(elemPos.z, sinVal);
                {
                    short cosVal = cos_fast((unsigned short)BTO_HILL_COAST_ANGLE);
                    int result = multiply_and_scale(elemPos.x, cosVal);
                    tempValue22 = result + di;
                }
                if (tempValue22 > 0) {
                    planindex = 5;
                    goto exit_func;
                }
                terrainHeight = BTO_HILL_HEIGHT;
            }
            goto exit_func;
        }
        /* terrVal > 18 */
        goto exit_func;
    }

exit_func:
    /* ===== Finalize planindex with orientation offset ===== */
    {
        if (planindex > 0) {
            planindex <<= BTO_PLANINDEX_SHIFT;
            if (elementOrientation == BTO_ORIENT_270) {
                planindex += 1;
            }
            else if (elementOrientation == BTO_ORIENT_180) {
                planindex += 2;
            }
            else if (elementOrientation == BTO_ORIENT_90) {
                planindex += 3;
            }
            /* else orient 0: no offset */
        }
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
    } else {
        terrainHeight += BTO_GRASS_HEIGHT_BIAS;
    }

    /* ===== Wall position computation ===== */
    if (wallindex < 0) return;

    {
        /* wallptr is an array of 6-byte entries: [orientation:2][startX:2][startZ:2] */
        short * wEntry = wallptr + wallindex * BTO_WALL_ENTRY_STRIDE;

        /* Compute wall orientation */
        {
            int wallRot = wEntry[0]; /* original wall orientation */
            wallRot = -wallRot + elementOrientation + wallOrientationOffset;
            wallRot &= BTO_ORIENT_MASK; /* and ah, 3 */
            wallOrientation = wallRot;
        }

        /* Rotate wall position based on element orientation */
        if (elementOrientation == 0) {
            wallStartX = wEntry[1];
            wallStartZ = wEntry[2];
        }
        else if (elementOrientation == BTO_ORIENT_270) {
            wallStartX = -wEntry[2];
            wallStartZ = wEntry[1];
        }
        else if (elementOrientation == BTO_ORIENT_180) {
            wallStartX = -wEntry[1];
            wallStartZ = -wEntry[2];
        }
        else if (elementOrientation == BTO_ORIENT_90) {
            wallStartX = wEntry[2];
            wallStartZ = -wEntry[1];
        }

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
/** @brief Subst hillroad track.
 * @param terr Parameter `terr`.
 * @param elem Parameter `elem`.
 * @return Function result.
 */
char  subst_hillroad_track(int terr, int elem)
{
    switch (terr) {
    case 7:
        switch (elem) {
        case 4: return (char)182;
        case 14: return (char)186;
        case 24: return (char)190;
        case 39:
        case 59:
        case 98: return (char)194;
        }
        break;
    case 8:
        switch (elem) {
        case 5: return (char)183;
        case 15: return (char)187;
        case 25: return (char)191;
        case 36:
        case 56:
        case 95: return (char)195;
        }
        break;
    case 9:
        switch (elem) {
        case 4: return (char)184;
        case 14: return (char)188;
        case 24: return (char)192;
        case 38:
        case 58:
        case 97: return (char)196;
        }
        break;
    case 10:
        switch (elem) {
        case 5: return (char)185;
        case 15: return (char)189;
        case 25: return (char)193;
        case 37:
        case 57:
        case 96: return (char)197;
        }
        break;
    }
    return 0;
}

/*
 * bto_auxiliary1 - Track dependency point lookup
 * Ported from seg004.asm lines 2756-3182
 *
/** @brief At.
 * @param col Parameter `col`.
 * @param row Parameter `row`.
 * @param fillers Parameter `fillers`.
 * @param type Parameter `type`.
 * @param out_points Parameter `out_points`.
 * @return Function result.
 */
/* Data tables in dseg - arrays of VECTOR (3 shorts = 6 bytes per entry) */

int  bto_auxiliary1(int tile_col, int tile_row, struct VECTOR* out_points)
{
    unsigned char tileElement;
    int tileCenterX;      /* x center */
    int tileCenterZ;      /* z center */
    int hillHeightOffset;      /* hill height offset */
    int elementOrientation;
    struct VECTOR* dependencyTable;
    int unusedTemp14;
    unsigned char multiTileFlags;
    unsigned char terrainByte;
    int physModel;
    int di;     /* point count */
    int si;     /* loop index */

    (void)unusedTemp14;

    /* Look up tile element at (col, row) */
    tileElement = *((unsigned char *)(track_elem_map + trackrows[tile_row] + tile_col));
    if (tileElement == 0)
        return 0;

    /* Get center positions */
    tileCenterX = trackcenterpos2[tile_col];
    tileCenterZ = trackcenterpos[tile_row];

/** @brief Elements.
 * @param BTO_MARKER_CORNER Parameter `BTO_MARKER_CORNER`.
 * @return Function result.
 */
    /* Handle multi-tile filler elements (253, 254, 255) */
    if (tileElement >= BTO_MARKER_CORNER) {
        switch (tileElement) {
        case BTO_MARKER_CORNER:
            /* Filler: look up tile at (col-1, row-1) using trackrows[row-1] */
            tileElement = *((unsigned char *)(track_elem_map + trackrows[tile_row - 1] + tile_col - 1));
            multiTileFlags = bto_trackobj_multi(tileElement);
            /* Check multiTileFlag bit 0 → update z center */
            if (multiTileFlags & 1) {
                tileCenterZ = trackpos[tile_row + 1];
            }
            /* Check multiTileFlag bit 1 → update x center */
            if (multiTileFlags & 2) {
                tileCenterX = trackpos2[tile_col];
                goto done_filler;
            }
            goto done_filler;

        case BTO_MARKER_VERTICAL:
            /* Filler: look up tile at (col, row-1) using trackrows[row-1] */
            tileElement = *((unsigned char *)(track_elem_map + trackrows[tile_row - 1] + tile_col));
            multiTileFlags = bto_trackobj_multi(tileElement);
            /* Check multiTileFlag bit 0 → update z center */
            if (multiTileFlags & 1) {
                tileCenterZ = trackpos[tile_row + 1];
            }
            goto check_bit1;

        case BTO_MARKER_HORIZONTAL:
            /* Filler: look up tile at (col-1, row) */
            tileElement = *((unsigned char *)(track_elem_map + trackrows[tile_row] + tile_col - 1));
            multiTileFlags = bto_trackobj_multi(tileElement);
            /* Check multiTileFlag bit 0 → update z center */
            if (multiTileFlags & 1) {
                tileCenterZ = trackpos[tile_row];
            }
/** @brief Center.
 * @param multiTileFlags Parameter `multiTileFlags`.
 * @return Function result.
 */
            /* Check multiTileFlag bit 1 → update x center (same path as 253) */
            if (multiTileFlags & 2) {
                tileCenterX = trackpos2[tile_col];
            }
            goto done_filler;
        }
        goto done_filler;
    }
    else {
        /* Normal element: check multi-tile flags */
        multiTileFlags = bto_trackobj_multi(tileElement);
        if (multiTileFlags == 0)
            goto done_filler;
        if (multiTileFlags & 1) {
            tileCenterZ = trackpos[tile_row];
        }
check_bit1:
        if (multiTileFlags & 2) {
            tileCenterX = trackpos2[tile_col + 1];
        }
    }

done_filler:
    /* Dispatch on physicalModel to get point count and table pointer */
    di = 0;
    dependencyTable = 0;

    physModel = bto_trackobj_phys(tileElement);

    switch (physModel) {
    case 11:
        di = 1;
        dependencyTable = (struct VECTOR*)phys_model_0B_points;
        break;
    case 18:
        di = 8;
        dependencyTable = (struct VECTOR*)phys_model_0x12_points;
        break;
    case 32:
        di = 2;
        dependencyTable = (struct VECTOR*)phys_model_0x20_points;
        break;
    case 33:
        di = 2;
        dependencyTable = (struct VECTOR*)phys_model_0x21_points;
        break;
    case 34:
        di = 4;
        dependencyTable = (struct VECTOR*)phys_model_0x22_points;
        break;
    case 35:
        di = 2;
        dependencyTable = (struct VECTOR*)phys_model_0x23_points;
        break;
    default:
        if (physModel >= 71 &&
            physModel <= 74) {
            di = 1;
            dependencyTable = (struct VECTOR*)phys_model_0B_points;
        }
        break;
    }

    if (di == 0)
        return 0;

    /* Get terrain type and hill height */
    terrainByte = *((unsigned char *)(track_terrain_map + terrainrows[tile_row] + tile_col));
    if (terrainByte == 6) {
        hillHeightOffset = hillHeightConsts[1];
    } else {
        hillHeightOffset = 0;
    }

    /* Get element orientation */
    elementOrientation = bto_trackobj_roty(tileElement);

    /* Output rotated points */
    for (si = 0; si < di; si++) {
        switch (elementOrientation) {
        case BTO_ORIENT_0:
            /* No rotation: (x, y, z) → (x + tileCenterX, y + hh, z + tileCenterZ) */
            out_points[si].x = dependencyTable[si].x + tileCenterX;
            out_points[si].y = dependencyTable[si].y + hillHeightOffset;
            out_points[si].z = dependencyTable[si].z + tileCenterZ;
            break;
        case BTO_ORIENT_90:
            /* 90° CW: (x,y,z) → (z + tileCenterX, y + hh, -x + tileCenterZ) */
            out_points[si].x = dependencyTable[si].z + tileCenterX;
            out_points[si].y = dependencyTable[si].y + hillHeightOffset;
            out_points[si].z = -dependencyTable[si].x + tileCenterZ;
            break;
        case BTO_ORIENT_180:
            /* 180°: (x,y,z) → (-x + tileCenterX, y + hh, -z + tileCenterZ) */
            out_points[si].x = -dependencyTable[si].x + tileCenterX;
            out_points[si].y = dependencyTable[si].y + hillHeightOffset;
            out_points[si].z = -dependencyTable[si].z + tileCenterZ;
            break;
        case BTO_ORIENT_270:
            /* 270° CW: (x,y,z) → (-z + tileCenterX, y + hh, x + tileCenterZ) */
            out_points[si].x = -dependencyTable[si].z + tileCenterX;
            out_points[si].y = dependencyTable[si].y + hillHeightOffset;
            out_points[si].z = dependencyTable[si].x + tileCenterZ;
            break;
        }
    }

    return di;
}

/*
 * load_opponent_data - Load AI opponent path data
 * Ported from seg004.asm lines 5842-6108
 *
 * Constructs "oppN" filename from opponent type, loads the resource,
 * extracts name/path/speed data, and performs branch-and-bound
 * shortest path search through the track for the opponent AI.
 */
/** @brief Load opponent data.
 */
void load_opponent_data(void)
{
    /* Stack arrays matching ASM layout */
    short pathNodes[905];         /* bp-2848: path node indices */
    short siArr[256];             /* bp-522: branch si (tile index) stack */
    short cntArr[256];            /* bp-1038: branch node count stack */
    long  costArr[256];           /* bp-3886: branch running cost stack */

    char * resourcePtr;
    char * speedDataPtr;
    short bestCostLow;
    short bestCostHigh;
    int stackDepth;     /* branch stack depth */
    int nodeCount;      /* current path node count */
    short currentNode;      /* current graph node */
    short nextNode;     /* next graph node */
    short isEndNode;        /* end-of-path flag */
    long runningCost;       /* running path cost (long) */
    short branchNode;     /* branch target node */

    int si;
    int di;

    /* Build "oppN" filename from opponent type */
    aOpp1[3] = gameconfig.game_opponenttype + '0';

    /* Load resource file */
    resourcePtr = (char *)file_load_resfile(aOpp1);

    /* Extract opponent name */
    copy_string(player_name_buffer, locate_text_res(resourcePtr, "nam"));

    /* Extract speed data pointer */
    speedDataPtr = locate_shape_alt(resourcePtr, "sped");

    /* Copy 16 bytes of speed data */
    for (si = 0; si < 16; si++) {
        opponent_speed_table[si] = ((unsigned char *)speedDataPtr)[si];
    }

    /* Initialize shortest path search */
    bestCostLow = 16959;
    bestCostHigh = 15;
    nodeCount = 0;
    runningCost = 0L;
    stackDepth = 0;
    si = 0;

    /* Branch-and-bound path search loop */
    for (;;) {
        isEndNode = 0;
        currentNode = track_waypoint_next[si];

        if (currentNode == 0) {
            /* End of track: finish node */
            nextNode = 1;
            isEndNode = 1;
        } else if (currentNode == -1) {
            /* Dead end */
            nextNode = 0;
            isEndNode = 1;
        } else {
            /* Check if this tile was already visited in current path */
            if (nodeCount > 0) {
                for (di = 0; di < nodeCount; di++) {
                    if (pathNodes[di] == si) {
                        nextNode = 0;
                        isEndNode = 1;
                        break;
                    }
                }
            }
        }

        /* Push current tile onto path */
        pathNodes[nodeCount] = si;
        nodeCount++;

        /* Add cost for this tile from speed table */
        {
            unsigned char elemIdx = ((unsigned char *)track_elem_ordered)[si];
            /* sped chunk has 16 entries; clamp elemIdx to stay in bounds.
             * Raw track element codes > 15 (jumps, loops, etc.) are mapped
             * to the last speed category, matching DOS flat-memory behaviour. */
            if (elemIdx >= 16) elemIdx = 15;
            unsigned char speed = opponent_speed_table[elemIdx];
            runningCost += (long)((unsigned short)speed + 1);
        }

        if (!isEndNode) {
            /* Not at end: check for branch point */
            branchNode = track_waypoint_alt[si];
            if (branchNode != -1) {
                /* Push branch state onto stack */
                siArr[stackDepth] = branchNode;
                cntArr[stackDepth] = nodeCount;
                costArr[stackDepth] = runningCost;
                stackDepth++;
            }
            /* Follow main track: next tile */
            si = currentNode;
            continue;
        }

        /* At end: check if this path is the best */
        if (nextNode != 0) {
            long bestCost = ((long)(unsigned short)bestCostHigh << 16) | (unsigned long)(unsigned short)bestCostLow;
            if (runningCost < bestCost) {
                /* Record termination marker */
                pathNodes[nodeCount] = 0;
                nodeCount++;

                /* Save best cost */
                bestCostLow = (short)(unsigned short)runningCost;
                bestCostHigh = (short)(unsigned short)((unsigned long)runningCost >> 16);

                /* Copy path to track_waypoint_order */
                for (di = 0; di < nodeCount; di++) {
                    ((short *)track_waypoint_order)[di] = pathNodes[di];
                }
                /* Write terminator pair */
                ((short *)track_waypoint_order)[nodeCount] = 0;
                ((short *)track_waypoint_order)[nodeCount + 1] = 1;
            }
        }

        /* Backtrack: pop from branch stack */
        if (stackDepth == 0) {
            /* No more branches: done */
            unload_resource(resourcePtr);
            return;
        }

        stackDepth--;
        si = siArr[stackDepth];
        nodeCount = cntArr[stackDepth];
        runningCost = costArr[stackDepth];
    }
}
