/**
 * test_string_format.cc — Tests for print_int_as_string_maybe and
 *                         format_frame_as_string
 *
 * Both functions are inlined here (from src/stunts.c) so the test binary
 * carries no external link-time dependencies.  A local stub provides the
 * `framespersec` global used by format_frame_as_string.
 */

#include <gtest/gtest.h>
#include <cstring>

/* ═══════════════════════════════════════════════════════════════════════════
 * Inline copies of the functions under test
 * ═══════════════════════════════════════════════════════════════════════════ */
namespace {

/* Stub for the framespersec global used by format_frame_as_string */
static unsigned short framespersec = 20;

/* ── stunts_itoa (dependency of print_int_as_string_maybe) ─────────────── */

static char* stunts_itoa(int value, char* str, int radix)
{
    unsigned int uvalue, digit;
    char tmp[34];
    int pos = 0, out = 0, negative = 0;

    if (str == nullptr)          return nullptr;
    if (radix < 2 || radix > 36) { str[0] = '\0'; return str; }

    if (value < 0 && radix == 10) {
        negative = 1;
        uvalue = static_cast<unsigned int>(-value);
    } else {
        uvalue = static_cast<unsigned int>(value);
    }
    do {
        digit = uvalue % static_cast<unsigned int>(radix);
        tmp[pos++] = static_cast<char>(digit < 10 ? '0' + digit
                                                   : 'a' + digit - 10);
        uvalue /= static_cast<unsigned int>(radix);
    } while (uvalue != 0);
    if (negative) str[out++] = '-';
    while (pos > 0) str[out++] = tmp[--pos];
    str[out] = '\0';
    return str;
}

/* ── print_int_as_string_maybe ──────────────────────────────────────────── */

static void print_int_as_string_maybe(char* dest, int value,
                                      int zeroPadFlag, int width)
{
    int len = 0, i;
    stunts_itoa(value, dest, 10);
    while (dest[len] != 0) len++;

    while (width != len) {
        if (width < len) {
            for (i = 0; i < len; i++) dest[i] = dest[i + 1];
            len--;
        } else {
            for (i = len; i >= 0; i--) dest[i + 1] = dest[i];
            dest[0] = ' ';
            len++;
        }
    }

    if (zeroPadFlag != 0)
        for (i = 0; dest[i] == ' '; i++) dest[i] = '0';
}

/* ── format_frame_as_string ─────────────────────────────────────────────── */

static void format_frame_as_string(char* dest, unsigned short frames,
                                   unsigned short showFractions)
{
    unsigned short framesPerMinute, minutes, seconds, remaining, fps;
    char tmp[24];

    fps = framespersec;
    if (fps == 0) fps = 20;

    framesPerMinute = static_cast<unsigned short>(60 * fps);
    minutes         = frames / framesPerMinute;
    remaining       = frames - static_cast<unsigned short>(minutes * framesPerMinute);
    seconds         = remaining / fps;
    remaining       = remaining - static_cast<unsigned short>(seconds * fps);

    print_int_as_string_maybe(tmp, minutes, 0, 2);
    strcpy(dest, tmp);
    strcat(dest, ":");
    print_int_as_string_maybe(tmp, seconds, 1, 2);
    strcat(dest, tmp);

    if (showFractions != 0) {
        unsigned short frac = static_cast<unsigned short>(
            (100 / fps) * remaining);
        strcat(dest, ".");
        print_int_as_string_maybe(tmp, frac, 1, 2);
        strcat(dest, tmp);
    }
}

} // namespace

/* ═══════════════════════════════════════════════════════════════════════════
 * print_int_as_string_maybe tests
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(PrintIntAsString, ZeroWidth1NoPad)
{
    char buf[32];
    print_int_as_string_maybe(buf, 0, 0, 1);
    EXPECT_STREQ("0", buf);
}

TEST(PrintIntAsString, ZeroWidth2NoPad)
{
    char buf[32];
    print_int_as_string_maybe(buf, 0, 0, 2);
    EXPECT_STREQ(" 0", buf);
}

TEST(PrintIntAsString, ZeroWidth2WithPad)
{
    char buf[32];
    print_int_as_string_maybe(buf, 0, 1, 2);
    EXPECT_STREQ("00", buf);
}

TEST(PrintIntAsString, ExactWidth)
{
    char buf[32];
    print_int_as_string_maybe(buf, 42, 0, 2);
    EXPECT_STREQ("42", buf);
}

TEST(PrintIntAsString, PadSpaceLeft)
{
    char buf[32];
    print_int_as_string_maybe(buf, 42, 0, 3);
    EXPECT_STREQ(" 42", buf);
}

TEST(PrintIntAsString, PadZeroLeft)
{
    char buf[32];
    print_int_as_string_maybe(buf, 42, 1, 3);
    EXPECT_STREQ("042", buf);
}

TEST(PrintIntAsString, SingleDigitZeroPad)
{
    char buf[32];
    print_int_as_string_maybe(buf, 5, 1, 2);
    EXPECT_STREQ("05", buf);
    print_int_as_string_maybe(buf, 9, 1, 2);
    EXPECT_STREQ("09", buf);
}

TEST(PrintIntAsString, DoubleDigitExact)
{
    char buf[32];
    print_int_as_string_maybe(buf, 10, 1, 2);
    EXPECT_STREQ("10", buf);
    print_int_as_string_maybe(buf, 59, 1, 2);
    EXPECT_STREQ("59", buf);
    print_int_as_string_maybe(buf, 99, 0, 2);
    EXPECT_STREQ("99", buf);
}

TEST(PrintIntAsString, TruncateFromLeft)
{
    /* "100" is 3 chars; width=2 forces a left-truncation to "00" */
    char buf[32];
    print_int_as_string_maybe(buf, 100, 0, 2);
    EXPECT_STREQ("00", buf);
}

TEST(PrintIntAsString, Width1SingleDigit)
{
    char buf[32];
    print_int_as_string_maybe(buf, 1, 1, 1);
    EXPECT_STREQ("1", buf);
    print_int_as_string_maybe(buf, 7, 0, 1);
    EXPECT_STREQ("7", buf);
}

TEST(PrintIntAsString, LargeValueWidthMatch)
{
    char buf[32];
    print_int_as_string_maybe(buf, 1234, 0, 4);
    EXPECT_STREQ("1234", buf);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * format_frame_as_string tests  (fps = 20, 1 second = 20 frames)
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Helper: set the stub FPS and format */
static std::string fmt(unsigned short frames, unsigned short frac = 0,
                       unsigned short fps = 20)
{
    framespersec = fps;
    char buf[32] = {};
    format_frame_as_string(buf, frames, frac);
    return buf;
}

TEST(FormatFrame, ZeroFrames)
{
    EXPECT_EQ(" 0:00", fmt(0));
}

TEST(FormatFrame, OneSecond)
{
    EXPECT_EQ(" 0:01", fmt(20));
}

TEST(FormatFrame, FullMinute)
{
    EXPECT_EQ(" 1:00", fmt(1200));
}

TEST(FormatFrame, TwoMinutes)
{
    EXPECT_EQ(" 2:00", fmt(2400));
}

TEST(FormatFrame, SixMinutes)
{
    EXPECT_EQ(" 6:00", fmt(7200));
}

TEST(FormatFrame, TenMinutesExact)
{
    /* 10 minutes is 12000 frames; minutes field is "10" (2 chars, no pad) */
    EXPECT_EQ("10:00", fmt(12000));
}

TEST(FormatFrame, Fifty9Seconds)
{
    /* 1199 frames = 59 full seconds (19 remaining) */
    EXPECT_EQ(" 0:59", fmt(1199));
}

TEST(FormatFrame, MinutesPlusSecs)
{
    /* 1 min 30 sec = 1200 + 600 = 1800 frames */
    EXPECT_EQ(" 1:30", fmt(1800));
}

TEST(FormatFrame, LargerTime)
{
    /* 52 min 30 sec = 52*1200 + 30*20 = 62400 + 600 = 63000 frames */
    EXPECT_EQ("52:30", fmt(63000));
}

TEST(FormatFrame, FpsZeroDefaultsto20)
{
    /* fps=0 should fall back to 20 */
    EXPECT_EQ(" 0:01", fmt(20, 0, 0));
}

/* ── with fractions ──────────────────────────────────────────────────────── */

TEST(FormatFrame, ZeroFramesFrac)
{
    EXPECT_EQ(" 0:00.00", fmt(0, 1));
}

TEST(FormatFrame, FiveFramesFrac)
{
    /* 5 remaining frames: fraction = (100/20)*5 = 25 */
    EXPECT_EQ(" 0:00.25", fmt(5, 1));
}

TEST(FormatFrame, TenFramesFrac)
{
    /* 10 remaining: (100/20)*10 = 50 */
    EXPECT_EQ(" 0:00.50", fmt(10, 1));
}

TEST(FormatFrame, OneFrameFrac)
{
    /* 1 remaining: (100/20)*1 = 5 → "05" */
    EXPECT_EQ(" 0:00.05", fmt(1, 1));
}

TEST(FormatFrame, NineteenFramesFrac)
{
    /* 19 remaining: (100/20)*19 = 95 */
    EXPECT_EQ(" 0:00.95", fmt(19, 1));
}

TEST(FormatFrame, FullSecondFracZero)
{
    /* 20 frames exactly = 1 second, 0 remaining → ".00" */
    EXPECT_EQ(" 0:01.00", fmt(20, 1));
}

TEST(FormatFrame, MixedMinSecFrac)
{
    /* 1200 + 40 + 5 = 1245 frames → 1min 02sec + 5 remaining = .25 */
    EXPECT_EQ(" 1:02.25", fmt(1245, 1));
}
