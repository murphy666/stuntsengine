/*
 * test_header_centralization.cc
 *
 * Guards the new single-header surface for shape2d/shape3d and umbrella include.
 */

#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <type_traits>

extern "C" {
#include "stunts.h"
}

static int expected_shape3d_cull1_offset(unsigned verts)
{
    return SHAPE3D_HEADER_SIZE_BYTES + (int)verts * SHAPE3D_VERTEX_SIZE_BYTES;
}

static int expected_shape3d_cull2_offset(unsigned verts, unsigned prims)
{
    return SHAPE3D_HEADER_SIZE_BYTES +
           (int)verts * SHAPE3D_VERTEX_SIZE_BYTES +
           (int)prims * SHAPE3D_CULL_ENTRY_SIZE_BYTES;
}

static int expected_shape3d_prims_offset(unsigned verts, unsigned prims)
{
    return SHAPE3D_HEADER_SIZE_BYTES +
           (int)verts * SHAPE3D_VERTEX_SIZE_BYTES +
           (int)prims * SHAPE3D_PRIMITIVE_SIZE_BYTES;
}

TEST(HeaderCentralization, UmbrellaCompilesAndExposesCoreTypes)
{
    EXPECT_EQ(sizeof(struct SHAPE2D), (size_t)16);
    EXPECT_EQ(sizeof(struct SHAPE3DHEADER), (size_t)4);
    EXPECT_EQ(sizeof(struct VECTOR), (size_t)6);
    EXPECT_EQ(offsetof(struct SHAPE2D, s2d_width), (size_t)0);
    EXPECT_EQ(offsetof(struct SHAPE3DHEADER, header_numverts), (size_t)0);
}

TEST(HeaderCentralization, Shape3DLayoutConstantsStable)
{
    EXPECT_EQ(SHAPE3D_HEADER_SIZE_BYTES, 4);
    EXPECT_EQ(SHAPE3D_VERTEX_SIZE_BYTES, 6);
    EXPECT_EQ(SHAPE3D_CULL_ENTRY_SIZE_BYTES, 4);
    EXPECT_EQ(SHAPE3D_PRIMITIVE_SIZE_BYTES, 8);
}

TEST(HeaderCentralization, Shape3DHeaderFieldOffsetsStable)
{
    EXPECT_EQ(offsetof(struct SHAPE3DHEADER, header_numverts), (size_t)0);
    EXPECT_EQ(offsetof(struct SHAPE3DHEADER, header_numprimitives), (size_t)1);
    EXPECT_EQ(offsetof(struct SHAPE3DHEADER, header_numpaints), (size_t)2);
    EXPECT_EQ(offsetof(struct SHAPE3DHEADER, header_reserved), (size_t)3);
}

TEST(HeaderCentralization, Shape3DInlineHelpersAccessibleFromSingleHeader)
{
    char raw[128];
    struct SHAPE3D out;

    memset(raw, 0, sizeof(raw));
    memset(&out, 0, sizeof(out));

    raw[0] = 4; /* numverts */
    raw[1] = 2; /* numprimitives */
    raw[2] = 1; /* numpaints */

    shape3d_init_shape_pure(raw, &out);

    EXPECT_EQ(out.shape3d_numverts, 4u);
    EXPECT_EQ(out.shape3d_numprimitives, 2u);
    EXPECT_EQ(out.shape3d_numpaints, 1u);
    EXPECT_EQ((char*)out.shape3d_verts - raw, 4);
    EXPECT_EQ(out.shape3d_cull1 - raw, expected_shape3d_cull1_offset(4));
    EXPECT_EQ(out.shape3d_cull2 - raw, expected_shape3d_cull2_offset(4, 2));
    EXPECT_EQ(out.shape3d_primitives - raw, expected_shape3d_prims_offset(4, 2));

    EXPECT_EQ(shape3d_project_radius((uint16_t)120, 40u, 20), 240u);
    EXPECT_EQ(shape3d_project_radius((uint16_t)120, 40u, 0), 0u);
}

TEST(HeaderCentralization, Shape3DInitPureZeroCounts)
{
    char raw[32];
    struct SHAPE3D out;

    memset(raw, 0, sizeof(raw));
    memset(&out, 0, sizeof(out));

    shape3d_init_shape_pure(raw, &out);

    EXPECT_EQ(out.shape3d_numverts, 0u);
    EXPECT_EQ(out.shape3d_numprimitives, 0u);
    EXPECT_EQ((char*)out.shape3d_verts - raw, SHAPE3D_HEADER_SIZE_BYTES);
    EXPECT_EQ(out.shape3d_cull1 - raw, SHAPE3D_HEADER_SIZE_BYTES);
    EXPECT_EQ(out.shape3d_cull2 - raw, SHAPE3D_HEADER_SIZE_BYTES);
    EXPECT_EQ(out.shape3d_primitives - raw, SHAPE3D_HEADER_SIZE_BYTES);
}

TEST(HeaderCentralization, Shape3DInitPureOffsetMatrix)
{
    char raw[1024];
    struct SHAPE3D out;

    for (unsigned verts = 0; verts <= 20; ++verts) {
        for (unsigned prims = 0; prims <= 20; ++prims) {
            memset(raw, 0, sizeof(raw));
            memset(&out, 0, sizeof(out));

            raw[0] = (char)verts;
            raw[1] = (char)prims;
            raw[2] = 3;

            shape3d_init_shape_pure(raw, &out);

            EXPECT_EQ((char*)out.shape3d_verts - raw, SHAPE3D_HEADER_SIZE_BYTES);
            EXPECT_EQ(out.shape3d_cull1 - raw, expected_shape3d_cull1_offset(verts));
            EXPECT_EQ(out.shape3d_cull2 - raw, expected_shape3d_cull2_offset(verts, prims));
            EXPECT_EQ(out.shape3d_primitives - raw, expected_shape3d_prims_offset(verts, prims));
        }
    }
}

TEST(HeaderCentralization, Shape3DInitPureByteUpperBound)
{
    char raw[4096];
    struct SHAPE3D out;

    memset(raw, 0, sizeof(raw));
    memset(&out, 0, sizeof(out));

    raw[0] = (char)255;
    raw[1] = (char)255;
    raw[2] = (char)255;

    shape3d_init_shape_pure(raw, &out);

    EXPECT_EQ(out.shape3d_numverts, 255u);
    EXPECT_EQ(out.shape3d_numprimitives, 255u);
    EXPECT_EQ(out.shape3d_numpaints, 255u);
    EXPECT_EQ(out.shape3d_cull1 - raw, expected_shape3d_cull1_offset(255));
    EXPECT_EQ(out.shape3d_cull2 - raw, expected_shape3d_cull2_offset(255, 255));
    EXPECT_EQ(out.shape3d_primitives - raw, expected_shape3d_prims_offset(255, 255));
}

TEST(HeaderCentralization, Shape3DProjectRadiusDepthGuards)
{
    EXPECT_EQ(shape3d_project_radius((uint16_t)300, 70u, 0), 0u);
    EXPECT_EQ(shape3d_project_radius((uint16_t)300, 70u, -1), 0u);
    EXPECT_EQ(shape3d_project_radius((uint16_t)300, 70u, -300), 0u);
}

TEST(HeaderCentralization, Shape3DProjectRadiusTruncationAndScale)
{
    EXPECT_EQ(shape3d_project_radius((uint16_t)10,  3u, 2), 15u);
    EXPECT_EQ(shape3d_project_radius((uint16_t)10,  3u, 4), 7u);
    EXPECT_EQ(shape3d_project_radius((uint16_t)11,  5u, 3), 18u);
    EXPECT_EQ(shape3d_project_radius((uint16_t)4096, 2u, 3), 2730u);
}

TEST(HeaderCentralization, Shape3DProjectRadiusLargeOperandSanity)
{
    EXPECT_EQ(shape3d_project_radius((uint16_t)65535, 65535u, 65535), 65535u);
    EXPECT_EQ(shape3d_project_radius((uint16_t)65535, 65535u, 32767), 131072u);
}

TEST(HeaderCentralization, Shape2DSurfaceAvailableFromSingleHeader)
{
    /* Strict signature checks: catches accidental API drift in shape2d.h. */
    EXPECT_TRUE((std::is_same<decltype(&sprite_hittest_point),
        short (*)(unsigned short, unsigned short, short,
                  const unsigned short*, const unsigned short*,
                  const unsigned short*, const unsigned short*)>::value));

    EXPECT_TRUE((std::is_same<decltype(&shape2d_resource_shape_count),
        unsigned short (*)(const unsigned char*)>::value));

    EXPECT_TRUE((std::is_same<decltype(&shape2d_resource_get_shape),
        struct SHAPE2D* (*)(unsigned char*, int)>::value));

    EXPECT_TRUE((std::is_same<decltype(&sprite_compute_clear_plan),
        int (*)(const struct SPRITE*, const unsigned int*, unsigned int*, int*, int*, int*)>::value));

    EXPECT_TRUE((std::is_same<decltype(&sprite_execute_clear_plan),
        void (*)(unsigned char*, unsigned int, int, int, int, unsigned char)>::value));

    EXPECT_TRUE((std::is_same<decltype(&sprite_phase_add_wrap),
        unsigned short (*)(unsigned short, unsigned short, unsigned short)>::value));

    EXPECT_TRUE((std::is_same<decltype(&sprite_pick_blink_frame),
        unsigned short (*)(unsigned short, unsigned short, unsigned short, unsigned short)>::value));

    EXPECT_TRUE((std::is_same<decltype(&shape2d_lineofs_flat),
        unsigned int* (*)(unsigned int*)>::value));
}

TEST(HeaderCentralization, UmbrellaExposesCrossSubsystemSymbols)
{
    /* Cross-header signature checks through stunts.h umbrella include. */
    EXPECT_TRUE((std::is_same<decltype(&set_projection),
        void (*)(int, int, int, int)>::value));
    EXPECT_TRUE((std::is_same<decltype(&init_rect_arrays),
        void (*)(void)>::value));
    EXPECT_TRUE((std::is_same<decltype(&load_skybox),
        void (*)(unsigned char)>::value));
    EXPECT_TRUE((std::is_same<decltype(&unload_skybox),
        void (*)(void)>::value));
    EXPECT_TRUE((std::is_same<decltype(&render_present_ingame_view),
        void (*)(struct RECTANGLE*)>::value));
    EXPECT_TRUE((std::is_same<decltype(&load_intro_resources),
        unsigned short (*)(void)>::value));
    EXPECT_TRUE((std::is_same<decltype(&run_tracks_menu),
        void (*)(unsigned short)>::value));
    EXPECT_TRUE((std::is_same<decltype(&update_gamestate),
        void (*)(void)>::value));
    EXPECT_TRUE((std::is_same<decltype(&update_crash_state),
        void (*)(int, int)>::value));
    EXPECT_TRUE((std::is_same<decltype(&audio_load_driver),
        short (*)(const char*, short, short)>::value));
    EXPECT_TRUE((std::is_same<decltype(&audio_unload),
        void (*)(void)>::value));
}

TEST(HeaderCentralization, LegacySplitHeadersRemainRemoved)
{
    namespace fs = std::filesystem;
    const fs::path test_file = fs::path(__FILE__);
    const fs::path repo_root = test_file.parent_path().parent_path();
    const fs::path include_dir = repo_root / "include";

    EXPECT_FALSE(fs::exists(include_dir / "shape2d_anim.h"));
    EXPECT_FALSE(fs::exists(include_dir / "shape2d_hit.h"));
    EXPECT_FALSE(fs::exists(include_dir / "shape2d_res.h"));
    EXPECT_FALSE(fs::exists(include_dir / "shape2d_sprite_plan.h"));
    EXPECT_FALSE(fs::exists(include_dir / "shape3d_init.h"));
    EXPECT_FALSE(fs::exists(include_dir / "shape3d_proj.h"));
}
