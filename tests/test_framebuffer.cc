/*
 * test_framebuffer.cc — Google Test coverage for src/framebuffer.c
 *
 * Tests the pure framebuffer memory functions (fb_*); these operate on the
 * VGA-layout pixel/palette buffers.
 * The SDL2 window/renderer functions (fb_sdl2_*) require a display and
 * are not exercised here.
 *
 * Requires SDL2 headers at compile time because framebuffer.c includes
 * <SDL2/SDL.h>.  No window is opened; SDL is never initialised.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>
#include <vector>

extern "C" {
#include "framebuffer.h"
}

// ============================================================
// fb_init
// ============================================================

TEST(Framebuffer, InitZerosPixels) {
    Framebuffer fb;
    // poison the buffer first
    memset(&fb, 255, sizeof(fb));
    fb_init(&fb);

    for (int i = 0; i < FB_PIXELS; ++i) {
        ASSERT_EQ(fb.pixels[i], 0u) << "pixel " << i << " should be 0 after init";
    }
}

TEST(Framebuffer, InitSetsGrayscaleRamp) {
    Framebuffer fb;
    memset(&fb, 255, sizeof(fb));
    fb_init(&fb);

    // fb_init sets palette[i] = (i & 63) for all three channels
    for (int i = 0; i < 256; ++i) {
        uint8_t expected = (uint8_t)(i & 63);
        ASSERT_EQ(fb.palette[i][0], expected) << "palette[" << i << "].r";
        ASSERT_EQ(fb.palette[i][1], expected) << "palette[" << i << "].g";
        ASSERT_EQ(fb.palette[i][2], expected) << "palette[" << i << "].b";
    }
}

TEST(Framebuffer, Dimensions) {
    EXPECT_EQ(FB_WIDTH,  320);
    EXPECT_EQ(FB_HEIGHT, 200);
    EXPECT_EQ(FB_PIXELS, 320 * 200);
}

// ============================================================
// fb_clear
// ============================================================

TEST(Framebuffer, ClearFillsAllPixels) {
    Framebuffer fb;
    fb_init(&fb);
    fb_clear(&fb, 7);

    for (int i = 0; i < FB_PIXELS; ++i) {
        ASSERT_EQ(fb.pixels[i], 7u) << "pixel " << i << " not filled";
    }
}

TEST(Framebuffer, ClearToZero) {
    Framebuffer fb;
    fb_init(&fb);
    fb_clear(&fb, 5);
    fb_clear(&fb, 0);

    for (int i = 0; i < FB_PIXELS; ++i) {
        ASSERT_EQ(fb.pixels[i], 0u) << "pixel " << i << " not cleared";
    }
}

TEST(Framebuffer, ClearColor255) {
    Framebuffer fb;
    fb_init(&fb);
    fb_clear(&fb, 255);
    EXPECT_EQ(fb.pixels[0], 255u);
    EXPECT_EQ(fb.pixels[FB_PIXELS - 1], 255u);
}

// ============================================================
// fb_set_pixel / fb_get_pixel
// ============================================================

TEST(Framebuffer, SetGetPixelCorners) {
    Framebuffer fb;
    fb_init(&fb);

    fb_set_pixel(&fb, 0, 0, 1);
    EXPECT_EQ(fb_get_pixel(&fb, 0, 0), 1u) << "top-left";

    fb_set_pixel(&fb, FB_WIDTH - 1, 0, 2);
    EXPECT_EQ(fb_get_pixel(&fb, FB_WIDTH - 1, 0), 2u) << "top-right";

    fb_set_pixel(&fb, 0, FB_HEIGHT - 1, 3);
    EXPECT_EQ(fb_get_pixel(&fb, 0, FB_HEIGHT - 1), 3u) << "bottom-left";

    fb_set_pixel(&fb, FB_WIDTH - 1, FB_HEIGHT - 1, 4);
    EXPECT_EQ(fb_get_pixel(&fb, FB_WIDTH - 1, FB_HEIGHT - 1), 4u) << "bottom-right";
}

TEST(Framebuffer, SetGetPixelCenter) {
    Framebuffer fb;
    fb_init(&fb);

    fb_set_pixel(&fb, 160, 100, 42);
    EXPECT_EQ(fb_get_pixel(&fb, 160, 100), 42u) << "center pixel";
}

TEST(Framebuffer, PixelRowStrideIsWidth) {
    Framebuffer fb;
    fb_init(&fb);

    fb_set_pixel(&fb, 0, 0, 10);
    fb_set_pixel(&fb, 0, 1, 20);

    // Row 0 col 0 is at index 0, row 1 col 0 is at index 320
    EXPECT_EQ(fb.pixels[0],          10u);
    EXPECT_EQ(fb.pixels[FB_WIDTH], 20u);
}

TEST(Framebuffer, OutOfBoundsDoesNotCrash) {
    Framebuffer fb;
    fb_init(&fb);
    // Negative or out-of-range coords should not crash (implementation clamps/ignores)
    fb_set_pixel(&fb, -1, 0, 9);
    fb_set_pixel(&fb,  0, -1, 9);
    fb_set_pixel(&fb, FB_WIDTH,  0, 9);
    fb_set_pixel(&fb, 0, FB_HEIGHT, 9);
    // get_pixel out of bounds should return 0
    EXPECT_EQ(fb_get_pixel(&fb, -1, 0),           0u);
    EXPECT_EQ(fb_get_pixel(&fb, FB_WIDTH, 0), 0u);
}

// ============================================================
// fb_set_palette_entry / fb_palette_index_to_rgba
// ============================================================

TEST(Framebuffer, PaletteEntryStoredRaw) {
    Framebuffer fb;
    fb_init(&fb);

    // Set entry 1 to pure red in VGA 6-bit (63, 0, 0)
    fb_set_palette_entry(&fb, 1, 63, 0, 0);
    EXPECT_EQ(fb.palette[1][0], 63u);
    EXPECT_EQ(fb.palette[1][1],  0u);
    EXPECT_EQ(fb.palette[1][2],  0u);
}

TEST(Framebuffer, PaletteIndexToRgba_Black) {
    Framebuffer fb;
    fb_init(&fb);
    fb_set_palette_entry(&fb, 0, 0, 0, 0);

    uint32_t rgba = fb_palette_index_to_rgba(&fb, 0);
    uint8_t a = (rgba >> 24) & 255;
    uint8_t r = (rgba >> 16) & 255;
    uint8_t g = (rgba >>  8) & 255;
    uint8_t b = (rgba      ) & 255;

    EXPECT_EQ(r, 0u) << "black R";
    EXPECT_EQ(g, 0u) << "black G";
    EXPECT_EQ(b, 0u) << "black B";
    EXPECT_EQ(a, 255u) << "alpha must be 255";
}

TEST(Framebuffer, PaletteIndexToRgba_White) {
    Framebuffer fb;
    fb_init(&fb);
    // VGA 63,63,63 → 8-bit: (63<<2)|(63>>4) = 252|3 = 255
    fb_set_palette_entry(&fb, 15, 63, 63, 63);

    uint32_t rgba = fb_palette_index_to_rgba(&fb, 15);
    uint8_t r = (rgba >> 16) & 255;
    uint8_t g = (rgba >>  8) & 255;
    uint8_t b = (rgba      ) & 255;

    EXPECT_EQ(r, 255u) << "white R";
    EXPECT_EQ(g, 255u) << "white G";
    EXPECT_EQ(b, 255u) << "white B";
}

TEST(Framebuffer, PaletteIndexToRgba_PureRed) {
    Framebuffer fb;
    fb_init(&fb);
    fb_set_palette_entry(&fb, 4, 63, 0, 0);

    uint32_t rgba = fb_palette_index_to_rgba(&fb, 4);
    uint8_t r = (rgba >> 16) & 255;
    uint8_t g = (rgba >>  8) & 255;
    uint8_t b = (rgba      ) & 255;

    EXPECT_EQ(r, 255u) << "red R";
    EXPECT_EQ(g,   0u) << "red G";
    EXPECT_EQ(b,   0u) << "red B";
}

// ============================================================
// fb_to_rgba
// ============================================================

TEST(Framebuffer, ToRgbaAllBlack) {
    Framebuffer fb;
    fb_init(&fb);
    fb_set_palette_entry(&fb, 0, 0, 0, 0);
    // pixels already all 0 after init

    std::vector<uint32_t> out(FB_PIXELS, 3735928559u);
    fb_to_rgba(&fb, out.data(), out.size());

    for (int i = 0; i < FB_PIXELS; ++i) {
        uint8_t r = (out[i] >> 16) & 255;
        uint8_t g = (out[i] >>  8) & 255;
        uint8_t b = (out[i]      ) & 255;
        ASSERT_EQ(r, 0u) << "pixel " << i;
        ASSERT_EQ(g, 0u) << "pixel " << i;
        ASSERT_EQ(b, 0u) << "pixel " << i;
    }
}

TEST(Framebuffer, ToRgbaPicksCorrectPaletteEntry) {
    Framebuffer fb;
    fb_init(&fb);
    fb_set_palette_entry(&fb, 0, 0, 0, 0);      // black at index 0
    fb_set_palette_entry(&fb, 2, 0, 63, 0);     // green at index 2

    fb_set_pixel(&fb, 10, 5, 2);   // one green pixel

    std::vector<uint32_t> out(FB_PIXELS, 0u);
    fb_to_rgba(&fb, out.data(), out.size());

    const uint32_t green_px = out[5 * FB_WIDTH + 10];
    uint8_t r = (green_px >> 16) & 255;
    uint8_t g = (green_px >>  8) & 255;
    uint8_t b = (green_px      ) & 255;
    EXPECT_EQ(r,   0u) << "green pixel R";
    EXPECT_EQ(g, 255u) << "green pixel G";
    EXPECT_EQ(b,   0u) << "green pixel B";
}

TEST(Framebuffer, ToRgbaPartialBuffer) {
    Framebuffer fb;
    fb_init(&fb);
    fb_set_palette_entry(&fb, 1, 32, 0, 0);
    fb_clear(&fb, 1);

    // Only request first 160 pixels (first half of row 0)
    std::vector<uint32_t> out(160, 0u);
    fb_to_rgba(&fb, out.data(), out.size());

    for (int i = 0; i < 160; ++i) {
        ASSERT_NE(out[i], 0u) << "pixel " << i << " should be non-black";
    }
}

// ============================================================
// Null-pointer safety
// ============================================================

TEST(Framebuffer, NullFbOpsDoNotCrash) {
    fb_clear(nullptr, 0);
    fb_set_pixel(nullptr, 0, 0, 1);
    EXPECT_EQ(fb_get_pixel(nullptr, 0, 0), 0u);
    EXPECT_EQ(fb_palette_index_to_rgba(nullptr, 0), 4278190080u);
    fb_to_rgba(nullptr, nullptr, 0);
}

TEST(Framebuffer, ToRgbaNullOutput) {
    Framebuffer fb;
    fb_init(&fb);
    fb_to_rgba(&fb, nullptr, FB_PIXELS);  // should not crash
}

TEST(Framebuffer, ToRgbaZeroLen) {
    Framebuffer fb;
    fb_init(&fb);
    std::vector<uint32_t> sentinel(4, 3735928559u);
    fb_to_rgba(&fb, sentinel.data(), 0);  // should write nothing
    for (auto v : sentinel)
        EXPECT_EQ(v, 3735928559u) << "sentinel overwritten";
}

// ============================================================
// Out-of-bounds pixel access
// ============================================================

TEST(Framebuffer, SetPixelOobDoesNotWrite) {
    Framebuffer fb;
    fb_init(&fb);
    fb_clear(&fb, 0);
    fb_set_pixel(&fb, -1,          0, 99);  // x negative
    fb_set_pixel(&fb,  0,         -1, 99);  // y negative
    fb_set_pixel(&fb, FB_WIDTH,    0, 99);  // x == width
    fb_set_pixel(&fb,  0, FB_HEIGHT, 99);   // y == height
    // No pixel should have been written; sample corners
    EXPECT_EQ(fb_get_pixel(&fb, 0,          0),           0u);
    EXPECT_EQ(fb_get_pixel(&fb, FB_WIDTH-1, 0),           0u);
    EXPECT_EQ(fb_get_pixel(&fb, 0,          FB_HEIGHT-1), 0u);
}

TEST(Framebuffer, GetPixelOobReturnsZero) {
    Framebuffer fb;
    fb_init(&fb);
    fb_clear(&fb, 7);
    EXPECT_EQ(fb_get_pixel(&fb, -1,       0),         0u) << "x<0";
    EXPECT_EQ(fb_get_pixel(&fb, FB_WIDTH, 0),         0u) << "x==width";
    EXPECT_EQ(fb_get_pixel(&fb, 0,       -1),         0u) << "y<0";
    EXPECT_EQ(fb_get_pixel(&fb, 0, FB_HEIGHT),        0u) << "y==height";
}

// ============================================================
// Palette masking and alpha channel
// ============================================================

TEST(Framebuffer, PaletteEntryClampsAbove63) {
    Framebuffer fb;
    fb_init(&fb);
    // Values >63 must be masked to 6 bits
    fb_set_palette_entry(&fb, 10, 255, 255, 255);
    uint32_t rgba = fb_palette_index_to_rgba(&fb, 10);
    uint8_t r = (rgba >> 16) & 255;
    uint8_t g = (rgba >>  8) & 255;
    uint8_t b = (rgba      ) & 255;
    // 255 & 63 = 63 = 63 → expanded to 255
    EXPECT_EQ(r, 255u) << "high-byte R clamped to 63 then expanded";
    EXPECT_EQ(g, 255u) << "high-byte G clamped";
    EXPECT_EQ(b, 255u) << "high-byte B clamped";
}

TEST(Framebuffer, AlphaChannelAlwaysOpaque) {
    Framebuffer fb;
    fb_init(&fb);
    for (int i = 0; i < 3; ++i) {
        fb_set_palette_entry(&fb, (uint8_t)i, (uint8_t)(i * 10), 0, 0);
        uint32_t rgba = fb_palette_index_to_rgba(&fb, (uint8_t)i);
        EXPECT_EQ((rgba >> 24) & 255, 255u) << "alpha opaque for index " << i;
    }
}

// ============================================================
// Multiple clear overwrites
// ============================================================

TEST(Framebuffer, MultipleClearsOverwrite) {
    Framebuffer fb;
    fb_init(&fb);
    fb_clear(&fb, 3);
    for (int i = 0; i < FB_PIXELS; ++i)
        ASSERT_EQ(fb.pixels[i], 3u) << "after first clear, pixel " << i;
    fb_clear(&fb, 7);
    for (int i = 0; i < FB_PIXELS; ++i)
        ASSERT_EQ(fb.pixels[i], 7u) << "after second clear, pixel " << i;
}

TEST(Framebuffer, ClearThenWriteSurvives) {
    Framebuffer fb;
    fb_init(&fb);
    fb_clear(&fb, 0);
    fb_set_pixel(&fb, 100, 50, 42);
    fb_clear(&fb, 5);                   // second clear wipes everything
    EXPECT_EQ(fb_get_pixel(&fb, 100, 50), 5u) << "clear overwrites written pixel";
}
