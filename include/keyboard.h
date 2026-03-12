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

#ifndef KEYBOARD_H
#define KEYBOARD_H

void kb_init_interrupt(void);
void kb_exit_handler(void);
void kb_int16_handler(unsigned bp, unsigned di, unsigned si,
                                unsigned ds, unsigned es, unsigned dx,
                                unsigned cx, unsigned bx, unsigned ax,
                                unsigned ip, unsigned cs, unsigned flags);
int kb_get_key_state(int key);
int kb_call_readchar_callback(void);
int kb_read_char(void);
int kb_checking(void);
int kb_check(void);
unsigned short  kb_read_key_or_joy(void);
void kb_poll_sdl_input(void);
void kb_sdl_requeue_key(unsigned short key);
void kb_sdl_set_mouse_limits(unsigned short min_x, unsigned short min_y, unsigned short max_x, unsigned short max_y);
void kb_sdl_set_mouse_position(unsigned short x, unsigned short y);
void kb_sdl_get_mouse_state(unsigned short* buttons, unsigned short* x, unsigned short* y);

/* Extended keyboard/input functions */
int    kb_get_char(void);
int    handle_ingame_kb_shortcuts(int);
void   kb_reg_callback(unsigned short code, void (* callback)(void));
void   kb_shift_checking2(void);

/* Input polling and state checking */
void           check_input(void);
int            input_do_checking(int unk);
unsigned short input_repeat_check(unsigned short delay);
unsigned short input_checking(unsigned short delta);
#define input_checking input_checking

/* Joystick / combined input */
short get_kb_or_joy_flags(void);
short get_joy_flags(void);
void  joystick_init_calibration(void);
short joystick_direction_lookup(unsigned short joy_flags);
short joystick_get_scaled_x(void);

#endif