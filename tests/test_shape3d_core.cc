/*
 * test_shape3d_core.cc — Google Test version of test_shape3d_core.c
 */

#include <gtest/gtest.h>
#include <cstdlib>
#include <cstring>
#include <cstdint>

/* -----------------------------------------------------------------------
 * Globals mirroring shape3d.c runtime state
 * ---------------------------------------------------------------------- */
static unsigned char* polyinfoptr;
static int word_40ED6[401];
static int* polyinfoptrs[401];
static unsigned polyinfonumpolys;
static unsigned polyinfoptrnext;

struct PolyinfoFixture : public ::testing::Test {
    void SetUp() override {
        polyinfoptr       = nullptr;
        polyinfonumpolys  = 0;
        polyinfoptrnext   = 0;
        memset(word_40ED6,   0, sizeof word_40ED6);
        memset(polyinfoptrs, 0, sizeof polyinfoptrs);
    }
    void TearDown() override {
        free(polyinfoptr);
        polyinfoptr = nullptr;
    }
};

/* -----------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */
TEST_F(PolyinfoFixture, Allocation) {
    constexpr size_t SZ = 10400;
    polyinfoptr = static_cast<unsigned char*>(calloc(1, SZ));
    ASSERT_NE(polyinfoptr, nullptr) << "allocation succeeds";
    bool all_zero = true;
    for (size_t i = 0; i < SZ; i++) if (polyinfoptr[i]) { all_zero = false; break; }
    EXPECT_TRUE(all_zero) << "calloc zeroes buffer";
}

TEST_F(PolyinfoFixture, Reset) {
    polyinfonumpolys  = 0;
    polyinfoptrnext   = 0;
    word_40ED6[400] = static_cast<int>(65535);
    EXPECT_EQ(polyinfonumpolys,       0u)                   << "count = 0";
    EXPECT_EQ(polyinfoptrnext,        0u)                   << "next ptr = 0";
    EXPECT_EQ(word_40ED6[400], static_cast<int>(65535))  << "head = 65535 (empty)";
}

TEST_F(PolyinfoFixture, Append) {
    polyinfoptr = static_cast<unsigned char*>(calloc(1, 10400));
    ASSERT_NE(polyinfoptr, nullptr);

    word_40ED6[400] = static_cast<int>(65535);

    auto append = [&](){
        int idx = static_cast<int>(polyinfoptrnext);
        polyinfoptrs[idx] = reinterpret_cast<int*>(polyinfoptr + idx * 100);
        word_40ED6[idx]   = word_40ED6[400];
        word_40ED6[400] = idx;
        polyinfoptrnext++;
        polyinfonumpolys++;
    };

    append();
    EXPECT_EQ(polyinfonumpolys,       1u)                   << "count after 1st append";
    EXPECT_EQ(word_40ED6[400],      0)                    << "head → index 0";
    EXPECT_EQ(word_40ED6[0], static_cast<int>(65535))      << "node 0 → sentinel";

    append();
    EXPECT_EQ(polyinfonumpolys,       2u)                   << "count after 2nd append";
    EXPECT_EQ(word_40ED6[400],      1)                    << "head → index 1";
    EXPECT_EQ(word_40ED6[1],          0)                    << "node 1 → node 0";
    EXPECT_EQ(word_40ED6[0], static_cast<int>(65535))      << "node 0 still terminal";
}

TEST_F(PolyinfoFixture, Traversal) {
    polyinfoptr = static_cast<unsigned char*>(calloc(1, 10400));
    ASSERT_NE(polyinfoptr, nullptr);

    polyinfonumpolys  = 3;
    polyinfoptrnext   = 3;
    for (int i = 0; i < 3; i++)
        polyinfoptrs[i] = reinterpret_cast<int*>(polyinfoptr + i * 100);

    word_40ED6[400] = 2;
    word_40ED6[2]     = 1;
    word_40ED6[1]     = 0;
    word_40ED6[0]     = static_cast<int>(65535);

    bool visited[3] = {};
    int count = 0;
    int idx = word_40ED6[400];
    while (idx != static_cast<int>(65535) && count < 400) {
        EXPECT_GE(idx, 0);
        EXPECT_LT(idx, 400);
        EXPECT_NE(polyinfoptrs[idx], nullptr);
        visited[idx] = true;
        count++;
        idx = word_40ED6[idx];
    }
    EXPECT_EQ(count, 3)               << "traversed 3 nodes";
    EXPECT_TRUE(visited[0] && visited[1] && visited[2]) << "all nodes visited";
    EXPECT_EQ(idx, static_cast<int>(65535)) << "ends at sentinel";
}

TEST_F(PolyinfoFixture, Bounds) {
    EXPECT_EQ(sizeof(word_40ED6)   / sizeof(word_40ED6[0]),   401u);
    EXPECT_EQ(sizeof(polyinfoptrs) / sizeof(polyinfoptrs[0]), 401u);
    word_40ED6[400] = static_cast<int>(65535);
    EXPECT_EQ(word_40ED6[400], static_cast<int>(65535)) << "index 400 accessible";
}

TEST_F(PolyinfoFixture, Alignment) {
    polyinfoptr = static_cast<unsigned char*>(calloc(1, 10400));
    ASSERT_NE(polyinfoptr, nullptr);
    int* p0 = reinterpret_cast<int*>(polyinfoptr);
    *p0 = 305419896;
    EXPECT_EQ(*p0, 305419896) << "int at ofs 0";
    int* p4 = reinterpret_cast<int*>(polyinfoptr + 4);
    *p4 = static_cast<int>(2882400000);
    EXPECT_EQ(static_cast<unsigned>(*p4), 2882400000u) << "int at ofs 4";
}

TEST_F(PolyinfoFixture, RecordStructure) {
    polyinfoptr = static_cast<unsigned char*>(calloc(1, 10400));
    ASSERT_NE(polyinfoptr, nullptr);
    unsigned char* rec = polyinfoptr;
    *reinterpret_cast<uint16_t*>(rec + 0) = 100;
    *reinterpret_cast<uint16_t*>(rec + 2) = 500;
    rec[4] = 1;
    rec[5] = 15;
    *reinterpret_cast<int*>(rec + 6)  = 10;
    *reinterpret_cast<int*>(rec + 10) = 20;
    *reinterpret_cast<int*>(rec + 14) = 30;
    *reinterpret_cast<int*>(rec + 18) = 40;
    EXPECT_EQ(*reinterpret_cast<uint16_t*>(rec + 0), 100u) << "priority";
    EXPECT_EQ(*reinterpret_cast<uint16_t*>(rec + 2), 500u) << "depth";
    EXPECT_EQ(rec[4],  1)  << "type=line";
    EXPECT_EQ(rec[5], 15)  << "material=white";
    EXPECT_EQ(*reinterpret_cast<int*>(rec + 6),  10) << "x1";
    EXPECT_EQ(*reinterpret_cast<int*>(rec + 10), 20) << "y1";
    EXPECT_EQ(*reinterpret_cast<int*>(rec + 14), 30) << "x2";
    EXPECT_EQ(*reinterpret_cast<int*>(rec + 18), 40) << "y2";
}

TEST_F(PolyinfoFixture, MultipleRecords) {
    polyinfoptr = static_cast<unsigned char*>(calloc(1, 10400));
    ASSERT_NE(polyinfoptr, nullptr);
    constexpr size_t REC = 24;
    EXPECT_GE(static_cast<int>(10400 / REC), 400) << "buffer holds >= 400 records";
    for (int i = 0; i < 10; i++) {
        polyinfoptr[i * REC + 4] = static_cast<unsigned char>(i);
        polyinfoptr[i * REC + 5] = static_cast<unsigned char>(i * 10);
    }
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(polyinfoptr[i * REC + 4], (unsigned char)i)       << "type[" << i << "]";
        EXPECT_EQ(polyinfoptr[i * REC + 5], (unsigned char)(i * 10)) << "material[" << i << "]";
    }
}
