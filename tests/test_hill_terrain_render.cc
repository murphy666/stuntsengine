/*
 * test_hill_terrain_render.cc — Tests for terrain tile culling on hills.
 *
 * Links against real engine code: shape3d.c, math.c, data_game.c
 * to exercise the actual shape3d_render_transformed() function with
 * realistic terrain tile geometry and camera positions.
 *
 * Reproduces the gameplay scenario: car is on a hill (elevated tile),
 * looking forward, flat terrain tiles left/right should be visible
 * in the lower part of the viewport but may be incorrectly culled.
 */

#include <gtest/gtest.h>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstdio>

extern "C" {
#include "shape3d.h"
#include "math.h"
}

/* ================================================================
 * Shape binary builder helpers
 * ================================================================ */

/*
 * Binary layout:
 *   Header:     4 bytes (numverts, numprims, numpaints, reserved)
 *   Vertices:   numverts × 6 bytes (VECTOR: x, y, z as int16 LE)
 *   Cull1:      numprims × 4 bytes (uint32 per primitive)
 *   Cull2:      numprims × 4 bytes (uint32 per primitive)
 *   Primitives: variable
 *
 * Primitive format:
 *   byte 0: type (4 = quad → 4 vertex indices, primitive type POLYGON)
 *   byte 1: flags (bit 0 = force accept for winding check)
 *   bytes 2..2+numpaints-1: paint job per material
 *   remaining: vertex indices
 */

static const int TILE_HALF_SIZE = 512;  /* 512 = half of 1024-unit tile */

/**
 * Build a flat terrain quad shape in the given buffer.
 * Returns pointer to the shape buffer.
 * The quad lies in the XZ plane at Y=0, centered at origin.
 */
static char* build_terrain_quad_shape(char* buf, size_t bufsize) {
    memset(buf, 0, bufsize);

    /* Header */
    buf[0] = 4;   /* numverts */
    buf[1] = 1;   /* numprimitives */
    buf[2] = 1;   /* numpaints */
    buf[3] = 0;   /* reserved */

    /* Vertices at offset 4: 4 × 6 = 24 bytes */
    struct VECTOR* verts = (struct VECTOR*)(buf + 4);
    /* Counter-clockwise when viewed from above (+Y) */
    verts[0] = { (short)-TILE_HALF_SIZE, 0, (short)-TILE_HALF_SIZE };  /* NW corner */
    verts[1] = { (short) TILE_HALF_SIZE, 0, (short)-TILE_HALF_SIZE };  /* NE corner */
    verts[2] = { (short) TILE_HALF_SIZE, 0, (short) TILE_HALF_SIZE };  /* SE corner */
    verts[3] = { (short)-TILE_HALF_SIZE, 0, (short) TILE_HALF_SIZE };  /* SW corner */

    /* Cull1 at offset 4 + 24 = 28: 1 × 4 = 4 bytes (accept all directions) */
    uint32_t* cull1 = (uint32_t*)(buf + 28);
    cull1[0] = 4294967295;

    /* Cull2 at offset 28 + 4 = 32: 1 × 4 = 4 bytes (accept all paint angles) */
    uint32_t* cull2 = (uint32_t*)(buf + 32);
    cull2[0] = 4294967295;

    /* Primitives at offset 32 + 4 = 36 */
    unsigned char* prim = (unsigned char*)(buf + 36);
    prim[0] = 4;     /* type: quad (4 vertex indices) */
    prim[1] = 1;     /* flags: bit 0 = force accept (skip winding check) */
    prim[2] = 11;  /* paint: light green */
    prim[3] = 0;     /* vertex index 0 */
    prim[4] = 1;     /* vertex index 1 */
    prim[5] = 2;     /* vertex index 2 */
    prim[6] = 3;     /* vertex index 3 */

    /* Terminator: next primitive type = 0 means end */
    prim[7] = 0;

    return buf;
}

/* ================================================================
 * Test fixture
 * ================================================================ */

class HillTerrainRender : public ::testing::Test {
protected:
    char shape_buf[256];
    struct SHAPE3D terrain_shape;
    struct RECTANGLE clip_rect;

    /* Gameplay-matching projection parameters */
    static constexpr int PROJECTION_FOV_H = 35;       /* horizontal FOV param */
    static constexpr int DASHBMP_Y = 130;                /* dashboard bitmap Y pos */
    static constexpr int PROJECTION_FOV_V_DIV = 6;      /* vertical FOV divisor */
    static constexpr int VIEWPORT_WIDTH = 320;
    static constexpr int ROOFBMP_HEIGHT = 10;            /* roof bitmap height */

    /* Hill gameplay constants */
    static constexpr int HILL_HEIGHT = 450;           /* 450 render units */
    static constexpr int CAMERA_Y_OFFSET = 270;       /* camera above car */
    static constexpr int TILE_SIZE = 1024;              /* 1024 render units */

    /* Computed camera height when on hill */
    static constexpr int CAMERA_Y_ON_HILL = HILL_HEIGHT + CAMERA_Y_OFFSET;  /* 720 */

    /* Terrain tile visibility threshold from frame.c */
    static constexpr int FRAME_VISIBILITY_FAR = 1024;
    static constexpr int REND_DIST_MULT = 8;
    static constexpr int TERRAIN_VIS_THRESHOLD = FRAME_VISIBILITY_FAR * REND_DIST_MULT;  /* 8192 */

    void SetUp() override {
        /* Build the terrain quad shape */
        build_terrain_quad_shape(shape_buf, sizeof(shape_buf));
        shape3d_init_shape_pure(shape_buf, &terrain_shape);

        /* Allocate polyinfo buffer */
        init_polyinfo();

        /* Set up projection matching in-game parameters */
        set_projection(PROJECTION_FOV_H, DASHBMP_Y / PROJECTION_FOV_V_DIV,
                       VIEWPORT_WIDTH, DASHBMP_Y);

        /* Set up clip rectangle (windshield viewport) */
        clip_rect.left = 0;
        clip_rect.right = VIEWPORT_WIDTH;
        clip_rect.top = ROOFBMP_HEIGHT;
        clip_rect.bottom = DASHBMP_Y;
    }

    /**
     * Set up the view matrix for looking forward from a given position.
     * Camera angles: roll=0, pitch=pitch, yaw=camera_yaw.
     */
    void setup_camera(int camera_yaw, int camera_pitch = 0) {
        select_cliprect_rotate(0, camera_pitch, camera_yaw, &clip_rect, 0);
    }

    /**
     * Compute the realistic camera pitch for the follow camera.
     * Matches frame.c: polarAngle(car_y - cam_y + 50, distance_xz)
     * At equilibrium: car_y - cam_y = -CAMERA_Y_OFFSET, distance_xz ≈ 450
     */
    int compute_follow_camera_pitch() {
        int y_delta = -CAMERA_Y_OFFSET + 50;  /* -220 */
        int distance_xz = 450;                 /* 450 (follow camera equilibrium) */
        return polarAngle(y_delta, distance_xz) & 1023;
    }

    /**
     * Create a TRANSFORMEDSHAPE3D for a terrain tile at given world position
     * relative to camera. This matches what frame.c does.
     */
    struct TRANSFORMEDSHAPE3D make_terrain_tile(short rel_x, short rel_y, short rel_z,
                                                short rot_y = 0) {
        struct TRANSFORMEDSHAPE3D ts;
        memset(&ts, 0, sizeof(ts));
        ts.pos.x = rel_x;
        ts.pos.y = rel_y;
        ts.pos.z = rel_z;
        ts.shapeptr = &terrain_shape;
        ts.rectptr = nullptr;
        ts.rotvec.x = 0;
        ts.rotvec.y = 0;
        ts.rotvec.z = rot_y;
        ts.shape_visibility_threshold = TERRAIN_VIS_THRESHOLD;
        ts.ts_flags = 5;  /* FORCE_UNSORTED | bit 2, matches frame.c base_ts_flags | 5 */
        ts.material = 0;
        return ts;
    }

    /**
     * Test if a tile renders (returns >= 0) or is culled (returns -1).
     * Resets polyinfo between calls to avoid buffer exhaustion.
     */
    int render_tile(struct TRANSFORMEDSHAPE3D* ts) {
        polyinfo_reset();
        return (int)shape3d_render_transformed(ts);
    }
};

/* ================================================================
 * Tests
 * ================================================================ */

/*
 * Sanity check: tile directly ahead on flat ground should render.
 */
TEST_F(HillTerrainRender, TileDirectlyAhead_FlatGround) {
    setup_camera(0);

    /* Camera at ground level, tile 2 tiles ahead */
    auto ts = make_terrain_tile(0, 0, 2 * TILE_SIZE);
    int result = render_tile(&ts);
    EXPECT_GE(result, 0)
        << "Tile at (0,0,2048) directly ahead should render, not be culled";
}

/*
 * Sanity check: tile far behind camera should be culled.
 */
TEST_F(HillTerrainRender, TileFarBehind_ShouldBeCulled) {
    setup_camera(0);

    /* Tile behind camera */
    auto ts = make_terrain_tile(0, 0, -3 * TILE_SIZE);
    int result = render_tile(&ts);
    EXPECT_EQ(result, -1)
        << "Tile at (0,0,-3072) far behind should be culled";
}

/*
 * Core test: camera on hill, flat tiles ahead should render.
 * The flat tile center is at Y=0, camera is at Y=CAMERA_Y_ON_HILL.
 * Relative Y: 0 - CAMERA_Y_ON_HILL = -720.
 */
TEST_F(HillTerrainRender, HillCamera_TileAhead_ShouldRender) {
    int pitch = compute_follow_camera_pitch();
    setup_camera(0, pitch);

    /* Camera on hill, looking forward at flat tile 2 tiles ahead */
    short rel_y = (short)(0 - CAMERA_Y_ON_HILL);  /* -720 */
    auto ts = make_terrain_tile(0, rel_y, 2 * TILE_SIZE);
    int result = render_tile(&ts);
    EXPECT_GE(result, 0)
        << "Flat tile at (0,-720,2048) ahead from hill should render";
}

/*
 * Core test: camera on hill, flat tile ahead and to the RIGHT should render.
 * This is the scenario the user reports as broken — terrain tiles
 * to the side of the car on hills are invisible.
 */
TEST_F(HillTerrainRender, HillCamera_TileAheadRight_ShouldRender) {
    int pitch = compute_follow_camera_pitch();
    setup_camera(0, pitch);

    short rel_y = (short)(0 - CAMERA_Y_ON_HILL);  /* -720 */

    /* Flat tile one tile right, 2 tiles ahead */
    auto ts = make_terrain_tile(TILE_SIZE, rel_y, 2 * TILE_SIZE);
    int result = render_tile(&ts);
    EXPECT_GE(result, 0)
        << "Flat tile at (1024,-720,2048) ahead-right from hill should render";
}

/*
 * Core test: camera on hill, flat tile ahead and to the LEFT should render.
 */
TEST_F(HillTerrainRender, HillCamera_TileAheadLeft_ShouldRender) {
    int pitch = compute_follow_camera_pitch();
    setup_camera(0, pitch);

    short rel_y = (short)(0 - CAMERA_Y_ON_HILL);  /* -720 */

    /* Flat tile one tile left, 2 tiles ahead */
    auto ts = make_terrain_tile(-TILE_SIZE, rel_y, 2 * TILE_SIZE);
    int result = render_tile(&ts);
    EXPECT_GE(result, 0)
        << "Flat tile at (-1024,-720,2048) ahead-left from hill should render";
}

/*
 * Tile directly beside camera (same Z row) on hill.
 * This tile has vertices both in front and behind the near plane.
 */
TEST_F(HillTerrainRender, HillCamera_TileBeside_SameRow) {
    int pitch = compute_follow_camera_pitch();
    setup_camera(0, pitch);

    short rel_y = (short)(0 - CAMERA_Y_ON_HILL);

    /* Tile directly to the right, same row (z center = 0) */
    auto ts = make_terrain_tile(TILE_SIZE, rel_y, 0);
    int result = render_tile(&ts);
    /* This might be legitimately off-screen, but let's see */
    fprintf(stderr, "  Tile (1024,-720,0) beside: result=%d\n", result);
}

/*
 * Systematic: scan tiles at various positions around the hill camera
 * and report which ones render vs get culled.
 */
TEST_F(HillTerrainRender, HillCamera_TileGrid_Survey) {
    int pitch = compute_follow_camera_pitch();
    setup_camera(0, pitch);

    short rel_y = (short)(0 - CAMERA_Y_ON_HILL);

    int rendered_count = 0;
    int culled_count = 0;

    fprintf(stderr, "\n=== Hill Terrain Tile Grid Survey (with pitch=0x%03X) ===\n",
            pitch);
    fprintf(stderr, "Camera at hill Y=%d, rel_y=%d\n", CAMERA_Y_ON_HILL, rel_y);
    fprintf(stderr, "Viewport: %d×%d..%d (rect %d,%d,%d,%d)\n",
            VIEWPORT_WIDTH, ROOFBMP_HEIGHT, DASHBMP_Y,
            clip_rect.left, clip_rect.right, clip_rect.top, clip_rect.bottom);
    fprintf(stderr, "\n  col\\row  ");
    for (int dz = -2; dz <= 6; dz++) {
        fprintf(stderr, " Z=%+2d", dz);
    }
    fprintf(stderr, "\n");

    for (int dx = -4; dx <= 4; dx++) {
        fprintf(stderr, "  X=%+2d     ", dx);
        for (int dz = -2; dz <= 6; dz++) {
            short rx = (short)(dx * TILE_SIZE);
            short rz = (short)(dz * TILE_SIZE);
            auto ts = make_terrain_tile(rx, rel_y, rz);
            int result = render_tile(&ts);
            if (result >= 0) {
                fprintf(stderr, "   OK");
                rendered_count++;
            } else {
                fprintf(stderr, "   --");
                culled_count++;
            }
        }
        fprintf(stderr, "\n");
    }

    fprintf(stderr, "\nRendered: %d, Culled: %d\n", rendered_count, culled_count);
    fprintf(stderr, "=== End Survey ===\n\n");

    /* With correct pitch, tiles at Z=1..4 directly ahead should render */
    for (int dz = 1; dz <= 4; dz++) {
        auto ts = make_terrain_tile(0, rel_y, (short)(dz * TILE_SIZE));
        int result = render_tile(&ts);
        EXPECT_GE(result, 0) << "Tile at X=0, Z=" << dz << " should render with pitch";
    }
}

/*
 * Test with different camera yaw angles.
 * Tiles to the side may become visible when the camera is rotated.
 */
TEST_F(HillTerrainRender, HillCamera_RotatedYaw_TilesShouldRender) {
    short rel_y = (short)(0 - CAMERA_Y_ON_HILL);
    int pitch = compute_follow_camera_pitch();

    /* yaw=256 (90°) rotates camera to look in world -X direction.
     * A tile at world (-2048, y, 0) is ahead in view space. */
    setup_camera(256, pitch);
    {
        auto ts = make_terrain_tile(-2 * TILE_SIZE, rel_y, 0);
        int result = render_tile(&ts);
        EXPECT_GE(result, 0)
            << "With yaw=256, tile at world (-2048,0) should be ahead";
    }

    /* yaw=768 (270°) rotates camera to look in world +X direction.
     * A tile at world (+2048, y, 0) is ahead. */
    setup_camera(768, pitch);
    {
        auto ts = make_terrain_tile(2 * TILE_SIZE, rel_y, 0);
        int result = render_tile(&ts);
        EXPECT_GE(result, 0)
            << "With yaw=768, tile at world (+2048,0) should be ahead";
    }
}

/*
 * Test flat ground (no hill) - tiles to the side should also render
 * when they're in the viewport.
 */
TEST_F(HillTerrainRender, FlatGround_TilesAhead_ShouldRender) {
    setup_camera(0);

    /* On flat ground, rel_y is small (camera slightly above ground) */
    short rel_y = (short)(0 - CAMERA_Y_OFFSET);  /* camera 270 above ground */

    auto ts_ahead = make_terrain_tile(0, rel_y, 2 * TILE_SIZE);
    int result = render_tile(&ts_ahead);
    EXPECT_GE(result, 0)
        << "Tile directly ahead on flat ground should render";

    auto ts_right = make_terrain_tile(TILE_SIZE, rel_y, 2 * TILE_SIZE);
    result = render_tile(&ts_right);
    EXPECT_GE(result, 0)
        << "Tile ahead-right on flat ground should render";
}

/*
 * Compare flat vs hill: identical XZ positions, different Y.
 * Both should render if in the same viewport direction.
 */
TEST_F(HillTerrainRender, CompareFlat_vs_Hill_RenderCoverage) {
    setup_camera(0);

    fprintf(stderr, "\n=== Flat vs Hill Coverage ===\n");

    struct { const char* label; short rel_y; } scenarios[] = {
        { "Flat (y=-270)", (short)(0 - CAMERA_Y_OFFSET) },
        { "Hill (y=-720)", (short)(0 - CAMERA_Y_ON_HILL) },
    };

    for (auto& sc : scenarios) {
        int count = 0;
        fprintf(stderr, "  %s: ", sc.label);
        for (int dx = -3; dx <= 3; dx++) {
            for (int dz = 1; dz <= 5; dz++) {
                auto ts = make_terrain_tile((short)(dx * TILE_SIZE), sc.rel_y,
                                             (short)(dz * TILE_SIZE));
                if (render_tile(&ts) >= 0) count++;
            }
        }
        fprintf(stderr, "%d tiles rendered\n", count);
    }
    fprintf(stderr, "=== End Comparison ===\n\n");
}
