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
 * Core game state variables — definitions only; no function code.
 * Declarations (extern) are in include/data_game.h for consumers.
 *
 * Covers the "DataCore" group of test_data_tables.cc:
 *   hillHeightConsts, terrain-name strings, rotation angles,
 *   HKeyFlag, cameramode, and the misc runtime state flags.
 */

#define DATA_CORE_IMPL
#include "data_core.h"
#include "game_types.h"

/* Pointer to a second unknown rectangle (defined/used by frame.c, gamemech.c) */
struct RECTANGLE * rect_buffer_secondary = 0;

/* Pointer to the primary unknown rectangle (frame, gamemech, render, stunts) */
struct RECTANGLE* rect_buffer_primary = 0;

/* Hill height constants: {flat=0, hill=450} */
short hillHeightConsts[] = { 0, 450 };

/* 3-D camera / free-cam rotation angles (degrees in 10-bit fixed-point units) */
unsigned short rotation_x_angle = 210;
unsigned short rotation_z_angle = 464;
unsigned short rotation_y_angle = 80;

/* Audio load state: 0 = not loaded, non-zero = loaded */
short is_audioloaded = 0;

/* Terrain-type resource name strings */
char aMain[]             = "main";
unsigned char aDesert[]  = "desert";
unsigned char aAlpine[]  = "alpine";
unsigned char aCity[]    = "city";
unsigned char aCountry[] = "country";

/* Hot-key toggle flag (gamemech.c XORs bit 0 on key press) */
unsigned char HKeyFlag[1] = { 0 };

/* Camera mode: 0=follow, 1=bumper, 2=overhead, 3=free */
char cameramode = 0;

/* Timer test flag (used by frame.c, render.c, stunts.c, ui.c) */
unsigned char timertestflag2 = 0;

/* Global "engine busy" flag */
char g_is_busy = 0;

/* Opponent resource prefix string (bto.c, menu.c) */
char aOpp1[] = "opp1";

/* Car resource prefix string — last 4 bytes are overwritten with car ID */
unsigned char aCarcoun[] = "carcoun";
