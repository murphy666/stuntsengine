/*
 * test_ui_keys.cc — Tests for UI key constants and convenience macros
 *
 * Pure header-only tests — no stubs or engine dependencies needed.
 */

#include <gtest/gtest.h>

extern "C" {
#include "ui_keys.h"
#include "ui_colors.h"
#include "ui_widgets.h"
}

/* ------------------------------------------------------------------ */
/* Key constant values                                                  */
/* ------------------------------------------------------------------ */

TEST(UIKeys, ArrowKeyValues) {
    EXPECT_EQ(UI_KEY_UP,    18432);
    EXPECT_EQ(UI_KEY_DOWN,  20480);
    EXPECT_EQ(UI_KEY_LEFT,  19200);
    EXPECT_EQ(UI_KEY_RIGHT, 19712);
}

TEST(UIKeys, ActionKeyValues) {
    EXPECT_EQ(UI_KEY_ENTER,   13);
    EXPECT_EQ(UI_KEY_ESCAPE,  27);
    EXPECT_EQ(UI_KEY_SPACE,   32);
    EXPECT_EQ(UI_KEY_TAB,      9);
    EXPECT_EQ(UI_KEY_BACKSPACE, 8);
}

TEST(UIKeys, ExtendedKeyValues) {
    EXPECT_EQ(UI_KEY_ENTER_EXT,  7181);
    EXPECT_EQ(UI_KEY_SPACE_EXT,  14624);
    EXPECT_EQ(UI_KEY_ESCAPE_EXT, 283);
}

TEST(UIKeys, SentinelValues) {
    EXPECT_EQ(UI_SEL_CANCEL,  255);
    EXPECT_EQ(UI_SEL_TIMEOUT, 254);
    /* Sentinel values must be distinct */
    EXPECT_NE(UI_SEL_CANCEL, UI_SEL_TIMEOUT);
}

/* ------------------------------------------------------------------ */
/* UI_IS_CONFIRM                                                       */
/* ------------------------------------------------------------------ */

TEST(UIKeys, IsConfirmAccepts) {
    EXPECT_TRUE(UI_IS_CONFIRM(UI_KEY_ENTER));
    EXPECT_TRUE(UI_IS_CONFIRM(UI_KEY_SPACE));
    EXPECT_TRUE(UI_IS_CONFIRM(UI_KEY_ENTER_EXT));
    EXPECT_TRUE(UI_IS_CONFIRM(UI_KEY_SPACE_EXT));
}

TEST(UIKeys, IsConfirmRejects) {
    EXPECT_FALSE(UI_IS_CONFIRM(0));
    EXPECT_FALSE(UI_IS_CONFIRM(UI_KEY_ESCAPE));
    EXPECT_FALSE(UI_IS_CONFIRM(UI_KEY_UP));
    EXPECT_FALSE(UI_IS_CONFIRM(UI_KEY_DOWN));
    EXPECT_FALSE(UI_IS_CONFIRM(UI_KEY_LEFT));
    EXPECT_FALSE(UI_IS_CONFIRM(UI_KEY_RIGHT));
    EXPECT_FALSE(UI_IS_CONFIRM(UI_KEY_TAB));
    EXPECT_FALSE(UI_IS_CONFIRM(42));
}

/* ------------------------------------------------------------------ */
/* UI_IS_CANCEL                                                        */
/* ------------------------------------------------------------------ */

TEST(UIKeys, IsCancelAccepts) {
    EXPECT_TRUE(UI_IS_CANCEL(UI_KEY_ESCAPE));
    EXPECT_TRUE(UI_IS_CANCEL(UI_KEY_ESCAPE_EXT));
}

TEST(UIKeys, IsCancelRejects) {
    EXPECT_FALSE(UI_IS_CANCEL(0));
    EXPECT_FALSE(UI_IS_CANCEL(UI_KEY_ENTER));
    EXPECT_FALSE(UI_IS_CANCEL(UI_KEY_SPACE));
    EXPECT_FALSE(UI_IS_CANCEL(UI_KEY_UP));
    EXPECT_FALSE(UI_IS_CANCEL(UI_KEY_DOWN));
    EXPECT_FALSE(UI_IS_CANCEL(1));
}

/* ------------------------------------------------------------------ */
/* UI_IS_NAV_PREV / UI_IS_NAV_NEXT                                     */
/* ------------------------------------------------------------------ */

TEST(UIKeys, IsNavPrevAccepts) {
    EXPECT_TRUE(UI_IS_NAV_PREV(UI_KEY_LEFT));
    EXPECT_TRUE(UI_IS_NAV_PREV(UI_KEY_UP));
}

TEST(UIKeys, IsNavPrevRejects) {
    EXPECT_FALSE(UI_IS_NAV_PREV(0));
    EXPECT_FALSE(UI_IS_NAV_PREV(UI_KEY_RIGHT));
    EXPECT_FALSE(UI_IS_NAV_PREV(UI_KEY_DOWN));
    EXPECT_FALSE(UI_IS_NAV_PREV(UI_KEY_ENTER));
    EXPECT_FALSE(UI_IS_NAV_PREV(UI_KEY_ESCAPE));
}

TEST(UIKeys, IsNavNextAccepts) {
    EXPECT_TRUE(UI_IS_NAV_NEXT(UI_KEY_RIGHT));
    EXPECT_TRUE(UI_IS_NAV_NEXT(UI_KEY_DOWN));
}

TEST(UIKeys, IsNavNextRejects) {
    EXPECT_FALSE(UI_IS_NAV_NEXT(0));
    EXPECT_FALSE(UI_IS_NAV_NEXT(UI_KEY_LEFT));
    EXPECT_FALSE(UI_IS_NAV_NEXT(UI_KEY_UP));
    EXPECT_FALSE(UI_IS_NAV_NEXT(UI_KEY_ENTER));
    EXPECT_FALSE(UI_IS_NAV_NEXT(UI_KEY_ESCAPE));
}

/* ------------------------------------------------------------------ */
/* Mutual exclusion between macro categories                           */
/* ------------------------------------------------------------------ */

TEST(UIKeys, ConfirmAndCancelAreMutuallyExclusive) {
    /* No key should be both confirm and cancel */
    unsigned short keys[] = {
        UI_KEY_ENTER, UI_KEY_SPACE, UI_KEY_ENTER_EXT, UI_KEY_SPACE_EXT,
        UI_KEY_ESCAPE, UI_KEY_ESCAPE_EXT,
        UI_KEY_UP, UI_KEY_DOWN, UI_KEY_LEFT, UI_KEY_RIGHT
    };
    for (auto k : keys) {
        EXPECT_FALSE(UI_IS_CONFIRM(k) && UI_IS_CANCEL(k))
            << "key " << k << " is both confirm and cancel";
    }
}

TEST(UIKeys, NavPrevAndNavNextAreMutuallyExclusive) {
    unsigned short keys[] = {
        UI_KEY_UP, UI_KEY_DOWN, UI_KEY_LEFT, UI_KEY_RIGHT,
        UI_KEY_ENTER, UI_KEY_ESCAPE, UI_KEY_SPACE
    };
    for (auto k : keys) {
        EXPECT_FALSE(UI_IS_NAV_PREV(k) && UI_IS_NAV_NEXT(k))
            << "key " << k << " is both nav_prev and nav_next";
    }
}

/* ------------------------------------------------------------------ */
/* Navigation mode constants                                           */
/* ------------------------------------------------------------------ */

TEST(UIKeys, NavModeConstants) {
    EXPECT_EQ(UI_NAV_HORIZONTAL,   0);
    EXPECT_EQ(UI_NAV_VERTICAL,     1);
    EXPECT_EQ(UI_NAV_BOTH,         2);
    EXPECT_EQ(UI_NAV_BOTH_LR_SWAP, 3);
    /* All distinct */
    EXPECT_NE(UI_NAV_HORIZONTAL, UI_NAV_VERTICAL);
    EXPECT_NE(UI_NAV_HORIZONTAL, UI_NAV_BOTH);
    EXPECT_NE(UI_NAV_HORIZONTAL, UI_NAV_BOTH_LR_SWAP);
    EXPECT_NE(UI_NAV_VERTICAL,   UI_NAV_BOTH);
    EXPECT_NE(UI_NAV_VERTICAL,   UI_NAV_BOTH_LR_SWAP);
    EXPECT_NE(UI_NAV_BOTH,       UI_NAV_BOTH_LR_SWAP);
}

/* ------------------------------------------------------------------ */
/* Color constants (from ui_colors.h, pulled in by ui_widgets.h)       */
/* ------------------------------------------------------------------ */

TEST(UIKeys, ScreenDimensionConstants) {
    /* Included via ui_keys.h -> independent but verify they're accessible */
    EXPECT_EQ(UI_SCREEN_W, 320);
    EXPECT_EQ(UI_SCREEN_H, 200);
}

TEST(UIKeys, DialogAutoPos) {
    EXPECT_EQ(UI_DIALOG_AUTO_POS, 65535);
}
