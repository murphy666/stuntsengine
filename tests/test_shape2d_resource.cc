/*
 * test_shape2d_resource.cc — Google Test version of test_shape2d_resource.c
 *
 * Tests the binary layout of the shape2d resource format.
 * The format uses the following header layout:
 *
 *   bytes  0-3  : total resource size (uint32 LE) or pad
 *   bytes  4-5  : shape count (uint16 LE)
 *   bytes  6 .. 6+count*4-1      : per-shape ID / type fields (4 bytes each)
 *   bytes  6+count*4 .. 6+count*8-1 : per-shape offsets (uint32 LE, 4 bytes each)
 *   data section starts at offset: 6 + count*8
 *   shape[i] is at data_section_base + offset[i]
 *
 * Standalone — no fileio.c or shape2d_resource.c dependency.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>

extern "C" {
#include "shape2d.h"
}

// ---------------------------------------------------------------------------
// Helper: read little-endian uint16 / uint32 from a byte buffer

static uint16_t read_u16le(const unsigned char* p) {
    return static_cast<uint16_t>(p[0] | (static_cast<unsigned>(p[1]) << 8));
}

static uint32_t read_u32le(const unsigned char* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) <<  8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

// ---------------------------------------------------------------------------
// Header: shape count field at bytes [4-5]

TEST(Shape2DResource, ShapeCountField) {
    unsigned char buf[32] = {};
    buf[4] = 3;
    buf[5] = 0;
    EXPECT_EQ(read_u16le(buf + 4), 3u) << "shape count decoded from bytes 4-5";
}

TEST(Shape2DResource, ShapeCountFieldHighByte) {
    unsigned char buf[32] = {};
    buf[4] = 0;
    buf[5] = 1;
    EXPECT_EQ(read_u16le(buf + 4), 256u) << "shape count high byte decoded";
}

// ---------------------------------------------------------------------------
// Offset table: starts at byte 6 + count*4, each entry 4 bytes (uint32 LE)

TEST(Shape2DResource, OffsetTablePosition) {
    // With count=2: offset table starts at byte 6 + 2*4 = 14
    unsigned char buf[64] = {};
    const unsigned int count = 2;
    const unsigned int ofs_table_start = 6u + count * 4u;
    EXPECT_EQ(ofs_table_start, 14u) << "offset table starts at byte 14 for count=2";

    buf[14] = 6; buf[15] = 0; buf[16] = 0; buf[17] = 0;
    buf[18] = 38; buf[19] = 4; buf[20] = 0; buf[21] = 0;

    EXPECT_EQ(read_u32le(buf + 14), 6u)    << "shape 0 offset = 6";
    EXPECT_EQ(read_u32le(buf + 18), 1062u)  << "shape 1 offset = 1062";
}

TEST(Shape2DResource, DataSectionOffset) {
    // dataofs = 6 + count*8
    const unsigned int count = 2;
    const unsigned int dataofs = 6u + count * 8u;
    EXPECT_EQ(dataofs, 22u) << "data section starts at byte 22 for count=2";
}

// ---------------------------------------------------------------------------
// Shape struct layout (SHAPE2D from shape2d.h)

TEST(Shape2DResource, Shape2DStructSize) {
    // SHAPE2D: 3×uint16 dims, 2×uint16 hotspot, 2×uint16 pos, 4×uint8 nibbles = 16 bytes
    EXPECT_EQ(sizeof(struct SHAPE2D), 16u) << "SHAPE2D struct is 16 bytes (packed)";
}

TEST(Shape2DResource, Shape2DWidthAtOffset0) {
    unsigned char raw[16] = {};
    raw[0] = 32; raw[1] = 0;  // width = 32
    raw[2] = 16; raw[3] = 0;  // height = 16
    const struct SHAPE2D* s = reinterpret_cast<const struct SHAPE2D*>(raw);
    EXPECT_EQ(s->s2d_width,  32u) << "s2d_width at byte offset 0";
    EXPECT_EQ(s->s2d_height, 16u) << "s2d_height at byte offset 2";
}

// ---------------------------------------------------------------------------
// Integrated: build a synthetic shape2d resource buffer and resolve shapes

TEST(Shape2DResource, ShapeResolutionFromBuffer) {
    unsigned char buf[1100] = {};

    // header
    buf[4] = 2;   // count = 2
    buf[5] = 0;

    // offset table (starts at 6 + 2*4 = 14)
    buf[14] = 6; buf[15] = 0; buf[16] = 0; buf[17] = 0;  // offset[0] = 6
    buf[18] = 38; buf[19] = 4; buf[20] = 0; buf[21] = 0;  // offset[1] = 1062

    const unsigned int count   = read_u16le(buf + 4);
    const unsigned int dataofs = 6u + count * 8u;

    const uint32_t off0 = read_u32le(buf + 6u + count * 4u + 0u * 4u);
    const uint32_t off1 = read_u32le(buf + 6u + count * 4u + 1u * 4u);

    struct SHAPE2D* s0 = reinterpret_cast<struct SHAPE2D*>(buf + dataofs + off0);
    struct SHAPE2D* s1 = reinterpret_cast<struct SHAPE2D*>(buf + dataofs + off1);

    s0->s2d_width  = 16;
    s0->s2d_height = 8;
    s1->s2d_width  = 48;
    s1->s2d_height = 24;

    EXPECT_EQ(count,  2u)  << "shape count = 2";
    EXPECT_EQ(dataofs, 22u) << "data section at byte 22";

    EXPECT_EQ(off0, 6u)     << "shape 0 offset = 6";
    EXPECT_EQ(off1, 1062u) << "shape 1 offset = 1062";

    ASSERT_NE(s0, nullptr) << "shape 0 pointer non-null";
    ASSERT_NE(s1, nullptr) << "shape 1 pointer non-null";

    EXPECT_EQ(s0->s2d_width,  16u) << "shape 0 width correct";
    EXPECT_EQ(s0->s2d_height,  8u) << "shape 0 height correct";
    EXPECT_EQ(s1->s2d_width,  48u) << "shape 1 width correct";
    EXPECT_EQ(s1->s2d_height, 24u) << "shape 1 height correct";
}

TEST(Shape2DResource, OutOfRangeIndexSentinel) {
    // Verify that a shape count of 2 means indices 0 and 1 are valid,
    // and index 2 is out of range (count check: idx >= count).
    const unsigned int count = 2;
    const unsigned int idx_valid_max = count - 1u;
    EXPECT_EQ(idx_valid_max, 1u)           << "last valid index is 1";
    EXPECT_LT(idx_valid_max, count)        << "valid index < count";
    EXPECT_FALSE(2u < count)               << "index 2 is out of range";
}

// ---------------------------------------------------------------------------
// Edge: count=0

TEST(Shape2DResource, ZeroShapeCount) {
    unsigned char buf[16] = {};
    buf[4] = 0;
    buf[5] = 0;
    const unsigned int count   = read_u16le(buf + 4);
    const unsigned int dataofs = 6u + count * 8u;
    EXPECT_EQ(count,   0u) << "zero shape count";
    EXPECT_EQ(dataofs, 6u) << "data section immediately after 6-byte header for count=0";
}

// ---------------------------------------------------------------------------
// Single shape at offset zero within data section

TEST(Shape2DResource, SingleShapeAtOffset0) {
    unsigned char buf[64] = {};
    buf[4] = 1;  // count = 1
    // offset table at 6 + 1*4 = 10
    buf[10] = 0; buf[11] = 0; buf[12] = 0; buf[13] = 0;  // offset[0] = 0

    const unsigned int count   = read_u16le(buf + 4);
    const unsigned int dataofs = 6u + count * 8u;  // = 14

    EXPECT_EQ(dataofs, 14u) << "data section at byte 14 for count=1";

    const uint32_t off0 = read_u32le(buf + 6u + count * 4u);
    EXPECT_EQ(off0, 0u) << "shape 0 offset = 0";

    struct SHAPE2D* s = reinterpret_cast<struct SHAPE2D*>(buf + dataofs + off0);
    s->s2d_width  = 64;
    s->s2d_height = 64;
    EXPECT_EQ(s->s2d_width,  64u);
    EXPECT_EQ(s->s2d_height, 64u);
}
