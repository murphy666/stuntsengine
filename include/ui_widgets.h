/*
 * Copyright (c) 2026 Stunts Engine Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * ui_widgets.h - Reusable widget functions for the Stunts menu system
 *
 * These widgets encapsulate the repeated button-select-with-blink loop,
 * text rendering helpers, and layout utilities used across every menu
 * screen.  They delegate to the existing low-level primitives
 * (draw_button, mouse_update_menu_blink, sprite_fill_rect, font_draw_text,
 * etc.) so the pixel output is identical to the original game.
 */

#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include "ui_keys.h"
#include "ui_colors.h"

/* Maximum number of buttons in a single menu strip */
#define UI_BTN_MAX 16

/* -------- Navigation modes for ui_button_menu_run() ----------------- */
#define UI_NAV_HORIZONTAL  0   /* Left/Right navigate, Up/Down ignored  */
#define UI_NAV_VERTICAL    1   /* Up/Down navigate, Left/Right ignored  */
#define UI_NAV_BOTH        2   /* Both axes navigate (original default) */
#define UI_NAV_BOTH_LR_SWAP 3  /* Both axes, LEFT=next/RIGHT=prev       */

/* -------- Button menu widget ---------------------------------------- */

/*
 * UIButtonMenu — Describes a set of on-screen buttons for the
 * blink-and-select loop used by every graphical menu in Stunts.
 *
 * The caller fills in the button rectangles, count, and navigation
 * mode.  ui_button_menu_run() drives the selection loop and returns
 * the chosen button index.
 */
typedef struct UIButtonMenu {
    unsigned short count;          /* Number of buttons (1..UI_BTN_MAX)  */
    unsigned short x1[UI_BTN_MAX]; /* Left edge of each button          */
    unsigned short x2[UI_BTN_MAX]; /* Right edge of each button         */
    unsigned short y1[UI_BTN_MAX]; /* Top edge of each button           */
    unsigned short y2[UI_BTN_MAX]; /* Bottom edge of each button        */
    unsigned short sprite_hi;      /* Passed to mouse_update_menu_blink */
    unsigned short sprite_lo;      /* Passed to mouse_update_menu_blink */
    unsigned char  default_sel;    /* Initially selected button index   */
    unsigned short idle_timeout;   /* Ticks before auto-select (0=off)  */
    unsigned char  nav_mode;       /* UI_NAV_HORIZONTAL / _VERTICAL / _BOTH */

    /*
     * Optional per-iteration callback.  If non-NULL, called once per
     * frame with the current selection index and the time delta from
     * mouse_update_menu_blink.  The callback can draw additional
     * per-frame content (e.g. 3D car rotation, opponent animation).
     * Return 0 to continue the loop.  Return non-zero to break out
     * (the return value becomes the key_code used to decide the action).
     */
    unsigned short (*frame_cb)(unsigned char sel, unsigned short delta, void *ctx);
    void *frame_cb_ctx;

    /*
     * Optional pre-draw callback.  If non-NULL, called once each time
     * the selection changes, before the blink redraw.  Use this to
     * blit the wndsprite base content / redraw selection-dependent
     * elements.
     */
    void (*selection_cb)(unsigned char sel, void *ctx);
    void *selection_cb_ctx;
} UIButtonMenu;

/*
 * Run the standard button-select loop.
 *
 * Drives mouse_update_menu_blink / mouse_multi_hittest / input_checking
 * in exactly the same order as the original menu code.
 *
 * Returns:
 *   0..count-1  — selected button index
 *   UI_SEL_CANCEL (255) — Escape pressed
 *   UI_SEL_TIMEOUT (254) — idle timeout triggered
 */
unsigned char ui_button_menu_run(UIButtonMenu *menu);

/* -------- Text rendering helpers ------------------------------------ */

/*
 * Draw text at (x, y) with the given palette colours.
 * Wraps font_set_colors + font_draw_text.  Returns the pixel width
 * of the rendered string.
 */
unsigned short ui_draw_text(const char *text, unsigned short x, unsigned short y,
                            unsigned short fg_color, unsigned short bg_color);

/*
 * Measure text width without drawing.
 * Wraps font_get_text_width.
 */
unsigned short ui_measure_text(const char *text);

/*
 * Return the X coordinate that centers text of the given width on
 * a 320-pixel screen.
 */
unsigned short ui_center_x(unsigned short item_w);

/*
 * Compute X position that centres an item inside a container.
 */
unsigned short ui_center_in(unsigned short container_x, unsigned short container_w,
                            unsigned short item_w);

/* -------- Panel drawing helper -------------------------------------- */

typedef struct {
    unsigned short x, y, w, h;
    unsigned short border_color;
    unsigned short fill_color;
    const char    *title;            /* NULL = no title bar             */
    unsigned short title_fg;
    unsigned short title_bg;
} UIPanel;

/*
 * Draw a panel: filled rectangle, 1px border, optional centered title.
 * Matches the original Stunts look.
 */
void ui_draw_panel(const UIPanel *panel);

/* -------- Text column table ----------------------------------------- */

#define UI_TABLE_MAX_COLS   8
#define UI_TABLE_MAX_ROWS   16

typedef struct {
    unsigned short num_columns;
    unsigned short col_x[UI_TABLE_MAX_COLS]; /* X position per column   */
    unsigned short y_start;                  /* Y of first row          */
    unsigned short row_height;               /* Pixels between rows     */
    unsigned short num_rows;
    unsigned short header_color;
    unsigned short text_color;
    unsigned short highlight_row;   /* 255 = none                      */
    unsigned short highlight_color;
} UITextTable;

/*
 * Draw a multi-column text table.
 *
 * cells[] is a flat array: cells[row * num_columns + col].
 * headers[] has num_columns entries (may be NULL to skip header row).
 */
void ui_draw_text_table(const UITextTable *table,
                        const char *cells[],
                        const char *headers[]);

/* -------- Dialog widget --------------------------------------------- */

/*
 * Dialog modes — these map directly to the show_dialog() type values
 * in the original engine but provide a cleaner, named interface.
 */
#define UI_DIALOG_AUTO_POS   65535  /* Sentinel: auto-center dialog on axis */

#define UI_DIALOG_INFO       1   /* Show text, wait for any key         */
#define UI_DIALOG_CONFIRM    2   /* Interactive button selection         */
#define UI_DIALOG_TIMED      4   /* Show text briefly, auto-dismiss     */

/* Maximum limits for dialog content */
#define UI_DIALOG_MAX_LINES     8
#define UI_DIALOG_MAX_BUTTONS  10

/*
 * UIDialog — Describes a modal dialog with optional buttons.
 *
 * Instead of constructing special marker-format strings, callers
 * fill in plain text lines and button labels.  The widget builds
 * the internal format string and calls the engine's show_dialog().
 *
 * Works identically in menus and in-game — the underlying modal
 * window push/pop handles both contexts.
 */
typedef struct {
    unsigned short mode;           /* UI_DIALOG_INFO / _CONFIRM / _TIMED */
    unsigned short border_color;   /* 0 = use default dialogarg2         */

    /* Dialog position (65535 = auto-center, matching show_dialog) */
    unsigned short x;
    unsigned short y;

    /* Text content — lines separated by ']' internally */
    unsigned short  num_lines;
    const char     *lines[UI_DIALOG_MAX_LINES];

    /* Buttons (only used when mode == UI_DIALOG_CONFIRM) */
    unsigned short  num_buttons;
    const char     *buttons[UI_DIALOG_MAX_BUTTONS];
    unsigned char   default_button;   /* Initially selected (0-based)   */

    /* Optional: coords_array for type-3 usage (advanced, usually NULL) */
    unsigned short *coords;
} UIDialog;

/*
 * Display a modal dialog and return the user's choice.
 *
 * Returns:
 *   UI_DIALOG_INFO:    0 on normal key, 0 on cancel
 *   UI_DIALOG_CONFIRM: 0..N-1 = selected button, 255 = cancelled
 *   UI_DIALOG_TIMED:   0 (always)
 */
short ui_dialog_run(const UIDialog *dlg);

/*
 * Convenience: show an info dialog (press any key to dismiss).
 * Uses default colors and auto-centering.
 */
short ui_dialog_info(const char *text);

/*
 * Convenience: show a yes/no confirmation dialog.
 * Returns the selected button index (0 = first, 1 = second),
 * or 255 if cancelled.
 */
short ui_dialog_confirm(const char *text,
                        const char *btn_yes, const char *btn_no);

/*
 * Display a dialog using a pre-formatted resource text string.
 *
 * This is the widget-API equivalent of calling show_dialog() directly
 * with a resource-text pointer.  It maps the UIDialog mode constants
 * to the engine's internal dialog types so that ALL dialog display
 * goes through the widget layer — both in menus and in-game.
 *
 * Parameters:
 *   mode           — UI_DIALOG_INFO / _CONFIRM / _TIMED
 *   create_window  — 1 to create a modal window, 0 for overlay
 *   text_res_ptr   — Resource text in the original ']'/'['/'}'/'@' format
 *   x, y           — Position (65535 = auto-center)
 *   border_color   — 0 = use default dialogarg2
 *   coords         — Optional coordinate output array (for type-3), or NULL
 *   default_button — Initially selected button (for CONFIRM mode)
 *
 * Returns:
 *   Same as show_dialog(): button index for CONFIRM, 0 for others.
 */
short ui_dialog_show_restext(unsigned short mode, unsigned short create_window,
                             void *text_res_ptr,
                             unsigned short x, unsigned short y,
                             unsigned short border_color,
                             unsigned short *coords,
                             unsigned char default_button);

/*
 * Convenience wrappers for common resource-text dialog patterns.
 */

/* Display-only: render text and return immediately (type 0). */
short ui_dialog_display_only(void *text_res_ptr, unsigned short x, unsigned short y,
                             unsigned short border_color);

/* Timed notice: show text briefly then dismiss (type 4). */
short ui_dialog_timed(void *text_res_ptr);

/* Info with any-key dismiss (type 1). */
short ui_dialog_info_restext(void *text_res_ptr);

/* Interactive button selection from resource text (type 2). */
short ui_dialog_confirm_restext(void *text_res_ptr, unsigned char default_button);

/* Coordinate/layout query (type 3) — returns button count. */
short ui_dialog_layout(void *text_res_ptr, unsigned short border_color,
                       unsigned short *coords);

#endif /* UI_WIDGETS_H */
