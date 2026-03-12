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

#ifndef DATA_MENU_TRACK_H
#define DATA_MENU_TRACK_H

/* Menu button layout tables — defined in src/data_menu_track.c */


#ifdef DATA_MENU_TRACK_IMPL
#  define _DMT_
#else
#  define _DMT_ extern
#endif

_DMT_ unsigned short animation_duration_table[];

_DMT_ unsigned short trackmenu_buttons_x1[];
_DMT_ unsigned short trackmenu_buttons_x2[];
_DMT_ unsigned short trackmenu_buttons_y1[];
_DMT_ unsigned short trackmenu_buttons_y2[];

_DMT_ unsigned short opponentmenu_buttons_x1[];
_DMT_ unsigned short opponentmenu_buttons_x2[];
_DMT_ unsigned short opponentmenu_buttons_y1[];
_DMT_ unsigned short opponentmenu_buttons_y2[];

#undef _DMT_

#endif /* DATA_MENU_TRACK_H */
