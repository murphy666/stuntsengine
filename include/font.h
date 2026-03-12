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

#ifndef FONT_H
#define FONT_H


#ifdef FONT_IMPL
#  define _FO_
#else
#  define _FO_ extern
#endif

_FO_ unsigned char* g_fontdef_ptr;
_FO_ void * fontnptr;
_FO_ void * fontdefptr;
_FO_ unsigned short fontdefseg;
_FO_ void * fontledresptr;
_FO_ unsigned short fontdef_line_height;
_FO_ unsigned char font_x_position_base[2];

#undef _FO_

void font_set_fontdef2(void * data);
void font_set_fontdef(void);
void set_fontdefseg(void * data);
void font_set_colors(int fg_color, unsigned short bg_color);
unsigned short font_op(char* text, unsigned short maxChars);
unsigned short font_get_text_width(char* text);
void font_draw_text(char* str, unsigned short x, unsigned short y);
int font_get_centered_x(char* text);

#endif /* FONT_H */
