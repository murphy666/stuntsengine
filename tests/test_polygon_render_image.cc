/*
 * test_polygon_render_image.cc — GTest version of test_polygon_render_image.c
 *
 * Writes test_outputs/polygon_render.ppm relative to CMAKE_BINARY_DIR.
 */

#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cmath>

/* -----------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------- */
constexpr int FB_W = 200, FB_H = 150;

struct Point2D { int x, y; };

static uint8_t framebuffer[FB_H][FB_W];

static void fb_clear() { memset(framebuffer, 0, sizeof framebuffer); }

static void set_pixel(int x, int y, uint8_t c) {
    if (x >= 0 && x < FB_W && y >= 0 && y < FB_H) framebuffer[y][x] = c;
}

/* Rasterise a filled triangle (top-left fill convention) */
static void fill_triangle(Point2D a, Point2D b, Point2D c, uint8_t color) {
    int minx = a.x, miny = a.y, maxx = a.x, maxy = a.y;
    if (b.x < minx) minx = b.x; if (b.x > maxx) maxx = b.x;
    if (c.x < minx) minx = c.x; if (c.x > maxx) maxx = c.x;
    if (b.y < miny) miny = b.y; if (b.y > maxy) maxy = b.y;
    if (c.y < miny) miny = c.y; if (c.y > maxy) maxy = c.y;
    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            int d0 = (b.x-a.x)*(y-a.y) - (b.y-a.y)*(x-a.x);
            int d1 = (c.x-b.x)*(y-b.y) - (c.y-b.y)*(x-b.x);
            int d2 = (a.x-c.x)*(y-c.y) - (a.y-c.y)*(x-c.x);
            if ((d0>=0&&d1>=0&&d2>=0)||(d0<=0&&d1<=0&&d2<=0))
                set_pixel(x, y, color);
        }
    }
}

/* Filled convex quad via two triangles */
static void fill_convex_quad(Point2D a, Point2D b, Point2D c, Point2D d, uint8_t color) {
    fill_triangle(a, b, c, color);
    fill_triangle(a, c, d, color);
}

static int count_pixels(uint8_t color) {
    int n = 0;
    for (int y = 0; y < FB_H; y++)
        for (int x = 0; x < FB_W; x++)
            if (framebuffer[y][x] == color) n++;
    return n;
}

static bool save_ppm(const char* path) {
    FILE* fp = fopen(path, "wb");
    if (!fp) return false;
    fprintf(fp, "P6\n%d %d\n255\n", FB_W, FB_H);
    for (int y = 0; y < FB_H; y++) for (int x = 0; x < FB_W; x++) {
        uint8_t c = framebuffer[y][x];
        uint8_t r = (c == 15) ? 255 : (c == 14) ? 0   : 0;
        uint8_t g = (c == 15) ? 255 : (c == 14) ? 0   : 128;
        uint8_t b2= (c == 15) ? 255 : (c == 14) ? 255 : 0;
        fputc(r, fp); fputc(g, fp); fputc(b2, fp);
    }
    fclose(fp);
    return true;
}

/* -----------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */
TEST(PolygonRender, TriangleFill) {
    fb_clear();
    fill_triangle({50,50}, {150,50}, {100,120}, 15);
    EXPECT_NE(framebuffer[50][50],   0u) << "left vertex pixel set";
    EXPECT_NE(framebuffer[50][150],  0u) << "right vertex pixel set";
    EXPECT_NE(framebuffer[120][100], 0u) << "bottom vertex pixel set";
    EXPECT_EQ(framebuffer[0][0],     0u) << "outside pixel unset";
}

TEST(PolygonRender, QuadFill) {
    fb_clear();
    fill_convex_quad({10,10}, {90,10}, {90,90}, {10,90}, 14);
    EXPECT_NE(framebuffer[10][10],  0u) << "top-left set";
    EXPECT_NE(framebuffer[90][90],  0u) << "bottom-right set";
    EXPECT_NE(framebuffer[50][50],  0u) << "centre set";
    EXPECT_EQ(framebuffer[ 0][ 0],  0u) << "outside unset";
    EXPECT_EQ(framebuffer[99][99],  0u) << "outside unset";
}

TEST(PolygonRender, AreaCounts) {
    fb_clear();
    // 80×80 axis-aligned quad → 6400 pixels
    fill_convex_quad({10,10}, {89,10}, {89,89}, {10,89}, 15);
    int area = count_pixels(15);
    EXPECT_GE(area, 6300) << "area >= 6300 (80×80)";
    EXPECT_LE(area, 6500) << "area <= 6500 (80×80)";
}

TEST(PolygonRender, SavePPMFile) {
    fb_clear();
    fill_triangle({50,10}, {150,10}, {100,110}, 15);
    fill_convex_quad({20,80}, {80,80}, {80,140}, {20,140}, 14);
    const char* path = "test_outputs/polygon_render.ppm";
    bool ok = save_ppm(path);
    if (!ok) GTEST_SKIP() << "test_outputs/ not writable — skipping PPM write check";
    FILE* fp = fopen(path, "rb");
    ASSERT_NE(fp, nullptr) << "PPM file readable after write";
    char header[4] = {};
    ASSERT_EQ(fread(header, 1, 2, fp), (size_t)2);
    fclose(fp);
    EXPECT_EQ(header[0], 'P') << "PPM magic 'P'";
    EXPECT_EQ(header[1], '6') << "PPM magic '6'";
}
