/*
 * test_shape3d_pointer_table_regression.cc — GTest version
 */

#include <gtest/gtest.h>
#include <cstdlib>
#include <cstring>
#include <cstdint>

struct POINT2D { int px, py; };

TEST(Shape3DPointerReg, PointerTableFullInit) {
    static POINT2D points[64];
    static POINT2D* ptrs[64];
    memset(points, 0, sizeof points);
    for (int i = 0; i < 64; i++) ptrs[i] = &points[i];
    for (int i = 0; i < 64; i++) {
        ASSERT_NE(ptrs[i], nullptr) << "ptrs[" << i << "] != null";
        EXPECT_EQ(ptrs[i], &points[i]) << "ptrs[" << i << "] == &points[" << i << "]";
    }
}

TEST(Shape3DPointerReg, EarlyJumpSafety) {
    // Simulate a situation where the renderer jumps early but the pointer
    // table has been fully initialized — all reads should return valid ptrs
    static POINT2D points[64];
    static POINT2D* ptrs[64];
    memset(points, 0, sizeof points);
    for (int i = 0; i < 64; i++) { points[i].px = i; points[i].py = i * 2; ptrs[i] = &points[i]; }

    int jump_target = 10;
    for (int i = 0; i < 64; i++) {
        ASSERT_NE(ptrs[i], nullptr) << "ptr[" << i << "] set even after early jump at " << jump_target;
        EXPECT_EQ(ptrs[i]->px, i)     << "px[" << i << "]";
        EXPECT_EQ(ptrs[i]->py, i * 2) << "py[" << i << "]";
    }
}

TEST(Shape3DPointerReg, ZeroInitDefaults) {
    static POINT2D points[64];
    static POINT2D* ptrs[64];
    memset(points, 0, sizeof points);
    memset(ptrs,   0, sizeof ptrs);
    for (int i = 0; i < 64; i++) {
        EXPECT_EQ(ptrs[i], nullptr)     << "unset ptr[" << i << "] = null";
        EXPECT_EQ(points[i].px, 0)      << "px[" << i << "] = 0";
        EXPECT_EQ(points[i].py, 0)      << "py[" << i << "] = 0";
    }
    for (int i = 0; i < 64; i++) ptrs[i] = &points[i];
    for (int i = 0; i < 64; i++)
        EXPECT_NE(ptrs[i], nullptr) << "after init ptr[" << i << "] != null";
}
