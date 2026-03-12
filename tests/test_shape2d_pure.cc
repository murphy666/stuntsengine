/*
 * test_shape2d_pure.cc — Google Test version of test_shape2d_pure.c
 *
 * Each test function from test_shape2d_pure.c becomes a TEST() case;
 * gtest_discover_tests() registers them individually in CTest.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cstddef>

extern "C" {
#include "shape2d.h"
} // extern "C"

/* -----------------------------------------------------------------------
 * Structure layout guarantees
 * ---------------------------------------------------------------------- */
TEST(Shape2D, StructLayout) {
    EXPECT_EQ(sizeof(struct SHAPE2D), (size_t)16)
        << "sizeof(SHAPE2D) == 16 (DOS on-disk layout)";

    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_width),          (size_t)0)  << "s2d_width at offset 0";
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_height),         (size_t)2)  << "s2d_height at offset 2";
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_hotspot_x),      (size_t)4)  << "s2d_hotspot_x at offset 4";
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_hotspot_y),      (size_t)6)  << "s2d_hotspot_y at offset 6";
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_pos_x),          (size_t)8)  << "s2d_pos_x at offset 8";
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_pos_y),          (size_t)10) << "s2d_pos_y at offset 10";
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_plane_nibbles),  (size_t)12) << "s2d_plane_nibbles at offset 12";

    EXPECT_EQ(offsetof(struct SPRITE, sprite_bitmapptr), (size_t)0)
        << "sprite_bitmapptr at 0";
    EXPECT_EQ(offsetof(struct SPRITE, sprite_reserved1),
              sizeof(struct SHAPE2D*))
        << "sprite_reserved1 immediately after bitmapptr (no ptr-alignment padding)";
    EXPECT_EQ(offsetof(struct SPRITE, sprite_left),
              sizeof(struct SHAPE2D*) + 3*sizeof(unsigned short) + sizeof(unsigned int*))
        << "sprite_left immediately after sprite_lineofs (no padding)";
    EXPECT_EQ(offsetof(struct SPRITE, sprite_pitch),
              offsetof(struct SPRITE, sprite_height) + sizeof(unsigned short))
        << "sprite_pitch immediately after sprite_height";
}

/* -----------------------------------------------------------------------
 * sprite_phase_add_wrap
 * ---------------------------------------------------------------------- */
TEST(Shape2D, PhaseAddWrapZeroPeriod) {
    EXPECT_EQ(sprite_phase_add_wrap(10, 5, 0), (unsigned)10) << "period=0 → phase unchanged";
    EXPECT_EQ(sprite_phase_add_wrap(0,  0, 0), (unsigned)0)  << "period=0, all zero → 0";
}

TEST(Shape2D, PhaseAddWrapZeroDelta) {
    EXPECT_EQ(sprite_phase_add_wrap(7, 0, 10), (unsigned)7) << "delta=0 → phase unchanged";
}

TEST(Shape2D, PhaseAddWrapNormal) {
    EXPECT_EQ(sprite_phase_add_wrap(5, 3, 10), (unsigned)8)  << "5+3=8 (no wrap)";
    EXPECT_EQ(sprite_phase_add_wrap(0, 1, 10), (unsigned)1)  << "0+1=1";
    EXPECT_EQ(sprite_phase_add_wrap(8, 2, 10), (unsigned)10) << "8+2=10 == period (not wrapped)";
}

TEST(Shape2D, PhaseAddWrapSingleWrap) {
    EXPECT_EQ(sprite_phase_add_wrap(8,  5, 10), (unsigned)3) << "8+5=13 → 13-10=3";
    EXPECT_EQ(sprite_phase_add_wrap(1, 10, 10), (unsigned)1) << "1+10=11 → 11-10=1";
}

TEST(Shape2D, PhaseAddWrapMultiWrap) {
    EXPECT_EQ(sprite_phase_add_wrap(1, 25, 10), (unsigned)6) << "1+25=26 → 26-10-10=6";
    EXPECT_EQ(sprite_phase_add_wrap(0, 31, 10), (unsigned)1) << "0+31=31 → 31-3*10=1";
}

TEST(Shape2D, PhaseAddWrapPeriodOne) {
    EXPECT_EQ(sprite_phase_add_wrap(1, 1, 1), (unsigned)1) << "period=1: 1+1=2 → 2-1=1";
    EXPECT_EQ(sprite_phase_add_wrap(0, 1, 1), (unsigned)1) << "period=1: 0+1=1 → stays 1";
}

TEST(Shape2D, PhaseAddWrap16BitOverflowTrap) {
    EXPECT_EQ(sprite_phase_add_wrap(65534, 3, 65535), (unsigned)2)
        << "16-bit trap: 65534+3 with period=FFFF → 2 (uint promotion required)";
    EXPECT_EQ(sprite_phase_add_wrap(65520, 32, 65535), (unsigned)17)
        << "16-bit trap: near-max phase + delta";
}

/* -----------------------------------------------------------------------
 * sprite_pick_blink_frame
 * ---------------------------------------------------------------------- */
TEST(Shape2D, PickBlinkFrameBelowThreshold) {
    EXPECT_EQ(sprite_pick_blink_frame(3,  5, 100, 200), 200) << "phase<threshold → lo";
    EXPECT_EQ(sprite_pick_blink_frame(0,  0, 100, 200), 200) << "phase==threshold=0 → lo";
    EXPECT_EQ(sprite_pick_blink_frame(5,  5, 100, 200), 200) << "phase==threshold → lo (strict >)";
    EXPECT_EQ(sprite_pick_blink_frame(99,99, 100, 200), 200) << "phase==threshold=99 → lo";
}

TEST(Shape2D, PickBlinkFrameAboveThreshold) {
    EXPECT_EQ(sprite_pick_blink_frame(6, 5, 100, 200), 100) << "phase>threshold → hi";
    EXPECT_EQ(sprite_pick_blink_frame(1, 0, 100, 200), 100) << "phase=1 > threshold=0 → hi";
}

TEST(Shape2D, PickBlinkFrameExtremes) {
    EXPECT_EQ(sprite_pick_blink_frame(65535, 65534, 7, 8), 7)
        << "max phase > max-1 threshold → hi";
    EXPECT_EQ(sprite_pick_blink_frame(65534, 65534, 7, 8), 8)
        << "max-1 phase == max-1 threshold → lo";
    EXPECT_EQ(sprite_pick_blink_frame(10, 5, 42, 42), 42) << "hi==lo: always same";
    EXPECT_EQ(sprite_pick_blink_frame(0, 0, 1, 2), 2) << "phase=0, threshold=0 → lo";
    EXPECT_EQ(sprite_pick_blink_frame(1, 0, 1, 2), 1) << "phase=1, threshold=0 → hi";
}

/* -----------------------------------------------------------------------
 * sprite_hittest_point
 * ---------------------------------------------------------------------- */
TEST(Shape2D, HittestPoint) {
    const unsigned short x1[2] = { 10, 30 };
    const unsigned short x2[2] = { 20, 40 };
    const unsigned short y1[2] = { 10, 30 };
    const unsigned short y2[2] = { 20, 40 };

    EXPECT_EQ(sprite_hittest_point(15, 15, 2, x1, x2, y1, y2),  0) << "interior of rect 0";
    EXPECT_EQ(sprite_hittest_point(35, 35, 2, x1, x2, y1, y2),  1) << "interior of rect 1";
    EXPECT_EQ(sprite_hittest_point(25, 25, 2, x1, x2, y1, y2), -1) << "miss entirely";
    EXPECT_EQ(sprite_hittest_point(10, 10, 2, x1, x2, y1, y2),  0) << "top-left corner → hit";
    EXPECT_EQ(sprite_hittest_point(20, 20, 2, x1, x2, y1, y2),  0) << "bottom-right corner → hit";
    EXPECT_EQ(sprite_hittest_point(9,  15, 2, x1, x2, y1, y2), -1) << "one pixel left → miss";
    EXPECT_EQ(sprite_hittest_point(21, 15, 2, x1, x2, y1, y2), -1) << "one pixel right → miss";
    EXPECT_EQ(sprite_hittest_point(15, 15, 0, x1, x2, y1, y2), -1) << "count=0 → -1";
    EXPECT_EQ(sprite_hittest_point(15, 15,-1, x1, x2, y1, y2), -1) << "count<0 → -1";
    EXPECT_EQ(sprite_hittest_point(15, 15, 2, nullptr, x2, y1, y2), -1) << "NULL x1 → -1";
    EXPECT_EQ(sprite_hittest_point(15, 15, 2, x1, nullptr, y1, y2), -1) << "NULL x2 → -1";

    {
        const unsigned short ax1[2] = { 0,   0 };
        const unsigned short ax2[2] = { 100, 100 };
        const unsigned short ay1[2] = { 0,   0 };
        const unsigned short ay2[2] = { 100, 100 };
        EXPECT_EQ(sprite_hittest_point(50, 50, 2, ax1, ax2, ay1, ay2), 0)
            << "first-match wins when rects overlap";
    }
}

/* -----------------------------------------------------------------------
 * sprite_copy_assign
 * ---------------------------------------------------------------------- */
TEST(Shape2D, SpriteCopyAssign) {
    struct SPRITE src, dst;
    memset(&src, 171, sizeof src);
    memset(&dst, 0, sizeof dst);

    src.sprite_left     = 10;
    src.sprite_right    = 100;
    src.sprite_top      = 5;
    src.sprite_height   = 200;
    src.sprite_pitch    = 320;
    src.sprite_width2   = 90;
    src.sprite_left2    = 10;
    src.sprite_widthsum = 100;

    sprite_copy_assign(&dst, &src);

    EXPECT_EQ(dst.sprite_left,     10)  << "copy: sprite_left";
    EXPECT_EQ(dst.sprite_right,   100)  << "copy: sprite_right";
    EXPECT_EQ(dst.sprite_top,       5)  << "copy: sprite_top";
    EXPECT_EQ(dst.sprite_height,  200)  << "copy: sprite_height";
    EXPECT_EQ(dst.sprite_pitch,   320)  << "copy: sprite_pitch";
    EXPECT_EQ(dst.sprite_width2,   90)  << "copy: sprite_width2";
    EXPECT_EQ(dst.sprite_left2,    10)  << "copy: sprite_left2";
    EXPECT_EQ(dst.sprite_widthsum, 100) << "copy: sprite_widthsum";
    EXPECT_EQ(memcmp(&src, &dst, sizeof(struct SPRITE)), 0) << "copy: byte-identical";

    /* NULL guards — must not crash */
    sprite_copy_assign(nullptr, &src);
    sprite_copy_assign(&dst, nullptr);

    /* Self-copy */
    src.sprite_left = 77;
    sprite_copy_assign(&src, &src);
    EXPECT_EQ(src.sprite_left, 77) << "self-assign preserves value";
}

/* -----------------------------------------------------------------------
 * sprite_compute_clear_plan + sprite_execute_clear_plan
 * ---------------------------------------------------------------------- */
TEST(Shape2D, ClearPlanCompute) {
    struct SPRITE spr;
    memset(&spr, 0, sizeof spr);
    spr.sprite_left   = 10;
    spr.sprite_right  = 50;   /* width = 40 */
    spr.sprite_top    = 5;
    spr.sprite_height = 15;   /* lines = height - top = 10 */
    spr.sprite_pitch  = 320;

    unsigned int lineofs[16];
    for (int i = 0; i < 16; i++) lineofs[i] = (unsigned int)(i * 320);

    unsigned int out_ofs;
    int out_lines, out_width, out_widthdiff;

    EXPECT_EQ(sprite_compute_clear_plan(&spr, lineofs,
              &out_ofs, &out_lines, &out_width, &out_widthdiff), 1)
        << "normal case → 1";
    EXPECT_EQ((int)out_ofs,       5*320 + 10) << "ofs = lineofs[top]+left";
    EXPECT_EQ(out_lines,          10)         << "lines = height-top";
    EXPECT_EQ(out_width,          40)         << "width = right-left";
    EXPECT_EQ(out_widthdiff,      280)        << "widthdiff = pitch-width";

    spr.sprite_top = 15;
    EXPECT_EQ(sprite_compute_clear_plan(&spr, lineofs,
              &out_ofs, &out_lines, &out_width, &out_widthdiff), 0)
        << "top==height → 0";
    spr.sprite_top = 5;

    spr.sprite_right = 10;
    EXPECT_EQ(sprite_compute_clear_plan(&spr, lineofs,
              &out_ofs, &out_lines, &out_width, &out_widthdiff), 0)
        << "right==left → 0";
    spr.sprite_right = 50;

    EXPECT_EQ(sprite_compute_clear_plan(nullptr, lineofs,
              &out_ofs, &out_lines, &out_width, &out_widthdiff), 0)
        << "NULL sprite → 0";
    EXPECT_EQ(sprite_compute_clear_plan(&spr, nullptr,
              &out_ofs, &out_lines, &out_width, &out_widthdiff), 0)
        << "NULL lineofs → 0";
}

TEST(Shape2D, ClearPlanExecute) {
    struct SPRITE spr;
    memset(&spr, 0, sizeof spr);
    spr.sprite_left   = 10;
    spr.sprite_right  = 50;
    spr.sprite_top    = 5;
    spr.sprite_height = 15;
    spr.sprite_pitch  = 320;

    unsigned int lineofs[16];
    for (int i = 0; i < 16; i++) lineofs[i] = (unsigned int)(i * 320);

    unsigned int out_ofs;
    int out_lines, out_width, out_widthdiff;
    sprite_compute_clear_plan(&spr, lineofs, &out_ofs, &out_lines, &out_width, &out_widthdiff);

    unsigned char bitmap[320 * 16];
    memset(bitmap, 204, sizeof bitmap);
    sprite_execute_clear_plan(bitmap, out_ofs, out_lines, out_width, out_widthdiff, 85);

    bool all_ok = true;
    for (int row = 0; row < out_lines && all_ok; row++) {
        int row_start = (int)out_ofs + row * 320;
        for (int col = 0; col < out_width; col++) {
            if (bitmap[row_start + col] != 85) { all_ok = false; break; }
        }
    }
    EXPECT_TRUE(all_ok) << "all filled pixels == 85";

    EXPECT_EQ(bitmap[out_ofs - 1], (unsigned char)204) << "byte before region untouched";
    EXPECT_EQ(bitmap[out_ofs + (size_t)out_lines * 320], (unsigned char)204)
        << "byte after last row untouched";
    EXPECT_EQ(bitmap[out_ofs + (size_t)out_width], (unsigned char)204)
        << "gap byte after row 0 not overwritten";

    /* NULL / zero-edge guards — must not crash */
    sprite_execute_clear_plan(nullptr, out_ofs, out_lines, out_width, out_widthdiff, 0);
    sprite_execute_clear_plan(bitmap, out_ofs, 0, out_width, out_widthdiff, 0);
    sprite_execute_clear_plan(bitmap, out_ofs, out_lines, 0, out_widthdiff, 0);
}

/* -----------------------------------------------------------------------
 * shape2d_resource_shape_count
 * ---------------------------------------------------------------------- */
TEST(Shape2D, ResourceShapeCount) {
    EXPECT_EQ(shape2d_resource_shape_count(nullptr), 0) << "NULL → 0";

    unsigned char buf[8] = { 0, 0, 0, 0, 5, 0, 0, 0 };
    EXPECT_EQ(shape2d_resource_shape_count(buf), 5) << "lo=5, hi=0 → 5";

    buf[4] = 0; buf[5] = 1;
    EXPECT_EQ(shape2d_resource_shape_count(buf), 256) << "lo=0, hi=1 → 256";

    buf[4] = 2; buf[5] = 3;
    EXPECT_EQ(shape2d_resource_shape_count(buf), 770) << "lo=2, hi=3 → 770";

    buf[4] = 0; buf[5] = 0;
    EXPECT_EQ(shape2d_resource_shape_count(buf), 0) << "lo=0, hi=0 → 0";
}

/* -----------------------------------------------------------------------
 * shape2d_resource_get_shape
 * ---------------------------------------------------------------------- */
TEST(Shape2D, ResourceGetShape) {
    /*
     * chunk with 3 shapes.
     *   N=3, dataofs = 3*8+6 = 30
     *   Offset table starts at 6 + 3*4 = 18.
     *     shape 0 offset = 0   → SHAPE2D at buf+30
     *     shape 1 offset = 16  → SHAPE2D at buf+46
     *     shape 2 offset = 32  → SHAPE2D at buf+62
     */
    unsigned char buf[128];
    memset(buf, 0, sizeof buf);
    buf[4] = 3; buf[5] = 0;

    buf[18] = 0;  buf[19] = 0;  buf[20] = 0; buf[21] = 0;
    buf[22] = 16; buf[23] = 0;  buf[24] = 0; buf[25] = 0;
    buf[26] = 32; buf[27] = 0;  buf[28] = 0; buf[29] = 0;

    *reinterpret_cast<unsigned short*>(buf + 30) = 100;
    *reinterpret_cast<unsigned short*>(buf + 32) =  50;
    *reinterpret_cast<unsigned short*>(buf + 46) = 200;
    *reinterpret_cast<unsigned short*>(buf + 48) =  80;
    *reinterpret_cast<unsigned short*>(buf + 62) = 320;
    *reinterpret_cast<unsigned short*>(buf + 64) = 200;

    EXPECT_EQ(shape2d_resource_get_shape(nullptr, 0), nullptr) << "NULL chunk → NULL";
    EXPECT_EQ(shape2d_resource_get_shape(buf,    -1), nullptr) << "index=-1 → NULL";
    EXPECT_EQ(shape2d_resource_get_shape(buf,     3), nullptr) << "index==count → NULL";
    EXPECT_EQ(shape2d_resource_get_shape(buf,   100), nullptr) << "index>>count → NULL";

    struct SHAPE2D* s0 = shape2d_resource_get_shape(buf, 0);
    struct SHAPE2D* s1 = shape2d_resource_get_shape(buf, 1);
    struct SHAPE2D* s2 = shape2d_resource_get_shape(buf, 2);

    ASSERT_NE(s0, nullptr); ASSERT_NE(s1, nullptr); ASSERT_NE(s2, nullptr);

    EXPECT_EQ(s0->s2d_width,  100) << "shape[0] width";
    EXPECT_EQ(s0->s2d_height,  50) << "shape[0] height";
    EXPECT_EQ(s1->s2d_width,  200) << "shape[1] width";
    EXPECT_EQ(s1->s2d_height,  80) << "shape[1] height";
    EXPECT_EQ(s2->s2d_width,  320) << "shape[2] width";
    EXPECT_EQ(s2->s2d_height, 200) << "shape[2] height";

    EXPECT_EQ((char*)s1 - (char*)s0, 16) << "s1 is 16 bytes after s0";
    EXPECT_EQ((char*)s2 - (char*)s1, 16) << "s2 is 16 bytes after s1";
}

TEST(Shape2D, ResourceGetShapeHighOffset) {
    /* 16-bit pointer trap: shape at offset > 32767 */
    /* N=1, dataofs = 1*8+6 = 14, offset for shape 0 = 32768 */
    const size_t buf_size = static_cast<size_t>(14 + 32768 + 16 + 1);
    unsigned char* big_buf = static_cast<unsigned char*>(malloc(buf_size));
    if (!big_buf) GTEST_SKIP() << "malloc failed, skipping";

    memset(big_buf, 0, buf_size);
    big_buf[4] = 1; big_buf[5] = 0;
    big_buf[10] = 0; big_buf[11] = 128; big_buf[12] = 0; big_buf[13] = 0;

    unsigned long shape_pos = 14 + 32768UL;
    *reinterpret_cast<unsigned short*>(big_buf + shape_pos)     = 48879;
    *reinterpret_cast<unsigned short*>(big_buf + shape_pos + 2) = 4660;

    struct SHAPE2D* big_s = shape2d_resource_get_shape(big_buf, 0);
    ASSERT_NE(big_s, nullptr) << "shape at offset >32767 → non-NULL";
    EXPECT_EQ(big_s->s2d_width,  (unsigned short)48879) << "s2d_width beyond offset 32767";
    EXPECT_EQ(big_s->s2d_height, (unsigned short)4660) << "s2d_height beyond offset 32767";

    free(big_buf);
}

/* -----------------------------------------------------------------------
 * sprite_hittest_point — additional boundary and multi-rect cases
 * ---------------------------------------------------------------------- */
TEST(Shape2D, HittestPointYBoundaries) {
    const unsigned short x1[1] = { 10 };
    const unsigned short x2[1] = { 20 };
    const unsigned short y1[1] = { 10 };
    const unsigned short y2[1] = { 20 };

    EXPECT_EQ(sprite_hittest_point(15, 10, 1, x1, x2, y1, y2),  0) << "py==y1 → hit";
    EXPECT_EQ(sprite_hittest_point(15, 20, 1, x1, x2, y1, y2),  0) << "py==y2 → hit";
    EXPECT_EQ(sprite_hittest_point(15,  9, 1, x1, x2, y1, y2), -1) << "py<y1 → miss";
    EXPECT_EQ(sprite_hittest_point(15, 21, 1, x1, x2, y1, y2), -1) << "py>y2 → miss";
}

TEST(Shape2D, HittestPointReturnsSecondIndex) {
    /* First rect never matches; second should be returned as index 1 */
    const unsigned short x1[2] = {  0, 30 };
    const unsigned short x2[2] = {  5, 40 };
    const unsigned short y1[2] = {  0, 30 };
    const unsigned short y2[2] = {  5, 40 };

    EXPECT_EQ(sprite_hittest_point(35, 35, 2, x1, x2, y1, y2), 1) << "second rect → index 1";
    EXPECT_EQ(sprite_hittest_point( 2,  2, 2, x1, x2, y1, y2), 0) << "first rect  → index 0";
}

/* -----------------------------------------------------------------------
 * sprite_compute_clear_plan — additional degenerate inputs
 * ---------------------------------------------------------------------- */
TEST(Shape2D, ClearPlanTopGreaterThanHeight) {
    struct SPRITE spr;
    memset(&spr, 0, sizeof spr);
    spr.sprite_left   = 0;
    spr.sprite_right  = 10;
    spr.sprite_top    = 20;  /* top > height → lines < 0 */
    spr.sprite_height = 10;
    spr.sprite_pitch  = 320;
    unsigned int lineofs[32];
    for (int i = 0; i < 32; i++) lineofs[i] = (unsigned int)(i * 320);
    unsigned int ofs; int lines, width, wdiff;
    EXPECT_EQ(sprite_compute_clear_plan(&spr, lineofs, &ofs, &lines, &width, &wdiff), 0)
        << "top > height → 0";
}

TEST(Shape2D, ClearPlanPitchEqualsWidth) {
    struct SPRITE spr;
    memset(&spr, 0, sizeof spr);
    spr.sprite_left   = 0;
    spr.sprite_right  = 320;  /* width == pitch → widthdiff == 0 */
    spr.sprite_top    = 0;
    spr.sprite_height = 4;
    spr.sprite_pitch  = 320;
    unsigned int lineofs[4];
    for (int i = 0; i < 4; i++) lineofs[i] = (unsigned int)(i * 320);
    unsigned int ofs; int lines, width, wdiff;
    EXPECT_EQ(sprite_compute_clear_plan(&spr, lineofs, &ofs, &lines, &width, &wdiff), 1);
    EXPECT_EQ(wdiff, 0) << "pitch==width → widthdiff=0";
    EXPECT_EQ(lines, 4) << "4 rows";
    EXPECT_EQ(width, 320) << "320 wide";

    /* execute with widthdiff=0 must not skip or corrupt */
    unsigned char bmp[320 * 4];
    memset(bmp, 170, sizeof bmp);
    sprite_execute_clear_plan(bmp, ofs, lines, width, wdiff, 119);
    for (int i = 0; i < 320 * 4; i++)
        ASSERT_EQ(bmp[i], (unsigned char)119) << "byte " << i;
}

TEST(Shape2D, ClearPlanExecuteColorZero) {
    /* Clearing with color=0 must actively write zeros, not skip */
    struct SPRITE spr;
    memset(&spr, 0, sizeof spr);
    spr.sprite_left   =  0;
    spr.sprite_right  = 10;
    spr.sprite_top    =  0;
    spr.sprite_height =  2;
    spr.sprite_pitch  = 10;
    unsigned int lineofs[2] = { 0, 10 };
    unsigned int ofs; int lines, width, wdiff;
    sprite_compute_clear_plan(&spr, lineofs, &ofs, &lines, &width, &wdiff);

    unsigned char bmp[20];
    memset(bmp, 255, sizeof bmp);
    sprite_execute_clear_plan(bmp, ofs, lines, width, wdiff, 0);
    for (int i = 0; i < 20; i++)
        ASSERT_EQ(bmp[i], (unsigned char)0) << "zero-fill byte " << i;
}
/* -----------------------------------------------------------------------
 * shape2d_blit_rows
 *
 * int shape2d_blit_rows(unsigned char* dst, size_t dst_size,
 *                       unsigned int destofs,
 *                       const unsigned char* src,
 *                       unsigned int width, unsigned int height,
 *                       unsigned int pitch);
 *
 * Copies `height` rows of `width` bytes from `src` (tightly packed) into
 * `dst` at `destofs`, with row stride `pitch`.  Returns 1 on success, 0 on
 * any invalid argument or buffer overflow.
 * ---------------------------------------------------------------------- */

TEST(Shape2D, BlitRowsNullDst) {
    unsigned char src[4] = {1, 2, 3, 4};
    EXPECT_EQ(shape2d_blit_rows(nullptr, 100, 0, src, 2, 2, 2), 0)
        << "null dst → 0";
}

TEST(Shape2D, BlitRowsNullSrc) {
    unsigned char dst[100] = {};
    EXPECT_EQ(shape2d_blit_rows(dst, sizeof dst, 0, nullptr, 2, 2, 2), 0)
        << "null src → 0";
}

TEST(Shape2D, BlitRowsZeroWidth) {
    unsigned char dst[100] = {};
    unsigned char src[4] = {1, 2, 3, 4};
    EXPECT_EQ(shape2d_blit_rows(dst, sizeof dst, 0, src, 0, 2, 4), 0)
        << "width=0 → 0";
}

TEST(Shape2D, BlitRowsZeroHeight) {
    unsigned char dst[100] = {};
    unsigned char src[4] = {1, 2, 3, 4};
    EXPECT_EQ(shape2d_blit_rows(dst, sizeof dst, 0, src, 2, 0, 2), 0)
        << "height=0 → 0";
}

TEST(Shape2D, BlitRowsPitchLessThanWidth) {
    unsigned char dst[100] = {};
    unsigned char src[4] = {1, 2, 3, 4};
    /* pitch=1 < width=2 → invalid */
    EXPECT_EQ(shape2d_blit_rows(dst, sizeof dst, 0, src, 2, 2, 1), 0)
        << "pitch < width → 0";
}

TEST(Shape2D, BlitRowsSimple_2x2_TightPitch) {
    /* width=2, height=2, pitch=2 (tight): src bytes copied verbatim */
    unsigned char dst[8];
    memset(dst, 204, sizeof dst);
    const unsigned char src[4] = {17, 34, 51, 68};

    EXPECT_EQ(shape2d_blit_rows(dst, sizeof dst, 0, src, 2, 2, 2), 1);
    EXPECT_EQ(dst[0], 17u); EXPECT_EQ(dst[1], 34u);
    EXPECT_EQ(dst[2], 51u); EXPECT_EQ(dst[3], 68u);
}

TEST(Shape2D, BlitRowsWithDestOffset) {
    /* destofs=4: first blit row starts at dst[4] */
    unsigned char dst[16];
    memset(dst, 204, sizeof dst);
    const unsigned char src[2] = {170, 187};

    EXPECT_EQ(shape2d_blit_rows(dst, sizeof dst, 4, src, 2, 1, 2), 1);
    EXPECT_EQ(dst[0], 204u) << "bytes before destofs untouched";
    EXPECT_EQ(dst[4], 170u) << "first pixel at destofs";
    EXPECT_EQ(dst[5], 187u) << "second pixel at destofs+1";
    EXPECT_EQ(dst[6], 204u) << "bytes after row untouched";
}

TEST(Shape2D, BlitRowsStride_WidthPadding) {
    /* width=2, height=2, pitch=4: gap bytes between rows must NOT be touched */
    unsigned char dst[8];
    memset(dst, 204, sizeof dst);
    const unsigned char src[4] = {17, 34, 51, 68};

    EXPECT_EQ(shape2d_blit_rows(dst, sizeof dst, 0, src, 2, 2, 4), 1);
    /* row 0: dst[0..1] */
    EXPECT_EQ(dst[0], 17u); EXPECT_EQ(dst[1], 34u);
    /* gap bytes: dst[2..3] must remain 204 */
    EXPECT_EQ(dst[2], 204u) << "gap byte 0 must be untouched";
    EXPECT_EQ(dst[3], 204u) << "gap byte 1 must be untouched";
    /* row 1: dst[4..5] */
    EXPECT_EQ(dst[4], 51u); EXPECT_EQ(dst[5], 68u);
    /* gap bytes: dst[6..7] */
    EXPECT_EQ(dst[6], 204u) << "trailing gap byte 0 must be untouched";
    EXPECT_EQ(dst[7], 204u) << "trailing gap byte 1 must be untouched";
}

TEST(Shape2D, BlitRowsOverflowLastRow) {
    /* Last row would write past dst_size: must return 0, not crash */
    unsigned char dst[6] = {};
    unsigned char src[8] = {1,2,3,4,5,6,7,8};
    /* width=3, height=2, pitch=3, destofs=4: last row_end = 4+1*3+3=10 > 6 */
    EXPECT_EQ(shape2d_blit_rows(dst, 6, 4, src, 3, 2, 3), 0)
        << "overflow on last row → 0";
}

/* -----------------------------------------------------------------------
 * shape2d_lineofs_flat
 *
 * unsigned int* shape2d_lineofs_flat(unsigned int* stored_lineofs);
 *
 * Identity function: returns the same pointer it receives.
 * ---------------------------------------------------------------------- */

TEST(Shape2D, LineOfsFlat_Identity) {
    unsigned int arr[10] = {};
    EXPECT_EQ(shape2d_lineofs_flat(arr), arr)
        << "lineofs_flat must return the same pointer";
}

TEST(Shape2D, LineOfsFlat_NullPassthrough) {
    EXPECT_EQ(shape2d_lineofs_flat(nullptr), nullptr)
        << "lineofs_flat(null) must return null";
}

/* -----------------------------------------------------------------------
 * sprite_wnd_stack_reserve / sprite_wnd_stack_release
 *
 * These functions operate on a global LIFO stack (g_wnd_stack_count,
 * g_wnd_stack_ptrs[], g_wnd_stack_sizes[]).  Each test uses its own
 * scratch buffer and always releases every reservation it made, so the
 * global count is the same before and after each test.
 * ---------------------------------------------------------------------- */

/* Helper: compute the byte size of one reserved entry for a given height */
static size_t wnd_entry_size(unsigned short height) {
    return sizeof(struct SPRITE) + (size_t)height * sizeof(unsigned int);
}

TEST(Shape2D, WndStackReserve_NullBase) {
    unsigned char buf[512];
    unsigned char* next = buf;
    struct SPRITE* sp = nullptr;
    unsigned int* lo = nullptr;
    EXPECT_EQ(sprite_wnd_stack_reserve(nullptr, sizeof buf, &next, 4, &sp, &lo), 0)
        << "null base → 0";
}

TEST(Shape2D, WndStackReserve_NullNext) {
    unsigned char buf[512];
    struct SPRITE* sp = nullptr;
    unsigned int* lo = nullptr;
    EXPECT_EQ(sprite_wnd_stack_reserve(buf, sizeof buf, nullptr, 4, &sp, &lo), 0)
        << "null next → 0";
}

TEST(Shape2D, WndStackReserve_InsufficientCapacity) {
    unsigned char buf[4];  /* too small for even one SPRITE */
    unsigned char* next = buf;
    struct SPRITE* sp = nullptr;
    unsigned int* lo = nullptr;
    EXPECT_EQ(sprite_wnd_stack_reserve(buf, sizeof buf, &next, 4, &sp, &lo), 0)
        << "buffer too small for SPRITE + lineofs → 0";
}

TEST(Shape2D, WndStack_ReserveAndReleaseCycle) {
    /* Basic LIFO: reserve one entry, verify outputs, then release it */
    unsigned char buf[512];
    unsigned char* next = buf;
    struct SPRITE* sp = nullptr;
    unsigned int* lo = nullptr;
    const unsigned short H = 4;
    size_t expected = wnd_entry_size(H);

    EXPECT_EQ(sprite_wnd_stack_reserve(buf, sizeof buf, &next, H, &sp, &lo), 1)
        << "reserve must succeed";
    /* next pointer must have advanced by exactly one entry */
    EXPECT_EQ((size_t)(next - buf), expected)
        << "next advanced by sizeof(SPRITE) + H*sizeof(uint)";
    /* out_sprite must point to start of buf */
    EXPECT_EQ((void*)sp, (void*)buf)
        << "out_sprite must point to buf start";
    /* out_lineofs must immediately follow the SPRITE struct */
    EXPECT_EQ((void*)lo, (void*)(buf + sizeof(struct SPRITE)))
        << "out_lineofs must follow SPRITE in buffer";

    /* Release must succeed */
    EXPECT_EQ(sprite_wnd_stack_release(buf, &next, sp), 1)
        << "release top-of-stack must succeed";
    /* next must have retreated back to buf */
    EXPECT_EQ(next, buf)
        << "next must be restored to buf after release";
}

TEST(Shape2D, WndStack_DoubleReserveLIFO) {
    /* Reserve two entries; release must follow LIFO order */
    unsigned char buf[512];
    unsigned char* next = buf;
    struct SPRITE* sp1 = nullptr;  unsigned int* lo1 = nullptr;
    struct SPRITE* sp2 = nullptr;  unsigned int* lo2 = nullptr;
    const unsigned short H = 3;
    size_t entry = wnd_entry_size(H);

    EXPECT_EQ(sprite_wnd_stack_reserve(buf, sizeof buf, &next, H, &sp1, &lo1), 1);
    EXPECT_EQ(sprite_wnd_stack_reserve(buf, sizeof buf, &next, H, &sp2, &lo2), 1);
    EXPECT_EQ((size_t)(next - buf), 2 * entry) << "two entries allocated";

    /* sp2 is on top; must release sp2 first */
    EXPECT_EQ(sprite_wnd_stack_release(buf, &next, sp2), 1) << "release top (sp2)";
    EXPECT_EQ((size_t)(next - buf), entry)    << "next retreated by one entry";
    EXPECT_EQ(sprite_wnd_stack_release(buf, &next, sp1), 1) << "release bottom (sp1)";
    EXPECT_EQ(next, buf) << "stack fully drained";
}

TEST(Shape2D, WndStack_ReleaseWrongOrder) {
    /* Releasing sp1 while sp2 is on top must fail */
    unsigned char buf[512];
    unsigned char* next = buf;
    struct SPRITE* sp1 = nullptr;  unsigned int* lo1 = nullptr;
    struct SPRITE* sp2 = nullptr;  unsigned int* lo2 = nullptr;

    EXPECT_EQ(sprite_wnd_stack_reserve(buf, sizeof buf, &next, 3, &sp1, &lo1), 1);
    EXPECT_EQ(sprite_wnd_stack_reserve(buf, sizeof buf, &next, 3, &sp2, &lo2), 1);

    /* Attempt to release sp1 while sp2 is on top → must fail */
    EXPECT_EQ(sprite_wnd_stack_release(buf, &next, sp1), 0)
        << "releasing non-top entry must return 0";

    /* Clean up: release in correct order */
    EXPECT_EQ(sprite_wnd_stack_release(buf, &next, sp2), 1);
    EXPECT_EQ(sprite_wnd_stack_release(buf, &next, sp1), 1);
}

TEST(Shape2D, WndStack_ReleaseFromEmpty) {
    /* Releasing from an empty stack must return 0 */
    unsigned char buf[512];
    unsigned char* next = buf;
    /* Create a fake sprite pointer — it points into buf but nothing is reserved */
    struct SPRITE* fake = (struct SPRITE*)buf;
    EXPECT_EQ(sprite_wnd_stack_release(buf, &next, fake), 0)
        << "release from empty stack must return 0";
}

TEST(Shape2D, WndStackReserve_NullOutSprite) {
    unsigned char buf[512];
    unsigned char* next = buf;
    unsigned int* lo = nullptr;
    EXPECT_EQ(sprite_wnd_stack_reserve(buf, sizeof buf, &next, 4, nullptr, &lo), 0)
        << "null out_sprite must fail";
}

TEST(Shape2D, WndStackReserve_NullOutLineofs) {
    unsigned char buf[512];
    unsigned char* next = buf;
    struct SPRITE* sp = nullptr;
    EXPECT_EQ(sprite_wnd_stack_reserve(buf, sizeof buf, &next, 4, &sp, nullptr), 0)
        << "null out_lineofs must fail";
}

TEST(Shape2D, BlitRowsAcceptsExactEndBoundary) {
    /* Last written byte lands exactly at dst_size boundary (allowed). */
    unsigned char dst[8];
    const unsigned char src[4] = { 1, 2, 3, 4 };
    memset(dst, 170, sizeof dst);

    /* destofs=4, width=2, height=2, pitch=2 => writes bytes [4..7] exactly */
    EXPECT_EQ(shape2d_blit_rows(dst, sizeof dst, 4, src, 2, 2, 2), 1);
    EXPECT_EQ(dst[4], 1u);
    EXPECT_EQ(dst[5], 2u);
    EXPECT_EQ(dst[6], 3u);
    EXPECT_EQ(dst[7], 4u);
}

TEST(Shape2D, BlitRowsDestOffsetAtBufferEndFails) {
    unsigned char dst[8] = {};
    unsigned char src[1] = { 9 };
    EXPECT_EQ(shape2d_blit_rows(dst, sizeof dst, 8, src, 1, 1, 1), 0)
        << "dest offset at end leaves no writable byte";
}

TEST(Shape2D, SpriteCopyBothRoundTrip) {
    struct SPRITE in[2];
    struct SPRITE out[2];
    memset(in, 0, sizeof in);
    memset(out, 0, sizeof out);

    in[0].sprite_left = 3;
    in[0].sprite_right = 120;
    in[0].sprite_top = 2;
    in[0].sprite_height = 40;
    in[0].sprite_pitch = 160;

    in[1].sprite_left = 10;
    in[1].sprite_right = 200;
    in[1].sprite_top = 5;
    in[1].sprite_height = 80;
    in[1].sprite_pitch = 320;

    sprite_copy_arg_to_both(in);
    sprite_copy_both_to_arg(out);

    EXPECT_EQ(out[0].sprite_left, in[0].sprite_left);
    EXPECT_EQ(out[0].sprite_right, in[0].sprite_right);
    EXPECT_EQ(out[0].sprite_top, in[0].sprite_top);
    EXPECT_EQ(out[0].sprite_height, in[0].sprite_height);
    EXPECT_EQ(out[0].sprite_pitch, in[0].sprite_pitch);

    EXPECT_EQ(out[1].sprite_left, in[1].sprite_left);
    EXPECT_EQ(out[1].sprite_right, in[1].sprite_right);
    EXPECT_EQ(out[1].sprite_top, in[1].sprite_top);
    EXPECT_EQ(out[1].sprite_height, in[1].sprite_height);
    EXPECT_EQ(out[1].sprite_pitch, in[1].sprite_pitch);
}

TEST(Shape2D, SpriteCopyArgToBothNullKeepsState) {
    struct SPRITE init[2];
    struct SPRITE before[2];
    struct SPRITE after[2];
    memset(init, 0, sizeof init);
    memset(before, 0, sizeof before);
    memset(after, 0, sizeof after);

    init[0].sprite_left = 7;
    init[0].sprite_right = 123;
    init[0].sprite_pitch = 200;
    init[0].sprite_height = 100;
    init[1].sprite_left = 11;
    init[1].sprite_right = 211;
    init[1].sprite_pitch = 320;
    init[1].sprite_height = 180;

    sprite_copy_arg_to_both(init);
    sprite_copy_both_to_arg(before);

    sprite_copy_arg_to_both(nullptr);
    sprite_copy_both_to_arg(after);

    EXPECT_EQ(memcmp(before, after, sizeof before), 0)
        << "NULL input must not mutate sprite1/sprite2";
}

TEST(Shape2D, SpriteCopyArgToBothSanitizesInvalidGeometry) {
    struct SPRITE bad[2];
    struct SPRITE out[2];
    memset(bad, 0, sizeof bad);
    memset(out, 0, sizeof out);

    /* Intentionally broken geometry/state to trigger sanitizer paths. */
    bad[0].sprite_left = 500;
    bad[0].sprite_right = 10;
    bad[0].sprite_top = 300;
    bad[0].sprite_height = 0;
    bad[0].sprite_pitch = 0;
    bad[0].sprite_lineofs = nullptr;
    bad[0].sprite_bitmapptr = nullptr;

    bad[1] = bad[0];

    sprite_copy_arg_to_both(bad);
    sprite_copy_both_to_arg(out);

    EXPECT_LT(out[0].sprite_left, out[0].sprite_right)
        << "sprite1 left/right sanitized to valid range";
    EXPECT_LT(out[1].sprite_left, out[1].sprite_right)
        << "sprite2 left/right sanitized to valid range";
    EXPECT_GT(out[0].sprite_pitch, 0)
        << "sprite1 pitch sanitized";
    EXPECT_GT(out[1].sprite_pitch, 0)
        << "sprite2 pitch sanitized";
    EXPECT_NE(out[0].sprite_lineofs, nullptr)
        << "sprite1 lineofs replaced with runtime table";
    EXPECT_NE(out[1].sprite_lineofs, nullptr)
        << "sprite2 lineofs replaced with runtime table";
    EXPECT_NE(out[0].sprite_bitmapptr, nullptr)
        << "sprite1 bitmap pointer sanitized";
    EXPECT_NE(out[1].sprite_bitmapptr, nullptr)
        << "sprite2 bitmap pointer sanitized";
}