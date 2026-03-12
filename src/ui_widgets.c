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
 * ui_widgets.c - Reusable menu widgets for the Stunts engine
 *
 * Implements the common button-select loop, text helpers, panel drawing,
 * and text table rendering.  All output matches the original Stunts
 * look pixel-for-pixel by delegating to the same low-level primitives.
 */

#include <string.h>
#include "ui_widgets.h"
#include "stunts.h"
#include "shape2d.h"
#include "font.h"
#include "mouse.h"
#include "keyboard.h"
#include "timer.h"
#include "highscore.h"   /* menu_reset_idle_timers */
#include "data_game.h"   /* camera_view_matrix, object_visibility_state, kbormouse, idle_expired, idle_counter */
#include "ui.h"          /* show_dialog */

/* ------------------------------------------------------------------ */
/* ui_button_menu_run                                                  */
/* ------------------------------------------------------------------ */

unsigned char ui_button_menu_run(UIButtonMenu *menu)
{
    unsigned char  selected;
    unsigned char  prev_selected;
    unsigned short delta;
    unsigned short key_code;
    short          hit;
    unsigned short local_idle;

    if (menu->count == 0) {
        return UI_SEL_CANCEL;
    }

    selected = menu->default_sel;
    if (selected >= menu->count) {
        selected = 0;
    }
    prev_selected = 255;  /* force first-frame redraw */
    local_idle = 0;

    /* Flush stale input so we don't immediately confirm. */
    check_input();
    mousebutinputcode = 0;
    joyinputcode = 0;
    timer_get_delta_alt();

    while (1) {
        /* ----- selection-change callback ----- */
        if (prev_selected != selected) {
            if (menu->selection_cb) {
                menu->selection_cb(selected, menu->selection_cb_ctx);
            }
            prev_selected = selected;
            menu_reset_idle_timers();
            local_idle = 0;
        }

        /* ----- blink / present ----- */
        delta = mouse_update_menu_blink(
            selected,
            menu->x1, menu->x2,
            menu->y1, menu->y2,
            menu->sprite_hi, menu->sprite_lo
        );
        if (delta == 0) {
            delta = 1;
        }

        /* ----- per-frame callback ----- */
        if (menu->frame_cb) {
            unsigned short cb_key = menu->frame_cb(selected, delta, menu->frame_cb_ctx);
            if (cb_key != 0) {
                key_code = cb_key;
                goto handle_key;
            }
        }

        /* ----- idle timeout ----- */
        if (menu->idle_timeout != 0) {
            local_idle += delta;
            idle_counter += delta;
            if (idle_expired != 0 || local_idle >= menu->idle_timeout) {
                idle_expired++;
                selected = menu->default_sel;
                return UI_SEL_TIMEOUT;
            }
        }

        /* ----- input polling ----- */
        key_code = input_checking(delta);

        /* ----- mouse hit test ----- */
        hit = mouse_multi_hittest(
            (short)menu->count,
            menu->x1, menu->x2,
            menu->y1, menu->y2
        );
        if (hit >= 0 && hit < (short)menu->count) {
            if (kbormouse != 0) {
                selected = (unsigned char)hit;
            }
            if (kbormouse != 0 && UI_IS_CONFIRM(key_code)) {
                return selected;
            }
        }

        if (key_code == 0) {
            continue;
        }

handle_key:
        /* ----- confirm ----- */
        if (UI_IS_CONFIRM(key_code)) {
            return selected;
        }

        /* ----- cancel ----- */
        if (UI_IS_CANCEL(key_code)) {
            return UI_SEL_CANCEL;
        }

        /* ----- navigation ----- */
        if (menu->nav_mode == UI_NAV_HORIZONTAL) {
            if (key_code == UI_KEY_LEFT) {
                selected = (selected == 0)
                    ? (unsigned char)(menu->count - 1)
                    : (unsigned char)(selected - 1);
            } else if (key_code == UI_KEY_RIGHT) {
                selected = (selected >= menu->count - 1)
                    ? 0
                    : (unsigned char)(selected + 1);
            }
        } else if (menu->nav_mode == UI_NAV_VERTICAL) {
            if (key_code == UI_KEY_UP) {
                selected = (selected == 0)
                    ? (unsigned char)(menu->count - 1)
                    : (unsigned char)(selected - 1);
            } else if (key_code == UI_KEY_DOWN) {
                selected = (selected >= menu->count - 1)
                    ? 0
                    : (unsigned char)(selected + 1);
            }
        } else if (menu->nav_mode == UI_NAV_BOTH_LR_SWAP) {
            /* Original main menu: RIGHT/UP = prev, LEFT/DOWN = next */
            if (key_code == UI_KEY_RIGHT || key_code == UI_KEY_UP) {
                selected = (selected == 0)
                    ? (unsigned char)(menu->count - 1)
                    : (unsigned char)(selected - 1);
            } else if (key_code == UI_KEY_LEFT || key_code == UI_KEY_DOWN) {
                selected = (selected >= menu->count - 1)
                    ? 0
                    : (unsigned char)(selected + 1);
            }
        } else {
            /* UI_NAV_BOTH (default — matches original) */
            if (UI_IS_NAV_PREV(key_code)) {
                selected = (selected == 0)
                    ? (unsigned char)(menu->count - 1)
                    : (unsigned char)(selected - 1);
            } else if (UI_IS_NAV_NEXT(key_code)) {
                selected = (selected >= menu->count - 1)
                    ? 0
                    : (unsigned char)(selected + 1);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Text rendering helpers                                              */
/* ------------------------------------------------------------------ */

unsigned short ui_draw_text(const char *text, unsigned short x, unsigned short y,
                            unsigned short fg_color, unsigned short bg_color)
{
    unsigned short w;
    font_set_colors((int)fg_color, bg_color);
    font_draw_text((char *)text, x, y);
    w = font_get_text_width((char *)text);
    return w;
}

unsigned short ui_measure_text(const char *text)
{
    return font_get_text_width((char *)text);
}

unsigned short ui_center_x(unsigned short item_w)
{
    if (item_w >= UI_SCREEN_W) return 0;
    return (UI_SCREEN_W - item_w) / 2;
}

unsigned short ui_center_in(unsigned short container_x, unsigned short container_w,
                            unsigned short item_w)
{
    if (item_w >= container_w) return container_x;
    return container_x + (container_w - item_w) / 2;
}

/* ------------------------------------------------------------------ */
/* Panel drawing                                                       */
/* ------------------------------------------------------------------ */

void ui_draw_panel(const UIPanel *panel)
{
    /* Fill background */
    sprite_fill_rect_clipped(panel->x, panel->y, panel->w, panel->h,
                             (unsigned char)panel->fill_color);

    /* 1-pixel border (matches original sprite_draw_rect_outline) */
    sprite_draw_rect_outline(panel->x, panel->y,
                             (unsigned short)(panel->x + panel->w - 1),
                             (unsigned short)(panel->y + panel->h - 1),
                             panel->border_color);

    /* Optional title */
    if (panel->title != NULL) {
        unsigned short tw = font_get_text_width((char *)panel->title);
        unsigned short tx = ui_center_in(panel->x, panel->w, tw);
        font_set_colors((int)panel->title_fg, panel->title_bg);
        font_draw_text((char *)panel->title, tx, (unsigned short)(panel->y + 2));
    }
}

/* ------------------------------------------------------------------ */
/* Text table                                                          */
/* ------------------------------------------------------------------ */

void ui_draw_text_table(const UITextTable *table,
                        const char *cells[],
                        const char *headers[])
{
    unsigned short row, col;
    unsigned short y;

    /* Draw header row */
    if (headers != NULL) {
        for (col = 0; col < table->num_columns && col < UI_TABLE_MAX_COLS; col++) {
            if (headers[col] != NULL) {
                font_set_colors(0, table->header_color);
                font_draw_text((char *)headers[col],
                               table->col_x[col], table->y_start);
            }
        }
    }

    /* Draw data rows */
    for (row = 0; row < table->num_rows && row < UI_TABLE_MAX_ROWS; row++) {
        y = (unsigned short)(table->y_start
            + (headers != NULL ? table->row_height : 0)
            + row * table->row_height);

        if (row == table->highlight_row) {
            font_set_colors(0, table->highlight_color);
        } else {
            font_set_colors(0, table->text_color);
        }

        for (col = 0; col < table->num_columns && col < UI_TABLE_MAX_COLS; col++) {
            const char *cell = cells[row * table->num_columns + col];
            if (cell != NULL) {
                font_draw_text((char *)cell, table->col_x[col], y);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Dialog widget                                                       */
/* ------------------------------------------------------------------ */

/*
 * Build a show_dialog-compatible format string from a UIDialog struct.
 *
 * The original show_dialog expects text in a specific format:
 *   "Line 1 text]Line 2 text][Button1][Button2]"
 *
 * Where:
 *   ']' ends a text line
 *   '[' starts a button definition, ']' ends it
 *   '}' creates a section separator (4px spacing)
 *   '@' marks coordinate output positions (type 3 only)
 */
static void ui_dialog_build_text(const UIDialog *dlg, char *buf, unsigned short buflen)
{
    unsigned short pos = 0;
    unsigned short i;

    buf[0] = '\0';

    /* Append text lines (each terminated by ']') */
    for (i = 0; i < dlg->num_lines && i < UI_DIALOG_MAX_LINES; i++) {
        const char *line = dlg->lines[i];
        if (line == NULL) continue;
        while (*line != '\0' && pos < buflen - 2) {
            buf[pos++] = *line++;
        }
        if (pos < buflen - 2) {
            buf[pos++] = ']';
        }
    }

    /* Append button definitions (each wrapped in [ ]) */
    for (i = 0; i < dlg->num_buttons && i < UI_DIALOG_MAX_BUTTONS; i++) {
        const char *btn = dlg->buttons[i];
        if (btn == NULL) continue;
        if (pos < buflen - 2) {
            buf[pos++] = '[';
        }
        while (*btn != '\0' && pos < buflen - 2) {
            buf[pos++] = *btn++;
        }
        if (pos < buflen - 2) {
            buf[pos++] = ']';
        }
    }

    buf[pos] = '\0';
}

short ui_dialog_run(const UIDialog *dlg)
{
    char text_buf[512];
    unsigned short border;
    unsigned short dx, dy;

    ui_dialog_build_text(dlg, text_buf, sizeof(text_buf));

    border = dlg->border_color;
    if (border == 0) {
        border = dialogarg2;
    }

    dx = dlg->x;
    dy = dlg->y;

    /* Map UI_DIALOG_TIMED to show_dialog type 4 (brief display) */
    if (dlg->mode == UI_DIALOG_TIMED) {
        return show_dialog(4, 1, text_buf, dx, dy, border, dlg->coords, dlg->default_button);
    }

    /* Map UI_DIALOG_INFO to show_dialog type 1 (wait for any key) */
    if (dlg->mode == UI_DIALOG_INFO) {
        return show_dialog(1, 1, text_buf, dx, dy, border, dlg->coords, 0);
    }

    /* UI_DIALOG_CONFIRM → show_dialog type 2 (interactive selection) */
    return show_dialog(2, 1, text_buf, dx, dy, border, dlg->coords, dlg->default_button);
}

short ui_dialog_info(const char *text)
{
    UIDialog dlg;
    memset(&dlg, 0, sizeof(dlg));
    dlg.mode = UI_DIALOG_INFO;
    dlg.border_color = 0;
    dlg.x = UI_DIALOG_AUTO_POS;
    dlg.y = UI_DIALOG_AUTO_POS;
    dlg.num_lines = 1;
    dlg.lines[0] = text;
    return ui_dialog_run(&dlg);
}

short ui_dialog_confirm(const char *text,
                        const char *btn_yes, const char *btn_no)
{
    UIDialog dlg;
    memset(&dlg, 0, sizeof(dlg));
    dlg.mode = UI_DIALOG_CONFIRM;
    dlg.border_color = 0;
    dlg.x = UI_DIALOG_AUTO_POS;
    dlg.y = UI_DIALOG_AUTO_POS;
    dlg.num_lines = 1;
    dlg.lines[0] = text;
    dlg.num_buttons = 2;
    dlg.buttons[0] = btn_yes;
    dlg.buttons[1] = btn_no;
    dlg.default_button = 0;
    return ui_dialog_run(&dlg);
}

/* ------------------------------------------------------------------ */
/* Resource-text dialog wrappers                                       */
/* ------------------------------------------------------------------ */

short ui_dialog_show_restext(unsigned short mode, unsigned short create_window,
                             void *text_res_ptr,
                             unsigned short x, unsigned short y,
                             unsigned short border_color,
                             unsigned short *coords,
                             unsigned char default_button)
{
    unsigned short type;

    if (border_color == 0) {
        border_color = dialogarg2;
    }

    /* Map widget mode to internal show_dialog type */
    switch (mode) {
    case UI_DIALOG_INFO:    type = 1; break;
    case UI_DIALOG_CONFIRM: type = 2; break;
    case UI_DIALOG_TIMED:   type = 4; break;
    default:                type = mode; break;  /* pass-through for 0, 3 */
    }

    return show_dialog(type, create_window, text_res_ptr,
                       x, y, border_color, coords, default_button);
}

short ui_dialog_display_only(void *text_res_ptr, unsigned short x, unsigned short y,
                             unsigned short border_color)
{
    if (border_color == 0) border_color = dialogarg2;
    return show_dialog(0, 0, text_res_ptr, x, y, border_color, 0, 0);
}

short ui_dialog_timed(void *text_res_ptr)
{
    return show_dialog(4, 1, text_res_ptr, UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, dialogarg2, 0, 0);
}

short ui_dialog_info_restext(void *text_res_ptr)
{
    return show_dialog(1, 1, text_res_ptr, UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, dialogarg2, 0, 0);
}

short ui_dialog_confirm_restext(void *text_res_ptr, unsigned char default_button)
{
    return show_dialog(2, 1, text_res_ptr, UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, dialogarg2, 0, default_button);
}

short ui_dialog_layout(void *text_res_ptr, unsigned short border_color,
                       unsigned short *coords)
{
    if (border_color == 0) border_color = dialogarg2;
    return show_dialog(3, 1, text_res_ptr, UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, border_color, coords, 0);
}
