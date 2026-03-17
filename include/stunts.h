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

#ifndef STUNTS_H
#define STUNTS_H

/* Types */
#include "game_types.h"

/* Data / global variables */
#include "data_game.h"
#include "data_menu_track.h"

/* Core modules */
#include "math.h"
#include "memmgr.h"
#include "ressources.h"
#include "timer.h"
#include "keyboard.h"
#include "mouse.h"
#include "carstate.h"

/* Rendering */
#include "shape2d.h"
#include "shape3d.h"
#include "render.h"

/* Game logic */
#include "game.h"
#include "game_timing.h"
#include "track.h"
#include "audio.h"
#include "highscore.h"
#include "menu.h"
#include "ui.h"
#include "font.h"

/* Global state — defined in src/stuntsengine.c */

#ifdef STUNTS_IMPL
#  define _SE_
#else
#  define _SE_ extern
#endif

_SE_ char             aDefault_1[];
_SE_ char             replay_file_path_buffer[];
_SE_ struct SIMD      simd_opponent_rt;
_SE_ char             track_highscore_path_buffer[];

#undef _SE_

/* Exit handler (defined in stunts.c) */
void add_exit_handler(void (*handler)(void));
void draw_beveled_border(unsigned short x, unsigned short y, unsigned short w,
                         unsigned short h, unsigned short col1,
                         unsigned short col2, unsigned short col3);

#endif