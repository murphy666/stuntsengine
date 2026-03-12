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
 * Menu UI coordinates, animation timing, and car-physics lookup tables.
 * Definitions only; no function code.
 *
 * Covers the "DataMenuTrack" group of test_data_tables.cc:
 *   trackmenu button coords, opponentmenu button coords,
 *   animation_duration_table, and the four carstate physics tables
 *   (previously in src/carstate_data.c — merged here per test layout).
 *
 * The carstate physics tables are still declared in include/carstate.h;
 * include that header in any translation unit that reads them.
 */

#include "carstate.h"
#define DATA_MENU_TRACK_IMPL
#include "data_menu_track.h"


/* ── Track-selection menu button bounding boxes (3 slots) ────────────── */
unsigned short trackmenu_buttons_x1[] = { 16, 112, 208 };
unsigned short trackmenu_buttons_x2[] = { 112, 208, 304 };
unsigned short trackmenu_buttons_y1[] = { 171, 171, 171 };
unsigned short trackmenu_buttons_y2[] = { 197, 197, 197 };

/* ── Opponent-selection menu button bounding boxes (5 slots) ─────────── */
unsigned short opponentmenu_buttons_x1[] = { 20, 76, 132, 188, 244 };
unsigned short opponentmenu_buttons_x2[] = { 76, 132, 188, 244, 300 };
unsigned short opponentmenu_buttons_y1[] = { 177, 177, 177, 177, 177 };
unsigned short opponentmenu_buttons_y2[] = { 197, 197, 197, 197, 197 };

/* ── Frame-animation phase thresholds (8 increasing frame counts) ────── */
unsigned short animation_duration_table[8] = {
    30, 200, 320, 400,
    530, 700, 880, 960
};

/* ── Car-physics lookup tables (previously in src/carstate_data.c) ───── */

/* Per-wheel grip coefficients (4 wheels × 2 bytes each, little-endian pairs) */
unsigned char  wheel_rating_coefficients[8] = { 21, 0, 21, 0, 15, 0, 15, 0 };

/* Grass-surface deceleration divisors indexed by wheel count on grass (0-4) */
unsigned short grassDecelDivTab[]           = { 255, 256, 192, 128, 64 };

/* Corner X-flip flags for collision position rotation (4 corners) */
short          position_rotation_matrix[]   = { 1, 0, 0, 1 };

/* Camera reference position/direction vector (4 elements) */
short          camera_position_vector[]     = { 0, 0, 1, 1 };
