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

#ifndef MOUSE_H
#define MOUSE_H

void mouse_get_state(unsigned short* buttons, unsigned short* x, unsigned short* y);
void mouse_set_pixratio(unsigned short hpix, unsigned short vpix);
unsigned short mouse_init(unsigned short width, unsigned short height);
void mouse_set_minmax(int hmin, int vmin, int hmax, int vmax);
void mouse_set_position(int x, int y);
void mouse_draw_transparent_check(void);
void mouse_draw_opaque_check(void);
short mouse_multi_hittest(short count, unsigned short* x1_array, unsigned short* x2_array, unsigned short* y1_array, unsigned short* y2_array);
void mouse_draw_opaque(void);
void mouse_draw_transparent(void);
unsigned int mouse_update_menu_blink(unsigned char selected, unsigned short* x1_arr, unsigned short* x2_arr, unsigned short* y1_arr, unsigned short* y2_arr, unsigned short sprite_hi, unsigned short sprite_lo);

/* Global state — defined in src/mouse.c */

#ifdef MOUSE_IMPL
#  define _MO_
#else
#  define _MO_ extern
#endif

_MO_ char             kbormouse;
_MO_ unsigned short   mouse_butstate;
_MO_ char             mouse_isdirty;
_MO_ char             mouse_motion_detected_flag;
_MO_ char             mouse_motion_state_flag;
_MO_ unsigned short   mouse_xpos;
_MO_ unsigned short   mouse_ypos;

#undef _MO_

#endif /* MOUSE_H */
