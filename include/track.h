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
int track_setup(void);
char setup_track(void);
void init_rect_arrays(void);
void load_tracks_menu_shapes(void);
void draw_track_preview(void);
char subst_hillroad_track(int terr, int elem);

/* Waypoint / navigation */
short track_waypoint_lookup(short car_waypoint_seq_index, struct VECTOR *waypoint_out,
                            short waypoint_index, short *lookup_aux);

/* Replay */
void replay_apply_steering_correction(void);
void replay_capture_frame_input(int mode);

/* Car physics / state update */
void update_player_car_state(char);
void update_opponent_car_state(void);
int car_car_speed_adjust_collision(struct CARSTATE *, struct CARSTATE *);
short carState_update_wheel_suspension(struct CARSTATE *, short, short);
short detect_penalty(short *waypointIdxPtr, short *penaltyOutPtr);
short car_car_detect_collision(short *pCollPoints, short *pWorldCrds, short *oCollPoints,
                               short *oWorldCrds);
void upd_statef20_from_steer_input(unsigned char steeringInput);
void state_spawn_debris_particles(int debrisType, int scatterAngle, int baseSpeed);
void update_world_debris_particles(void);
void update_grip(struct CARSTATE *carstate, struct SIMD *simd, int isPlayerFlag);
void update_follow_camera_vectors(void);
void update_car_speed(char carInputFlags, int isOpponentCar, struct CARSTATE *carState,
                      struct SIMD *simdData);
void update_crash_state(int a1, int a2);

/* Aerodynamics */
void setup_aero_trackdata(void *carresptr, int is_opponent);

/* BTO track helpers */
int bto_auxiliary1(int tile_col, int tile_row, struct VECTOR *out_points);

/* Track object info (packed on-disk layout) */
#pragma pack(push, 1)
struct TRKOBJINFO_RAW {
    uint8_t si_noOfBlocks;
    uint8_t si_entryPoint;
    uint8_t si_exitPoint;
    uint8_t si_entryType;
    uint8_t si_exitType;
    uint8_t si_arrowType;
    int16_t si_arrowOrient;
    uint16_t si_cameraDataOffset;
    uint8_t si_opp1;
    uint8_t si_opp2;
    uint8_t si_opp3;
    uint8_t si_oppSpedCode;
};

struct TCOMP_ENTRY {
    unsigned char tc_col;        /* +0 */
    unsigned char tc_row;        /* +1 */
    unsigned char tc_tileElem;   /* +2 */
    unsigned char tc_subBlock;   /* +3 */
    unsigned char tc_connStatus; /* +4 */
    unsigned char tc_distCount;  /* +5 */
    unsigned char tc_prevCol;    /* +6 */
    unsigned char tc_prevRow;    /* +7 */
    unsigned char tc_prevElem;   /* +8 */
    unsigned char tc_prevSub;    /* +9 */
    unsigned char tc_prevConn;   /* +A */
    unsigned char tc_prevCode;   /* +B */
    short tc_prevIdx;            /* +C */
};
#pragma pack(pop)

/* Raw track lookup tables */
extern unsigned char track_camera_coords[];
extern unsigned char shapeinfos[];
extern unsigned char trkObjectList[];
extern unsigned char sceneshapes2[];
extern unsigned char sceneshapes3[];
extern int terraincenterpos[];
extern int trackpos2[];
extern int trackpos[];
extern int terrainrows[];
extern int terrainpos[];
extern int trackrows[];
extern int trackcenterpos2[];
extern int trackcenterpos[];
extern short hillHeightConsts[];
extern short *obstacle_world_pos;
extern int wallStartX;
extern short *track_waypoint_next;
extern int wallStartZ;
extern short *track_waypoint_alt;
extern bool track_object_render_enabled;
extern char *track_waypoint_order;
extern int wallindex;
extern int elRdWallRelated;
extern int startcol2;
extern int hillFlag;
extern int startrow2;
extern char *track_elem_ordered;
extern short *waypoint_world_pos;
extern int elem_xCenter;
extern int terrainHeight;
extern int elem_zCenter;
extern unsigned char *track_elem_map;
extern char current_surf_type;
extern char *track_element_height_ofs;
extern unsigned char *track_terrain_map;
extern char *track_cam_height_base;
extern char *path_conn_flags;
extern short track_angle;
extern char *path_col;
extern char *path_row;
extern int wallHeight;
extern short wallOrientation;
extern short *obstacle_rot_z;
extern short track_pieces_counter;
extern unsigned char *tile_obstacle_map;
extern unsigned char *obstacle_scene_index;
extern short *wallptr;

/*
 * Legacy 16-bit offsets embedded in track-object records refer into one of two
 * packed data blobs loaded at startup: shape metadata (shapeinfos) or per-object
 * camera tables (track_camera_coords).
 */
enum {
    TRACK_SHAPEINFOS_OFS_BASE = 6664,
    TRACK_SHAPEINFOS_SIZE = 1680,
    TRACK_CAMERA_DATA_OFS_BASE = 3220,
    TRACK_CAMERA_DATA_SIZE = 3444,
};

/**
 * @brief Resolve a legacy track-data offset to a host pointer.
 *
 * @return Pointer into shapeinfos or track_camera_coords, or NULL if unknown.
 */
static inline unsigned char *
track_resolve_resource_offset(unsigned short legacy_offset) {
    if ((unsigned int)legacy_offset >= (unsigned int)TRACK_SHAPEINFOS_OFS_BASE
        && (unsigned int)legacy_offset
               < (unsigned int)(TRACK_SHAPEINFOS_OFS_BASE + TRACK_SHAPEINFOS_SIZE)) {
        return shapeinfos + ((unsigned int)legacy_offset - (unsigned int)TRACK_SHAPEINFOS_OFS_BASE);
    }

    if ((unsigned int)legacy_offset >= (unsigned int)TRACK_CAMERA_DATA_OFS_BASE
        && (unsigned int)legacy_offset
               < (unsigned int)(TRACK_CAMERA_DATA_OFS_BASE + TRACK_CAMERA_DATA_SIZE)) {
        return track_camera_coords
               + ((unsigned int)legacy_offset - (unsigned int)TRACK_CAMERA_DATA_OFS_BASE);
    }

    return (unsigned char *)0;
}

/*
 * Track-object scene entries (trkObjectList, sceneshapes2, sceneshapes3) share a
 * fixed 14-byte binary layout.  Helpers below decode fields and resolve shape
 * pointers without exposing DOS segment arithmetic at every call site.
 */
enum {
    TRACKOBJECT_RAW_SIZE = 14,
    TRACKOBJECT_ROT_Y_OFFSET = 2,
    TRACKOBJECT_SHAPE_OFS_OFFSET = 4,
    TRACKOBJECT_LOSHAPE_OFS_OFFSET = 6,
    TRACKOBJECT_OVERLAY_OFFSET = 8,
    TRACKOBJECT_SURFACE_OFFSET = 9,
    TRACKOBJECT_IGNORE_ZBIAS_OFFSET = 10,
    TRACKOBJECT_MULTI_OFFSET = 11,
    TRACKOBJECT_PHYS_OFFSET = 12,
    TRACKOBJECT_LIST_COUNT = 215,
    TRACKOBJECT_SCENESHAPES2_COUNT = 19,
    TRACKOBJECT_SCENESHAPES3_COUNT = 13,
};

static inline const unsigned char *
trkobj_entry(const unsigned char *table, unsigned index) {
    return table + (size_t)(index * TRACKOBJECT_RAW_SIZE);
}

static inline unsigned short
trkobj_u16_field(const unsigned char *obj, unsigned offset) {
    return (unsigned short)obj[offset] | ((unsigned short)obj[offset + 1] << 8);
}

/** Map a combined scene index onto trkObjectList / sceneshapes2 / sceneshapes3. */
static inline const unsigned char *
trkobj_entry_by_scene_index(unsigned index) {
    if (index < TRACKOBJECT_LIST_COUNT) {
        return trkobj_entry(trkObjectList, index);
    }
    index -= TRACKOBJECT_LIST_COUNT;
    if (index < TRACKOBJECT_SCENESHAPES2_COUNT) {
        return trkobj_entry(sceneshapes2, index);
    }
    index -= TRACKOBJECT_SCENESHAPES2_COUNT;
    if (index < TRACKOBJECT_SCENESHAPES3_COUNT) {
        return trkobj_entry(sceneshapes3, index);
    }
    return (const unsigned char *)0;
}

static inline unsigned short
trkobj_shape_resource_offset(const unsigned char *obj) {
    return trkobj_u16_field(obj, TRACKOBJECT_SHAPE_OFS_OFFSET);
}

static inline unsigned short
trkobj_loshape_resource_offset(const unsigned char *obj) {
    return trkobj_u16_field(obj, TRACKOBJECT_LOSHAPE_OFS_OFFSET);
}

static inline struct SHAPE3D *
trkobj_shape(const unsigned char *obj) {
    return shape3d_from_resource_offset(trkobj_shape_resource_offset(obj));
}

static inline struct SHAPE3D *
trkobj_loshape(const unsigned char *obj) {
    return shape3d_from_resource_offset(trkobj_loshape_resource_offset(obj));
}

static inline short
trkobj_roty(const unsigned char *obj) {
    return (short)trkobj_u16_field(obj, TRACKOBJECT_ROT_Y_OFFSET);
}

static inline unsigned char
trkobj_overlay(const unsigned char *obj) {
    return obj[TRACKOBJECT_OVERLAY_OFFSET];
}

static inline signed char
trkobj_surface(const unsigned char *obj) {
    return (signed char)obj[TRACKOBJECT_SURFACE_OFFSET];
}

static inline unsigned char
trkobj_ignore_zbias(const unsigned char *obj) {
    return obj[TRACKOBJECT_IGNORE_ZBIAS_OFFSET];
}

static inline unsigned char
trkobj_multi(const unsigned char *obj) {
    return obj[TRACKOBJECT_MULTI_OFFSET];
}

static inline unsigned char
trkobj_physical(const unsigned char *obj) {
    return obj[TRACKOBJECT_PHYS_OFFSET];
}

#endif /* TRACK_H */
