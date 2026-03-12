/*
 * test_terrain_track_frame_check.cc — GTest version
 *
 * Tests helper logic (classify_greenish, frame_looks_like_terrain_and_track)
 * with synthetic data; optionally reads a real PPM if available.
 */

#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

/* -----------------------------------------------------------------------
 * Helpers ported from test_terrain_track_frame_check.c
 * ---------------------------------------------------------------------- */
struct RgbPixel { uint8_t r, g, b; };
struct FrameStats {
    int width, height;
    long total_pixels;
    long greenish_count;
    long grayish_count;
    long dark_count;
    long other_count;
    double greenish_ratio;
    double grayish_ratio;
    double dark_ratio;
    int has_reasonable_size;
};

static bool classify_greenish(uint8_t r, uint8_t g, uint8_t b) {
    return (g > r + 20) && (g > b + 10) && (g > 40);
}

static bool classify_grayish(uint8_t r, uint8_t g, uint8_t b) {
    int avg    = (r + g + b) / 3;
    int dr = abs(r - avg), dg = abs(g - avg), db = abs(b - avg);
    return (dr < 25) && (dg < 25) && (db < 25) && (avg > 40) && (avg < 200);
}

static bool frame_looks_like_terrain_and_track(const FrameStats* f) {
    if (!f->has_reasonable_size)  return false;
    if (f->greenish_ratio < 0.05) return false;
    if (f->grayish_ratio  < 0.05) return false;
    if (f->greenish_ratio + f->grayish_ratio < 0.15) return false;
    return true;
}

static FrameStats make_stats(int w, int h, long green, long gray, long dark, long other) {
    FrameStats f = {};
    f.width             = w; f.height = h;
    f.total_pixels      = w * h;
    f.greenish_count    = green;
    f.grayish_count     = gray;
    f.dark_count        = dark;
    f.other_count       = other;
    f.has_reasonable_size = (w >= 100 && h >= 80 && w <= 2000 && h <= 2000);
    if (f.total_pixels > 0) {
        f.greenish_ratio = (double)green / f.total_pixels;
        f.grayish_ratio  = (double)gray  / f.total_pixels;
        f.dark_ratio     = (double)dark  / f.total_pixels;
    }
    return f;
}

/* -----------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */
TEST(TerrainCheck, ClassifyGreenish) {
    EXPECT_TRUE (classify_greenish( 50, 180,  60)) << "obvious green";
    EXPECT_TRUE (classify_greenish( 30, 100,  50)) << "muted green";
    EXPECT_FALSE(classify_greenish(200,  50,  50)) << "red";
    EXPECT_FALSE(classify_greenish( 50,  50, 200)) << "blue";
    EXPECT_FALSE(classify_greenish(150, 150, 150)) << "gray";
    EXPECT_FALSE(classify_greenish(  0,  10,   0)) << "dark nearly black";
}

TEST(TerrainCheck, ClassifyGrayish) {
    EXPECT_TRUE (classify_grayish(100, 100, 100)) << "neutral gray";
    EXPECT_TRUE (classify_grayish( 80,  90,  85)) << "slight off-gray";
    EXPECT_FALSE(classify_grayish(200,  50,  50)) << "red not gray";
    EXPECT_FALSE(classify_grayish(255, 255, 255)) << "white excluded";
}

TEST(TerrainCheck, FrameLooksLikeTerrainAndTrack) {
    const long TOTAL = 320L * 200L;
    // Good frame: 20% green + 20% gray
    FrameStats good = make_stats(320, 200, (long)(TOTAL*0.20), (long)(TOTAL*0.20), (long)(TOTAL*0.30), (long)(TOTAL*0.30));
    EXPECT_TRUE(frame_looks_like_terrain_and_track(&good)) << "good frame passes";

    // Missing green
    FrameStats no_green = make_stats(320, 200, 0, (long)(TOTAL*0.40), (long)(TOTAL*0.30), (long)(TOTAL*0.30));
    EXPECT_FALSE(frame_looks_like_terrain_and_track(&no_green)) << "no green fails";

    // Missing gray
    FrameStats no_gray = make_stats(320, 200, (long)(TOTAL*0.40), 0, (long)(TOTAL*0.30), (long)(TOTAL*0.30));
    EXPECT_FALSE(frame_looks_like_terrain_and_track(&no_gray)) << "no gray fails";

    // Size too small
    FrameStats too_small = make_stats(10, 10, 40, 40, 10, 10);
    EXPECT_FALSE(frame_looks_like_terrain_and_track(&too_small)) << "too small fails";
}

TEST(TerrainCheck, AnalyzeRealPPM) {
    // Only check if a polygon_render.ppm was produced by a sibling test
    const char* path = "test_outputs/polygon_render.ppm";
    FILE* fp = fopen(path, "rb");
    if (!fp) GTEST_SKIP() << path << " not found — skipping real PPM analysis";

    char magic[3] = {};
    if (fread(magic, 1, 2, fp) == 2 && magic[0] == 'P' && magic[1] == '6') {
        // Valid PPM signature — just verify it's readable
        SUCCEED() << "PPM file has valid P6 header";
    } else {
        ADD_FAILURE() << "PPM file missing P6 header";
    }
    fclose(fp);
}
