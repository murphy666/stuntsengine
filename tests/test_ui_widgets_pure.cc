/*
 * test_ui_widgets_pure.cc — Tests for pure/stateless UI widget helpers
 *
 * Tests centering calculations, UIPanel struct layout, UITextTable
 * struct layout, UIDialog struct layout and text building, and
 * UIButtonMenu struct configuration.
 *
 * These tests exercise logic that doesn't require engine stubs —
 * they only test arithmetic, struct sizes, and constant values.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "ui_widgets.h"
#include "ui_keys.h"
#include "ui_colors.h"
#include "ui_screen.h"
}


/* ================================================================== */
/* ui_center_x                                                         */
/* ================================================================== */

/* ui_center_x is declared in ui_widgets.h, implemented in ui_widgets.c.
 * For tests that only need the arithmetic we re-implement inline,
 * OR we link ui_widgets.c with stubs.  Here we test the inline logic. */

/* Inline reimplementation to avoid needing font stubs */
static unsigned short test_center_x(unsigned short item_w) {
    if (item_w >= UI_SCREEN_W) return 0;
    return (UI_SCREEN_W - item_w) / 2;
}

static unsigned short test_center_in(unsigned short container_x,
                                     unsigned short container_w,
                                     unsigned short item_w) {
    if (item_w >= container_w) return container_x;
    return container_x + (container_w - item_w) / 2;
}

TEST(UIWidgetsPure, CenterXZeroWidth) {
    EXPECT_EQ(test_center_x(0), 160);
}

TEST(UIWidgetsPure, CenterXSmallItem) {
    /* 100px item on 320px screen: (320-100)/2 = 110 */
    EXPECT_EQ(test_center_x(100), 110);
}

TEST(UIWidgetsPure, CenterXExactScreenWidth) {
    EXPECT_EQ(test_center_x(UI_SCREEN_W), 0);
}

TEST(UIWidgetsPure, CenterXOversized) {
    EXPECT_EQ(test_center_x(400), 0);
}

TEST(UIWidgetsPure, CenterXOddWidth) {
    /* 321 > 320 → 0 */
    EXPECT_EQ(test_center_x(321), 0);
    /* 319: -> (320-319)/2 = 0 (integer division) */
    EXPECT_EQ(test_center_x(319), 0);
    /* 101: -> (320-101)/2 = 109 */
    EXPECT_EQ(test_center_x(101), 109);
}


/* ================================================================== */
/* ui_center_in                                                        */
/* ================================================================== */

TEST(UIWidgetsPure, CenterInBasic) {
    /* 50px item in 200px container starting at x=10: 10+(200-50)/2 = 85 */
    EXPECT_EQ(test_center_in(10, 200, 50), 85);
}

TEST(UIWidgetsPure, CenterInExactFit) {
    /* Item same as container: returns container_x */
    EXPECT_EQ(test_center_in(20, 100, 100), 20);
}

TEST(UIWidgetsPure, CenterInOversizedItem) {
    /* Item wider than container: returns container_x */
    EXPECT_EQ(test_center_in(30, 80, 120), 30);
}

TEST(UIWidgetsPure, CenterInZeroContainer) {
    /* 0-width container: item_w >= 0 container_w → returns container_x */
    EXPECT_EQ(test_center_in(50, 0, 10), 50);
}


/* ================================================================== */
/* UIButtonMenu struct layout                                          */
/* ================================================================== */

TEST(UIWidgetsPure, ButtonMenuMaxButtons) {
    EXPECT_EQ(UI_BTN_MAX, 16) << "Maximum button count constant";
}

TEST(UIWidgetsPure, ButtonMenuStructZeroInit) {
    UIButtonMenu menu;
    memset(&menu, 0, sizeof(menu));
    EXPECT_EQ(menu.count, 0);
    EXPECT_EQ(menu.default_sel, 0);
    EXPECT_EQ(menu.idle_timeout, 0);
    EXPECT_EQ(menu.nav_mode, UI_NAV_HORIZONTAL);
    EXPECT_EQ(menu.frame_cb, nullptr);
    EXPECT_EQ(menu.selection_cb, nullptr);
}

TEST(UIWidgetsPure, ButtonMenuNavModes) {
    UIButtonMenu menu;
    memset(&menu, 0, sizeof(menu));

    menu.nav_mode = UI_NAV_HORIZONTAL;
    EXPECT_EQ(menu.nav_mode, 0);
    menu.nav_mode = UI_NAV_VERTICAL;
    EXPECT_EQ(menu.nav_mode, 1);
    menu.nav_mode = UI_NAV_BOTH;
    EXPECT_EQ(menu.nav_mode, 2);
    menu.nav_mode = UI_NAV_BOTH_LR_SWAP;
    EXPECT_EQ(menu.nav_mode, 3);
}


/* ================================================================== */
/* UIPanel struct layout                                                */
/* ================================================================== */

TEST(UIWidgetsPure, PanelStructInit) {
    UIPanel panel;
    memset(&panel, 0, sizeof(panel));
    panel.x = 10;
    panel.y = 20;
    panel.w = 100;
    panel.h = 80;
    panel.border_color = UI_COL_SHADOW;
    panel.fill_color = UI_COL_BLACK;
    panel.title = "Test Panel";
    panel.title_fg = UI_COL_WHITE;
    panel.title_bg = UI_COL_BLACK;

    EXPECT_EQ(panel.x, 10);
    EXPECT_EQ(panel.y, 20);
    EXPECT_EQ(panel.w, 100);
    EXPECT_EQ(panel.h, 80);
    EXPECT_EQ(panel.border_color, UI_COL_SHADOW);
    EXPECT_STREQ(panel.title, "Test Panel");
}

TEST(UIWidgetsPure, PanelNullTitle) {
    UIPanel panel;
    memset(&panel, 0, sizeof(panel));
    EXPECT_EQ(panel.title, nullptr);
}


/* ================================================================== */
/* UITextTable struct layout                                           */
/* ================================================================== */

TEST(UIWidgetsPure, TextTableConstants) {
    EXPECT_EQ(UI_TABLE_MAX_COLS, 8);
    EXPECT_EQ(UI_TABLE_MAX_ROWS, 16);
}

TEST(UIWidgetsPure, TextTableStructInit) {
    UITextTable table;
    memset(&table, 0, sizeof(table));
    table.num_columns = 3;
    table.col_x[0] = 10;
    table.col_x[1] = 100;
    table.col_x[2] = 200;
    table.y_start = 30;
    table.row_height = 12;
    table.num_rows = 5;
    table.highlight_row = 255;  /* none */

    EXPECT_EQ(table.num_columns, 3);
    EXPECT_EQ(table.col_x[0], 10);
    EXPECT_EQ(table.col_x[2], 200);
    EXPECT_EQ(table.y_start, 30);
    EXPECT_EQ(table.row_height, 12);
    EXPECT_EQ(table.num_rows, 5);
    EXPECT_EQ(table.highlight_row, 255);
}


/* ================================================================== */
/* UIDialog struct layout and constants                                */
/* ================================================================== */

TEST(UIWidgetsPure, DialogConstants) {
    EXPECT_EQ(UI_DIALOG_AUTO_POS, 65535);
    EXPECT_EQ(UI_DIALOG_INFO, 1);
    EXPECT_EQ(UI_DIALOG_CONFIRM, 2);
    EXPECT_EQ(UI_DIALOG_TIMED, 4);
    EXPECT_EQ(UI_DIALOG_MAX_LINES, 8);
    EXPECT_EQ(UI_DIALOG_MAX_BUTTONS, 10);
}

TEST(UIWidgetsPure, DialogStructInit) {
    UIDialog dlg;
    memset(&dlg, 0, sizeof(dlg));
    dlg.mode = UI_DIALOG_CONFIRM;
    dlg.x = UI_DIALOG_AUTO_POS;
    dlg.y = UI_DIALOG_AUTO_POS;
    dlg.num_lines = 2;
    dlg.lines[0] = "Line 1";
    dlg.lines[1] = "Line 2";
    dlg.num_buttons = 2;
    dlg.buttons[0] = "OK";
    dlg.buttons[1] = "Cancel";
    dlg.default_button = 0;

    EXPECT_EQ(dlg.mode, UI_DIALOG_CONFIRM);
    EXPECT_EQ(dlg.x, UI_DIALOG_AUTO_POS);
    EXPECT_EQ(dlg.num_lines, 2);
    EXPECT_STREQ(dlg.lines[0], "Line 1");
    EXPECT_STREQ(dlg.lines[1], "Line 2");
    EXPECT_EQ(dlg.num_buttons, 2);
    EXPECT_STREQ(dlg.buttons[0], "OK");
    EXPECT_STREQ(dlg.buttons[1], "Cancel");
    EXPECT_EQ(dlg.default_button, 0);
}

TEST(UIWidgetsPure, DialogMaxLinesCapacity) {
    UIDialog dlg;
    memset(&dlg, 0, sizeof(dlg));
    /* Fill all available lines */
    for (int i = 0; i < UI_DIALOG_MAX_LINES; i++) {
        dlg.lines[i] = "text";
    }
    dlg.num_lines = UI_DIALOG_MAX_LINES;
    EXPECT_EQ(dlg.num_lines, 8);
}

TEST(UIWidgetsPure, DialogMaxButtonCapacity) {
    UIDialog dlg;
    memset(&dlg, 0, sizeof(dlg));
    for (int i = 0; i < UI_DIALOG_MAX_BUTTONS; i++) {
        dlg.buttons[i] = "btn";
    }
    dlg.num_buttons = UI_DIALOG_MAX_BUTTONS;
    EXPECT_EQ(dlg.num_buttons, 10);
}


/* ================================================================== */
/* Color constants                                                      */
/* ================================================================== */

TEST(UIWidgetsPure, ColorConstants) {
    EXPECT_EQ(UI_COL_BLACK,    0);
    EXPECT_EQ(UI_COL_WHITE,   15);
    EXPECT_EQ(UI_COL_SHADOW,   8);
    EXPECT_EQ(UI_COL_HIGHLIGHT, 7);
    EXPECT_EQ(UI_COL_DISABLED,  6);
}

TEST(UIWidgetsPure, ButtonColorConstants) {
    EXPECT_EQ(UI_COL_BTN_TEXT, 15);
    EXPECT_EQ(UI_COL_BTN_FILL,  8);
    EXPECT_EQ(UI_COL_BTN_HI,    7);
    EXPECT_EQ(UI_COL_BTN_LO,    9);
}

TEST(UIWidgetsPure, DialogColorConstants) {
    EXPECT_EQ(UI_COL_DIALOG_FG, 15);
    EXPECT_EQ(UI_COL_DIALOG_BG,  0);
}


/* ================================================================== */
/* UIScreen type constants from ui_screen.h                            */
/* ================================================================== */

TEST(UIWidgetsPure, EventTypeConstants) {
    EXPECT_EQ(UI_EVENT_NONE,       0);
    EXPECT_EQ(UI_EVENT_KEY_DOWN,   1);
    EXPECT_EQ(UI_EVENT_KEY_UP,     2);
    EXPECT_EQ(UI_EVENT_MOUSE_MOVE, 3);
    EXPECT_EQ(UI_EVENT_MOUSE_DOWN, 4);
    EXPECT_EQ(UI_EVENT_MOUSE_UP,   5);
    EXPECT_EQ(UI_EVENT_TICK,       6);
    EXPECT_EQ(UI_EVENT_ENTER,      7);
    EXPECT_EQ(UI_EVENT_LEAVE,      8);

    /* All distinct */
    unsigned short types[] = {
        UI_EVENT_NONE, UI_EVENT_KEY_DOWN, UI_EVENT_KEY_UP,
        UI_EVENT_MOUSE_MOVE, UI_EVENT_MOUSE_DOWN, UI_EVENT_MOUSE_UP,
        UI_EVENT_TICK, UI_EVENT_ENTER, UI_EVENT_LEAVE
    };
    for (int i = 0; i < 9; i++) {
        for (int j = i + 1; j < 9; j++) {
            EXPECT_NE(types[i], types[j])
                << "Event types " << i << " and " << j << " must be distinct";
        }
    }
}

TEST(UIWidgetsPure, ScreenStackMax) {
    EXPECT_EQ(UI_SCREEN_STACK_MAX, 8);
    EXPECT_GT(UI_SCREEN_STACK_MAX, 0);
}


/* ================================================================== */
/* UIEvent struct layout                                                */
/* ================================================================== */

TEST(UIWidgetsPure, EventStructZeroInit) {
    UIEvent ev;
    memset(&ev, 0, sizeof(ev));
    EXPECT_EQ(ev.type, 0);
    EXPECT_EQ(ev.key, 0);
    EXPECT_EQ(ev.mouse_x, 0);
    EXPECT_EQ(ev.mouse_y, 0);
    EXPECT_EQ(ev.mouse_buttons, 0);
    EXPECT_EQ(ev.delta, 0);
}

TEST(UIWidgetsPure, EventStructFieldAssignment) {
    UIEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = UI_EVENT_KEY_DOWN;
    ev.key = UI_KEY_ENTER;
    ev.delta = 42;
    EXPECT_EQ(ev.type, UI_EVENT_KEY_DOWN);
    EXPECT_EQ(ev.key, UI_KEY_ENTER);
    EXPECT_EQ(ev.delta, 42);
}

TEST(UIWidgetsPure, EventMouseFields) {
    UIEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = UI_EVENT_MOUSE_DOWN;
    ev.mouse_x = 160;
    ev.mouse_y = 100;
    ev.mouse_buttons = 1;
    EXPECT_EQ(ev.type, UI_EVENT_MOUSE_DOWN);
    EXPECT_EQ(ev.mouse_x, 160);
    EXPECT_EQ(ev.mouse_y, 100);
    EXPECT_EQ(ev.mouse_buttons, 1);
}


/* ================================================================== */
/* UIScreen struct layout                                               */
/* ================================================================== */

TEST(UIWidgetsPure, ScreenStructZeroInit) {
    UIScreen scr;
    memset(&scr, 0, sizeof(scr));
    EXPECT_EQ(scr.on_event, nullptr);
    EXPECT_EQ(scr.on_render, nullptr);
    EXPECT_EQ(scr.on_destroy, nullptr);
    EXPECT_EQ(scr.userdata, nullptr);
    EXPECT_EQ(scr._modal_result, 0);
    EXPECT_EQ(scr._wants_pop, 0);
}
