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

#ifndef HIGHSCORE_H
#define HIGHSCORE_H

struct RECTANGLE;

unsigned short enter_hiscore(unsigned short score, void* textres_ptr, unsigned char arg6);
unsigned short highscore_write_b(void);
unsigned short end_hiscore(void);
struct RECTANGLE* hiscore_draw_text(char* str, int x, int y, int colour, int shadowColour);
void menu_reset_idle_timers(void);
unsigned short highscore_write_a_(unsigned short write_defaults);
void print_highscore_entry_(unsigned short index, unsigned char* lengths);
void highscore_draw_screen(void);

#endif /* HIGHSCORE_H */
