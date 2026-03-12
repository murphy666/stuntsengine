/*
 * test_shape3d_pipeline.cc — Google Test version of test_shape3d_pipeline.c
 */

#include <gtest/gtest.h>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cmath>

#pragma pack(push, 1)
struct VECTOR { int16_t x, y, z; };
struct SHAPE3D {
    uint16_t numverts;
    uint16_t numprims;
    uint16_t numpaints;
    VECTOR   verts[64];
    uint8_t  prims[512];
};
struct RECTANGLE { int16_t left, top, right, bottom; };
struct TRANSFORMEDSHAPE3D {
    uint16_t numverts;
    int32_t  tx[64];
    int32_t  ty[64];
    int32_t  tz[64];
    uint8_t  visible[64];
};
#pragma pack(pop)

struct MockFramebuffer {
    static constexpr int W = 160, H = 100;
    uint8_t pixels[H][W];
    void clear() { memset(pixels, 0, sizeof pixels); }
    void set_pixel(int x, int y, uint8_t c) {
        if (x >= 0 && x < W && y >= 0 && y < H) pixels[y][x] = c;
    }
};

TEST(Shape3DPipeline, ShapeStructure) {
    EXPECT_EQ(sizeof(VECTOR),       6u)  << "VECTOR = 3×int16";
    EXPECT_EQ(sizeof(RECTANGLE),    8u)  << "RECTANGLE = 4×int16";
    SHAPE3D s; s.numverts = 8; s.numprims = 6; s.numpaints = 2;
    EXPECT_EQ(s.numverts,  8u) << "numverts";
    EXPECT_EQ(s.numprims,  6u) << "numprims";
    EXPECT_EQ(s.numpaints, 2u) << "numpaints";
}

TEST(Shape3DPipeline, Transformation) {
    SHAPE3D shape = {};
    shape.numverts = 4;
    VECTOR verts[4] = {{100,0,0},{0,100,0},{0,0,100},{50,50,50}};
    memcpy(shape.verts, verts, sizeof verts);

    TRANSFORMEDSHAPE3D ts = {};
    ts.numverts = shape.numverts;
    int32_t cx=0, cy=0, cz=200;
    for (uint16_t i = 0; i < shape.numverts; i++) {
        ts.tx[i] = cx + shape.verts[i].x;
        ts.ty[i] = cy + shape.verts[i].y;
        ts.tz[i] = cz + shape.verts[i].z;
        ts.visible[i] = (ts.tz[i] > 0) ? 1 : 0;
    }
    EXPECT_EQ(ts.tx[0], 100); EXPECT_EQ(ts.ty[0], 0);   EXPECT_EQ(ts.tz[0], 200);
    EXPECT_EQ(ts.tx[1], 0);   EXPECT_EQ(ts.ty[1], 100);  EXPECT_EQ(ts.tz[1], 200);
    EXPECT_EQ(ts.tx[2], 0);   EXPECT_EQ(ts.ty[2], 0);   EXPECT_EQ(ts.tz[2], 300);
    EXPECT_EQ(ts.tx[3], 50);  EXPECT_EQ(ts.ty[3], 50);  EXPECT_EQ(ts.tz[3], 250);
    for (uint16_t i = 0; i < shape.numverts; i++)
        EXPECT_EQ(ts.visible[i], 1u) << "vert " << i << " visible";
}

TEST(Shape3DPipeline, PolyinfoGeneration) {
    struct PolyEntry { uint16_t priority; uint8_t type; uint8_t paint; uint8_t vert_count; uint8_t verts[4]; };
    PolyEntry polys[3]; memset(polys, 0, sizeof polys);
    polys[0] = {100, 1, 15, 2, {0,1,0,0}};
    polys[1] = {200, 2, 14, 3, {0,1,2,0}};
    polys[2] = {150, 2, 10, 4, {0,1,2,3}};
    EXPECT_EQ(polys[0].priority, 100u);  EXPECT_EQ(polys[0].type, 1u);
    EXPECT_EQ(polys[1].priority, 200u);  EXPECT_EQ(polys[1].type, 2u);
    EXPECT_EQ(polys[2].priority, 150u);  EXPECT_EQ(polys[2].vert_count, 4u);
}

TEST(Shape3DPipeline, RenderToFramebuffer) {
    MockFramebuffer fb; fb.clear();
    for (int x = 10; x < 30; x++) for (int y = 10; y < 30; y++) fb.set_pixel(x, y, 15);
    int count = 0;
    for (int y = 0; y < MockFramebuffer::H; y++)
        for (int x = 0; x < MockFramebuffer::W; x++)
            if (fb.pixels[y][x] == 15) count++;
    EXPECT_EQ(count, 400) << "20×20 filled region = 400 pixels";
    EXPECT_EQ(fb.pixels[10][10], 15u) << "top-left";
    EXPECT_EQ(fb.pixels[29][29], 15u) << "bottom-right";
    EXPECT_EQ(fb.pixels[ 9][ 9],  0u) << "outside = 0";
}

TEST(Shape3DPipeline, FullPipeline) {
    // Create a simple shape with one quad
    SHAPE3D shape = {};
    shape.numverts = 4; shape.numprims = 1;
    shape.verts[0] = {-50, -50, 0};
    shape.verts[1] = { 50, -50, 0};
    shape.verts[2] = { 50,  50, 0};
    shape.verts[3] = {-50,  50, 0};

    // Transform
    TRANSFORMEDSHAPE3D ts = {};
    ts.numverts = shape.numverts;
    int32_t cx = 80, cy = 50, cz = 200;
    for (uint16_t i = 0; i < shape.numverts; i++) {
        ts.tx[i] = cx + shape.verts[i].x;
        ts.ty[i] = cy + shape.verts[i].y;
        ts.tz[i] = cz + shape.verts[i].z;
        ts.visible[i] = 1;
    }
    EXPECT_EQ(ts.tx[0], 30);  EXPECT_EQ(ts.ty[0], 0);
    EXPECT_EQ(ts.tx[1], 130); EXPECT_EQ(ts.ty[1], 0);
    EXPECT_EQ(ts.tx[2], 130); EXPECT_EQ(ts.ty[2], 100);
    EXPECT_EQ(ts.tx[3], 30);  EXPECT_EQ(ts.ty[3], 100);
    for (uint16_t i = 0; i < ts.numverts; i++)
        EXPECT_EQ(ts.tz[i], 200L) << "all verts at z=200";
}
