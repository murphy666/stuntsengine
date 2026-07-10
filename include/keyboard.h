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

#include "game_types.h"

extern unsigned short newjoyflags;
extern unsigned short mouse_oldbut;
extern unsigned short joyinputcode;
extern unsigned short mousebutinputcode;
extern unsigned const char g_ascii_props[];
extern bool game_startup_flag;
extern unsigned char kbinput[];
extern unsigned short kbjoyflags;
extern char mouse_button_press_state[];

enum {
    KB_INPUT_FLAG_UP = 0x01,
    KB_INPUT_FLAG_DOWN = 0x02,
    KB_INPUT_FLAG_RIGHT = 0x04,
    KB_INPUT_FLAG_LEFT = 0x08,
    KB_INPUT_FLAG_PRIMARY_ACTION = 0x10,
    KB_INPUT_FLAG_SECONDARY_ACTION = 0x20,
    KB_INPUT_FLAG_ACTION_MASK = KB_INPUT_FLAG_PRIMARY_ACTION | KB_INPUT_FLAG_SECONDARY_ACTION,
    KB_KEYSTATE_CTRL = 29,
    KB_KEYSTATE_A = 30,
    KB_KEYSTATE_Z = 44
};

void kb_init_interrupt(void);
void kb_exit_handler(void);
void kb_int16_handler(unsigned bp, unsigned di, unsigned si, unsigned ds, unsigned es, unsigned dx,
                      unsigned cx, unsigned bx, unsigned ax, unsigned ip, unsigned cs,
                      unsigned flags);
int kb_get_key_state(int key);
int kb_call_readchar_callback(void);
int kb_read_char(void);
int kb_checking(void);
int kb_check(void);
unsigned short kb_read_key_or_joy(void);
void kb_poll_input(void);
void kb_requeue_key(unsigned short key);
unsigned short kb_input_flags_to_keycode(unsigned short input_flags);

/* Extended keyboard/input functions */
int kb_get_char(void);
bool handle_ingame_kb_shortcuts(int);
void kb_reg_callback(unsigned short code, void (*callback)(void));
void kb_shift_checking2(void);

/* Input polling and state checking */
void check_input(void);
int input_do_checking(int unk);
unsigned short input_repeat_check(unsigned short delay);
unsigned short input_checking(unsigned short delta);
#define input_checking input_checking

/* Joystick / combined input */
short get_kb_or_joy_flags(void);
short get_joy_flags(void);
void joystick_init_calibration(void);
short joystick_direction_lookup(unsigned short joy_flags);
short joystick_get_scaled_x(void);

#endif