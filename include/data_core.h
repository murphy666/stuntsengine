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

#ifndef DATA_CORE_H
#define DATA_CORE_H

#include "game_types.h"

/* Global variables — defined in src/data_core.c */

#ifdef DATA_CORE_IMPL
#  define _DC_
#else
#  define _DC_ extern
#endif

_DC_ unsigned char    HKeyFlag[];
_DC_ unsigned char    aCarcoun[];
_DC_ char             aOpp1[];
_DC_ char             cameramode;
_DC_ char             g_is_busy;
_DC_ short            hillHeightConsts[];
_DC_ short            is_audioloaded;
_DC_ struct RECTANGLE *rect_buffer_primary;
_DC_ struct RECTANGLE *rect_buffer_secondary;
_DC_ unsigned short   rotation_x_angle;
_DC_ unsigned short   rotation_y_angle;
_DC_ unsigned short   rotation_z_angle;
_DC_ unsigned char    timertestflag2;

#undef _DC_

#endif /* DATA_CORE_H */
