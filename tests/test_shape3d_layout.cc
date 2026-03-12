/*
 * test_shape3d_layout.cc — Google Test version of test_shape3d_layout.c.
 *
 * Tests struct packing, shape3d_init_shape_pure pointer arithmetic, and
 * shape3d_project_radius with overflow regression cases.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cstddef>

extern "C" {
#include "shape2d.h"
#include "shape3d.h"
} // extern "C"

/* -----------------------------------------------------------------------
 * 1. Struct packing
 * ---------------------------------------------------------------------- */
TEST(Shape3D, StructPacking) {
    EXPECT_EQ(sizeof(struct SHAPE3DHEADER), (size_t)4) << "sizeof(SHAPE3DHEADER) == 4";
    EXPECT_EQ(offsetof(struct SHAPE3DHEADER, header_numverts),      (size_t)0) << "header_numverts at 0";
    EXPECT_EQ(offsetof(struct SHAPE3DHEADER, header_numprimitives), (size_t)1) << "header_numprimitives at 1";
    EXPECT_EQ(offsetof(struct SHAPE3DHEADER, header_numpaints),     (size_t)2) << "header_numpaints at 2";
    EXPECT_EQ(offsetof(struct SHAPE3DHEADER, header_reserved),      (size_t)3) << "header_reserved at 3";

    EXPECT_EQ(sizeof(struct VECTOR), (size_t)6) << "sizeof(VECTOR) == 6 (3 × int16_t, packed)";
    EXPECT_EQ(offsetof(struct VECTOR, x), (size_t)0) << "VECTOR.x at 0";
    EXPECT_EQ(offsetof(struct VECTOR, y), (size_t)2) << "VECTOR.y at 2";
    EXPECT_EQ(offsetof(struct VECTOR, z), (size_t)4) << "VECTOR.z at 4";

    EXPECT_EQ(sizeof(struct RECTANGLE), (size_t)16) << "sizeof(RECTANGLE) == 16 (4 × int, packed)";
    EXPECT_EQ(offsetof(struct RECTANGLE, left),   (size_t)0)  << "RECTANGLE.left at 0";
    EXPECT_EQ(offsetof(struct RECTANGLE, right),  (size_t)4)  << "RECTANGLE.right at 4";
    EXPECT_EQ(offsetof(struct RECTANGLE, top),    (size_t)8)  << "RECTANGLE.top at 8";
    EXPECT_EQ(offsetof(struct RECTANGLE, bottom), (size_t)12) << "RECTANGLE.bottom at 12";

    EXPECT_EQ(sizeof(struct SHAPE2D), (size_t)16) << "sizeof(SHAPE2D) == 16 (packed)";
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_width),         (size_t)0)  << "s2d_width at 0";
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_height),        (size_t)2)  << "s2d_height at 2";
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_hotspot_x),     (size_t)4)  << "s2d_hotspot_x at 4";
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_hotspot_y),     (size_t)6)  << "s2d_hotspot_y at 6";
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_pos_x),         (size_t)8)  << "s2d_pos_x at 8";
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_pos_y),         (size_t)10) << "s2d_pos_y at 10";
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_plane_nibbles), (size_t)12) << "s2d_plane_nibbles at 12";
}

/* -----------------------------------------------------------------------
 * 2. shape3d_init_shape_pure — pointer arithmetic
 *
 * Buffer layout (offsets from buf start):
 *   [0..3]            SHAPE3DHEADER (H=4)
 *   [H..H+V*VS-1]     vertices      (V × VS=6)
 *   [H+V*VS..]        cull1         (P × CS=4)
 *   [H+V*VS+P*CS..]   cull2         (P × CS=4)
 *   [H+V*VS+P*PS..]   primitives    (P × PS=8)
 * ---------------------------------------------------------------------- */
TEST(Shape3D, InitShapeLayoutSmall) {
    const int H  = 4, VS = 6, CS = 4, PS = 8;
    int V = 3, P = 2;
    int buf_size = H + V*VS + P*PS + 64;
    char* buf = static_cast<char*>(calloc(1, static_cast<size_t>(buf_size)));
    ASSERT_NE(buf, nullptr);

    buf[0] = static_cast<char>(V);
    buf[1] = static_cast<char>(P);
    buf[2] = 0; buf[3] = 0;

    struct SHAPE3D gs;
    memset(&gs, 0, sizeof gs);
    shape3d_init_shape_pure(buf, &gs);

    EXPECT_EQ(gs.shape3d_numverts,      static_cast<unsigned>(V)) << "numverts";
    EXPECT_EQ(gs.shape3d_numprimitives, static_cast<unsigned>(P)) << "numprimitives";

    EXPECT_EQ((char*)gs.shape3d_verts - buf, H)             << "verts at H";
    EXPECT_EQ(gs.shape3d_cull1 - buf,        H + V*VS)      << "cull1 at H+V*VS";
    EXPECT_EQ(gs.shape3d_cull2 - buf,        H + V*VS+P*CS) << "cull2 at H+V*VS+P*CS";
    EXPECT_EQ(gs.shape3d_primitives - buf,   H + V*VS+P*PS) << "primitives at H+V*VS+P*PS";

    free(buf);
}

TEST(Shape3D, InitShapeLayoutZero) {
    char buf[8] = { 0 };
    struct SHAPE3D gs;
    memset(&gs, 0, sizeof gs);
    shape3d_init_shape_pure(buf, &gs);

    EXPECT_EQ(gs.shape3d_numverts,       0u) << "numverts=0";
    EXPECT_EQ(gs.shape3d_numprimitives,  0u) << "numprimitives=0";
    EXPECT_EQ((char*)gs.shape3d_verts - buf, 4)   << "verts immediately after header";
    EXPECT_EQ(gs.shape3d_cull1 - buf,        4)   << "cull1 at H";
    EXPECT_EQ(gs.shape3d_cull2 - buf,        4)   << "cull2 at H";
    EXPECT_EQ(gs.shape3d_primitives - buf,   4)   << "primitives at H";
}

TEST(Shape3D, InitShapeLayout16BitTrap43Verts) {
    /* V=43: V*VS = 258 — overflows uint8 (max 255), traps byte-wide multiply */
    const int H  = 4, VS = 6, CS = 4, PS = 8;
    int V = 43, P = 30;
    int buf_size = H + V*VS + P*PS + 64;
    char* buf = static_cast<char*>(calloc(1, static_cast<size_t>(buf_size)));
    ASSERT_NE(buf, nullptr);
    buf[0] = static_cast<char>(V);
    buf[1] = static_cast<char>(P);

    struct SHAPE3D gs;
    memset(&gs, 0, sizeof gs);
    shape3d_init_shape_pure(buf, &gs);

    EXPECT_EQ(gs.shape3d_cull1 - buf,      H + V*VS)      << "cull1 not truncated (16-bit trap)";
    EXPECT_EQ(gs.shape3d_primitives - buf, H + V*VS+P*PS) << "primitives offset correct";

    free(buf);
}

TEST(Shape3D, InitShapeLayoutLarge200Verts) {
    /* V=200 verts: V*VS=1200, wraps multiple times as bare uint8 multiply */
    const int H  = 4, VS = 6, CS = 4, PS = 8;
    int V = 200, P = 100;
    int buf_size = H + V*VS + P*PS + 64;
    char* buf = static_cast<char*>(calloc(1, static_cast<size_t>(buf_size)));
    ASSERT_NE(buf, nullptr);
    buf[0] = static_cast<char>(V);  /* 200 fits in uint8 */
    buf[1] = static_cast<char>(P);  /* 100 fits in uint8 */

    struct SHAPE3D gs;
    memset(&gs, 0, sizeof gs);
    shape3d_init_shape_pure(buf, &gs);

    EXPECT_EQ(gs.shape3d_cull1 - buf,      H + V*VS)       << "cull1 uses int-width offset";
    EXPECT_EQ(gs.shape3d_cull2 - buf,      H + V*VS+P*CS)  << "cull2 offset";
    EXPECT_EQ(gs.shape3d_primitives - buf, H + V*VS+P*PS)  << "primitives offset";

    free(buf);
}

/* -----------------------------------------------------------------------
 * 3. shape3d_project_radius
 * ---------------------------------------------------------------------- */
TEST(Shape3D, ProjectRadiusZeroOrNegativeDepth) {
    EXPECT_EQ(shape3d_project_radius(320, 100,      0), 0u) << "depth=0 → 0";
    EXPECT_EQ(shape3d_project_radius(320, 100,     -1), 0u) << "depth<0 → 0";
    EXPECT_EQ(shape3d_project_radius(320, 100, -99999), 0u) << "depth very negative → 0";
}

TEST(Shape3D, ProjectRadiusSimple) {
    /* 320*100/320 = 100 */
    EXPECT_EQ(shape3d_project_radius(320, 100, 320),  100u) << "scale=depth → result==radius";
    /* 320*100/64 = 32000/64 = 500 */
    EXPECT_EQ(shape3d_project_radius(320, 100,  64),  500u) << "d=64 → 500";
    /* 320*100/3200 = 10 */
    EXPECT_EQ(shape3d_project_radius(320, 100, 3200), 10u)  << "large depth → 10";
}

TEST(Shape3D, ProjectRadiusProportionality) {
    unsigned r1 = shape3d_project_radius(320, 100, 200);
    unsigned r2 = shape3d_project_radius(320, 100, 400);
    EXPECT_EQ(r1, 2 * r2) << "doubling depth halves result";
}

TEST(Shape3D, ProjectRadiusZeroInputs) {
    EXPECT_EQ(shape3d_project_radius(320,   0, 100), 0u) << "radius=0 → 0";
    EXPECT_EQ(shape3d_project_radius(  0, 100, 100), 0u) << "proj_scale=0 → 0";
}

TEST(Shape3D, ProjectRadius16BitOverflowTrap) {
    /* 40000 * 2000 = 80,000,000 — overflows uint16 but fits in uint32 */
    EXPECT_EQ(shape3d_project_radius(40000u, 2000u, 1000), 80000u)
        << "40000*2000 requires 32-bit intermediate";

    /* 32767^2 = 1,073,676,289 — would overflow uint16 */
    unsigned expected = static_cast<unsigned>(
        (static_cast<unsigned long>(32767u) * 32767u) / 1u);
    EXPECT_EQ(shape3d_project_radius(32767u, 32767u, 1), expected)
        << "32767^2 requires unsigned long intermediate";
}

/* -----------------------------------------------------------------------
 * 4. SHAPE3D region non-overlap
 * ---------------------------------------------------------------------- */
TEST(Shape3D, RegionNonOverlap) {
    const int H  = 4, VS = 6, CS = 4, PS = 8;
    int V = 10, P = 8;
    int buf_size = H + V*VS + P*PS + 64;
    char* buf = static_cast<char*>(calloc(1, static_cast<size_t>(buf_size)));
    ASSERT_NE(buf, nullptr);

    buf[0] = static_cast<char>(V);
    buf[1] = static_cast<char>(P);

    struct SHAPE3D gs;
    memset(&gs, 0, sizeof gs);
    shape3d_init_shape_pure(buf, &gs);

    EXPECT_LT((char*)gs.shape3d_verts,  gs.shape3d_cull1)      << "verts < cull1";
    EXPECT_LT(gs.shape3d_cull1,         gs.shape3d_cull2)       << "cull1 < cull2";
    EXPECT_LT(gs.shape3d_cull2,         gs.shape3d_primitives)  << "cull2 < primitives";

    char* verts_end = (char*)gs.shape3d_verts + V * VS;
    EXPECT_LE(verts_end, gs.shape3d_cull1) << "vert array fits before cull1";

    EXPECT_EQ(gs.shape3d_cull1 + P*CS, gs.shape3d_cull2)      << "cull1 + P*4 == cull2";
    EXPECT_EQ(gs.shape3d_cull2 + P*CS, gs.shape3d_primitives)  << "cull2 + P*4 == primitives";

    free(buf);
}

TEST(Shape3D, InitShapeLayoutByteMax) {
    const int H  = 4, VS = 6, CS = 4, PS = 8;
    int V = 255, P = 255;
    int buf_size = H + V*VS + P*PS + 64;
    char* buf = static_cast<char*>(calloc(1, static_cast<size_t>(buf_size)));
    ASSERT_NE(buf, nullptr);

    buf[0] = static_cast<char>(V);
    buf[1] = static_cast<char>(P);
    buf[2] = static_cast<char>(123);

    struct SHAPE3D gs;
    memset(&gs, 0, sizeof gs);
    shape3d_init_shape_pure(buf, &gs);

    EXPECT_EQ(gs.shape3d_numverts, 255u);
    EXPECT_EQ(gs.shape3d_numprimitives, 255u);
    EXPECT_EQ(gs.shape3d_numpaints, 123u);
    EXPECT_EQ(gs.shape3d_cull1 - buf,      H + V*VS);
    EXPECT_EQ(gs.shape3d_cull2 - buf,      H + V*VS + P*CS);
    EXPECT_EQ(gs.shape3d_primitives - buf, H + V*VS + P*PS);

    free(buf);
}

TEST(Shape3D, InitShapeOffsetMonotonicMatrix) {
    const int H = 4, VS = 6, CS = 4, PS = 8;
    char buf[4096];
    struct SHAPE3D gs;

    for (int V = 0; V <= 64; V += 8) {
        for (int P = 0; P <= 64; P += 8) {
            memset(buf, 0, sizeof buf);
            memset(&gs, 0, sizeof gs);
            buf[0] = static_cast<char>(V);
            buf[1] = static_cast<char>(P);

            shape3d_init_shape_pure(buf, &gs);

            EXPECT_EQ((char*)gs.shape3d_verts - buf, H);
            EXPECT_EQ(gs.shape3d_cull1 - buf,      H + V*VS);
            EXPECT_EQ(gs.shape3d_cull2 - buf,      H + V*VS + P*CS);
            EXPECT_EQ(gs.shape3d_primitives - buf, H + V*VS + P*PS);
            EXPECT_LE(gs.shape3d_cull1, gs.shape3d_cull2);
            EXPECT_LE(gs.shape3d_cull2, gs.shape3d_primitives);
        }
    }
}

TEST(Shape3D, ProjectRadiusMonotonicWithDepth) {
    const uint16_t proj = 900;
    const unsigned radius = 123;
    unsigned prev = shape3d_project_radius(proj, radius, 1);

    for (int depth = 2; depth <= 2000; depth++) {
        unsigned cur = shape3d_project_radius(proj, radius, depth);
        EXPECT_LE(cur, prev) << "depth=" << depth << " must not increase projected radius";
        prev = cur;
    }
}

TEST(Shape3D, ProjectRadiusLinearInRadius) {
    const uint16_t proj = 640;
    const int depth = 320;

    for (unsigned r = 0; r <= 500; r += 25) {
        unsigned r2 = r * 2;
        EXPECT_EQ(shape3d_project_radius(proj, r2, depth),
                  shape3d_project_radius(proj, r, depth) * 2)
            << "linear scaling expected at depth divisor";
    }
}

TEST(Shape3D, ProjectRadiusDropsToZeroAtLargeDepth) {
    EXPECT_EQ(shape3d_project_radius(1, 1, 2), 0u);
    EXPECT_EQ(shape3d_project_radius(10, 1, 11), 0u);
    EXPECT_EQ(shape3d_project_radius(100, 3, 301), 0u);
}
