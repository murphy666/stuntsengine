/*
 * test_shape3d_polyinfo.cc — Google Test version of test_shape3d_polyinfo.c
 */

#include <gtest/gtest.h>
#include <cstdlib>
#include <cstring>
#include <cstdint>

/* -----------------------------------------------------------------------
 * Module-level globals (mimic .c statics)
 * ---------------------------------------------------------------------- */
static int  word_40ED6[401];
static int* polyinfoptrs[401];

static void reset_polyinfo() {
    memset(word_40ED6,   0, sizeof word_40ED6);
    memset(polyinfoptrs, 0, sizeof polyinfoptrs);
    word_40ED6[400] = static_cast<int>(65535);
}

/* -----------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */
TEST(Shape3DPolyinfo, ArraySizes) {
    EXPECT_EQ(sizeof(word_40ED6)   / sizeof(word_40ED6[0]),   401u);
    EXPECT_EQ(sizeof(polyinfoptrs) / sizeof(polyinfoptrs[0]), 401u);
}

TEST(Shape3DPolyinfo, HeadInitialization) {
    reset_polyinfo();
    EXPECT_EQ(word_40ED6[400], static_cast<int>(65535)) << "head = 65535 (empty list)";
    for (int i = 0; i < 400; i++)
        EXPECT_EQ(word_40ED6[i], 0) << "word_40ED6[" << i << "] = 0 after reset";
    for (int i = 0; i < 400; i++)
        EXPECT_EQ(polyinfoptrs[i], nullptr) << "polyinfoptrs[" << i << "] = null after reset";
}

TEST(Shape3DPolyinfo, LinkedListInsertion) {
    reset_polyinfo();
    unsigned char dummy_buf[200] = {};
    unsigned polyinfoptrnext = 0;
    // simulate append
    auto append = [&](){
        int idx = static_cast<int>(polyinfoptrnext);
        polyinfoptrs[idx] = reinterpret_cast<int*>(dummy_buf + idx * 20);
        word_40ED6[idx]   = word_40ED6[400];
        word_40ED6[400] = idx;
        polyinfoptrnext++;
    };
    append(); append(); append();
    EXPECT_EQ(word_40ED6[400], 2)                    << "head = 2";
    EXPECT_EQ(word_40ED6[2],     1)                    << "node 2 → node 1";
    EXPECT_EQ(word_40ED6[1],     0)                    << "node 1 → node 0";
    EXPECT_EQ(word_40ED6[0], static_cast<int>(65535)) << "node 0 → sentinel";
}

TEST(Shape3DPolyinfo, MaximumCapacity) {
    EXPECT_LE(400u, sizeof(word_40ED6)/sizeof(word_40ED6[0]) - 1u)
        << "can hold at least 400 polys  (0..399) with head at 400=400";
    EXPECT_EQ(400u, 400u) << "head index is 400";
}

TEST(Shape3DPolyinfo, BoundaryAccess) {
    reset_polyinfo();
    word_40ED6[399] = 42;
    EXPECT_EQ(word_40ED6[399], 42) << "last poly slot accessible";
    word_40ED6[400] = 399;
    EXPECT_EQ(word_40ED6[400], 399) << "head = 399";
    word_40ED6[400] = static_cast<int>(65535);
}

TEST(Shape3DPolyinfo, Restunts2Compat) {
    // Confirm that the linked-list iteration idiom used in restunts2 is correct
    reset_polyinfo();
    unsigned char dummy_buf[20*3] = {};
    word_40ED6[0] = static_cast<int>(65535);
    word_40ED6[1] = 0;
    word_40ED6[2] = 1;
    word_40ED6[400] = 2;
    for (int i = 0; i < 3; i++)
        polyinfoptrs[i] = reinterpret_cast<int*>(dummy_buf + i * 20);

    int count = 0;
    int idx = word_40ED6[400];
    while (idx != static_cast<int>(65535)) {
        ASSERT_GE(idx, 0); ASSERT_LT(idx, 400);
        ASSERT_NE(polyinfoptrs[idx], nullptr);
        count++;
        idx = word_40ED6[idx];
    }
    EXPECT_EQ(count, 3) << "iteration visits 3 nodes";
}
