/*
 * test_font.cc — Google Test version of test_font.c
 *
 * Requires WORKING_DIRECTORY to be the project source root
 * so "ressources/FONTDEF.FNT" is reachable.
 */

#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

/* -----------------------------------------------------------------------
 * Helpers (ported verbatim from test_font.c)
 * ---------------------------------------------------------------------- */
static unsigned char* load_file(const char* path, size_t* out_size) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return nullptr;
    fseek(fp, 0, SEEK_END);
    size_t size = static_cast<size_t>(ftell(fp));
    fseek(fp, 0, SEEK_SET);
    unsigned char* buf = static_cast<unsigned char*>(malloc(size));
    if (!buf) { fclose(fp); return nullptr; }
    if (fread(buf, 1, size, fp) != size) { fclose(fp); free(buf); return nullptr; }
    fclose(fp);
    *out_size = size;
    return buf;
}

static uint16_t read_u16_le(const unsigned char* data, size_t offset) {
    return static_cast<uint16_t>(data[offset] | (data[offset + 1] << 8));
}

static unsigned short font_width_for_text(const unsigned char* data, size_t size, const char* text) {
    unsigned char proportional = data[12];
    unsigned short default_width = read_u16_le(data, 18);
    const uint16_t* char_table = reinterpret_cast<const uint16_t*>(data + 22);
    unsigned short width = 0;
    if (default_width == 0) default_width = 8;
    while (*text) {
        unsigned char ch = static_cast<unsigned char>(*text++);
        uint16_t glyph_offset = char_table[ch];
        unsigned short char_width = default_width;
        if (glyph_offset == 0 || glyph_offset >= size) continue;
        if (proportional) char_width = data[glyph_offset];
        width += char_width;
    }
    return width;
}

/* -----------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */
class FontTest : public ::testing::Test {
protected:
    unsigned char* data = nullptr;
    size_t size = 0;

    void SetUp() override {
        data = load_file("ressources/FONTDEF.FNT", &size);
        if (!data) GTEST_SKIP() << "ressources/FONTDEF.FNT not found (run from project root)";
    }
    void TearDown() override { free(data); }
};

TEST_F(FontTest, FileProperties) {
    ASSERT_NE(data, nullptr);
    EXPECT_GT(size, (size_t)512)              << "file size reasonable";
    EXPECT_GE(size, (size_t)(22 + 256u * 2u)) << "char table fits in header";
    EXPECT_NE(read_u16_le(data, 14), 0u)       << "font height non-zero";
    EXPECT_NE(read_u16_le(data, 18), 0u)       << "default width non-zero";
}

TEST_F(FontTest, GlyphValidation) {
    const uint16_t* char_table = reinterpret_cast<const uint16_t*>(data + 22);
    unsigned char proportional = data[12];
    unsigned short font_height = read_u16_le(data, 14);
    unsigned short default_width = read_u16_le(data, 18);
    if (font_height == 0) font_height = 8;
    if (default_width == 0) default_width = 8;

    for (unsigned char ch : { 'A', 'B', '0', ' ' }) {
        uint16_t glyph_offset = char_table[ch];
        EXPECT_NE(glyph_offset, 0u) << "glyph '" << (char)ch << "' offset non-zero";
        EXPECT_LT((size_t)glyph_offset, size) << "glyph '" << (char)ch << "' offset within file";

        unsigned short glyph_width;
        uint16_t data_start = glyph_offset;
        if (proportional) {
            glyph_width = data[glyph_offset];
            data_start++;
        } else {
            glyph_width = default_width;
        }
        EXPECT_GT(glyph_width, (unsigned short)0) << "glyph '" << (char)ch << "' width > 0";
        size_t bytes_per_row = (glyph_width + 7) / 8;
        size_t glyph_size = bytes_per_row * font_height;
        EXPECT_LE((size_t)data_start + glyph_size, size)
            << "glyph '" << (char)ch << "' data fits in file";
    }
}

TEST_F(FontTest, WidthCalculation) {
    unsigned short wa  = font_width_for_text(data, size, "A");
    unsigned short wb  = font_width_for_text(data, size, "B");
    unsigned short wab = font_width_for_text(data, size, "AB");
    EXPECT_GT(wa,  (unsigned short)0) << "width of 'A' > 0";
    EXPECT_GT(wb,  (unsigned short)0) << "width of 'B' > 0";
    EXPECT_EQ(wab, (unsigned short)(wa + wb)) << "width('AB') == width('A') + width('B')";
}
