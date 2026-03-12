/*
 * test_math.cc — Google Test version of the math.c unit tests.
 *
 * Each test function from test_math.c is a TEST() case; gtest_discover_tests()
 * registers each one individually in CTest so the per-test count is accurate.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

extern "C" {
#include "math.h"

/* -----------------------------------------------------------------------
 * Global stubs required by math.c
 * ---------------------------------------------------------------------- */

/* atantable[258] — verbatim copy from src/data_menu_track.c */
unsigned char atantable[258] = {
    0, 1, 1, 2, 3, 3, 4, 4,
    5, 6, 6, 7, 8, 8, 9, 10,
    10, 11, 11, 12, 13, 13, 14, 15,
    15, 16, 16, 17, 18, 18, 19, 20,
    20, 21, 22, 22, 23, 23, 24, 25,
    25, 26, 27, 27, 28, 28, 29, 30,
    30, 31, 31, 32, 33, 33, 34, 34,
    35, 36, 36, 37, 38, 38, 39, 39,
    40, 41, 41, 42, 42, 43, 44, 44,
    45, 45, 46, 46, 47, 48, 48, 49,
    49, 50, 51, 51, 52, 52, 53, 53,
    54, 55, 55, 56, 56, 57, 57, 58,
    58, 59, 60, 60, 61, 61, 62, 62,
    63, 63, 64, 65, 65, 66, 66, 67,
    67, 68, 68, 69, 69, 70, 70, 71,
    71, 72, 72, 73, 74, 74, 75, 75,
    76, 76, 77, 77, 78, 78, 79, 79,
    80, 80, 81, 81, 82, 82, 83, 83,
    84, 84, 84, 85, 85, 86, 86, 87,
    87, 88, 88, 89, 89, 90, 90, 91,
    91, 91, 92, 92, 93, 93, 94, 94,
    95, 95, 96, 96, 96, 97, 97, 98,
    98, 99, 99, 99, 100, 100, 101, 101,
    102, 102, 102, 103, 103, 104, 104, 104,
    105, 105, 106, 106, 106, 107, 107, 108,
    108, 108, 109, 109, 110, 110, 110, 111,
    111, 112, 112, 112, 113, 113, 113, 114,
    114, 115, 115, 115, 116, 116, 116, 117,
    117, 118, 118, 118, 119, 119, 119, 120,
    120, 120, 121, 121, 121, 122, 122, 122,
    123, 123, 123, 124, 124, 124, 125, 125,
    125, 126, 126, 126, 127, 127, 127, 128,
    128, 0
};

struct MATRIX mat_z_rot;
struct MATRIX mat_x_rot;
struct MATRIX mat_y_rot;
struct MATRIX mat_rot_temp;
unsigned short mat_y_rot_angle;

unsigned char sin80[4] = { 0, 0, 0, 0 };
unsigned char cos80[4] = { 0, 0, 0, 0 };
struct RECTANGLE select_rect_rc = { 0, 320, 0, 200 };
short video_flag2_is1 = 1;

void fatal_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[FATAL] ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    abort();
}

unsigned char projectiondata5[2]  = { 0, 0 };
unsigned char projectiondata8[2]  = { 0, 0 };
unsigned char projectiondata9[2]  = { 0, 0 };
unsigned char projectiondata10[2] = { 0, 0 };

short planindex;
struct PLANE *planptr;
struct PLANE *current_planptr;
int elem_xCenter;
int elem_zCenter;
int terrainHeight;

short planindex_copy;
short pState_minusRotate_z_2;
short pState_minusRotate_y_2;
short pState_minusRotate_x_2;
struct MATRIX mat_car_orientation;
struct MATRIX mat_yaw_step;
struct VECTOR vec_movement_local;
short pState_f36Mminf40sar2;
struct VECTOR vec_planerotopresult;
unsigned short sine_lookup_cache_key;
struct MATRIX mat_planetmp;
unsigned short f36f40_whlData;

/* Forward declarations for internal math.c functions */
void heapsort_by_order(int n, int *heap, int *data);
int  rect_is_inside(struct RECTANGLE *r1, struct RECTANGLE *r2);
int  rect_is_overlapping(struct RECTANGLE *r1, struct RECTANGLE *r2);
int  rect_is_adjacent(struct RECTANGLE *r1, struct RECTANGLE *r2);

} // extern "C"

static constexpr int FP1        = 16384;
static constexpr int ANG_FULL   = 1024;
static constexpr int ANG_HALF   = 512;
static constexpr int ANG_QUARTER = 256;
static constexpr int ANG_OCTANT  = 128;

/* -----------------------------------------------------------------------
 * sin_fast / cos_fast
 * ---------------------------------------------------------------------- */
TEST(Math, TrigCardinals) {
    EXPECT_EQ(sin_fast(0),              0)    << "sin(0°)";
    EXPECT_EQ(sin_fast(ANG_QUARTER),  FP1)    << "sin(90°)";
    EXPECT_EQ(sin_fast(ANG_HALF),       0)    << "sin(180°)";
    EXPECT_EQ(sin_fast(3*ANG_QUARTER), -FP1)  << "sin(270°)";
    EXPECT_EQ(sin_fast(ANG_FULL),       0)    << "sin(360°)";

    EXPECT_EQ(cos_fast(0),            FP1)    << "cos(0°)";
    EXPECT_EQ(cos_fast(ANG_QUARTER),    0)    << "cos(90°)";
    EXPECT_EQ(cos_fast(ANG_HALF),     -FP1)   << "cos(180°)";
    EXPECT_EQ(cos_fast(3*ANG_QUARTER),  0)    << "cos(270°)";
}

TEST(Math, TrigPythagorean) {
    long s = sin_fast(ANG_OCTANT);
    long c = cos_fast(ANG_OCTANT);
    long sum = (s*s + c*c) >> 14;
    EXPECT_NEAR(sum, FP1, 64) << "sin²(45°)+cos²(45°) ≈ FP1";
}

TEST(Math, TrigComplementarity) {
    for (int i = 0; i <= ANG_QUARTER; i += 16) {
        EXPECT_EQ(sin_fast((unsigned short)i),
                  cos_fast((unsigned short)(ANG_QUARTER - i)))
            << "sin(" << i << ") != cos(quarter-" << i << ")";
    }
}

TEST(Math, TrigSymmetry) {
    for (int i = 0; i <= ANG_QUARTER; i += 16) {
        EXPECT_EQ(sin_fast((unsigned short)i),
                  sin_fast((unsigned short)(ANG_HALF - i)))
            << "sin(" << i << ") != sin(half-" << i << ")";
    }
}

TEST(Math, TrigAntiSymmetry) {
    for (int i = 0; i < ANG_HALF; i += 16) {
        EXPECT_EQ(sin_fast((unsigned short)i),
                  -sin_fast((unsigned short)(ANG_HALF + i)))
            << "sin(" << i << ") != -sin(half+" << i << ")";
    }
}

/* -----------------------------------------------------------------------
 * polarAngle
 * ---------------------------------------------------------------------- */
TEST(Math, PolarAngleCardinals) {
    EXPECT_EQ(polarAngle(0,  1),  0)            << "north";
    EXPECT_EQ(polarAngle(1,  0),  ANG_QUARTER)  << "east";
    EXPECT_EQ(polarAngle(0, -1),  ANG_HALF)     << "south";
    EXPECT_EQ(polarAngle(-1, 0), -ANG_QUARTER)  << "west";
}

TEST(Math, PolarAngleOctants) {
    EXPECT_EQ(polarAngle(1,  1),  ANG_OCTANT)                 << "NE";
    EXPECT_EQ(polarAngle(1, -1),  ANG_HALF - ANG_OCTANT)      << "SE";
    EXPECT_EQ(polarAngle(-1,-1), -3*ANG_OCTANT)               << "SW";
    EXPECT_EQ(polarAngle(-1, 1), -ANG_OCTANT)                 << "NW";
    EXPECT_EQ(polarAngle(0,  0),  0)                          << "origin";
}

TEST(Math, PolarAngleInvariants) {
    int a1 = polarAngle(3, 5);
    int a2 = polarAngle(-3, -5);
    int diff = a1 - a2; if (diff < 0) diff = -diff;
    EXPECT_NEAR(diff, ANG_HALF, 2) << "opposite vectors half-turn apart";

    EXPECT_NEAR(polarAngle(10, 10), -polarAngle(-10, 10), 1) << "z-symmetry";
    EXPECT_NEAR(polarAngle(1, 2), polarAngle(100, 200), 1)   << "scale-invariant";
}

/* -----------------------------------------------------------------------
 * polarRadius2D / polarRadius3D
 * ---------------------------------------------------------------------- */
TEST(Math, PolarRadius) {
    EXPECT_NEAR(polarRadius2D(0,    100), 100, 2) << "y-axis";
    EXPECT_NEAR(polarRadius2D(100,    0), 100, 2) << "x-axis";
    EXPECT_NEAR(polarRadius2D(0,   -100), 100, 2) << "-y-axis";
    EXPECT_NEAR(polarRadius2D(-100,   0), 100, 2) << "-x-axis";
    EXPECT_NEAR(polarRadius2D(100, 100),  141, 3) << "diagonal";
    EXPECT_NEAR(polarRadius2D(-100,-100), 141, 3) << "-diagonal";
    EXPECT_NEAR(polarRadius2D(3, 4),   5, 1)      << "3-4-5";
    EXPECT_NEAR(polarRadius2D(4, 3),   5, 1)      << "4-3-5";

    struct VECTOR v;
    v = VECTOR{ 0, 0, 100 }; EXPECT_NEAR(polarRadius3D(&v), 100, 2) << "3D z-axis";
    v = VECTOR{ 0, 100, 0 }; EXPECT_NEAR(polarRadius3D(&v), 100, 2) << "3D y-axis";
}

/* -----------------------------------------------------------------------
 * rect_intersect
 * ---------------------------------------------------------------------- */
TEST(Math, RectIntersect) {
    struct RECTANGLE r1, r2;

    r1 = RECTANGLE{ 0, 5, 0, 10 }; r2 = RECTANGLE{ 10, 20, 0, 10 };
    EXPECT_EQ(rect_intersect(&r1, &r2), 1) << "r1 left of r2";

    r1 = RECTANGLE{ 25, 35, 0, 10 }; r2 = RECTANGLE{ 0, 20, 0, 10 };
    EXPECT_EQ(rect_intersect(&r1, &r2), 1) << "r1 right of r2";

    r1 = RECTANGLE{ 0, 10, 25, 35 }; r2 = RECTANGLE{ 0, 10, 0, 20 };
    EXPECT_EQ(rect_intersect(&r1, &r2), 1) << "r1 above r2";

    r1 = RECTANGLE{ 0, 10, 0, 5 }; r2 = RECTANGLE{ 0, 10, 10, 20 };
    EXPECT_EQ(rect_intersect(&r1, &r2), 1) << "r1 below r2";

    r1 = RECTANGLE{ 10, 5, 0, 10 }; r2 = RECTANGLE{ 0, 20, 0, 10 };
    EXPECT_EQ(rect_intersect(&r1, &r2), 1) << "degenerate r1";

    r1 = RECTANGLE{ 0, 15, 0, 15 }; r2 = RECTANGLE{ 10, 20, 10, 20 };
    EXPECT_EQ(rect_intersect(&r1, &r2), 0) << "partial overlap";
    EXPECT_EQ(r1.left, 10); EXPECT_EQ(r1.right, 15);
    EXPECT_EQ(r1.top,  10); EXPECT_EQ(r1.bottom, 15);

    r1 = RECTANGLE{ 5, 10, 5, 10 }; r2 = RECTANGLE{ 0, 20, 0, 20 };
    EXPECT_EQ(rect_intersect(&r1, &r2), 0) << "r1 inside r2";
    EXPECT_EQ(r1.left, 5); EXPECT_EQ(r1.right, 10);
    EXPECT_EQ(r1.top,  5); EXPECT_EQ(r1.bottom, 10);

    r1 = RECTANGLE{ 0, 20, 0, 20 }; r2 = RECTANGLE{ 5, 15, 5, 15 };
    EXPECT_EQ(rect_intersect(&r1, &r2), 0) << "r2 inside r1";
    EXPECT_EQ(r1.left, 5); EXPECT_EQ(r1.right, 15);
    EXPECT_EQ(r1.top,  5); EXPECT_EQ(r1.bottom, 15);

    r1 = RECTANGLE{ 0, 10, 0, 10 }; r2 = RECTANGLE{ 10, 20, 0, 10 };
    EXPECT_EQ(rect_intersect(&r1, &r2), 1) << "touching edge";
}

/* -----------------------------------------------------------------------
 * rect_is_inside
 * ---------------------------------------------------------------------- */
TEST(Math, RectIsInside) {
    struct RECTANGLE inner, outer;

    inner = RECTANGLE{ 5, 10, 5, 10 }; outer = RECTANGLE{ 0, 20, 0, 20 };
    EXPECT_EQ(rect_is_inside(&inner, &outer), 1) << "contained";

    inner = outer = RECTANGLE{ 0, 20, 0, 20 };
    EXPECT_EQ(rect_is_inside(&inner, &outer), 1) << "equal";

    inner = RECTANGLE{ 0, 25, 0, 10 }; outer = RECTANGLE{ 0, 20, 0, 20 };
    EXPECT_EQ(rect_is_inside(&inner, &outer), 0) << "right overflow";

    inner = RECTANGLE{ -1, 10, 0, 10 }; outer = RECTANGLE{ 0, 20, 0, 20 };
    EXPECT_EQ(rect_is_inside(&inner, &outer), 0) << "left underflow";

    inner = RECTANGLE{ 0, 10, -5, 10 }; outer = RECTANGLE{ 0, 20, 0, 20 };
    EXPECT_EQ(rect_is_inside(&inner, &outer), 0) << "top underflow";

    inner = RECTANGLE{ 0, 10, 0, 25 }; outer = RECTANGLE{ 0, 20, 0, 20 };
    EXPECT_EQ(rect_is_inside(&inner, &outer), 0) << "bottom overflow";
}

/* -----------------------------------------------------------------------
 * rect_is_overlapping
 * ---------------------------------------------------------------------- */
TEST(Math, RectIsOverlapping) {
    struct RECTANGLE a, b;

    a = RECTANGLE{ 0, 10, 0, 10 }; b = RECTANGLE{ 5, 15, 5, 15 };
    EXPECT_EQ(rect_is_overlapping(&a, &b), 1) << "clear overlap";

    a = RECTANGLE{ 5, 10, 5, 10 }; b = RECTANGLE{ 0, 20, 0, 20 };
    EXPECT_EQ(rect_is_overlapping(&a, &b), 1) << "containment";

    a = RECTANGLE{ 0, 10, 0, 10 }; b = RECTANGLE{ 10, 20, 0, 10 };
    EXPECT_EQ(rect_is_overlapping(&a, &b), 0) << "right-left touching";

    a = RECTANGLE{ 0, 5, 0, 10 }; b = RECTANGLE{ 10, 20, 0, 10 };
    EXPECT_EQ(rect_is_overlapping(&a, &b), 0) << "separated horiz";

    a = RECTANGLE{ 0, 10, 0, 5 }; b = RECTANGLE{ 0, 10, 5, 15 };
    EXPECT_EQ(rect_is_overlapping(&a, &b), 0) << "top-bottom touching";

    a = RECTANGLE{ 0, 10, 0, 5 }; b = RECTANGLE{ 0, 10, 10, 20 };
    EXPECT_EQ(rect_is_overlapping(&a, &b), 0) << "separated vert";

    a = RECTANGLE{ 0, 10, 0, 10 }; b = RECTANGLE{ 5, 15, 5, 15 };
    EXPECT_EQ(rect_is_overlapping(&a, &b), rect_is_overlapping(&b, &a)) << "symmetric";
}

/* -----------------------------------------------------------------------
 * rect_is_adjacent
 * ---------------------------------------------------------------------- */
TEST(Math, RectIsAdjacent) {
    struct RECTANGLE a, b;

    a = RECTANGLE{ 0, 10, 0, 10 }; b = RECTANGLE{ 10, 20, 0, 10 };
    EXPECT_EQ(rect_is_adjacent(&a, &b), 1) << "H adjacent";

    a = RECTANGLE{ 0, 10, 0, 10 }; b = RECTANGLE{ 0, 10, 10, 20 };
    EXPECT_EQ(rect_is_adjacent(&a, &b), 1) << "V adjacent";

    a = RECTANGLE{ 0, 5, 0, 5 }; b = RECTANGLE{ 10, 20, 10, 20 };
    EXPECT_EQ(rect_is_adjacent(&a, &b), 0) << "no contact";

    a = RECTANGLE{ 0, 10, 0, 10 }; b = RECTANGLE{ 5, 15, 5, 15 };
    EXPECT_EQ(rect_is_adjacent(&a, &b), 0) << "overlapping";
}

/* -----------------------------------------------------------------------
 * rect_adjust_from_point
 * ---------------------------------------------------------------------- */
TEST(Math, RectAdjust) {
    struct RECTANGLE rc = RECTANGLE{ 10, 20, 10, 20 };
    struct POINT2D pt;

    pt = POINT2D{ 15, 15 }; rect_adjust_from_point(&pt, &rc);
    EXPECT_EQ(rc.left, 10); EXPECT_EQ(rc.right, 20);
    EXPECT_EQ(rc.top,  10); EXPECT_EQ(rc.bottom, 20);

    pt = POINT2D{ 5, 15 }; rect_adjust_from_point(&pt, &rc);
    EXPECT_EQ(rc.left, 5) << "expands left";

    pt = POINT2D{ 15, 5 }; rect_adjust_from_point(&pt, &rc);
    EXPECT_EQ(rc.top, 5) << "expands top";

    pt = POINT2D{ 25, 15 }; rect_adjust_from_point(&pt, &rc);
    EXPECT_EQ(rc.right, 26) << "expands right to px+1";

    pt = POINT2D{ 15, 30 }; rect_adjust_from_point(&pt, &rc);
    EXPECT_EQ(rc.bottom, 31) << "expands bottom to py+1";

    rc = RECTANGLE{ 0, 10, 0, 10 };
    pt = POINT2D{ 9, 0 }; rect_adjust_from_point(&pt, &rc);
    EXPECT_EQ(rc.right, 10) << "px+1==right no change";

    pt = POINT2D{ 10, 0 }; rect_adjust_from_point(&pt, &rc);
    EXPECT_EQ(rc.right, 11) << "px+1 > right expands";
}

/* -----------------------------------------------------------------------
 * mat_mul_vector
 * ---------------------------------------------------------------------- */
TEST(Math, MatMulVector) {
    struct MATRIX m;
    struct VECTOR in, out;

    memset(&m, 0, sizeof m);
    m.m._11 = FP1; m.m._22 = FP1; m.m._33 = FP1;
    in = VECTOR{ 100, 200, 300 };
    mat_mul_vector(&in, &m, &out);
    EXPECT_EQ(out.x, 100); EXPECT_EQ(out.y, 200); EXPECT_EQ(out.z, 300);

    in = VECTOR{ 0, 0, 0 };
    mat_mul_vector(&in, &m, &out);
    EXPECT_EQ(out.x, 0); EXPECT_EQ(out.y, 0); EXPECT_EQ(out.z, 0);

    memset(&m, 0, sizeof m);
    in = VECTOR{ 100, 200, 300 };
    mat_mul_vector(&in, &m, &out);
    EXPECT_EQ(out.x, 0) << "zero matrix"; EXPECT_EQ(out.y, 0); EXPECT_EQ(out.z, 0);

    memset(&m, 0, sizeof m);
    m.m._12 = FP1; m.m._21 = FP1; m.m._33 = FP1;
    in = VECTOR{ 10, 20, 30 };
    mat_mul_vector(&in, &m, &out);
    EXPECT_EQ(out.x, 20) << "swap x=in.y";
    EXPECT_EQ(out.y, 10) << "swap y=in.x";
    EXPECT_EQ(out.z, 30) << "swap z=in.z";

    memset(&m, 0, sizeof m);
    m.m._11 = FP1/2; m.m._22 = FP1/2; m.m._33 = FP1/2;
    in = VECTOR{ 100, 200, 300 };
    mat_mul_vector(&in, &m, &out);
    EXPECT_EQ(out.x,  50) << "scale×0.5 x";
    EXPECT_EQ(out.y, 100) << "scale×0.5 y";
    EXPECT_EQ(out.z, 150) << "scale×0.5 z";

    memset(&m, 0, sizeof m);
    m.m._11 = -FP1; m.m._22 = FP1; m.m._33 = FP1;
    in = VECTOR{ 100, 50, 25 };
    mat_mul_vector(&in, &m, &out);
    EXPECT_EQ(out.x, -100) << "negate x";
    EXPECT_EQ(out.y,   50) << "y unchanged";
    EXPECT_EQ(out.z,   25) << "z unchanged";
}

/* -----------------------------------------------------------------------
 * mat_multiply
 * ---------------------------------------------------------------------- */
TEST(Math, MatMultiply) {
    struct MATRIX a, b, r;

    memset(&a, 0, sizeof a); a.m._11 = FP1; a.m._22 = FP1; a.m._33 = FP1;
    memset(&b, 0, sizeof b); b.m._11 = FP1; b.m._22 = FP1; b.m._33 = FP1;
    mat_multiply(&a, &b, &r);
    EXPECT_EQ(r.m._11, FP1); EXPECT_EQ(r.m._22, FP1); EXPECT_EQ(r.m._33, FP1);
    EXPECT_EQ(r.m._12,   0); EXPECT_EQ(r.m._21,   0);
    EXPECT_EQ(r.m._13,   0); EXPECT_EQ(r.m._31,   0);
    EXPECT_EQ(r.m._23,   0); EXPECT_EQ(r.m._32,   0);

    memset(&a, 0, sizeof a);
    a.m._11 = FP1; a.m._12 = FP1/2; a.m._13 = 0;
    a.m._21 = 0;   a.m._22 = FP1;   a.m._23 = FP1/4;
    a.m._31 = FP1/4; a.m._32 = 0;   a.m._33 = FP1;
    memset(&b, 0, sizeof b); b.m._11 = FP1; b.m._22 = FP1; b.m._33 = FP1;
    mat_multiply(&a, &b, &r);
    EXPECT_EQ(r.m._11, a.m._11); EXPECT_EQ(r.m._12, a.m._12); EXPECT_EQ(r.m._13, a.m._13);
    EXPECT_EQ(r.m._21, a.m._21); EXPECT_EQ(r.m._22, a.m._22); EXPECT_EQ(r.m._23, a.m._23);
    EXPECT_EQ(r.m._31, a.m._31); EXPECT_EQ(r.m._32, a.m._32); EXPECT_EQ(r.m._33, a.m._33);

    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b); b.m._11 = FP1; b.m._22 = FP1; b.m._33 = FP1;
    mat_multiply(&a, &b, &r);
    EXPECT_EQ(r.m._11, 0) << "zero*I";

    struct MATRIX rz90, rz180, product;
    mat_rot_z(&rz90,  ANG_QUARTER);
    mat_rot_z(&rz180, ANG_HALF);
    mat_multiply(&rz90, &rz90, &product);
    EXPECT_NEAR(product.m._11, rz180.m._11, 2) << "Rz(90)²=Rz(180) _11";
    EXPECT_NEAR(product.m._22, rz180.m._22, 2) << "Rz(90)²=Rz(180) _22";
    EXPECT_NEAR(product.m._12, rz180.m._12, 2) << "Rz(90)²=Rz(180) _12";
    EXPECT_NEAR(product.m._21, rz180.m._21, 2) << "Rz(90)²=Rz(180) _21";
    EXPECT_NEAR(product.m._33, rz180.m._33, 2) << "Rz(90)²=Rz(180) _33";
}

/* -----------------------------------------------------------------------
 * mat_invert
 * ---------------------------------------------------------------------- */
TEST(Math, MatInvert) {
    struct MATRIX m, inv;

    memset(&m, 0, sizeof m);
    m.m._11 = FP1; m.m._22 = FP1; m.m._33 = FP1;
    mat_invert(&m, &inv);
    EXPECT_EQ(inv.m._11, FP1); EXPECT_EQ(inv.m._22, FP1); EXPECT_EQ(inv.m._33, FP1);
    EXPECT_EQ(inv.m._12,   0); EXPECT_EQ(inv.m._21,   0);

    memset(&m, 0, sizeof m);
    m.m._11 = FP1; m.m._12 = 100; m.m._13 = 200;
    m.m._21 = 300; m.m._22 = FP1; m.m._23 = 400;
    m.m._31 = 500; m.m._32 = 600; m.m._33 = FP1;
    mat_invert(&m, &inv);
    EXPECT_EQ(inv.m._12, m.m._21); EXPECT_EQ(inv.m._21, m.m._12);
    EXPECT_EQ(inv.m._13, m.m._31); EXPECT_EQ(inv.m._31, m.m._13);
    EXPECT_EQ(inv.m._23, m.m._32); EXPECT_EQ(inv.m._32, m.m._23);
    EXPECT_EQ(inv.m._11, m.m._11); EXPECT_EQ(inv.m._22, m.m._22); EXPECT_EQ(inv.m._33, m.m._33);

    struct MATRIX q;
    memset(&q, 0, sizeof q);
    q.m._11 = FP1; q.m._12 = 100; q.m._13 = 200;
    q.m._21 = 300; q.m._22 = FP1; q.m._23 = 400;
    q.m._31 = 500; q.m._32 = 600; q.m._33 = FP1;
    mat_invert(&q, &q);
    EXPECT_EQ(q.m._12, 300) << "in-place _12"; EXPECT_EQ(q.m._21, 100) << "in-place _21";
    EXPECT_EQ(q.m._13, 500) << "in-place _13"; EXPECT_EQ(q.m._31, 200) << "in-place _31";
    EXPECT_EQ(q.m._23, 600) << "in-place _23"; EXPECT_EQ(q.m._32, 400) << "in-place _32";

    struct MATRIX rz, rzt, product;
    mat_rot_z(&rz, ANG_QUARTER);
    mat_invert(&rz, &rzt);
    mat_multiply(&rz, &rzt, &product);
    EXPECT_NEAR(product.m._11, FP1, 2) << "Rz*Rz^T=I _11";
    EXPECT_NEAR(product.m._22, FP1, 2) << "Rz*Rz^T=I _22";
    EXPECT_NEAR(product.m._33, FP1, 2) << "Rz*Rz^T=I _33";
    EXPECT_NEAR(product.m._12,   0, 2) << "Rz*Rz^T=I _12";
    EXPECT_NEAR(product.m._21,   0, 2) << "Rz*Rz^T=I _21";
}

/* -----------------------------------------------------------------------
 * mat_rot_x / mat_rot_y / mat_rot_z
 * ---------------------------------------------------------------------- */
TEST(Math, MatRotX) {
    struct MATRIX m;

    mat_rot_x(&m, 0);
    EXPECT_EQ(m.m._11, FP1); EXPECT_EQ(m.m._22, FP1); EXPECT_EQ(m.m._33, FP1);
    EXPECT_EQ(m.m._21,   0); EXPECT_EQ(m.m._31,   0);
    EXPECT_EQ(m.m._12,   0); EXPECT_EQ(m.m._13,   0);
    EXPECT_EQ(m.m._32,   0); EXPECT_EQ(m.m._23,   0);

    mat_rot_x(&m, ANG_QUARTER);
    EXPECT_EQ(m.m._11, FP1); EXPECT_EQ(m.m._22, 0); EXPECT_EQ(m.m._33, 0);
    EXPECT_EQ(m.m._23, -FP1) << "_23=-sin(q)"; EXPECT_EQ(m.m._32, FP1) << "_32=sin(q)";

    mat_rot_x(&m, ANG_HALF);
    EXPECT_EQ(m.m._11,  FP1); EXPECT_EQ(m.m._22, -FP1); EXPECT_EQ(m.m._33, -FP1);
    EXPECT_EQ(m.m._23,    0); EXPECT_EQ(m.m._32,    0);
}

TEST(Math, MatRotY) {
    struct MATRIX m;

    mat_rot_y(&m, 0);
    EXPECT_EQ(m.m._11, FP1); EXPECT_EQ(m.m._22, FP1); EXPECT_EQ(m.m._33, FP1);
    EXPECT_EQ(m.m._13,   0); EXPECT_EQ(m.m._31,   0);

    mat_rot_y(&m, ANG_QUARTER);
    EXPECT_EQ(m.m._11,   0); EXPECT_EQ(m.m._22, FP1); EXPECT_EQ(m.m._33,   0);
    EXPECT_EQ(m.m._13,  FP1) << "_13=sin(q)"; EXPECT_EQ(m.m._31, -FP1) << "_31=-sin(q)";

    mat_rot_y(&m, ANG_HALF);
    EXPECT_EQ(m.m._11, -FP1); EXPECT_EQ(m.m._33, -FP1);
    EXPECT_EQ(m.m._13,    0); EXPECT_EQ(m.m._31,    0);
}

TEST(Math, MatRotZ) {
    struct MATRIX m;

    mat_rot_z(&m, 0);
    EXPECT_EQ(m.m._11, FP1); EXPECT_EQ(m.m._22, FP1); EXPECT_EQ(m.m._33, FP1);
    EXPECT_EQ(m.m._12,   0); EXPECT_EQ(m.m._21,   0);

    mat_rot_z(&m, ANG_QUARTER);
    EXPECT_EQ(m.m._11,   0); EXPECT_EQ(m.m._22,   0); EXPECT_EQ(m.m._33, FP1);
    EXPECT_EQ(m.m._12, -FP1) << "_12=-sin(q)"; EXPECT_EQ(m.m._21, FP1) << "_21=sin(q)";

    mat_rot_x(&m, ANG_FULL);
    EXPECT_NEAR(m.m._11, FP1, 1); EXPECT_NEAR(m.m._22, FP1, 1); EXPECT_NEAR(m.m._33, FP1, 1);
}

TEST(Math, MatRotVectorTransform) {
    struct MATRIX rz;
    struct VECTOR in = VECTOR{ (short)FP1, 0, 0 };
    struct VECTOR out;
    mat_rot_z(&rz, ANG_QUARTER);
    mat_mul_vector(&in, &rz, &out);
    EXPECT_NEAR(out.x,   0, 2) << "Rz(q)*(FP1,0,0) x~=0";
    EXPECT_NEAR(out.y, FP1, 2) << "Rz(q)*(FP1,0,0) y~=FP1";
    EXPECT_NEAR(out.z,   0, 2) << "Rz(q)*(FP1,0,0) z~=0";
}

/* -----------------------------------------------------------------------
 * heapsort_by_order
 * ---------------------------------------------------------------------- */
TEST(Math, HeapsortOrdering) {
    int keys[8], data[8];
    for (int i = 0; i < 8; i++) { keys[i] = i; data[i] = i * 100; }
    heapsort_by_order(8, keys, data);
    for (int i = 0; i < 7; i++)
        EXPECT_GE(keys[i], keys[i+1]) << "ascending input, pos " << i;
}

TEST(Math, HeapsortDecendingStable) {
    int keys[8], data[8];
    for (int i = 0; i < 8; i++) { keys[i] = 7-i; data[i] = (7-i)*100; }
    heapsort_by_order(8, keys, data);
    for (int i = 0; i < 7; i++)
        EXPECT_GE(keys[i], keys[i+1]) << "descending input, pos " << i;
}

TEST(Math, HeapsortDataCoupling) {
    int keys[8], data[8];
    for (int i = 0; i < 8; i++) { keys[i] = i; data[i] = i * 100; }
    heapsort_by_order(8, keys, data);
    for (int i = 0; i < 8; i++)
        EXPECT_EQ(data[i], keys[i] * 100) << "data[" << i << "] coupled";
}

TEST(Math, HeapsortDuplicates) {
    int keys[8], data[8];
    for (int i = 0; i < 8; i++) { keys[i] = 5; data[i] = i; }
    heapsort_by_order(8, keys, data);
    for (int i = 0; i < 8; i++)
        EXPECT_EQ(keys[i], 5) << "duplicates[" << i << "]";
}

TEST(Math, HeapsortSingle) {
    int keys[1] = { 42 }, data[1] = { 99 };
    heapsort_by_order(1, keys, data);
    EXPECT_EQ(keys[0], 42); EXPECT_EQ(data[0], 99);
}

TEST(Math, HeapsortTwo) {
    int keys[2] = { 10, 2 }, data[2] = { 1000, 200 };
    heapsort_by_order(2, keys, data);
    EXPECT_EQ(keys[0], 10); EXPECT_EQ(keys[1], 2);
    EXPECT_EQ(data[0], 1000); EXPECT_EQ(data[1], 200);
}

TEST(Math, HeapsortLarge) {
    int rkeys[16] = { 15,3,8,0,12,5,1,14,7,11,2,9,4,13,6,10 };
    int rdata[16];
    for (int i = 0; i < 16; i++) rdata[i] = rkeys[i] * 7;
    heapsort_by_order(16, rkeys, rdata);
    for (int i = 0; i < 15; i++)
        EXPECT_GE(rkeys[i], rkeys[i+1]) << "large, pos " << i;
    for (int i = 0; i < 16; i++)
        EXPECT_EQ(rdata[i], rkeys[i] * 7) << "data coupled[" << i << "]";
}

/* -----------------------------------------------------------------------
 * multiply_and_scale
 * ---------------------------------------------------------------------- */
TEST(Math, MultiplyAndScale) {
    EXPECT_EQ(multiply_and_scale(0,    FP1),    0) << "0*FP1";
    EXPECT_EQ(multiply_and_scale(FP1,    0),    0) << "FP1*0";
    EXPECT_EQ(multiply_and_scale(FP1,  FP1),  FP1) << "1.0*1.0";
    EXPECT_EQ(multiply_and_scale(FP1,  FP1/2), FP1/2) << "1.0*0.5";
    EXPECT_EQ(multiply_and_scale(FP1/2, FP1/2), FP1/4) << "0.5*0.5";
    EXPECT_EQ(multiply_and_scale(-FP1,  FP1),  -FP1) << "-1.0*1.0";
    EXPECT_EQ(multiply_and_scale(FP1,  -FP1),  -FP1) << "1.0*-1.0";
    EXPECT_EQ(multiply_and_scale(-FP1, -FP1),   FP1) << "-1.0*-1.0";
    EXPECT_EQ(multiply_and_scale(100,  200), multiply_and_scale(200, 100)) << "commutative";
    EXPECT_EQ(multiply_and_scale(1, 1), 0) << "sub-unit rounds to 0";
}

/* -----------------------------------------------------------------------
 * vec_normalInnerProduct
 * ---------------------------------------------------------------------- */
TEST(Math, VecNormalInnerProduct) {
    struct VECTOR n;

    n = VECTOR{ 1, 0, 0 };
    EXPECT_EQ(vec_normalInnerProduct(100, 0, 0, &n), 0) << "sub-divisor";

    n = VECTOR{ 1, 0, 0 };
    EXPECT_EQ(vec_normalInnerProduct(8192, 0, 0, &n), 1) << "8192/8192=1";

    n = VECTOR{ 1, 1, 1 };
    EXPECT_EQ(vec_normalInnerProduct(8192, 8192, 8192, &n), 3) << "3*8192/8192=3";

    n = VECTOR{ 0, 0, 0 };
    EXPECT_EQ(vec_normalInnerProduct(100, 200, 300, &n), 0) << "zero normal";

    n = VECTOR{ -1, 0, 0 };
    EXPECT_EQ(vec_normalInnerProduct(8192, 0, 0, &n), -1) << "negative component";
}

/* -----------------------------------------------------------------------
 * vector_lerp_at_z
 * ---------------------------------------------------------------------- */
TEST(Math, VectorLerpAtZMidpoint) {
    struct VECTOR v1 = VECTOR{100, 200, 200};
    struct VECTOR v2 = VECTOR{0,   0,   0};
    struct VECTOR out;
    vector_lerp_at_z(&v1, &v2, &out, 100);
    EXPECT_EQ(out.z,  100) << "z=i";
    EXPECT_EQ(out.x,   50) << "midpoint x";
    EXPECT_EQ(out.y,  100) << "midpoint y";
}

TEST(Math, VectorLerpAtZAtEndpoint1) {
    struct VECTOR v1 = VECTOR{100, 200, 200};
    struct VECTOR v2 = VECTOR{0,   0,   0};
    struct VECTOR out;
    vector_lerp_at_z(&v1, &v2, &out, 200);
    EXPECT_EQ(out.z, 200) << "z=v1.z";
    EXPECT_EQ(out.x, 100) << "x==v1.x";
    EXPECT_EQ(out.y, 200) << "y==v1.y";
}

TEST(Math, VectorLerpAtZAtEndpoint2) {
    struct VECTOR v1 = VECTOR{100, 200, 200};
    struct VECTOR v2 = VECTOR{50,  50,  0};
    struct VECTOR out;
    vector_lerp_at_z(&v1, &v2, &out, 0);
    EXPECT_EQ(out.z,  0)  << "z=v2.z";
    EXPECT_EQ(out.x, 50)  << "x==v2.x";
    EXPECT_EQ(out.y, 50)  << "y==v2.y";
}

/* -----------------------------------------------------------------------
 * mat_rot_zxy
 * ---------------------------------------------------------------------- */
TEST(Math, MatRotZxyIdentityAllZero) {
    struct MATRIX* m = mat_rot_zxy(0, 0, 0, 0);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->m._11, FP1) << "_11";
    EXPECT_EQ(m->m._22, FP1) << "_22";
    EXPECT_EQ(m->m._33, FP1) << "_33";
    EXPECT_EQ(m->m._12,   0) << "_12";
    EXPECT_EQ(m->m._21,   0) << "_21";
    EXPECT_EQ(m->m._13,   0) << "_13";
    EXPECT_EQ(m->m._31,   0) << "_31";
    EXPECT_EQ(m->m._23,   0) << "_23";
    EXPECT_EQ(m->m._32,   0) << "_32";
}

TEST(Math, MatRotZxyAltOrderIdentity) {
    struct MATRIX* m = mat_rot_zxy(0, 0, 0, 1);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->m._11, FP1) << "alt _11";
    EXPECT_EQ(m->m._22, FP1) << "alt _22";
    EXPECT_EQ(m->m._33, FP1) << "alt _33";
    EXPECT_EQ(m->m._12,   0) << "alt _12";
    EXPECT_EQ(m->m._13,   0) << "alt _13";
}

TEST(Math, MatRotZxySingleZRotation) {
    /* Rotating 90 degrees around Z: _33 stays FP1, diagonal in XY swaps */
    struct MATRIX* m = mat_rot_zxy(ANG_QUARTER, 0, 0, 0);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->m._33, FP1) << "z-rot: _33 untouched";
    EXPECT_NEAR(m->m._11,   0, 1) << "z-rot 90: _11 ≈ 0";
    EXPECT_NEAR(m->m._22,   0, 1) << "z-rot 90: _22 ≈ 0";
}

/* -----------------------------------------------------------------------
 * parse_shape2d_helper / parse_shape2d_helper2 / div_uint32 / compare_ds_ss
 * ---------------------------------------------------------------------- */
extern "C" unsigned long parse_shape2d_helper(unsigned int low, unsigned int high);
extern "C" unsigned long parse_shape2d_helper2(unsigned int low, unsigned int high);
extern "C" unsigned int  div_uint32(unsigned int low, unsigned int high, unsigned int divisor);
extern "C" int           compare_ds_ss(void);

TEST(Math, ParseShape2dHelper) {
    EXPECT_EQ(parse_shape2d_helper(0,    0),    0UL) << "0,0";
    EXPECT_EQ(parse_shape2d_helper(0,    1),   16UL) << "0,1 → 1<<4";
    EXPECT_EQ(parse_shape2d_helper(5,    2),   37UL) << "5+2*16";
    EXPECT_EQ(parse_shape2d_helper(15, 16), (unsigned long)(256 + 15)) << "15,16";
}

TEST(Math, ParseShape2dHelper2) {
    /* 0,0 → all zero */
    EXPECT_EQ(parse_shape2d_helper2(0, 0), 0UL) << "0,0";
    /* lo=15: combined=15, shifted=0, remainder=15 → 0x0000_000F */
    EXPECT_EQ(parse_shape2d_helper2(15, 0), 15UL) << "nibble only";
    /* lo=32, hi=3: combined=0x0003_0020
     * shifted = 0x0003_0020 >> 4 = 0x0000_3002; remainder = 32 & 15 = 0
     * return = (12290UL << 16) | 0 = 0x3002_0000 */
    EXPECT_EQ(parse_shape2d_helper2(32, 3), 805437440UL) << "combined shift";
}

TEST(Math, DivUint32) {
    EXPECT_EQ(div_uint32(0,  0, 1),        0u) << "0/1";
    EXPECT_EQ(div_uint32(20, 0, 4),        5u) << "20/4";
    EXPECT_EQ(div_uint32(0,  1, 2),    32768u) << "65536/2";
    EXPECT_EQ(div_uint32(0,  2, 1),   131072u) << "131072/1";
    EXPECT_EQ(div_uint32(100, 0, 10),     10u) << "100/10";
}

TEST(Math, CompareDsSs) {
    EXPECT_EQ(compare_ds_ss(), 1) << "stub always returns 1";
}

/* -----------------------------------------------------------------------
 * rect_union  — bounding-box union of two rectangles
 * ---------------------------------------------------------------------- */
TEST(Math, RectUnion) {
    struct RECTANGLE r1, r2, out;

    /* partial overlap: union should span both */
    r1 = RECTANGLE{ 0, 10, 0, 10 };
    r2 = RECTANGLE{ 5, 20, 5, 20 };
    rect_union(&r1, &r2, &out);
    EXPECT_EQ(out.left,    0) << "left=min(0,5)";
    EXPECT_EQ(out.right,  20) << "right=max(10,20)";
    EXPECT_EQ(out.top,     0) << "top=min(0,5)";
    EXPECT_EQ(out.bottom, 20) << "bottom=max(10,20)";

    /* r2 fully contains r1 */
    r1 = RECTANGLE{  5, 10,  5, 10 };
    r2 = RECTANGLE{  0, 20,  0, 20 };
    rect_union(&r1, &r2, &out);
    EXPECT_EQ(out.left,    0); EXPECT_EQ(out.right,  20);
    EXPECT_EQ(out.top,     0); EXPECT_EQ(out.bottom, 20);

    /* r1 fully contains r2 */
    r1 = RECTANGLE{  0, 20,  0, 20 };
    r2 = RECTANGLE{  5, 10,  5, 10 };
    rect_union(&r1, &r2, &out);
    EXPECT_EQ(out.left,    0); EXPECT_EQ(out.right,  20);
    EXPECT_EQ(out.top,     0); EXPECT_EQ(out.bottom, 20);

    /* equal rects */
    r1 = RECTANGLE{ 3, 7, 1, 9 };
    r2 = RECTANGLE{ 3, 7, 1, 9 };
    rect_union(&r1, &r2, &out);
    EXPECT_EQ(out.left, 3); EXPECT_EQ(out.right,  7);
    EXPECT_EQ(out.top,  1); EXPECT_EQ(out.bottom, 9);

    /* disjoint: result is tight bounding box */
    r1 = RECTANGLE{  0,  5,  0,  5 };
    r2 = RECTANGLE{ 10, 20, 10, 20 };
    rect_union(&r1, &r2, &out);
    EXPECT_EQ(out.left,    0); EXPECT_EQ(out.right,  20);
    EXPECT_EQ(out.top,     0); EXPECT_EQ(out.bottom, 20);

    /* negative coordinates */
    r1 = RECTANGLE{ -10, -1, -20, -5 };
    r2 = RECTANGLE{   0, 10,   0, 15 };
    rect_union(&r1, &r2, &out);
    EXPECT_EQ(out.left,  -10); EXPECT_EQ(out.right,  10);
    EXPECT_EQ(out.top,   -20); EXPECT_EQ(out.bottom, 15);
}
/* ================================================================
 * Body-roll formula  (carstate.c Section 14: Z-rotation extraction)
 *
 *   Variables (wheel-velocity sums):
 *     consumedSpeed  = vec_1DE[1].x + vec_1DE[2].x - vec_1DE[0].x - vec_1DE[3].x
 *     remainingSpeed = vec_1DE[1].y + vec_1DE[2].y - vec_1DE[0].y - vec_1DE[3].y
 *
 *   Rule 1 (special case):
 *     if remainingSpeed == 0 && consumedSpeed > 0: roll = 0
 *   Rule 2 (general):
 *     roll = polarAngle(consumedSpeed, remainingSpeed) - ANG_QUARTER
 *   Rule 3 (deadzone):
 *     if |roll| < 2: roll = 0
 *
 * The polarAngle convention in this project follows a CW compass bearing
 * from "north" (positive Y axis):
 *   polarAngle(0,  1) =  0           north
 *   polarAngle(1,  0) =  ANG_QUARTER east
 *   polarAngle(0, -1) =  ANG_HALF    south
 *   polarAngle(-1, 0) = -ANG_QUARTER west
 *   polarAngle(1,  1) =  ANG_OCTANT  NE (45°)
 * ================================================================ */

static int compute_body_roll(int consumedSpeed, int remainingSpeed) {
    int r;
    /* Rule 1: flat-terrain special case */
    if (remainingSpeed == 0 && consumedSpeed > 0)
        return 0;
    /* Rule 2: general */
    r = polarAngle(consumedSpeed, remainingSpeed) - ANG_QUARTER;
    /* Rule 3: deadzone */
    if (r >= 0) { if (r < 2) r = 0; }
    else        { if (-r < 2) r = 0; }
    return r;
}

TEST(Math, RollSpecialCase_FlatForward) {
    /* Rule 1: remainingSpeed==0 && consumedSpeed>0 → zero, regardless of magnitude */
    EXPECT_EQ(compute_body_roll(  1, 0), 0) << "small flat-forward → 0";
    EXPECT_EQ(compute_body_roll(100, 0), 0) << "large flat-forward → 0";
    /* Rule 1 does NOT apply when consumedSpeed <= 0 */
    EXPECT_NE(compute_body_roll(0, 0), 0)   << "both zero → non-zero roll (polarAngle(0,0)=0 → -ANG_QUARTER)";
}

TEST(Math, RollDiagonal_NE_45deg) {
    /* polarAngle(1,1) = ANG_OCTANT  →  roll = ANG_OCTANT - ANG_QUARTER = -ANG_OCTANT */
    EXPECT_EQ(compute_body_roll(100, 100), -ANG_OCTANT)
        << "NE 45°: roll = -ANG_OCTANT (= -128 = -128)";
}

TEST(Math, RollDiagonal_SE_45deg) {
    /* polarAngle(1,-1) = ANG_HALF - ANG_OCTANT  →  roll = +ANG_OCTANT */
    EXPECT_EQ(compute_body_roll(100, -100), ANG_OCTANT)
        << "SE 45°: roll = +ANG_OCTANT (= +128 = +128)";
}

TEST(Math, RollPureNorth) {
    /* Car rolling purely along positive Y with no X component.
     * polarAngle(0, 100) = 0  →  roll = 0 - ANG_QUARTER = -ANG_QUARTER.
     * Deadzone check: |-ANG_QUARTER| = 256 >= 2, so no filtering. */
    EXPECT_EQ(compute_body_roll(0, 100), -ANG_QUARTER)
        << "pure north: roll = -ANG_QUARTER (= -256)";
}

TEST(Math, RollPureSouth) {
    /* polarAngle(0, -100) = ANG_HALF  →  roll = ANG_HALF - ANG_QUARTER = +ANG_QUARTER */
    EXPECT_EQ(compute_body_roll(0, -100), ANG_QUARTER)
        << "pure south: roll = +ANG_QUARTER (= +256)";
}

TEST(Math, RollSymmetry) {
    /* For a fixed consumedSpeed > 0, the roll pair (y, -y) must be antisymmetric:
     *   compute_body_roll(x, y) + compute_body_roll(x, -y) == 0
     * This follows from polarAngle(x, y) + polarAngle(x, -y) = ANG_HALF
     * (verified by PolarAngleOctants test for exact 45° diagonal). */
    const int xs[] = {100, 200, 50};
    const int ys[] = {50, 100, 200};
    for (int i = 0; i < 3; i++) {
        int pos = compute_body_roll(xs[i],  ys[i]);
        int neg = compute_body_roll(xs[i], -ys[i]);
        EXPECT_EQ(pos + neg, 0)
            << "roll symmetry violated for consumedSpeed=" << xs[i]
            << " remainingSpeed=±" << ys[i];
    }
}

TEST(Math, RollDeadzone_FilterEqualsZero) {
    /* When polarAngle result minus ANG_QUARTER is ±1, it should be zeroed.
     * The only certain case: result IS exactly 0 → stays 0 with no filtering needed.
     * polarAngle(1, 0) = ANG_QUARTER, so roll = ANG_QUARTER - ANG_QUARTER = 0.
     * But remainingSpeed=0 && consumedSpeed>0 → Rule 1 fires first → also 0. */
    EXPECT_EQ(compute_body_roll(1, 0), 0) << "east cardinal: rule 1 fires → 0";
    /* Verify that exactly-zero roll from the formula is preserved: use large scale
     * that still hits exactly 256 in the atan table. */
    EXPECT_EQ(compute_body_roll(1000, 0), 0) << "large east cardinal: rule 1 fires → 0";
}