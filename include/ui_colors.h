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
 * ui_colors.h - Palette-index constants for UI elements
 *
 * These are the exact VGA palette indices used by the original Stunts
 * game for its menus.  Do NOT change the values — they must match the
 * loaded .RES palette data to produce the authentic look.
 */

#ifndef UI_COLORS_H
#define UI_COLORS_H

/* ---- Standard UI palette indices ----------------------------------- */
#define UI_COL_BLACK        0
#define UI_COL_WHITE        15
#define UI_COL_SHADOW       8
#define UI_COL_HIGHLIGHT    7
#define UI_COL_DISABLED     6    /* Greyed-out / inactive text          */

/* ---- Button defaults (3D beveled buttons) -------------------------- */
#define UI_COL_BTN_TEXT     15   /* button_text_color in original       */
#define UI_COL_BTN_FILL     8   /* button_highlight_color in original  */
#define UI_COL_BTN_HI       7   /* top/left highlight edge             */
#define UI_COL_BTN_LO       9   /* bottom/right shadow edge            */

/* ---- Dialog colours ------------------------------------------------ */
#define UI_COL_DIALOG_FG    15   /* dialog_fnt_colour                  */
#define UI_COL_DIALOG_BG    0    /* dialog_text_color (background)     */

/* ---- Screen dimensions (original Stunts 320×200 VGA) --------------- */
#define UI_SCREEN_W         320
#define UI_SCREEN_H         200

#endif /* UI_COLORS_H */
