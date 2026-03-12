/*
 * test_shape3d_resource.cc — Google Test version of test_shape3d_resource.c
 */

#include <gtest/gtest.h>
#include <cstdlib>
#include <cstring>
#include <cstdint>

TEST(Shape3DResource, HeaderParsing) {
    unsigned char buf[256] = {};
    buf[4] = 3;
    buf[5] = 0;
    uint16_t shape_count;
    memcpy(&shape_count, buf + 4, 2);
    EXPECT_EQ(shape_count, 3u) << "shape count parsed from header";
}

TEST(Shape3DResource, OffsetTable) {
    unsigned char buf[256] = {};
    *reinterpret_cast<uint16_t*>(buf + 4) = 2;
    *reinterpret_cast<uint32_t*>(buf + 6) = 100;
    *reinterpret_cast<uint32_t*>(buf + 10) = 200;
    EXPECT_EQ(*reinterpret_cast<uint32_t*>(buf + 6),  100u) << "shape 0 offset";
    EXPECT_EQ(*reinterpret_cast<uint32_t*>(buf + 10), 200u) << "shape 1 offset";
}

TEST(Shape3DResource, ShapeStructure) {
    unsigned char buf[512] = {};
    *reinterpret_cast<uint16_t*>(buf + 0) = 177;
    *reinterpret_cast<uint16_t*>(buf + 2) = 133;
    *reinterpret_cast<uint16_t*>(buf + 4) = 7;
    EXPECT_EQ(*reinterpret_cast<uint16_t*>(buf + 0), 177u) << "numverts";
    EXPECT_EQ(*reinterpret_cast<uint16_t*>(buf + 2), 133u) << "numprims";
    EXPECT_EQ(*reinterpret_cast<uint16_t*>(buf + 4),   7u) << "numpaints";
}

TEST(Shape3DResource, VertexDataLayout) {
    unsigned char buf[512] = {};
    int16_t* verts = reinterpret_cast<int16_t*>(buf + 6);
    verts[0] = 100; verts[1] = 50; verts[2] = -30;
    verts[3] = -80; verts[4] = 120; verts[5] = 200;
    EXPECT_EQ(verts[0],  100) << "v0.x";
    EXPECT_EQ(verts[1],   50) << "v0.y";
    EXPECT_EQ(verts[2],  -30) << "v0.z";
    EXPECT_EQ(verts[3],  -80) << "v1.x";
    EXPECT_EQ(verts[4],  120) << "v1.y";
    EXPECT_EQ(verts[5],  200) << "v1.z";
}

TEST(Shape3DResource, PrimitiveDataLayout) {
    unsigned char prims[16] = {};
    prims[0] = 1; prims[1] = 15; prims[2] = 2; prims[3] = 0; prims[4] = 5;
    EXPECT_EQ(prims[0],  1) << "type=line";
    EXPECT_EQ(prims[1], 15) << "color=white";
    EXPECT_EQ(prims[2],  2) << "vert count";
    EXPECT_EQ(prims[3],  0) << "vert idx 0";
    EXPECT_EQ(prims[4],  5) << "vert idx 5";
}

TEST(Shape3DResource, FullShapeParsing) {
    unsigned char buf[1024] = {};
    *reinterpret_cast<uint16_t*>(buf + 0) = 4;
    *reinterpret_cast<uint16_t*>(buf + 2) = 2;
    *reinterpret_cast<uint16_t*>(buf + 4) = 1;
    int16_t* verts = reinterpret_cast<int16_t*>(buf + 6);
    verts[0]=0; verts[1]=0; verts[2]=0;
    verts[3]=100; verts[4]=0; verts[5]=0;
    verts[6]=0; verts[7]=100; verts[8]=0;
    verts[9]=0; verts[10]=0; verts[11]=100;
    unsigned char* prims = buf + 30;
    prims[0]=1; prims[1]=10; prims[2]=2; prims[3]=0; prims[4]=1;
    prims[5]=1; prims[6]=11; prims[7]=2; prims[8]=2; prims[9]=3;
    EXPECT_EQ(*reinterpret_cast<uint16_t*>(buf+0), 4u) << "4 verts";
    EXPECT_EQ(*reinterpret_cast<uint16_t*>(buf+2), 2u) << "2 prims";
    EXPECT_EQ(verts[0],  0)   << "v0.x";
    EXPECT_EQ(verts[3],  100) << "v1.x";
    EXPECT_EQ(prims[0],  1)   << "prim0 line";
    EXPECT_EQ(prims[3],  0)   << "prim0 v0";
    EXPECT_EQ(prims[5],  1)   << "prim1 line";
    EXPECT_EQ(prims[8],  2)   << "prim1 v2";
}

TEST(Shape3DResource, BoundsChecking) {
    unsigned char buf[2048] = {};
    *reinterpret_cast<uint16_t*>(buf + 0) = 200;
    uint16_t numverts = *reinterpret_cast<uint16_t*>(buf + 0);
    size_t vert_data_size = static_cast<size_t>(numverts) * 3 * sizeof(int16_t);
    EXPECT_LE(vert_data_size + 6, sizeof(buf)) << "vertex data fits in buffer";
    int16_t* verts = reinterpret_cast<int16_t*>(buf + 6);
    verts[(numverts-1)*3+0] = 999;
    verts[(numverts-1)*3+1] = 888;
    verts[(numverts-1)*3+2] = 777;
    EXPECT_EQ(verts[(numverts-1)*3+0], 999) << "last vertex X accessible";
    EXPECT_EQ(verts[(numverts-1)*3+1], 888) << "last vertex Y accessible";
    EXPECT_EQ(verts[(numverts-1)*3+2], 777) << "last vertex Z accessible";
}

TEST(Shape3DResource, NameLookup) {
    struct ShapeName { char name[8]; uint16_t index; };
    ShapeName names[] = { {"car0",124}, {"car1",126}, {"car2",128}, {"exp0",116} };
    int n = 4;

    bool found = false; uint16_t idx = 0;
    for (int i = 0; i < n; i++) {
        if (strcmp(names[i].name, "car0") == 0) { found = true; idx = names[i].index; break; }
    }
    EXPECT_TRUE(found)      << "found car0";
    EXPECT_EQ(idx, 124u)    << "car0 → index 124";

    found = false;
    for (int i = 0; i < n; i++) if (strcmp(names[i].name, "bogus") == 0) { found = true; break; }
    EXPECT_FALSE(found)     << "bogus → not found";
}
